/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************
*
*          Copyright (c) 1994, HMI, INC. All Rights Reserved
*
*---------------------------------------------------------------------------
*
* FILE
*     soscodec.cpp (was soscodec.asm)
*
* DESCRIPTION
*     HMI SOS ADPCM decompression, fast path for 16-bit mono output.
*
* PROGRAMMER
*     Nick Skrepetos
*     Denzil E. Long, Jr. (Fixed bugs, rewrote for watcom)
*     Bill Petro          (Added stereo support)
*     Jonathan Lanier
*
* DATE
*     Febuary 15, 1995
*
* PORT (S09, 2026-10-14)
*     Replaces SOSCODEC.ASM. Following ADR-0010, the decoder loop is the
*     existing C++ version CODE/ADPCM.CPP (a table-driven IMA ADPCM decoder:
*     combined index = step index * 16 + nibble, DiffTable/IndexTable), with
*     three fixes so it matches the asm it replaces:
*       1. NumBytes counts OUTPUT bytes (the caller, Sample_Copy, passes the
*          uncompressed frame size). ADPCM.CPP treated it as input bytes.
*       2. It returns NumBytes. ADPCM.CPP returned 0 (it shifted the loop
*          counter after counting it down).
*       3. The step index lives in wIndex (0..88) and the predictor in
*          dwPredicted, the fields sosCODECInitStream resets. ADPCM.CPP used
*          dwSampleIndex, which the asm init routine never touched.
*     Instead of copying the 1,424-entry tables from CODE/DTABLE.CPP and
*     CODE/ITABLE.CPP, they are computed from the IMA step table at start-up;
*     tests/s09_decoder_test.cpp checks them against those files.
*     See docs/codebase/audio.md.
*
****************************************************************************/

#include "soundint.h"

namespace {

/*
** DiffTable[index*16 + token]  = signed sample difference for that step and token.
** IndexTable[index*16 + token] = next step index * 16.
** Identical to CODE/DTABLE.CPP and CODE/ITABLE.CPP.
*/
struct SOSFastTables {
	long DiffTable[89 * 16];
	unsigned short IndexTable[89 * 16];

	SOSFastTables() {
		for (int index = 0; index < 89; index++) {
			long step = SOS_Step_Table[index];
			for (int token = 0; token < 16; token++) {
				long diff = step >> 3;
				if (token & 4) diff += step;
				if (token & 2) diff += step >> 1;
				if (token & 1) diff += step >> 2;
				if (token & 8) diff = -diff;
				int next = index + SOS_Index_Adjust[token];
				if (next < 0) next = 0;
				if (next > 88) next = 88;
				DiffTable[index * 16 + token] = diff;
				IndexTable[index * 16 + token] = (unsigned short)(next * 16);
			}
		}
	}
};

SOSFastTables const &Fast_Tables(void)
{
	static SOSFastTables const tables;		// Built once, thread-safe (C++11).
	return(tables);
}

}	// namespace


/*
** For tests: the generated tables (see the PORT note above).
*/
long const *SOS_Fast_Diff_Table(void) { return(Fast_Tables().DiffTable); }
unsigned short const *SOS_Fast_Index_Table(void) { return(Fast_Tables().IndexTable); }


/****************************************************************************
*
* NAME
*     sosCODECInitStream - Initialize compression stream.
*
* SYNOPSIS
*     sosCODECInitStream(CompInfo)
*
*     void sosCODECInitStream(_SOS_COMPRESS_INFO *);
*
* FUNCTION
*     Initialize compression stream for compression and decompression.
*
* NOTES (port)
*     SOSCODEC.ASM and OLSOSDEC.ASM both defined this routine; which copy the
*     Watcom librarian kept is unknown. SOSCODEC's reset wIndex and
*     dwPredicted (both channels); OLSOSDEC's also set wStep = 7 and
*     dwSampleIndex = 0, which the general decoder needs. This port does all
*     of it, so both decoders start from the same state.
*
****************************************************************************/
extern "C" void __cdecl sosCODECInitStream(_SOS_COMPRESS_INFO *info)
{
	info->wIndex = 0;					// starting index 0
	info->wStep = 7;					// start with a step of 7
	info->dwPredicted = 0;			// no predicted value
	info->dwSampleIndex = 0;		// start at head of index
	info->wIndex2 = 0;
	info->wStep2 = 7;
	info->dwPredicted2 = 0;
	info->dwSampleIndex2 = 0;
}


/****************************************************************************
*
* NAME
*     sosCODECDecompressData - Decompress audio data.
*
* SYNOPSIS
*     Size = sosCODECDecompressData(CompInfo, NumBytes)
*
*     long sosCODECDecompressData(_SOS_COMPRESS_INFO *, long);
*
* FUNCTION
*     Decompress data from a 4:1 ADPCM compressed stream to 16-bit mono PCM.
*     NumBytes is the number of OUTPUT bytes, so NumBytes/2 samples are
*     decoded from NumBytes/4 source bytes, low nibble first. The asm only
*     implemented 16-bit mono and returned without decoding (EAX undefined)
*     for any other wBitSize/wChannels; this port returns 0 for those.
*
* RESULT
*     Size - NumBytes (0 for an unsupported format).
*
****************************************************************************/
extern "C" unsigned long __cdecl sosCODECDecompressData(_SOS_COMPRESS_INFO *info, unsigned long wBytes)
{
	if (info->wBitSize != 16 || info->wChannels != 1) {
		return(0);
	}

	SOSFastTables const &tables = Fast_Tables();
	unsigned char const *inbuff = (unsigned char const *)info->lpSource;
	unsigned char *outbuff = (unsigned char *)info->lpDest;

	/*
	** Preload variables before the big loop
	*/
	unsigned int index = (unsigned short)info->wIndex;
	if (index > 88) index = 88;
	unsigned int fastindex = index * 16;
	long sample = info->dwPredicted;
	unsigned long samples = wBytes >> 1;

	for (unsigned long i = 0; i < samples; i++) {
		unsigned token = inbuff[i >> 1];
		token = (i & 1) ? (token >> 4) : (token & 0x0F);

		fastindex += token;
		sample += tables.DiffTable[fastindex];
		fastindex = tables.IndexTable[fastindex];
		if (sample > 32767L) sample = 32767L;
		if (sample < -32768L) sample = -32768L;

		unsigned short value = (unsigned short)(short)sample;
		outbuff[0] = (unsigned char)value;
		outbuff[1] = (unsigned char)(value >> 8);
		outbuff += 2;
	}

	/*
	** Put local vars back
	*/
	info->wIndex = (short)(fastindex / 16);
	info->wStep = (short)SOS_Step_Table[fastindex / 16];
	info->dwPredicted = sample;
	return(wBytes);
}


/****************************************************************************
*
* sosCODEC_Lock / sosCODEC_Unlock -- the asm locked its code and tables with
* DPMI so they could run in an interrupt handler. Nothing to do now.
*
* OUTPUT:      BOOL true is lock sucessful, false otherwise
*
****************************************************************************/
extern "C" int __cdecl sosCODEC_Lock(void)
{
	return(1);
}

extern "C" int __cdecl sosCODEC_Unlock(void)
{
	return(1);
}
