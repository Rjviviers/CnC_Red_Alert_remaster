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
*     olsosdec.cpp (was olsosdec.asm)
*
* DESCRIPTION
*     HMI SOS ADPCM decompression: the general routine for every channel
*     count and bit size (mono/stereo, 8/16-bit output).
*
* PROGRAMMER
*     Nick Skrepetos
*     Denzil E. Long, Jr. (Fixed bugs, rewrote for watcom)
*     Bill Petro          (Added stereo support)
*
* DATE
*     Febuary 15, 1995
*
* PORT (S09, 2026-10-14)
*     C++ translation of OLSOSDEC.ASM (TASM), which this file replaces.
*     The asm also defined sosCODECInitStream, sosCODEC_Lock and
*     sosCODEC_Unlock, duplicating SOSCODEC.ASM; the single C++ copies are in
*     SOSCODEC.CPP. See docs/codebase/audio.md.
*
****************************************************************************/

#include "soundint.h"

/*
** Index table for stepping into step table (wCODECIndexTab).
*/
short const SOS_Index_Adjust[16] = {
	-1,-1,-1,-1,2,4,6,8,
	-1,-1,-1,-1,2,4,6,8
};

/*
** Step table (wCODECStepTab). These are the standard IMA ADPCM step sizes.
*/
unsigned short const SOS_Step_Table[89] = {
	7,		8,		9,		10,		11,		12,		13,		14,
	16,	17,	19,	21,		23,		25,		28,		31,
	34,	37,	41,	45,		50,		55,		60,		66,
	73,	80,	88,	97,		107,		118,		130,		143,
	157,	173,	190,	209,		230,		253,		279,		307,
	337,	371,	408,	449,		494,		544,		598,		658,
	724,	796,	876,	963,		1060,		1166,		1282,		1411,
	1552,	1707,	1878,	2066,		2272,		2499,		2749,		3024,
	3327,	3660,	4026,	4428,		4871,		5358,		5894,		6484,
	7132,	7845,	8630,	9493,		10442,	11487,	12635,	13899,
	15289,	16818,	18500,	20350,	22385,	24623,	27086,	29794,
	32767
};


/*
** One channel's decoder state. The asm keeps two copies of these fields in
** _SOS_COMPRESS_INFO (wIndex/wStep/... and wIndex2/wStep2/...); this struct
** points at one set so the same code serves both channels.
*/
struct SOSChannel {
	unsigned long	*SampleIndex;
	long				*Predicted;
	long				*Difference;
	short				*CodeBuf;
	short				*Code;
	short				*Step;
	short				*Index;
};


/*
** Decode 'samples' samples of one channel. The source advances by
** 'src_stride' bytes per code byte (1 for mono, 2 for stereo, where the left
** and right channels use alternate bytes), and every code byte holds two
** samples, low nibble first. The destination advances by 'dst_stride' bytes.
*/
static void Decode_Channel(SOSChannel const &ch, unsigned char const *src, int src_stride,
									unsigned char *dst, int dst_stride, int bits, unsigned long samples)
{
	while (samples--) {
		unsigned code;

		/*
		**	Determine if sample index is even or odd. This will determine
		**	if we need to get a new token or not.
		*/
		if (*ch.SampleIndex & 1) {
			code = ((unsigned short)*ch.CodeBuf >> 4) & 0x0F;
		} else {
			*ch.CodeBuf = (short)*src;
			src += src_stride;
			code = (unsigned)*ch.CodeBuf & 0x0F;
		}
		*ch.Code = (short)code;

		/*
		** Calculate the difference: step*code/4 + step/8, built from shifts
		** exactly like the asm.
		*/
		unsigned long step = (unsigned short)*ch.Step;
		long diff = 0;
		if (code & 4) diff += (long)step;
		if (code & 2) diff += (long)(step >> 1);
		if (code & 1) diff += (long)(step >> 2);
		diff += (long)(step >> 3);
		if (code & 8) diff = -diff;
		*ch.Difference = diff;

		/*
		** Add difference to predicted value and make sure there is no
		** under or overflow.
		*/
		long sample = *ch.Predicted + diff;
		if (sample > 32767) sample = 32767;
		if (sample < -32768) sample = -32768;
		*ch.Predicted = sample;

		if (bits == 16) {
			short value = (short)sample;
			dst[0] = (unsigned char)value;
			dst[1] = (unsigned char)((unsigned short)value >> 8);
		} else {

			/*
			** Output 8 bit sample: the high byte, converted to unsigned.
			*/
			dst[0] = (unsigned char)((((unsigned long)sample >> 8) & 0xFF) ^ 0x80);
		}
		dst += dst_stride;

		/*
		** Adjust the step index: below zero (unsigned 16-bit compare in the
		** asm) resets to 0, above 88 clamps to 88.
		*/
		unsigned short index = (unsigned short)(*ch.Index + SOS_Index_Adjust[code]);
		if (index >= 0x8000) {
			index = 0;
		} else if (index > 88) {
			index = 88;
		}
		*ch.Index = (short)index;

		/*
		** Advance index and store step value.
		*/
		*ch.SampleIndex += 1;
		*ch.Step = (short)SOS_Step_Table[index];
	}
}


