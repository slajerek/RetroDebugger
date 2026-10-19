#include "CDebugStackDepth.h"
#include "CDebugDatabase.h"
#include "CDebugInterface.h"
#include "CDebugDataAdapter.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "C64Opcodes.h"
#include "C64SettingsStorage.h"
#include <cstring>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

const char *DebugStackDecodingName(DebugStackDecoding mode)
{
	switch (mode)
	{
		case DebugStackDecoding::Off:		return "off";
		case DebugStackDecoding::Limited:	return "limited";
		case DebugStackDecoding::Full:		return "full";
	}
	return "?";
}

DebugStackDecoding DebugGetStackDecoding()
{
	int mode = c64SettingsCc65StackDecoding;
	return mode <= 0 ? DebugStackDecoding::Off : mode == 1 ? DebugStackDecoding::Limited : DebugStackDecoding::Full;
}

namespace
{

const u8 kJsr = 0x20, kJmp = 0x4C, kJmpIndirect = 0x6C, kRts = 0x60, kRti = 0x40, kBrk = 0x00, kLdyImmediate = 0xA0;
const int kMaxInstructions = 20000;		// per function: a runaway walk is a decoding failure, not a hang

// what a runtime helper does to cc65's sp: bytes pushed (negative: popped)
bool HelperEffect(const char *name, int lastY, int *delta)
{
	struct Fixed { const char *name; int delta; };
	static const Fixed fixed[] = {
		{ "pusha", 1 }, { "pushaysp", 1 }, { "pushc0", 1 }, { "pushc1", 1 }, { "pushc2", 1 },
		{ "pushax", 2 }, { "pusha0", 2 }, { "pushaFF", 2 }, { "push0", 2 }, { "push1", 2 }, { "push2", 2 },
		{ "push3", 2 }, { "push4", 2 }, { "push5", 2 }, { "push6", 2 }, { "push7", 2 }, { "pushw", 2 },
		{ "pushwysp", 2 }, { "pushw0sp", 2 }, { "pushl0", 4 }, { "pusheax", 4 }, { "pushlysp", 4 },
		{ "popa", -1 }, { "popax", -2 }, { "popptr1", -2 }, { "popeax", -4 }, { "popsreg", -2 },
	};
	for (const Fixed &f : fixed)
	{
		if (strcmp(name, f.name) == 0)
		{
			*delta = f.delta;
			return true;
		}
	}
	if (strncmp(name, "decsp", 5) == 0 && name[5] >= '1' && name[5] <= '8' && name[6] == 0)
	{
		*delta = name[5] - '0';
		return true;
	}
	if (strncmp(name, "incsp", 5) == 0 && name[5] >= '1' && name[5] <= '8' && name[6] == 0)
	{
		*delta = -(name[5] - '0');
		return true;
	}
	if (strcmp(name, "subysp") == 0 || strcmp(name, "addysp") == 0 || strcmp(name, "addysp1") == 0)
	{
		if (lastY < 0)
			return false;
		*delta = name[0] == 's' ? lastY : -(lastY + (name[6] == '1' ? 1 : 0));
		return true;
	}
	// the binary operators take their left operand from the stack: tosaddax,
	// tosmula0, toseqax, ... pop a word; the long ones (tos...eax) four bytes
	if (strncmp(name, "tos", 3) == 0)
	{
		size_t n = strlen(name);
		*delta = (n > 3 && strcmp(name + n - 3, "eax") == 0) ? -4 : -2;
		return true;
	}
	// helpers that do not touch sp
	static const char *neutral[] = { "ldaxysp", "ldaxidx", "ldeaxysp", "ldaysp", "staxysp", "staspidx", "stax0sp",
		"steaxysp", "incax1", "incax2", "decax1", "decax2", "aslax1", "asrax1", "shlax1", "shrax1", "negax", "complax",
		"bnega", "bnegax", "boolne", "booleq", "boollt", "boolgt", "boolle", "boolge", "boolult", "boolugt", "boolule",
		"booluge", "axlong", "axulong", "aulong", "along", "callax", "callptr4", "jmpvec", "ldaui", "ldauidx", "ldai",
		"ldaidx", "ldaxi", "ldeaxi", "ldeaxidx", "regswap", "regswap1", "regswap2", "zerobss", "copydata" };
	for (const char *n : neutral)
	{
		if (strcmp(name, n) == 0)
		{
			*delta = 0;
			return true;
		}
	}
	return false;
}

struct Decoder
{
	const CDebugDatabase *database;
	CDebugInterface *debugInterface;
	u32 regionId;
	DebugStackDecoding mode;
	CDebugDataAdapter *memory;
	CDebugSymbolsSegment *segment;
	std::map<u32, int> calleeEffect;		// function start -> its net effect on the caller's depth
	std::set<u32> calleeExact;
	std::set<u32> inProgress;
	int instructions = 0;

