#include "CTestDebugDatabase.h"
#include "CDebugDatabase.h"
#include <vector>

#define DB_CHECK(cond, msg) \
	do { \
		if (!(cond)) { TestCompleted(false, msg); return; } \
		StepCompleted(stepNum++, true, msg); \
	} while (0)

static CDebugSourceMapping Mapping(u32 region, u32 start, u32 end, i32 line)
{
	CDebugSourceMapping m;
	m.regionId = region;
	m.start = start;
	m.end = end;
	m.fileId = 1;
	m.lineStart = line;
	m.lineEnd = line;
	return m;
}

static i32 LineAt(const CDebugDatabase &db, u32 region, u32 address)
{
	const CDebugSourceMapping *m = db.Resolve(region, address);
	return m ? m->lineStart : -1;
}

void CTestDebugDatabase::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	int stepNum = 1;

	CDebugDatabase db("test");
	db.AddFile(1, "C:\\Users\\someone\\project\\main.asm");
	db.AddFile(2, "/home/someone/project/lib/util.c");
	u32 a = db.AddRegion("Code", "");
	u32 b = db.AddRegion("Patch", "");
	db.AddMapping(Mapping(a, 0x1000, 0x1010, 10));
	db.AddMapping(Mapping(a, 0x1004, 0x1008, 20));	// inside the first: splits it
	db.AddMapping(Mapping(a, 0x2000, 0x2001, 30));
	db.AddMapping(Mapping(a, 0x2001, 0x2003, 31));	// adjacent, half-open
	db.AddMapping(Mapping(b, 0x0FFF, 0x1001, 40));
	DB_CHECK(db.Resolve(a, 0x1000) == NULL, "nothing resolves before Finalize");
	db.AddMapping(Mapping(a, 0x3000, 0x3000, 99));	// empty range: rejected
	db.AddMapping(Mapping(7, 0x3000, 0x3001, 99));	// unknown region: rejected
	db.Finalize();
	DB_CHECK(db.NumMappings() == 5, "empty ranges and unknown regions are rejected");

	DB_CHECK(LineAt(db, a, 0x1000) == 10 && LineAt(db, a, 0x1003) == 10 && LineAt(db, a, 0x1004) == 20
			 && LineAt(db, a, 0x1007) == 20 && LineAt(db, a, 0x1008) == 10 && LineAt(db, a, 0x100F) == 10,
			 "a later range overrides the middle of an earlier one, which survives on both sides");
	DB_CHECK(LineAt(db, a, 0x0FFF) == -1 && LineAt(db, a, 0x1010) == -1,
			 "ranges are half-open: the end address is not mapped");
	DB_CHECK(LineAt(db, a, 0x2000) == 30 && LineAt(db, a, 0x2001) == 31 && LineAt(db, a, 0x2002) == 31,
			 "adjacent ranges do not bleed into each other");
	DB_CHECK(LineAt(db, b, 0x1000) == 40 && LineAt(db, b, 0x1004) == -1 && LineAt(db, a, 0x0FFF) == -1,
			 "regions are independent: the same address can mean different code");

	std::vector<const CDebugSourceMapping *> candidates;
	db.ResolveCandidates(a, 0x1005, candidates);
	DB_CHECK(candidates.size() == 2 && candidates[0]->lineStart == 10 && candidates[1]->lineStart == 20,
			 "every covering mapping stays a candidate, in producer order");

	std::vector<const CDebugSourceMapping *> ranges;
	db.ResolveSource(1, 20, ranges);
	DB_CHECK(ranges.size() == 1 && ranges[0]->start == 0x1004 && ranges[0]->end == 0x1008,
			 "the reverse index finds the address range of a source line");
	db.ResolveSource(2, 20, ranges);
	DB_CHECK(ranges.empty(), "a line with no code has no ranges");

	DB_CHECK(db.FindRegion("Patch") == (int)b && db.FindRegion("Missing") == -1 && db.GetRegion(9) == NULL,
			 "regions are found by name");
	DB_CHECK(db.GetFile(1)->fileName == "main.asm" && db.GetFile(2)->fileName == "util.c"
			 && db.GetFile(3) == NULL && !db.GetFile(1)->HasText(),
			 "file names come from Windows and POSIX paths alike");

	// A range covering several earlier ones replaces all of them.
	CDebugDatabase db2("test");
	u32 r = db2.AddRegion("Code", "");
	db2.AddMapping(Mapping(r, 0x1000, 0x1002, 1));
	db2.AddMapping(Mapping(r, 0x1004, 0x1006, 2));
	db2.AddMapping(Mapping(r, 0x0FF0, 0x1100, 3));
	db2.AddMapping(Mapping(r, 0x1050, 0x1060, 4));
	db2.Finalize();
	DB_CHECK(LineAt(db2, r, 0x0FF0) == 3 && LineAt(db2, r, 0x1001) == 3 && LineAt(db2, r, 0x1005) == 3
			 && LineAt(db2, r, 0x1055) == 4 && LineAt(db2, r, 0x1060) == 3 && LineAt(db2, r, 0x10FF) == 3
			 && LineAt(db2, r, 0x1100) == -1,
			 "a wide later range replaces the earlier ones it covers, a narrow one splits it again");

	TestCompleted(true, "debug database painting, candidates and reverse index hold");
}
