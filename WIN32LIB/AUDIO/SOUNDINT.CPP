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

/***************************************************************************
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : SOUNDINT.CPP                             *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : June 23, 1995                            *
 *                                                                         *
 *                  Last Update : June 28, 1995   [PWG]                    *
 *                                                                         *
 * This module contains all of the functions that are used within our      *
 * sound interrupt.  They are stored in a seperate module because memory	*
 * around these functions must be locked or they will cause a read to		*
 * be generated while in an interrupt.													*
 *                                                                         *
 * Port (S09, 2026-10-14): these functions now run on the mixer's side:    *
 * Audio_Voice_Fill is the mixer's fill callback (it replaced the          *
 * DirectSound buffer refill in maintenance_callback), and                 *
 * maintenance_callback is the mixer's 40 Hz tick (fades, stream queue).   *
 * Both run with the mixer lock held, possibly on the audio thread.        *
 * See docs/codebase/audio.md.                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Simple_Copy -- Copyies 1 or 2 source chuncks to a dest                *
 *   Sample_Copy -- Copies sound data from source format to raw format.    *
 *   Audio_Promote_Stream_Queue -- Attach the next streamed block.         *
 *   Audio_Voice_Fill -- Mixer fill callback.                              *
 *   maintenance_callback -- 40 Hz service: fades and stream queue.        *
 *   Convert_HMI_To_Direct_Sound_Volume -- Linear volume to hundredths dB  *
 *   Audio_Volume_To_Gain -- Linear volume to mixer gain                   *
 *   Audio_Update_Voice_Volume -- Push a tracker's volume to the mixer     *
 *   Audio_Add_Long_To_Pointer -- Adds an offset to a ptr casted void      *
 *   Audio_Mem_Set -- Quick see routine to set memory to a value           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "audio_compat.h"
#include "audio.h"
#include "soundint.h"
#include "audio_mixer.h"

#include <math.h>
#include <string.h>


/***************************************************************************
 * SIMPLE_COPY -- Copyies 1 or 2 source chuncks to a dest                  *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/23/1995 PWG : Created.                                             *
 *=========================================================================*/
long Simple_Copy(void ** source, long * ssize, void ** alternate, long * altsize, void **dest, long size)
{
	long	out = 0;		// Number of bytes copied to the destination.

	/*
	**	It could happen that entering this routine, the source buffer
	**	has been exhausted, but the alternate buffer is still valid.
	**	Move the alternate into the primary position before proceeding.
	*/
	if (!(*ssize)) {
		*source = *alternate;
		*ssize = *altsize;
		*alternate = 0;
		*altsize = 0;
	}

	if (*source && *ssize) {
		long	s;				// Scratch length var.

		/*
		**	Copy as much as possible from the primary source, but no
		**	more than the primary source has to offer.
		*/
		s = size;
		if (*ssize < s) s = *ssize;
		memcpy(*dest, *source, (size_t)s);
		*source = Audio_Add_Long_To_Pointer(*source, s);
		*ssize -= s;
		*dest = Audio_Add_Long_To_Pointer(*dest, s);
		size -= s;
		out += s;

		/*
		**	If the primary source was insufficient to fill the request, then
		**	move the alternate into the primary position and try again.
		*/
		if (size) {
			*source = *alternate;
			*ssize = *altsize;
			*alternate = 0;
			*altsize = 0;
			out += Simple_Copy(source, ssize, alternate, altsize, dest, size);
		}
	}

	return(out);
}


/***********************************************************************************************
 * Sample_Copy -- Copies sound data from source format to raw format.                          *
 *                                                                                             *
 *    This routine is used to copy the sound data (possibly compressed) to the destination     *
 *    buffer in raw format.                                                                    *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the source data (possibly compressed).                      *
 *                                                                                             *
 *          dest     -- Pointer to the destination buffer.                                     *
 *                                                                                             *
 *          size     -- The size of the destination buffer.                                    *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes placed into the output buffer.  This is usually   *
 *          the number of bytes requested except in the case when the source is exhausted.     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * PORT NOTES (S09):                                                                           *
 *    The original consumed a frame header and then gave up if the frame didn't fit in        *
 *    'size' or wasn't fully available yet (a streamed block not loaded in time). Either case  *
 *    desynchronised the stream. The port rewinds the source to the frame start instead, so   *
 *    the frame is decoded by the next call. Well-formed, fully loaded data decodes exactly    *
 *    as before. Frames larger than the staging buffer are rejected instead of overflowing it. *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/03/1994 JLB : Created.                                                                 *
 *   09/04/1994 JLB : Revamped entirely.                                                       *
 *=============================================================================================*/
