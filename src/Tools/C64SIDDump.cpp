#include "CDebugInterfaceVice.h"
#include "C64SIDDump.h"
#include <list>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// this dumps CSidData history in siddump format, note this is not async, mutex must be locked elsewhere
// the code below is heavily based on siddump print to file code by Cadaver
// https://github.com/cadaver/siddump/blob/master/siddump.c

typedef struct
{
  unsigned short freq;
  unsigned short pulse;
  unsigned short adsr;
  unsigned char wave;
  int note;
} CHANNEL;

typedef struct
{
  unsigned short cutoff;
  unsigned char ctrl;
  unsigned char type;
} FILTER;

// int basefreq = 0;
// int basenote = 0xb0;
// int timeseconds = 0;	// show time as seconds or frames
// int oldnotefactor = 1;
// int pattspacing = 0;
// int spacing = 0;
// int lowres = 0;

void C64SIDHistoryToByteBuffer(std::list<CSidData *> *sidDataHistory, CByteBuffer *byteBuffer, u8 format,
		int basefreq, int basenote, int spacing, int oldnotefactor, int pattspacing, int timeseconds, int lowres)
{
	CHANNEL chn[3];
	CHANNEL prevchn[3];
	CHANNEL prevchn2[3];
	FILTER filt;
	FILTER prevfilt;

	const char *notename[] =
	 {"C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
	  "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	  "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	  "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
	  "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
	  "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
	  "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
	  "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7"};

	const char *filtername[] =
	 {"Off", "Low", "Bnd", "L+B", "Hi ", "L+H", "B+H", "LBH"};

	unsigned char freqtbllo[] = {
	  0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
	  0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
	  0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
	  0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
	  0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
	  0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
	  0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
	  0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff};

	unsigned char freqtblhi[] = {
	  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
	  0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
	  0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
	  0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
	  0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
	  0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
	  0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
	  0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff};
	
	if (format == SID_HISTORY_FORMAT_SIDDUMP)
	{
		byteBuffer->printf("Middle C frequency is $%04X\n\n", freqtbllo[48] | (freqtblhi[48] << 8));
		byteBuffer->printf("| Frame | Freq Note/Abs WF ADSR Pul | Freq Note/Abs WF ADSR Pul | Freq Note/Abs WF ADSR Pul | FCut RC Typ V |");
		//not supported (yet): if (profiling)
		//{ // CPU cycles, Raster lines, Raster lines with badlines on every 8th line, first line included
		//	printf(" Cycl RL RB |");
		//}
		byteBuffer->printf("\n");
		byteBuffer->printf("+-------+---------------------------+---------------------------+---------------------------+---------------+");
	//	 if (profiling)
	//	 {
	//	   printf("------------+");
	//	 }
		byteBuffer->printf("\n");
	}
	else if (format == SID_HISTORY_FORMAT_CSV)
	{
		byteBuffer->printf("Frame, Freq, Note/Abs, WF, ADSR, Pul, Freq, Note/Abs, WF, ADSR, Pul, Freq, Note/Abs, WF, ADSR, Pul, FCut, RC, Typ, V\n");
	}

	//
	int c;

	// configuration
	int frames = 0;
	int firstframe = 0;
	int counter = 0;
	int rows = 0;
	int sidNum = 0;

	// Recalibrate frequencytable
	if (basefreq)
	{
		basenote &= 0x7f;
		if ((basenote < 0) || (basenote > 96))
		{
			LOGError("Warning: Calibration note out of range. Aborting recalibration.\n");
		}
		else
		{
			for (c = 0; c < 96; c++)
			{
				double note = c - basenote;
				double freq = (double)basefreq * pow(2.0, note/12.0);
				int f = freq;
				if (freq > 0xffff) freq = 0xffff;
				freqtbllo[c] = f & 0xff;
				freqtblhi[c] = f >> 8;
			}
		}
	}
	
	// Check other parameters for correctness
	if ((lowres) && (!spacing)) lowres = 0;

	
	for (std::list<CSidData *>::reverse_iterator it = sidDataHistory->rbegin(); it != sidDataHistory->rend(); it++)
	{
		// Get SID parameters from each channel and the filter
		CSidData *sidData = *it;
		u8 *sidRegs =  sidData->sidRegs[sidNum];
		for (c = 0; c < 3; c++)
		{
			chn[c].freq = sidRegs[0x00 + 7*c] | (sidRegs[0x01 + 7*c] << 8);
			chn[c].pulse = (sidRegs[0x02 + 7*c] | (sidRegs[0x03 + 7*c] << 8)) & 0xfff;
			chn[c].wave = sidRegs[0x04 + 7*c];
			chn[c].adsr = sidRegs[0x06 + 7*c] | (sidRegs[0x05 + 7*c] << 8);
		}
		filt.cutoff = (sidRegs[0x15] << 5) | (sidRegs[0x16] << 8);
		filt.ctrl = sidRegs[0x17];
		filt.type = sidRegs[0x18];
		
		// Frame display
		if (frames >= firstframe)
		{
			char output[512];
			int time = frames - firstframe;
			output[0] = 0;
			
			if (format == SID_HISTORY_FORMAT_SIDDUMP)
			{
				if (!timeseconds)
					sprintf(&output[strlen(output)], "| %5d | ", time);
				else
					sprintf(&output[strlen(output)], "|%01d:%02d.%02d| ", time/3000, (time/50)%60, time%50);
			}
			else
			{
				if (!timeseconds)
					sprintf(&output[strlen(output)], "%5d, ", time);
				else
					sprintf(&output[strlen(output)], "%01d:%02d.%02d, ", time/3000, (time/50)%60, time%50);
			}
			
			// Loop for each channel
			for (c = 0; c < 3; c++)
			{
				int newnote = 0;
				
				// Keyoff-keyon sequence detection
				if (chn[c].wave >= 0x10)
				{
					if ((chn[c].wave & 1) && ((!(prevchn2[c].wave & 1)) || (prevchn2[c].wave < 0x10)))
						prevchn[c].note = -1;
				}
				
				// Frequency
				if ((frames == firstframe) || (prevchn[c].note == -1) || (chn[c].freq != prevchn[c].freq))
				{
					int d;
					int dist = 0x7fffffff;
					int delta = ((int)chn[c].freq) - ((int)prevchn2[c].freq);
					
					if (format == SID_HISTORY_FORMAT_SIDDUMP)
					{
						sprintf(&output[strlen(output)], "%04X ", chn[c].freq);
					}
					else
					{
						sprintf(&output[strlen(output)], "%04X, ", chn[c].freq);
					}
					
					if (chn[c].wave >= 0x10)
					{
						// Get new note number
						for (d = 0; d < 96; d++)
						{
							int cmpfreq = freqtbllo[d] | (freqtblhi[d] << 8);
							int freq = chn[c].freq;
							
							if (abs(freq - cmpfreq) < dist)
							{
								dist = abs(freq - cmpfreq);
								// Favor the old note
								if (d == prevchn[c].note) dist /= oldnotefactor;
								chn[c].note = d;
							}
						}
						
						// Print new note
						if (chn[c].note != prevchn[c].note)
						{
							if (format == SID_HISTORY_FORMAT_SIDDUMP)
							{
								if (prevchn[c].note == -1)
								{
									if (lowres) newnote = 1;
									sprintf(&output[strlen(output)], " %s %02X  ", notename[chn[c].note], chn[c].note | 0x80);
								}
								else
									sprintf(&output[strlen(output)], "(%s %02X) ", notename[chn[c].note], chn[c].note | 0x80);
							}
							else if (format == SID_HISTORY_FORMAT_CSV)
							{
								if (prevchn[c].note == -1)
								{
									if (lowres) newnote = 1;
									sprintf(&output[strlen(output)], " %s %02X,  ", notename[chn[c].note], chn[c].note | 0x80);
								}
								else
									sprintf(&output[strlen(output)], "(%s %02X), ", notename[chn[c].note], chn[c].note | 0x80);
							}
						}
						else
						{
							if (format == SID_HISTORY_FORMAT_SIDDUMP)
							{
								// If same note, print frequency change (slide/vibrato)
								if (delta)
								{
									if (delta > 0)
										sprintf(&output[strlen(output)], "(+ %04X) ", delta);
									else
										sprintf(&output[strlen(output)], "(- %04X) ", -delta);
								}
								else sprintf(&output[strlen(output)], " ... ..  ");
							}
							else if (format == SID_HISTORY_FORMAT_CSV)
							{
								// If same note, print frequency change (slide/vibrato)
								if (delta)
								{
									if (delta > 0)
										sprintf(&output[strlen(output)], "(+ %04X), ", delta);
									else
										sprintf(&output[strlen(output)], "(- %04X), ", -delta);
								}
								else sprintf(&output[strlen(output)], ",  ");

							}
						}
					}
					else
					{
						if (format == SID_HISTORY_FORMAT_SIDDUMP)
						{
							sprintf(&output[strlen(output)], " ... ..  ");
						}
						else if (format == SID_HISTORY_FORMAT_CSV)
						{
							sprintf(&output[strlen(output)], ", ");
						}
					}
				}
				else
				{
					if (format == SID_HISTORY_FORMAT_SIDDUMP)
					{
						sprintf(&output[strlen(output)], "....  ... ..  ");
					}
					else
					{
						sprintf(&output[strlen(output)], ", , ");
					}
				}
				
				// Waveform
				if (format == SID_HISTORY_FORMAT_SIDDUMP)
				{
					if ((frames == firstframe) || (newnote) || (chn[c].wave != prevchn[c].wave))
						sprintf(&output[strlen(output)], "%02X ", chn[c].wave);
					else sprintf(&output[strlen(output)], ".. ");
					
					// ADSR
					if ((frames == firstframe) || (newnote) || (chn[c].adsr != prevchn[c].adsr)) sprintf(&output[strlen(output)], "%04X ", chn[c].adsr);
					else sprintf(&output[strlen(output)], ".... ");
					
					// Pulse
					if ((frames == firstframe) || (newnote) || (chn[c].pulse != prevchn[c].pulse)) sprintf(&output[strlen(output)], "%03X ", chn[c].pulse);
					else sprintf(&output[strlen(output)], "... ");
					
					sprintf(&output[strlen(output)], "| ");
				}
				else if (format == SID_HISTORY_FORMAT_CSV)
				{
					if ((frames == firstframe) || (newnote) || (chn[c].wave != prevchn[c].wave))
						sprintf(&output[strlen(output)], "%02X, ", chn[c].wave);
					else sprintf(&output[strlen(output)], ", ");
					
					// ADSR
					if ((frames == firstframe) || (newnote) || (chn[c].adsr != prevchn[c].adsr)) sprintf(&output[strlen(output)], "%04X, ", chn[c].adsr);
					else sprintf(&output[strlen(output)], ", ");
					
					// Pulse
					if ((frames == firstframe) || (newnote) || (chn[c].pulse != prevchn[c].pulse)) sprintf(&output[strlen(output)], "%03X, ", chn[c].pulse);
					else sprintf(&output[strlen(output)], ", ");
				}
			}
			
			if (format == SID_HISTORY_FORMAT_SIDDUMP)
			{
				// Filter cutoff
				if ((frames == firstframe) || (filt.cutoff != prevfilt.cutoff)) sprintf(&output[strlen(output)], "%04X ", filt.cutoff);
				else sprintf(&output[strlen(output)], ".... ");
				
				// Filter control
				if ((frames == firstframe) || (filt.ctrl != prevfilt.ctrl))
					sprintf(&output[strlen(output)], "%02X ", filt.ctrl);
				else sprintf(&output[strlen(output)], ".. ");
				
				// Filter passband
				if ((frames == firstframe) || ((filt.type & 0x70) != (prevfilt.type & 0x70)))
					sprintf(&output[strlen(output)], "%s ", filtername[(filt.type >> 4) & 0x7]);
				else sprintf(&output[strlen(output)], "... ");
				
				// Mastervolume
				if ((frames == firstframe) || ((filt.type & 0xf) != (prevfilt.type & 0xf))) sprintf(&output[strlen(output)], "%01X ", filt.type & 0xf);
				else sprintf(&output[strlen(output)], ". ");
				
				//			  // Rasterlines / cycle count
				//			  if (profiling)
				//			  {
				//				int cycles = cpucycles;
				//				int rasterlines = (cycles + 62) / 63;
				//				int badlines = ((cycles + 503) / 504);
				//				int rasterlinesbad = (badlines * 40 + cycles + 62) / 63;
				//				sprintf(&output[strlen(output)], "| %4d %02X %02X ", cycles, rasterlines, rasterlinesbad);
				//			  }
				
				// End of frame display, print info so far and copy SID registers to old registers
				sprintf(&output[strlen(output)], "|\n");
				if ((!lowres) || (!((frames - firstframe) % spacing)))
				{
					byteBuffer->printf("%s", output);
					for (c = 0; c < 3; c++)
					{
						prevchn[c] = chn[c];
					}
					prevfilt = filt;
				}
				for (c = 0; c < 3; c++) prevchn2[c] = chn[c];
				
				// Print note/pattern separators
				if (spacing)
				{
					counter++;
					if (counter >= spacing)
					{
						counter = 0;
						if (pattspacing)
						{
							rows++;
							if (rows >= pattspacing)
							{
								rows = 0;
								byteBuffer->printf("+=======+===========================+===========================+===========================+===============+\n");
							}
							else
								if (!lowres)
								{
									byteBuffer->printf("+-------+---------------------------+---------------------------+---------------------------+---------------+\n");
								}
						}
						else
							if (!lowres)
							{
								byteBuffer->printf("+-------+---------------------------+---------------------------+---------------------------+---------------+\n");
							}
					}
				}
			}
			else if (format == SID_HISTORY_FORMAT_CSV)
			{
				// Filter cutoff
				if ((frames == firstframe) || (filt.cutoff != prevfilt.cutoff)) sprintf(&output[strlen(output)], "%04X, ", filt.cutoff);
				else sprintf(&output[strlen(output)], ", ");
				
				// Filter control
				if ((frames == firstframe) || (filt.ctrl != prevfilt.ctrl))
					sprintf(&output[strlen(output)], "%02X, ", filt.ctrl);
				else sprintf(&output[strlen(output)], ", ");
				
				// Filter passband
				if ((frames == firstframe) || ((filt.type & 0x70) != (prevfilt.type & 0x70)))
					sprintf(&output[strlen(output)], "%s, ", filtername[(filt.type >> 4) & 0x7]);
				else sprintf(&output[strlen(output)], ", ");
				
				// Mastervolume
				if ((frames == firstframe) || ((filt.type & 0xf) != (prevfilt.type & 0xf))) sprintf(&output[strlen(output)], "%01X, ", filt.type & 0xf);
				else sprintf(&output[strlen(output)], ", ");
				
				//			  // Rasterlines / cycle count
				//			  if (profiling)
				//			  {
				//				int cycles = cpucycles;
				//				int rasterlines = (cycles + 62) / 63;
				//				int badlines = ((cycles + 503) / 504);
				//				int rasterlinesbad = (badlines * 40 + cycles + 62) / 63;
				//				sprintf(&output[strlen(output)], "| %4d %02X %02X ", cycles, rasterlines, rasterlinesbad);
				//			  }
				
				// End of frame display, print info so far and copy SID registers to old registers
				sprintf(&output[strlen(output)], "\n");
				if ((!lowres) || (!((frames - firstframe) % spacing)))
				{
					byteBuffer->printf("%s", output);
					for (c = 0; c < 3; c++)
					{
						prevchn[c] = chn[c];
					}
					prevfilt = filt;
				}
				for (c = 0; c < 3; c++) prevchn2[c] = chn[c];
			}

		}
		
		// Advance to next frame
		frames++;
	}
}