/****************************************************************************
*
* NAME
*     General_sosCODECDecompressData - Decompress audio data.
*
* SYNOPSIS
*     Size = General_sosCODECDecompressData(CompInfo, NumBytes)
*
*     long General_sosCODECDecompressData(_SOS_COMPRESS_INFO *, long);
*
* FUNCTION
*     Decompress data from a 4:1 ADPCM compressed stream. The number of
*     bytes decompressed is returned.
*
* INPUTS
*     CompInfo - Compress information structure. lpSource/lpDest point at the
*                data; wBitSize (8 or 16) and wChannels (1 or 2) pick the
*                output format; the predictor/step state carries over from
*                the previous call.
*     NumBytes - Number of bytes to decompress, measured at the OUTPUT.
*
* RESULT
*     Size - NumBytes.
*
* NOTES (port)
*     - The nibble position (dwSampleIndex, dwSampleIndex2) restarts at zero on
*       every call, so every call starts on a fresh source byte.
*     - Stereo source data alternates bytes: even bytes are the left channel,
*       odd bytes the right; each byte holds two samples of its channel.
*     - The asm looped "decrement, until zero" without checking for a zero
*       count first, so NumBytes == 0 (or an odd stereo count) ran away. The
*       port returns immediately for 0 and rounds odd stereo counts down.
*
****************************************************************************/
extern "C" unsigned long __cdecl General_sosCODECDecompressData(_SOS_COMPRESS_INFO *info, unsigned long wBytes)
{
	info->dwSampleIndex = 0;		// start at head of index
	info->dwSampleIndex2 = 0;

	unsigned long count = wBytes;
	int const bits = (info->wBitSize == 16) ? 16 : 8;
	if (info->wBitSize == 16) count >>= 1;		// Divide size by two: now in samples.
	if (count == 0) return(wBytes);

	unsigned char const *src = (unsigned char const *)info->lpSource;
	unsigned char *dst = (unsigned char *)info->lpDest;
	int const sample_bytes = bits / 8;

	SOSChannel left = {
		&info->dwSampleIndex, &info->dwPredicted, &info->dwDifference,
		&info->wCodeBuf, &info->wCode, &info->wStep, &info->wIndex
	};

	if (info->wChannels == 2) {
		SOSChannel right = {
			&info->dwSampleIndex2, &info->dwPredicted2, &info->dwDifference2,
			&info->wCodeBuf2, &info->wCode2, &info->wStep2, &info->wIndex2
		};
		unsigned long per_channel = count / 2;

		/*
		** Left channel first, then the right channel (skip one source byte
		** and one output sample).
		*/
		Decode_Channel(left, src, 2, dst, 2 * sample_bytes, bits, per_channel);
		Decode_Channel(right, src + 1, 2, dst + sample_bytes, 2 * sample_bytes, bits, per_channel);
	} else {
		Decode_Channel(left, src, 1, dst, sample_bytes, bits, count);
	}

	/*
	** Set up return value for number of bytes processed.
	*/
	return(wBytes);
}