long Sample_Copy(SampleTrackerType *st, void ** source, long * ssize, void ** alternate, long * altsize, void * dest, long size, SCompressType scomp, void * , short int *)
{
	long	s;
	long	datasize = 0;		// Output bytes.

	switch (scomp) {
		default:

		case SCOMP_NONE:
			datasize = Simple_Copy(source, ssize, alternate, altsize, &dest, size);
			break;

		case SCOMP_WESTWOOD:
		case SCOMP_SOS:
			while (size > 0) {

				/*
				** Remember where this frame starts, so it can be retried.
				*/
				void *save_source = *source;
				long save_ssize = *ssize;
				void *save_alternate = *alternate;
				long save_altsize = *altsize;

				unsigned char header[AUD_FRAME_HEADER_SIZE];
				void *hptr = header;
				s = Simple_Copy(source, ssize, alternate, altsize, &hptr, AUD_FRAME_HEADER_SIZE);

				unsigned fsize = Aud_Get_U16(header);
				unsigned dsize = Aud_Get_U16(header + 2);
				unsigned long magic = Aud_Get_U32(header + 4);

				if (s < AUD_FRAME_HEADER_SIZE || size < (long)dsize || magic != (unsigned long)LockedData.MagicNumber || fsize > UNCOMP_BUFFER_SIZE) {
					*source = save_source;
					*ssize = save_ssize;
					*alternate = save_alternate;
					*altsize = save_altsize;
					return(datasize);
				}

				/*
				**	If the frame and uncompressed data size are identical, then this
				**	indicates that the frame is not compressed.  Just copy it directly
				**	to the destination buffer in this case.
				*/
				if (fsize == dsize) {
					s = Simple_Copy(source, ssize, alternate, altsize, &dest, fsize);
					if (s < (long)dsize) {
						return (datasize);
					}
				} else {

					/*
					**	The frame was compressed, so copy it to the staging buffer, and then
					**	uncompress it into the final destination buffer.
					*/
					void *fptr = LockedData.UncompBuffer;
					s = Simple_Copy(source, ssize, alternate, altsize, &fptr, fsize);
					if (s < (long)fsize) {
						*source = save_source;
						*ssize = save_ssize;
						*alternate = save_alternate;
						*altsize = save_altsize;
						return (datasize);
					}
					if (scomp == SCOMP_WESTWOOD) {
						Decompress_Frame(LockedData.UncompBuffer, dest, dsize);
					} else {
						st->sosinfo.lpSource = (char *)LockedData.UncompBuffer;
						st->sosinfo.lpDest	 = (char *)dest;
						if (st->sosinfo.wBitSize==16 && st->sosinfo.wChannels==1){
							sosCODECDecompressData(&st->sosinfo, dsize);
						} else {
							General_sosCODECDecompressData(&st->sosinfo, dsize);
						}
					}
					dest = Audio_Add_Long_To_Pointer(dest, dsize);
				}
				datasize += dsize;
				size -= dsize;
			}

			break;
	}
	return(datasize);
}


/***************************************************************************
 * AUDIO_PROMOTE_STREAM_QUEUE -- Attach the next streamed block            *
 *                                                                         *
 *	For file streamed samples, fill the queue pointer if needed.            *
 *	This allows for delays in calling the Sound_Callback function.          *
 * (Was part of maintenance_callback; the fill callback calls it too.)     *
 *                                                                         *
 * Call with the mixer lock held.                                          *
 *=========================================================================*/
void Audio_Promote_Stream_Queue(SampleTrackerType *st)
{
	if (!st->QueueBuffer && st->FilePending) {
		st->QueueBuffer = Audio_Add_Long_To_Pointer(st->FileBuffer, (long)(st->Odd%LockedData.StreamBufferCount)*(long)LockedData.StreamBufferSize);
		st->FilePending--;
		st->Odd++;
		if (!st->FilePending) {
			st->QueueSize = st->FilePendingSize;
		} else {
			st->QueueSize = LockedData.StreamBufferSize;
		}
	}
}


/***************************************************************************
 * AUDIO_VOICE_FILL -- Mixer fill callback                                 *
 *                                                                         *
 * Copies (decompressing as needed) up to 'size' bytes of the sample on    *
 * tracker 'voice' into 'dest'. This is what maintenance_callback used to  *
 * do for a quarter of the DirectSound secondary buffer.                   *
 *                                                                         *
 * OUTPUT:  Bytes written; 0 when the sample has no more data.             *
 *                                                                         *
 * WARNINGS: Runs with the mixer lock held, maybe on the audio thread.     *
 *=========================================================================*/
long Audio_Voice_Fill(void *, int voice, void *dest, long size)
{
	if ((unsigned)voice >= MAX_SFX) return(0);
	SampleTrackerType *st = &LockedData.SampleTracker[voice];
	if (!st->Active) return(0);

	Audio_Promote_Stream_Queue(st);
	return(Sample_Copy(st,
							&st->Source,
							&st->Remainder,
							&st->QueueBuffer,
							&st->QueueSize,
							dest,
							size,
							st->Compression,
							&st->Trailer[0],
							&st->TrailerLen));
}


/***********************************************************************************************
 * maintenance_callback -- routine to service the samples 40 times a second                    *
 *                                                                                             *
 * Port: called by the mixer every 1/MAINTENANCE_RATE seconds of output. The DirectSound      *
 * buffer refill it used to do is Audio_Voice_Fill now.                                        *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: Runs with the mixer lock held, maybe on the audio thread.                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *     ....Unknown                                                                             *
 *    10/17/95 10:15PM ST : tidied up a tad for direct sound                                   *
 *=============================================================================================*/