static inline std::string Trim(const std::string &s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
		++start;

	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
		--end;

	return s.substr(start, end - start);
}

static std::vector<std::string> SplitTokens(const std::string &line)
{
	std::vector<std::string> tokens;
	std::istringstream iss(line);
	std::string tok;
	while (iss >> tok)
		tokens.push_back(tok);
	return tokens;
}

static bool IsNumber(const std::string &s)
{
	if (s.empty())
		return false;
	for (char c : s)
	{
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
	}
	return true;
}

// token == ".." / "...." / "." etc.
static bool IsDots(const std::string &s)
{
	if (s.empty())
		return false;
	for (char c : s)
	{
		if (c != '.')
			return false;
	}
	return true;
}

// Safe hex parsers with exception handling
static bool TryParseHex8(const std::string &s, uint8_t &outValue)
{
	try
	{
		unsigned long v = std::stoul(s, nullptr, 16);
		if (v > 0xFF)
			return false;
		outValue = static_cast<uint8_t>(v & 0xFFu);
		return true;
	}
	catch (const std::exception &)
	{
		return false;
	}
}

static bool TryParseHex16(const std::string &s, uint16_t &outValue)
{
	try
	{
		unsigned long v = std::stoul(s, nullptr, 16);
		if (v > 0xFFFF)
			return false;
		outValue = static_cast<uint16_t>(v & 0xFFFFu);
		return true;
	}
	catch (const std::exception &)
	{
		return false;
	}
}