	u8 Byte(u32 address) { return memory->AdapterReadByteModulus((int)(address & 0xFFFF)); }
	u16 Word(u32 address) { return (u16)(Byte(address) | (Byte(address + 1) << 8)); }

	const char *LabelAt(u16 address)
	{
		auto runtime = database->runtimeSymbols.find(address);
		if (runtime != database->runtimeSymbols.end())
			return runtime->second.c_str();
		CDebugSymbolsCodeLabel *label = segment ? segment->FindLabel(address) : NULL;
		return label ? label->GetLabelText() : NULL;
	}

	const CDebugFunction *FunctionStartingAt(u16 address)
	{
		for (const CDebugFunction &f : database->Functions())
			if (f.regionId == regionId && f.start == address)
				return &f;
		return NULL;
	}

	// What the JSR at ADDRESS does to the depth: false when it is not known
	bool CallEffect(u16 target, int lastY, int *delta, bool *exact, std::string *reason)
	{
		const CDebugFunction *callee = FunctionStartingAt(target);
		if (callee)
		{
			int effect = 0;
			bool calleeIsExact = false;
			if (!CalleeEffect(*callee, &effect, &calleeIsExact))
			{
				*reason = "the stack effect of " + callee->name + "() is not known";
				return false;
			}
			*delta = effect;
			if (!calleeIsExact)
			{
				*exact = false;
				*reason = "the stack effect of " + callee->name + "() is not certain";
			}
			return true;
		}
		const char *name = LabelAt(target);
		if (name && HelperEffect(name, lastY, delta))
			return true;
		*reason = std::string("an unknown routine at $") + Hex(target) + (name ? std::string(" (") + name + ")" : std::string());
		return false;
	}

	static std::string Hex(u16 v)
	{
		char buf[8];
		snprintf(buf, sizeof(buf), "%04X", v);
		return buf;
	}

	// the walk of FUNCTION: the depth at each instruction reached, the depth
	// at its exits, and whether anything on the way was guessed
	struct Walk
	{
		std::map<u16, int> depth;
		std::set<u16> conflicts;
		bool exitKnown = false;
		int exitDepth = 0;
		bool exitConflict = false;
		bool exact = true;
		bool failed = false;
		bool branched = false;		// Limited: a jump or branch was passed
		std::string reason;
	};

	void Note(Walk &w, const std::string &why)
	{
		w.exact = false;
		if (w.reason.empty())
			w.reason = why;
	}

	void Exit(Walk &w, int depth)
	{
		if (!w.exitKnown)
		{
			w.exitKnown = true;
			w.exitDepth = depth;
		}
		else if (w.exitDepth != depth)
			w.exitConflict = true;
	}