void maintenance_callback(void)
{
	int					index;				//index used in for loop
	SampleTrackerType	*st;					//ptr to SampleTracker structure

	st = &LockedData.SampleTracker[0];
	for (index = 0; index < MAX_SFX; index++) {
		if (st->Active) {
			Audio_Promote_Stream_Queue(st);
		}
		st++;
	}

	if (!LockedData._int) {

		LockedData._int++;
		st = &LockedData.SampleTracker[0];
		for (index = 0; index < MAX_SFX; index++) {

			/*
			**	If there are any samples that require fading, then do so at this
			**	time.
			*/
			if (st->Active && st->Reducer && st->Volume) {
				if (st->Reducer >= st->Volume) {
					st->Volume = 0;
				} else {
					st->Volume -= st->Reducer;
				}
				Audio_Update_Voice_Volume(index);
			}
			st++;
		}
		LockedData._int--;
	}
}


/***********************************************************************************************
 * Convert_HMI_To_Direct_Sound_Volume -- Converts a linear volume value into an expotential    *
 *                                        value                                                *
 *                                                                                             *
 *  This function converts a linear C&C volume in the range 0-255 (255 loudest) to a direct     *
 *  sound volume in the range 0 to -10000 (with 0 being the loadest)                           *
 *                                                                                             *
 * INPUT:    volume in range 0-255                                                             *
 *                                                                                             *
 * OUTPUT:   volume in range -10000 to 0                                                       *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * Note: The 27.685 value comes from 255 divided by the log of 10001                           *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/18/96 11:36AM ST : Created                                                             *
 *    (S09: moved here from SOUNDIO.CPP, unchanged)                                            *
 *=============================================================================================*/
int Convert_HMI_To_Direct_Sound_Volume(int volume)
{
	if (volume == 0) return -10000;
	if (volume == 255) return 0;

	float vol = (float)volume;
	float retval = exp ( (255.0-vol)/27.68597374) -1;
	return ((int)-retval);
}


/***************************************************************************
 * AUDIO_VOLUME_TO_GAIN -- Linear volume to mixer gain                     *
 *                                                                         *
 * DirectSound volumes are attenuations in hundredths of a decibel, so the *
 * amplitude factor of Convert_HMI_To_Direct_Sound_Volume(v) is            *
 * 10^(ds/2000). This keeps the original volume curve.                     *
 *                                                                         *
 * INPUT:   volume 0..255 (clamped)                                        *
 *                                                                         *
 * OUTPUT:  gain, 16.16 fixed point (65536 = unchanged, 0 = silent)        *
 *=========================================================================*/
int Audio_Volume_To_Gain(int volume)
{
	struct GainTable {
		int Gain[256];
		GainTable(void) {
			for (int v = 0; v < 256; v++) {
				int ds = Convert_HMI_To_Direct_Sound_Volume(v);
				Gain[v] = (ds <= -10000) ? 0 : (int)(pow(10.0, ds / 2000.0) * 65536.0 + 0.5);
			}
		}
	};
	static GainTable const table;		// Built once, thread-safe (C++11).

	if (volume < 0) volume = 0;
	if (volume > 255) volume = 255;
	return(table.Gain[volume]);
}


/***************************************************************************
 * AUDIO_UPDATE_VOICE_VOLUME -- Push a tracker's volume to the mixer       *
 *                                                                         *
 * Same formula the original passed to IDirectSoundBuffer::SetVolume in    *
 * the fade code and in Set_Score_Vol.                                     *
 *=========================================================================*/
void Audio_Update_Voice_Volume(int index)
{
	SampleTrackerType *st = &LockedData.SampleTracker[index];
	unsigned master = st->IsScore ? LockedData.ScoreVolume : LockedData.SoundVolume;
	Audio_Mixer().Set_Voice_Gain(index, Audio_Volume_To_Gain((int)((master * (unsigned)(st->Volume >> 7)) / 256)));
}


/***************************************************************************
 * ADD_LONG_TO_POINTER -- Adds an offset to a ptr casted void              *
 *                                                                         *
 * INPUT:		void * ptr - the pointer to add to                            *
 *					long size  - the size to add to it								  *
 *                                                                         *
 * OUTPUT:     void * ptr - the new location it will point to              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/23/1995 PWG : Created.                                             *
 *=========================================================================*/
void *Audio_Add_Long_To_Pointer(void const *ptr, long size)
{
	return ((void *) ( (char const *) ptr + size));
}


/***************************************************************************
 * AUDIO_MEM_SET -- Quick see routine to set memory to a value             *
 *                                                                         *
 * INPUT:		void const *  - the memory that needs to be set               *
 *					unsigned char - the value to set the memory to				  *
 *					long size     - how big an area to set							  *
 *                                                                         *
 * OUTPUT:		none                                                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/28/1995 PWG : Created.                                             *
 *=========================================================================*/
extern "C" void __cdecl Audio_Mem_Set(void const *ptr, unsigned char value, long size)
{
	unsigned char *temp = (unsigned char *)ptr;
	for (int lp = 0; lp < size; lp ++) {
		*temp++ = value;
	}
}