// this loads siddump style file, note is not parsed - only frequency
void C64SIDHistoryFromByteBuffer(std::list<CSidData *> *sidDataHistory, CByteBuffer *byteBuffer, u8 format, bool keepPreviousValueWhenZero)
{
	LOGD("C64SIDHistoryFromByteBuffer");
	if (format == SID_HISTORY_FORMAT_CSV)
	{
		LOGTODO("CSV format for SID history is not supported yet");
		return;
	}
	
	// TODO: delete objs
	sidDataHistory->clear();
	
	// Copy whole buffer to string
	std::string text;
	text.reserve(4096);

	while (!byteBuffer->IsEof())
	{
		uint8_t b = byteBuffer->GetByte();
		text.push_back(static_cast<char>(b));
	}

	std::istringstream input(text);
	std::string line;

	CSidData *prevSid = sidDataHistory->empty() ? nullptr : sidDataHistory->back();

	LOGD("C64SIDHistoryFromByteBuffer: parse lines");
	while (std::getline(input, line))
	{
		std::cout << line << std::endl;
		
		line = Trim(line);
		if (line.empty())
			continue;
		if (line[0] != '|')
			continue;

		std::vector<std::string> tokens = SplitTokens(line);
		if (tokens.size() < 10)
			continue;

		if (tokens[0] != "|" || tokens[2] != "|" || !IsNumber(tokens[1]))
			continue;

		const size_t expectedMinTokens = 24 + 5;
		if (tokens.size() < expectedMinTokens)
			continue;

		LOGD("tokens.size=%d", tokens.size());
		
		CSidData *sidData = new CSidData();
		std::memset(sidData->sidRegs, 0, sizeof(sidData->sidRegs));
		u8 *sidRegs = sidData->sidRegs[0];

		u8 *prevRegs = nullptr;
		if (prevSid)
			prevRegs = prevSid->sidRegs[0];

		bool frameOk = true;

		// -------- 3 channels --------
		for (int c = 0; c < 3 && frameOk; ++c)
		{
			// Layout per channel (7 tokens + '|'):
			// base+0: Freq
			// base+1: Note name (ignored)
			// base+2: Note value (ignored)
			// base+3: WF
			// base+4: ADSR
			// base+5: Pul
			// base+6: "|"
			size_t base = 3 + c * 7;

			const std::string &freqStr = tokens[base + 0];
			const std::string &wfStr   = tokens[base + 3];
			const std::string &adsrStr = tokens[base + 4];
			const std::string &pulStr  = tokens[base + 5];

			bool freqDots = IsDots(freqStr);
			bool wfDots   = IsDots(wfStr);
			bool adsrDots = IsDots(adsrStr);
			bool pulDots  = IsDots(pulStr);

			uint16_t freq = 0;
			uint8_t  wave = 0;
			uint16_t adsr = 0;
			uint16_t pulse = 0;

			// Freq
			if (freqDots && prevRegs)
			{
				int regBasePrev = c * 7;
				freq = static_cast<uint16_t>(
					prevRegs[regBasePrev + 0] |
					(static_cast<uint16_t>(prevRegs[regBasePrev + 1]) << 8));
			}
			else if (!freqDots)
			{
				if (!TryParseHex16(freqStr, freq))
				{
					frameOk = false;
					break;
				}
			}

			// Waveform
			if (wfDots && prevRegs)
			{
				int regBasePrev = c * 7;
				wave = prevRegs[regBasePrev + 4];
			}
			else if (!wfDots)
			{
				if (!TryParseHex8(wfStr, wave))
				{
					frameOk = false;
					break;
				}
			}

			// ADSR
			if (adsrDots && prevRegs)
			{
				int regBasePrev = c * 7;
				adsr = static_cast<uint16_t>(
					(static_cast<uint16_t>(prevRegs[regBasePrev + 5]) << 8) |
					prevRegs[regBasePrev + 6]);
			}
			else if (!adsrDots)
			{
				if (!TryParseHex16(adsrStr, adsr))
				{
					frameOk = false;
					break;
				}
			}

			// Pulse
			if (pulDots && prevRegs)
			{
				int regBasePrev = c * 7;
				pulse = static_cast<uint16_t>(
					prevRegs[regBasePrev + 2] |
					((prevRegs[regBasePrev + 3] & 0x0F) << 8));
			}
			else if (!pulDots)
			{
				if (!TryParseHex16(pulStr, pulse))
				{
					frameOk = false;
					break;
				}
			}

			pulse &= 0x0FFFu;

			int regBase = c * 7;

			// keepPreviousValueWhenZero (only for literal zero, after dots have been handled)
			if (keepPreviousValueWhenZero && prevRegs)
			{
				if (freq == 0)
				{
					uint16_t prevFreq = static_cast<uint16_t>(
						prevRegs[regBase + 0] |
						(static_cast<uint16_t>(prevRegs[regBase + 1]) << 8));
					freq = prevFreq;
				}

				if (pulse == 0)
				{
					uint16_t prevPulse = static_cast<uint16_t>(
						prevRegs[regBase + 2] |
						((prevRegs[regBase + 3] & 0x0F) << 8));
					pulse = prevPulse;
				}

				if (wave == 0)
				{
					uint8_t prevWave = prevRegs[regBase + 4];
					wave = prevWave;
				}

				if (adsr == 0)
				{
					uint16_t prevAdsr = static_cast<uint16_t>(
						(static_cast<uint16_t>(prevRegs[regBase + 5]) << 8) |
						prevRegs[regBase + 6]);
					adsr = prevAdsr;
				}
			}

			// Write SID registers
			sidRegs[regBase + 0] = static_cast<u8>(freq & 0xFF);
			sidRegs[regBase + 1] = static_cast<u8>((freq >> 8) & 0xFF);

			sidRegs[regBase + 2] = static_cast<u8>(pulse & 0xFF);
			sidRegs[regBase + 3] = static_cast<u8>((pulse >> 8) & 0x0F);

			sidRegs[regBase + 4] = wave;

			sidRegs[regBase + 5] = static_cast<u8>((adsr >> 8) & 0xFF);
			sidRegs[regBase + 6] = static_cast<u8>(adsr & 0xFF);
		}

		if (!frameOk)
		{
			delete sidData;
			continue;
		}

		// -------- filter part --------
		size_t filterBase = 3 + 3 * 7;

		const std::string &fcutStr = tokens[filterBase + 0];
		const std::string &rcStr   = tokens[filterBase + 1];
		std::string typToken       = tokens[filterBase + 2];
		const std::string &volStr  = tokens[filterBase + 3];

		bool fcutDots = IsDots(fcutStr);
		bool rcDots   = IsDots(rcStr);
		bool volDots  = IsDots(volStr);
		bool typDots  = IsDots(typToken);

		uint16_t fcut = 0;
		uint8_t rc    = 0;
		uint8_t vol   = 0;

		if (fcutDots && prevRegs)
		{
			fcut = static_cast<uint16_t>(
				prevRegs[0x15] | (static_cast<uint16_t>(prevRegs[0x16]) << 8));
		}
		else if (!fcutDots)
		{
			if (!TryParseHex16(fcutStr, fcut))
			{
				delete sidData;
				continue;
			}
			
			LOGD("fcut=%04x", fcut);
		}

		if (rcDots && prevRegs)
		{
			rc = prevRegs[0x17];
		}
		else if (!rcDots)
		{
			if (!TryParseHex8(rcStr, rc))
			{
				delete sidData;
				continue;
			}
		}

		if (volDots && prevRegs)
		{
			uint8_t prevModeVol = prevRegs[0x18];
			vol = prevModeVol & 0x0F;
		}
		else if (!volDots)
		{
			if (!TryParseHex8(volStr, vol))
			{
				delete sidData;
				continue;
			}
		}
		vol &= 0x0F;

		if (keepPreviousValueWhenZero && prevRegs)
		{
			if (fcut == 0)
			{
				uint16_t prevFcut = static_cast<uint16_t>(
					prevRegs[0x15] | (static_cast<uint16_t>(prevRegs[0x16]) << 8));
				fcut = prevFcut;
			}

			if (rc == 0)
			{
				uint8_t prevRc = prevRegs[0x17];
				rc = prevRc;
			}

			if (vol == 0)
			{
				uint8_t prevModeVol = prevRegs[0x18];
				uint8_t prevVol = prevModeVol & 0x0F;
				vol = prevVol;
			}
		}

		sidRegs[0x15] = static_cast<u8>(fcut & 0xFF);
		sidRegs[0x16] = static_cast<u8>((fcut >> 8) & 0xFF);
		sidRegs[0x17] = rc;

		std::string typUpper = typToken;
		std::transform(typUpper.begin(), typUpper.end(), typUpper.begin(),
					   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

		uint8_t modeBits = 0;

		if (typDots && prevRegs)
		{
			uint8_t prevModeVol = prevRegs[0x18];
			modeBits = prevModeVol & 0xF0;
		}
		else if (keepPreviousValueWhenZero && prevRegs && typUpper == "NONE")
		{
			uint8_t prevModeVol = prevRegs[0x18];
			modeBits = prevModeVol & 0xF0;
		}
		else
		{
			if (typUpper != "NONE" && typUpper != "OFF")
			{
				if (typUpper.find('L') != std::string::npos)
					modeBits |= (1u << 4);
				if (typUpper.find('B') != std::string::npos)
					modeBits |= (1u << 5);
				if (typUpper.find('H') != std::string::npos)
					modeBits |= (1u << 6);
			}
		}

		uint8_t modeVol = static_cast<uint8_t>((modeBits & 0xF0) | (vol & 0x0F));
		sidRegs[0x18] = modeVol;

		sidDataHistory->push_front(sidData);
		prevSid = sidData;
	}
}