	// one instruction at ADDRESS with DEPTH before it: the successors and the
	// depth after it; false when the walk cannot go on from here
	bool Step(const CDebugFunction &f, Walk &w, u16 address, int depth, int &lastY, std::vector<std::pair<u16, int>> &next)
	{
		instructions++;
		u8 op = Byte(address);
		int length = opcodes[op].addressingLength;
		if (length < 1)
			length = 1;
		u16 fall = (u16)(address + length);
		auto inside = [&](u16 a) { return a >= f.start && a < f.end; };

		if (op == kLdyImmediate)
			lastY = Byte(address + 1);
		else if (op == 0xA8 || op == 0x88 || op == 0xC8 || op == 0xAC || op == 0xA4 || op == 0xB4 || op == 0xBC || op == 0xB6)
			lastY = -1;		// TAY, DEY, INY, LDY abs/zp/zp,x/abs,x: no longer a known constant

		if (op == kRts || op == kRti)
		{
			Exit(w, depth);
			return true;
		}
		if (op == kBrk || op == kJmpIndirect)
		{
			Note(w, op == kBrk ? "a BRK in the function" : "an indirect jump in the function");
			return true;
		}
		if (op == kJsr)
		{
			u16 target = Word(address + 1);
			int delta = 0;
			bool exact = true;
			std::string reason;
			if (!CallEffect(target, lastY, &delta, &exact, &reason))
			{
				Note(w, reason);
				w.failed = true;
				return false;
			}
			if (!exact)
				Note(w, reason);
			lastY = -1;
			next.push_back(std::make_pair(fall, depth + delta));
			return true;
		}
		if (op == kJmp)
		{
			u16 target = Word(address + 1);
			if (inside(target))
			{
				// cc65 ends a function with "jmp L0001 / L0001:": no other path
				if (target != fall)
					w.branched = true;
				next.push_back(std::make_pair(target, depth));
				return true;
			}
			// a tail call: jmp incsp2 returns for the function, jmp _f calls f and returns
			int delta = 0;
			bool exact = true;
			std::string reason;
			if (!CallEffect(target, lastY, &delta, &exact, &reason))
			{
				Note(w, reason);
				w.failed = true;
				return false;
			}
			if (!exact)
				Note(w, reason);
			Exit(w, depth + delta);
			return true;
		}
		if (opcodes[op].addressingMode == ADDR_REL)
		{
			u16 target = (u16)(fall + (i8)Byte(address + 1));
			w.branched = true;
			if (inside(target))
				next.push_back(std::make_pair(target, depth));
			else
				Note(w, "a branch out of the function");
			next.push_back(std::make_pair(fall, depth));
			return true;
		}
		next.push_back(std::make_pair(fall, depth));
		return true;
	}

	// Full: every path from the entry; Limited: the code in address order
	Walk WalkFunction(const CDebugFunction &f, u16 stopAt, bool linear)
	{
		Walk w;
		if (linear)
		{
			int depth = 0, lastY = -1;
			u32 address = f.start;
			while (address < f.end && instructions < kMaxInstructions)
			{
				w.depth[(u16)address] = depth;
				if (address == stopAt)
					break;
				std::vector<std::pair<u16, int>> next;
				if (!Step(f, w, (u16)address, depth, lastY, next))
					break;
				u8 op = Byte(address);
				int length = opcodes[op].addressingLength < 1 ? 1 : opcodes[op].addressingLength;
				// in address order: after a JSR the depth it returns with, else unchanged
				int after = depth;
				for (const std::pair<u16, int> &n : next)
					if (n.first == (u16)(address + length))
						after = n.second;
				depth = after;
				address += length;
			}
			return w;
		}

		std::vector<std::pair<u16, std::pair<int, int>>> work;		// address, depth, lastY
		work.push_back(std::make_pair((u16)f.start, std::make_pair(0, -1)));
		while (!work.empty() && instructions < kMaxInstructions)
		{
			u16 address = work.back().first;
			int depth = work.back().second.first, lastY = work.back().second.second;
			work.pop_back();
			auto seen = w.depth.find(address);
			if (seen != w.depth.end())
			{
				if (seen->second != depth)
					w.conflicts.insert(address);
				continue;
			}
			w.depth[address] = depth;
			std::vector<std::pair<u16, int>> next;
			if (!Step(f, w, address, depth, lastY, next))
				continue;
			for (const std::pair<u16, int> &n : next)
				work.push_back(std::make_pair(n.first, std::make_pair(n.second, lastY)));
		}
		if (instructions >= kMaxInstructions)
			Note(w, "the function is too large to decode");
		if (!w.conflicts.empty())
			Note(w, "two paths reach the code with different stack depths");
		return w;
	}

	bool CalleeEffect(const CDebugFunction &f, int *effect, bool *exact)
	{
		auto memo = calleeEffect.find(f.start);
		if (memo != calleeEffect.end())
		{
			*effect = memo->second;
			// a recursive call: the assumed effect, checked once the walk is done
			*exact = inProgress.count(f.start) > 0 || calleeExact.count(f.start) > 0;
			return true;
		}
		// recursion: assume no net effect, then check the assumption
		calleeEffect[f.start] = 0;
		inProgress.insert(f.start);
		Walk w = WalkFunction(f, 0, mode == DebugStackDecoding::Limited);
		inProgress.erase(f.start);
		if (!w.exitKnown || w.exitConflict || w.failed)
		{
			calleeEffect.erase(f.start);
			return false;
		}
		bool consistent = w.exitDepth == 0 || calleeEffect[f.start] == w.exitDepth;
		calleeEffect[f.start] = w.exitDepth;
		bool isExact = w.exact && consistent && !(mode == DebugStackDecoding::Limited && w.branched);
		if (isExact)
			calleeExact.insert(f.start);
		*effect = w.exitDepth;
		*exact = isExact;
		return true;
	}
};

}	// namespace

CDebugStackDepth DebugCc65StackDepth(const CDebugDatabase *database, CDebugInterface *debugInterface,
									 const CDebugFunction &function, u32 regionId, u16 pc, DebugStackDecoding mode)
{
	CDebugStackDepth r;
	if (mode == DebugStackDecoding::Off)
	{
		r.reason = "stack depth decoding is off (Settings)";
		return r;
	}
	if (database == NULL || debugInterface == NULL || debugInterface->GetDataAdapter() == NULL)
	{
		r.reason = "no debug info";
		return r;
	}
	Decoder d;
	d.database = database;
	d.debugInterface = debugInterface;
	d.regionId = regionId;
	d.mode = mode;
	d.memory = debugInterface->GetDataAdapter();
	d.segment = debugInterface->symbols ? debugInterface->symbols->currentSegment : NULL;

	// the prologue ends where the body's first line starts: the first mapping
	// of the function's own source file past its definition line (cc65 also
	// maps the generated assembly, from the prologue on)
	u32 bodyStart = function.end;
	for (const CDebugSourceMapping &m : database->Mappings())
		if (m.regionId == regionId && m.fileId == function.fileId && m.start >= function.start && m.start < function.end
			&& m.lineStart != function.line && m.start < bodyStart)
			bodyStart = m.start;

	bool linear = mode == DebugStackDecoding::Limited;
	Decoder::Walk w = d.WalkFunction(function, linear ? (u16)std::max<u32>(pc, bodyStart) : 0, linear);
	r.instructions = d.instructions;
	auto at = w.depth.find(pc);
	auto ref = w.depth.find((u16)bodyStart);
	if (at == w.depth.end() || ref == w.depth.end())
	{
		r.reason = !w.reason.empty() ? w.reason : "the PC is not reached from the function's entry";
		return r;
	}
	r.known = true;
	r.depth = at->second;
	r.refDepth = ref->second;
	bool conflicted = w.conflicts.count(pc) > 0 || w.conflicts.count((u16)bodyStart) > 0;
	r.exact = w.exact && !conflicted && !(linear && w.branched);
	if (!r.exact)
		r.reason = !w.reason.empty() ? w.reason
			: linear && w.branched ? "a jump or branch lies between the entry and the PC (Limited: one linear walk; Full follows them)"
			: "two paths reach the PC with different stack depths";
	return r;
}
