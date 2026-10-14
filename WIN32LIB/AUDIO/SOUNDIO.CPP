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

/* $Id: soundio.cpp 1.41 1994/06/20 15:01:39 joe_bostic Exp $ */
/***********************************************************************************************
 **             C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S             **
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Sound Library                                                *
 *                                                                                             *
 *                    File Name : SOUND.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : July 22, 1991                                                *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Load_Long_Sample -- Loads a sample into XMS for double buffer system.                     *
 *   Read_Long_Sample -- Loads/Processes/Formats/Builds offset.                                             *
 *   Save_Table_Entry -- Put an entry in the offset table.                                     *
 *   Play_Long_Sample -- Calls Init_Long_Sample and Start_Long_Sample.                         *
 *   Start_Long_Sample -- Starts a sample playing that has be initialized.                     *
 *   Get_Table_Entry -- Gets next entry in table.                                              *
 *   Long_Sample_Ticks -- Gets number of ticks in sample if in header.                         *
 *   Long_Sample_Status -- Returns the status of the sample.                                   *
 *   Find_Table_Entry -- Finds next entry in table that matches mask.                          *
 *   Get_Table_Start -- Returns a pointer to first entry in table.                             *
 *   Long_Sample_Ticks_Played -- Number of ticks since sample started.                         *
 *   Install_Sample_Driver_Callback -- Pokes callback function into JM driver                  *
 *   Stop_Long_Sample -- Stops current long sample from playing.                               *
 *   Long_Sample_Loaded_Size -- Max buffer size to load a long sample.                         *
 *   Sound_Callback -- Audio driver callback function.                                         *
 *   DigiCallback -- Low level double buffering handler.                                       *
 *   Load_Sample_Into_Buffer -- Loads a digitized sample into a buffer.                        *
 *   Stream_Sample -- Streams a sample directly from a file.                                   *
 *   Sample_Read -- Reads sample data from an openned file.                                    *
 *   Continue_Sample -- Tags another block of data onto the currently playing.                 *
 *   Sample_Copy -- Copies sound data from source format to raw format.                        *
 *   File_Stream_Preload -- Handles initial proload of a streaming samples bu*
 *   Sample_Length -- returns length of a sample in ticks                  *
 *   (S09: several of the functions listed above no longer exist; see the port note below.)   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
** Port (S09, 2026-10-14): DirectSound is gone. Every sample tracker plays
** through a voice of the software mixer (audio_mixer.h), and the mixer's
** output goes to an AudioBackendClass (audio_backend.h): SDL3, or the null
** backend. The public functions keep their signatures (audio.h). What
** changed, function by function, is listed in docs/codebase/audio.md.
**
** Threading: the backend may call the mixer from its own thread. The mixer's
** recursive lock replaces GlobalAudioCriticalSection: every function here
** that touches LockedData takes it (AudioLockClass). File reads happen on the
** game thread with the lock released, as before.
*/

#include "audio_compat.h"
#include "audio.h"
#include "soundint.h"
#include "audio_mixer.h"
#include "audio_backend.h"

#include <stdlib.h>
#include <string.h>

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

/*
**      If this is defined, then the streaming audio buffer will be filled
**      to maximum whenever filling is to occur. If undefined, it will fill
**      the streaming buffer in smaller chunks.
*/
#define SIMPLE_FILLING

/*
** Define the number of "StreamBufferSize" blocks that are read in
** at a minimum when the streaming sample load callback routine
** is called.  We will IGNORE loads that are less that this in order
** to avoid constant seeking on the CD.
*/
#define STREAM_CUSHION_BLOCKS   4


//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// structs ///////////////////////////////////////

static BOOL				StartingFileStream = FALSE;
int						StreamLowImpact = FALSE;
int							Misc;
SFX_Type					SoundType;
Sample_Type				SampleType;
int							ReverseChannels = FALSE;
void						*FileStreamBuffer = NULL;	//Buffer for streaming audio from CD

/*
** Function to call if we detect focus loss. (Port: kept for the game, which
** sets it; nothing calls it now, because SDL has no "buffer lost" error.)
*/
void (*Audio_Focus_Loss_Function)(void) = NULL;

static BOOL				AudioDone = FALSE;

/*
** Port: the mixer and the open backend.
*/
static AudioMixerClass		Mixer;
static AudioBackendClass	*Backend = NULL;
static bool						DevicePaused = true;

AudioMixerClass &Audio_Mixer(void)
{
	return(Mixer);
}


/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

static int File_Callback(short id, short *odd, void **buffer, long *size);
static int Stream_Sample_Vol(void *buffer, long size, int (*callback)(short id, short *odd, void **buffer, long *size), int volume, int handle);
void File_Stream_Preload(int handle);
void Suspend_Audio_Thread(void);
void Resume_Audio_Thread(void);
/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


/*
** Backend -> mixer, and mixer -> maintenance_callback glue.
*/
static void Mix_Callback(void *user, short *out, int frames)
{
	((AudioMixerClass *)user)->Mix(out, frames);
}

static void Tick_Callback(void *)
{
	maintenance_callback();
}


/***********************************************************************************************
 * Reap_Finished_Samples -- marks trackers whose mixer voice has finished as stopped.         *
 *                                                                                             *
 *    The original's maintenance_callback called Stop_Sample when DirectSound played past the *
 *    end of the data. The mixer runs on the audio thread, where file handles must not be     *
 *    touched, so the game-side functions do that bookkeeping here instead.                    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/14/2026 S09 : Created.                                                                 *
 *=============================================================================================*/
static void Reap_Finished_Samples(void)
{
	AudioLockClass lock(Mixer);
	for (int index = 0; index < MAX_SFX; index++) {
		SampleTrackerType *st = &LockedData.SampleTracker[index];
		if (st->Active && !st->Loading && !Mixer.Is_Voice_Playing(index)) {
			Stop_Sample(index);
		}
	}
}


// This callback is called whenever the queue buffer playback has begun
// and another buffer is needed for queuing up.  Returns TRUE if there
// is more data to read from the file.
static int File_Callback(short id, short *odd, void **buffer, long *size)
{
	SampleTrackerType       *st;            // Pointer to sample playback control struct.
	void                    *ptr;           // Pointer to working portion of file buffer.

	if (id != -1) {
		st = &LockedData.SampleTracker[id];
	 	ptr = st->FileBuffer;
	 	if (ptr) {
			int pending;

	 	  	/*
	 	  	** Move the next pending block into the primary
	 	  	** position.  Do this only if the queue pointer is
	 	  	** null.
	 	  	*/
			{
				AudioLockClass lock(Mixer);
				if (!*buffer && st->FilePending) {
	 	  			*buffer = Audio_Add_Long_To_Pointer(ptr, (long)(*odd % LockedData.StreamBufferCount)*(long)LockedData.StreamBufferSize);
	 	  			st->FilePending--;
	 	  			*odd = (short)(*odd + 1);
	 	  			if (!st->FilePending) {
	 	  				*size = st->FilePendingSize;
	 	  			} else {
	 	  				*size = LockedData.StreamBufferSize;
	 	  			}
	 	  		}
				pending = st->FilePending;
			}

	 	  	/*
	 	  	** If the file handle is still valid, then read in the next
	 	  	** block and add it to the next pending slot available.
	 	  	*/
	 	  	if (pending <
	 	  		(StreamLowImpact ? (LockedData.StreamBufferCount>>1) : ((LockedData.StreamBufferCount-3))) && st->FileHandle != WW_ERROR) {

	 	  		int num_empty_buffers;

#ifdef SIMPLE_FILLING
				num_empty_buffers = (LockedData.StreamBufferCount-2) - pending;
#else
				//
				// num_empty_buffers will be from 1 to StreamBufferCount
				//
				if (StreamLowImpact) {
					num_empty_buffers = MIN((LockedData.StreamBufferCount >> 1)+STREAM_CUSHION_BLOCKS, (LockedData.StreamBufferCount - 2) - pending);
				}
				else {
					num_empty_buffers = (LockedData.StreamBufferCount - 2) - pending;
				}
#endif

				while (num_empty_buffers && (st->FileHandle != WW_ERROR)) {
					int     tofill;
					long    psize;

					/*
					** The mixer may promote a pending block meanwhile; that moves Odd and
					** FilePending by one each in opposite directions, so 'tofill' stays put.
					*/
					{
						AudioLockClass lock(Mixer);
						tofill = (*odd + st->FilePending) % LockedData.StreamBufferCount;
					}

					ptr = Audio_Add_Long_To_Pointer(st->FileBuffer, (long)tofill * (long)LockedData.StreamBufferSize);
					psize = Read_File(st->FileHandle, ptr, LockedData.StreamBufferSize);

				 	/*
				 	**      If less than the requested amount of data was read, this
				 	**      indicates that the source file is exhausted.  Flag the source
				 	**      file as closed so that no further reading is attempted.
				 	*/
				 	if (psize != LockedData.StreamBufferSize) {
				 		Close_File(st->FileHandle);
						st->FileHandle = WW_ERROR;
				 	}

                /*
				 	**      If any real data went into the pending buffer, then flag
				 	**      that this buffer is valid.
                */
				 	if (psize) {
						AudioLockClass lock(Mixer);
				 	  	st->FilePendingSize = psize;
				 	  	st->FilePending++;
				 	}
				 	num_empty_buffers--;
				}

				/*
             **  After filling all pending buffers, check to see if the queue buffer
				**  is empty.  If so, then assign the first available pending buffer to the
             **  queue.
				*/
				{
					AudioLockClass lock(Mixer);
					Audio_Promote_Stream_Queue(st);
				}
			}

			/*
          **      If there are no more buffers that the callback routine
			**      can slot into the primary position, then signal that
			**      no furthur callbacks are needed.
			*/
			AudioLockClass lock(Mixer);
			if (st->FilePending) {
				return(TRUE);
			}
		}
	}
	return(FALSE);
}


// Generic streaming sample playback initialization.
static int Stream_Sample_Vol(void *buffer, long size, int (*callback)(short id, short *odd, void **buffer, long *size), int volume, int handle)
{
	int								playid=-1;      // Sample play ID.
	SampleTrackerType               *st;                    // Working pointer to sample control structure.
	long							oldsize;                // Copy of original sound size.

	if (!AudioDone && buffer && size && LockedData.DigiHandle != -1) {

		/*
	 	** Start the first section of the sound playing. The header's size
	 	** field is patched to the size of this first block for the call.
	 	*/
		AudioLockClass lock(Mixer);
	 	oldsize = AUD_SIZE(buffer);
		Aud_Put_U32((char *)buffer + 2, (unsigned long)(size - AUD_HEADER_SIZE));
	 	playid = Play_Sample_Handle(buffer, 0xFF, volume, 0x0, handle);
		Aud_Put_U32((char *)buffer + 2, (unsigned long)oldsize);

	 	/*
	 	** If the sample actually started playing, then flag this
	 	** sample as a streaming type and signal for a callback
	 	** to occur.
	 	*/
	 	if (playid != -1) {
	 		st = &LockedData.SampleTracker[playid];
	 		st->Callback = callback;
	 		st->Odd = 0;
    	}
	}
	return (playid);
}


/***********************************************************************************************
 * File_Stream_Sample -- Streams a sample directly from a file.                                *
 *                                                                                             *
 *    This will take the file specified and play it directly from disk.                        *
 *    It performs this by allocating a temporary buffer in XMS/EMS and                         *
 *    then keeping this buffer filled by the Sound_Callback() routine.                         *
 *                                                                                             *
 * INPUT:   filename -- The name of the file to play.                                          *
 *                                                                                             *
 * OUTPUT:  Returns the handle to the sound -- just like Play_Sample().                        *
 *                                                                                             *
 * WARNINGS:   The temporary buffer is allocated when this routine is                          *
 *             called and then freed when the sound is finished.  Keep                         *
 *             this in mind.                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int File_Stream_Sample(char const *filename, BOOL real_time_start)
{
	return File_Stream_Sample_Vol(filename, 0xFF, real_time_start);
}


/***************************************************************************
 * FILE_STREAM_PRELOAD -- Handles initial proload of streaming samples     *
 *                                                                         *
 * This function is called before a sample which streams from disk is      *
 * started.  It can be called to either fill the buffer in small chunks    *
 * from the call back routine or to fill the entire buffer at once.  This  *
 * is wholely dependant on whether the Loading bit is set within the       *
 * sample tracker.                                                         *
 *                                                                         *
 * INPUT:LockedData.SampleTracker * to the header which tracks this samples*
 *                                                              processing.*
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS: (Port) The reads happen without the lock: the tracker isn't   *
 *           playing yet, so the mixer doesn't look at it. Starting the    *
 *           sample and setting up its queue is one locked step, so the    *
 *           mixer can't pull from the first block before the queue exists.*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/05/1995 PWG : Created.                                             *
 *=========================================================================*/
void File_Stream_Preload(int handle)
{
	SampleTrackerType	*st		= &LockedData.SampleTracker[handle];
	int					fh			= st->FileHandle;
	int					maxnum	= (LockedData.StreamBufferCount >> 1) + STREAM_CUSHION_BLOCKS;
	void					*buffer	= st->FileBuffer;
	int					num;
	int					index;

	/*
    ** Figure just how much we need to load.  If we are doing the load in progress
	** then we will only load two blocks.
	*/
    if (st->Loading) {
    	num = st->FilePending + 2;
       num = MIN(num, maxnum);
    } else {
		num = maxnum;
	}

    /*
    ** Loop through the blocks and load up the number we need.
    */
    for (index = st->FilePending; index < num; index++) {
		long s = Read_File(fh, Audio_Add_Long_To_Pointer(buffer, (long)index * (long)LockedData.StreamBufferSize), LockedData.StreamBufferSize);
       if (s) {
         	st->FilePendingSize = s;
	  		st->FilePending++;
		}
		if (s < LockedData.StreamBufferSize) break;
    }

	/*
	** If the last block was incomplete (ie. it didn't completely fill the buffer) or
	** we have now filled up as much of the Streaming Buffer as we need to, then now is
	** the time to kick off the sample.
	*/
	if (st->FilePendingSize < LockedData.StreamBufferSize || index == maxnum) {
		AudioLockClass lock(Mixer);

		/*
		** Actually start the sample playing, and don't worry about the file callback
		** it won't be called for a while.
		*/
 		int old						= LockedData.SoundVolume;
		int size						= (st->FilePending == 1) ? st->FilePendingSize : LockedData.StreamBufferSize;
		LockedData.SoundVolume	= LockedData.ScoreVolume;
		StartingFileStream		= TRUE;

		Stream_Sample_Vol(buffer, size, File_Callback, st->Volume, handle);
		StartingFileStream		= FALSE;
       LockedData.SoundVolume	= old;

		/*
		** The Sample is finished loading (if it was loading in small pieces) so record that
		** so that it will now use the active logic in the file call back.
		*/
		st->Loading					= FALSE;

		/*
		** Decrement the file pending because the first block is already playing thanks
		** to the play sample call above.
		*/
		st->FilePending--;

		/*
		** If File pending is now a zero, then we only preloaded one block and there
		** is nothing more to play.  So clear the sample tracing structure of the
		** information it no longer needs.
		*/
		if (!st->FilePending) {
			st->Odd					= 0;
          st->QueueBuffer		= 0;
			st->QueueSize			= 0;
			st->FilePendingSize	= 0;
          st->Callback			= NULL;
			Close_File(fh);
			st->FileHandle			= WW_ERROR;		// Port: the original left the closed handle here.
		} else {
			/*
			** The QueueBuffer counts as an already played block so remove it from the total.
          ** Note: We didn't remove it before because there might not have been one.
          */
          st->FilePending--;

          /*
          ** When we start loading we need to start past the first two blocks.  Why this
          ** is called Odd, I haven't got the slightest.
			*/
          st->Odd = 2;

          /*
          ** If the file pending size is less than the stream buffer, then the last block
          ** we loaded was the last block period.  So close the file and reset the handle.
          */
          if (st->FilePendingSize != LockedData.StreamBufferSize) {
				Close_File(fh);
             st->FileHandle = WW_ERROR;
          }

          /*
          ** The Queue buffer needs to point at the next block to be processed.  The size
          ** of the queue is dependant on how many more blocks there are.
          */
			st->QueueBuffer = Audio_Add_Long_To_Pointer(buffer, LockedData.StreamBufferSize);
			if (!st->FilePending) {
          	st->QueueSize = st->FilePendingSize;
          } else {
          	st->QueueSize = LockedData.StreamBufferSize;
          }
		}
	}
}


/***********************************************************************************************
 * File_Stream_Sample_Vol -- Streams a sample directly from a file.                            *
 *                                                                                             *
 *    This will take the file specified and play it directly from disk.                        *
 *    It performs this by allocating a temporary buffer in XMS/EMS and                         *
 *    then keeping this buffer filled by the Sound_Callback() routine.                         *
 *                                                                                             *
 * INPUT:   filename -- The name of the file to play.                                          *
 *                                                                                             *
 * OUTPUT:  Returns the handle to the sound -- just like Play_Sample().                        *
 *                                                                                             *
 * WARNINGS:   The temporary buffer is allocated when this routine is                          *
 *             called and then freed when the sound is finished.  Keep                         *
 *             this in mind.                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
int File_Stream_Sample_Vol(char const *filename, int volume, BOOL real_time_start)
{
	SampleTrackerType       *st;
	int     fh;
	int     handle = -1;
	int     index;

	if (!AudioDone && LockedData.DigiHandle != -1 && filename && Find_File(filename)) {

		/*
		**  Make sure all sample tracker structures point to the same
		**  upper memory buffer.  This allocation only occurs if at
		**  least one sample gets "streamed".
		*/
		if (!FileStreamBuffer) {
			FileStreamBuffer = malloc((size_t)(LockedData.StreamBufferSize * LockedData.StreamBufferCount));
			AudioLockClass lock(Mixer);
       	for (index = 0; index < MAX_SFX; index++) {
       		LockedData.SampleTracker[index].FileBuffer = FileStreamBuffer;
			}
		}

		/*
		** If after trying to allocate the buffer we still fail then
		** we can stream this sample.
		*/
		if (!FileStreamBuffer) return(-1);

    	/*
    	** Lets see if we can sucessfully open up the file.  If we can't,
    	** then there is no point in going any farther.
    	*/
    	if ((fh = Open_File(filename, READ)) == -1) {
 	 		return (-1);
    	}

		/*
		** Reserve a handle so that we can fill in the sample tracker
		** with the needed information.  If we dont get valid handle then
		** we might as well give up.
		*/
		if ((unsigned)(handle = Get_Free_Sample_Handle(0xFF)) >= MAX_SFX) {
			Close_File(fh);		// Port: the original leaked the handle here.
 			return(-1);
		}

		/*
		** Now lets get a pointer to the proper sample handler and start
		** our manipulations.
		*/
		{
			AudioLockClass lock(Mixer);
			st							= &LockedData.SampleTracker[handle];
			st->IsScore				= TRUE;
			st->FilePending		= 0;
			st->FilePendingSize	= 0;
			st->Loading				= real_time_start;
			st->Volume				= volume;
			st->FileHandle			= fh;
		}

		/*
		** Now that we have setup our initial data properly, let load up
		** the beginning of the sample we intend to stream.
		*/
		File_Stream_Preload(handle);
	}

	return (handle);
}


/***********************************************************************************************
 * Sound_Callback -- Audio driver callback function.                                           *
 *                                                                                             *
 *    Maintains the audio buffers.  This routine must be called at least                       *
 *    11 times per second or else audio glitches will occur.                                   *
 *                                                                                             *
 *    (Port: also lets the backend advance -- the null backend mixes here -- and marks        *
 *    samples whose voice has finished as stopped.)                                            *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   If this routine is not called often enough then audio                           *
 *             glitches will occur.                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void __cdecl Sound_Callback(void)
{
	int					index;
	SampleTrackerType	*st;

	if (!AudioDone && LockedData.DigiHandle != -1) {

		if (Backend) Backend->Update();
		Reap_Finished_Samples();

		st = &LockedData.SampleTracker[0];
		for (index = 0; index < MAX_SFX; index++) {
			if (st->Loading) {
				File_Stream_Preload(index);
			} else {

				/*
				**  General service routine to handle moving small blocks from the
				**  source into the low RAM staging buffers.
				*/
				if (st->Active) {

					/*
					**  Special check to see if the sample is a fading one AND
					**  it has faded to silence, then stop it here.
					*/
					bool faded;
					bool want_more;
					{
						AudioLockClass lock(Mixer);
						faded = st->Reducer && !st->Volume;

						/*
						**  Fill the queuebuffer if it is currently empty
						**  and there is a callback function defined to fill it.
						**
						** PWG/CDY & CO: We should be down by at least two blocks
						**                                        before we bother with this
						*/
						want_more = (!st->QueueBuffer ||
							(st->FileHandle != WW_ERROR && st->FilePending < LockedData.StreamBufferCount-3)) &&
							st->Callback;
					}

					if (faded) {
						Stop_Sample(index);
					} else if (want_more) {
						if (!st->Callback((short)index, (short int *)&st->Odd, &st->QueueBuffer, &st->QueueSize)) {
							AudioLockClass lock(Mixer);
							st->Callback = NULL;
						}
					}

				} else {

					/*
					** This catches the case where a streaming sample gets
					** aborted prematurely because of failure to call the
					** callback function frequently enough.  In this case, the
					** sample will be flagged as inactive, but the file handle
					** will not have been closed.
					*/
					if (st->FileHandle != WW_ERROR) {
						Close_File(st->FileHandle);
						st->FileHandle = WW_ERROR;
					}
				}
			}

			/*
			**      Advance to the next sample control structure.
			*/
			st++;
		}
	}
}


/***********************************************************************************************
 * Load_Sample -- Loads a digitized sample into RAM.                                           *
 *                                                                                             *
 *    This routine loads a digitized sample into RAM.                                          *
 *                                                                                             *
 * INPUT:   filename -- Name of the sound file to load.                                        *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the loaded sound file.  This pointer                     *
 *          is passed to Play_Sample when playback is desired.                                 *
 *                                                                                             *
 * WARNINGS:   If there is insufficient memory to load the sample, then                        *
 *             NULL will be returned.                                                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/17/1992 JLB : Created.                                                                 *
 *   01/06/1994 JLB : HMI version.                                                             *
 *=============================================================================================*/
void *Load_Sample(char const *filename)
{
	void *buffer = NULL;
	long    size;
	int     fh;

	if (LockedData.DigiHandle == -1 || !filename || !Find_File(filename)) {
		return (NULL);
	}

	fh = Open_File(filename, READ);
	if (fh != WW_ERROR) {
		size = (long)File_Size(fh)+AUD_HEADER_SIZE;
		buffer = malloc((size_t)size);

		if (buffer) {
			Sample_Read(fh, buffer, size);
		}
		Close_File(fh);
		Misc = size;
	}
	return(buffer);
}


/***********************************************************************************************
 * Load_Sample_Into_Buffer -- Loads a digitized sample into a buffer.                          *
 *                                                                                             *
 *    This routine is used to load a digitized sample into a buffer                            *
 *    provided by the programmer.  This buffer can be in XMS or EMS.                           *
 *                                                                                             *
 * INPUT:   filename -- The filename to load.                                                  *
 *                                                                                             *
 *          buffer   -- Pointer to the buffer to load into.                                    *
 *                                                                                             *
 *          size     -- The size of the buffer to load into.                                   *
 *                                                                                             *
 * OUTPUT:  Returns the number of bytes actually used in the buffer.                           *
 *                                                                                             *
 * WARNINGS:   This routine will not overflow the buffer provided.  This                       *
 *             means that the buffer must be big enough to hold the data                       *
 *             or else the sound will be cut short.                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long Load_Sample_Into_Buffer(char const *filename, void *buffer, long size)
{
	int     fh;

	/*
	**      Verify legality of parameters.
	*/
	if (!buffer || !size || LockedData.DigiHandle == -1 || !filename || !Find_File(filename)) {
		return (0);
	}

	fh = Open_File(filename, READ);
	if (fh != WW_ERROR) {
		size = Sample_Read(fh, buffer, size);
		Close_File(fh);
	} else {
		return(0);
	}
	return(size);
}


/***********************************************************************************************
 * Sample_Read -- Reads sample data from an openned file.                                      *
 *                                                                                             *
 *    This routine reads a sample file.  It is presumed that the file is                       *
 *    already positioned at the start of the sample.  From this, it can                        *
 *    determine if it is a VOC or raw data and proceed accordingly.                            *
 *                                                                                             *
 * INPUT:   fh       -- File handle of already openned sample file.                            *
 *                                                                                             *
 *          buffer   -- Pointer to the buffer to load data into.                               *
 *                                                                                             *
 *          size     -- The size of the buffer.                                                *
 *                                                                                             *
 * OUTPUT:  Returns the number of bytes actually used in the buffer.                           *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
long Sample_Read(int fh, void *buffer, long size)
{
	unsigned char	RawHeader[AUD_HEADER_SIZE];
	void    			*outbuffer;				// Pointer to start of raw data.
	long    			actual_bytes_read;	// Actual bytes read in, including header

	if (!buffer || fh == WW_ERROR || size <= AUD_HEADER_SIZE) return(0);

	size -= AUD_HEADER_SIZE;
	outbuffer = Audio_Add_Long_To_Pointer(buffer, AUD_HEADER_SIZE);
	memset(RawHeader, 0, sizeof(RawHeader));
  	actual_bytes_read = Read_File(fh, RawHeader, AUD_HEADER_SIZE);
	long datasize = AUD_SIZE(RawHeader);
	if (datasize < 0) datasize = 0;
	actual_bytes_read += Read_File(fh, outbuffer, (unsigned long)MIN(size, datasize));
	memcpy(buffer, RawHeader, AUD_HEADER_SIZE);
	return(actual_bytes_read);
}


/***********************************************************************************************
 * Free_Sample -- Frees a previously loaded digitized sample.                                  *
 *                                                                                             *
 *    Use this routine to free the memory allocated by a previous call to                      *
 *    Load_Sample.                                                                             *
 *                                                                                             *
 * INPUT:   sample   -- Pointer to the sample to be freed.                                     *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/17/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
void Free_Sample(void const *sample)
{
	if (sample) free((void *)sample);
}


/***********************************************************************************************
 * Set_Primary_Buffer_Format -- set the format of the primary sound buffer                     *
 *                                                                                             *
 *    Port: there is no primary buffer; the backend converts formats itself.                   *
 *                                                                                             *
 * OUTPUT:   TRUE if a device is open                                                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    12/22/95 4:06PM ST : Created                                                             *
 *=============================================================================================*/
BOOL Set_Primary_Buffer_Format(void)
{
	return (Backend != NULL);
}


/***********************************************************************************************
 * Audio_Init -- Initialise the sound system                                                   *
 *                                                                                             *
 * INPUT:    window - window to send callback messages to (port: unused)                       *
 *           maximum bits_per_sample - 8 or 16 (port: unused, the mixer is 16-bit)            *
 *           stereo - will stereo samples be played (= output channels)                        *
 *           rate - maximum sample rate required (= output rate)                               *
 *           reverse_channels                                                                  *
 *                                                                                             *
 * OUTPUT:   TRUE if correctly initialised. FALSE if the backend couldn't open a device;      *
 *           the game then runs without sound (SampleType stays SAMPLE_NONE), as before.       *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   Unknown....                                                                               *
 *   08-24-95 10:01am ST : Modified for Windows 95 Direct Sound                                *
 *   10-14-26 S09 : Backend + mixer instead of DirectSound                                     *
 *=============================================================================================*/
BOOL Audio_Init( HWND , int , BOOL stereo , int rate , int reverse_channels)
{
	int	index;
	int	sample=1;

	/*
	** Port: a second call is harmless. (The original re-initialised the locked
	** data and then skipped the device setup, which left sound disabled.)
	*/
	if (Backend) return(TRUE);

	Init_Locked_Data();
	FileStreamBuffer = NULL;
	memset(&LockedData.SampleTracker[0], 0, sizeof(LockedData.SampleTracker));

	if (rate <= 0) rate = 22050;
	AudioDeviceFormat format;
	format.Rate = rate;
	format.Channels = stereo ? 2 : 1;

	Mixer.Configure(format.Rate, format.Channels, reverse_channels != 0);
	Mixer.Set_Fill_Callback(Audio_Voice_Fill, NULL);
	Mixer.Set_Tick_Callback(Tick_Callback, NULL, MAINTENANCE_RATE);
	Mixer.Set_Master_Gain(AudioMixerClass::UNITY_GAIN);

	AudioBackendClass *backend = Audio_Get_Backend();
	if (!backend || !backend->Open(format, Mix_Callback, &Mixer)) {
		return (FALSE);
	}
	Backend = backend;

	/*
	** Allocate a decompression buffer equal to the size of a SONARC frame
	** block.
	*/
	LockedData.UncompBuffer = calloc(1, UNCOMP_BUFFER_SIZE);

	for (index = 0; index < MAX_SFX; index++) {
		LockedData.SampleTracker[index].PlaybackRate	= rate;
		LockedData.SampleTracker[index].Stereo			= (stereo) ? AUD_FLAG_STEREO : 0;
		LockedData.SampleTracker[index].BitSize 		= AUD_FLAG_16BIT;
		LockedData.SampleTracker[index].FileHandle 	= WW_ERROR;
		LockedData.SampleTracker[index].QueueBuffer 	= NULL;
	}

	SoundType = (SFX_Type)sample;
	SampleType = (Sample_Type)sample;
	ReverseChannels = reverse_channels;

	LockedData.DigiHandle = 1;
	AudioDone = FALSE;

	/*
	** Start the device playing
	*/
	DevicePaused = true;
	Start_Primary_Sound_Buffer(TRUE);
	return(TRUE);
}


/***********************************************************************************************
 * Sound_End -- Uninitializes the sound driver.                                                *
 *                                                                                             *
 *    This routine will uninitialize the sound driver (if any was                              *
 *    installed).  This routine must be called at program termination                          *
 *    time.                                                                                    *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/23/1991 JLB : Created.                                                                 *
 *   11/02/1995 ST  : Modified for Direct Sound                                                *
 *   10/14/2026 S09 : Closes the backend; Audio_Init may be called again afterwards           *
 *=============================================================================================*/
void Sound_End(void)
{
	int	index;

	if (Backend) {

		/*
		** Stop all sounds, then close the device so no mix callback can run.
		*/
		for (index=0 ; index < MAX_SFX; index++){
			Stop_Sample (index);
		}
		Backend->Pause(true);
		Backend->Close();
		Backend = NULL;
	}

	AudioDone = TRUE;

	{
		AudioLockClass lock(Mixer);
		for (index = 0; index < MAX_SFX; index++) {
			Mixer.Stop_Voice(index);
			LockedData.SampleTracker[index].FileBuffer = NULL;
		}
		LockedData.DigiHandle = -1;
	}

	if (FileStreamBuffer){
		free (FileStreamBuffer);
		FileStreamBuffer = NULL;
	}

	if (LockedData.UncompBuffer) {
		free(LockedData.UncompBuffer);
		LockedData.UncompBuffer = 0;
	}
}


/***********************************************************************************************
 * Stop_Sample -- Stops any currently playing sampled sound.                                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/02/1992 JLB : Created.                                                                 *
 *   11/2/95 4:09PM ST : Modified for Direct Sound                                             *
 *=============================================================================================*/
void Stop_Sample(int handle)
{
	if (LockedData.DigiHandle != -1 && (unsigned)handle < MAX_SFX) {

		if (AudioDone)	return;

		AudioLockClass lock(Mixer);

		if (LockedData.SampleTracker[handle].Active || LockedData.SampleTracker[handle].Loading) {

			LockedData.SampleTracker[handle].Active = FALSE;

			if (!LockedData.SampleTracker[handle].IsScore) {
          	LockedData.SampleTracker[handle].Original = NULL;
			}

			LockedData.SampleTracker[handle].Priority = 0;

			/*
          **  Stop the sample if it is playing.
          */
			if (!LockedData.SampleTracker[handle].Loading) {
				Mixer.Stop_Voice(handle);
          }

			LockedData.SampleTracker[handle].Loading = FALSE;

          /*
			**  If this is a streaming sample, then close the source file.
          */
			if (LockedData.SampleTracker[handle].FileHandle != WW_ERROR) {
				Close_File(LockedData.SampleTracker[handle].FileHandle);
				LockedData.SampleTracker[handle].FileHandle = WW_ERROR;
			}

			LockedData.SampleTracker[handle].QueueBuffer = NULL;
		}
	}
}


/***********************************************************************************************
 * Sample_Status -- Queries the current playing sample status (if any).                        *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/02/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
BOOL Sample_Status(int handle)
{
	if (AudioDone) return (FALSE);

	/*
	** If its an invalid handle or we do not have a sound driver then
	** the sample in question is not playing.
	*/
	if (LockedData.DigiHandle == -1 || (unsigned)handle >= MAX_SFX) return(FALSE);

	/*
	** Port: let a thread-less backend (null) advance time here too. The game
	** busy-waits on speech with "while (Is_Speaking()) {};"
	** (CODE/SCENARIO.CPP), which never reaches Sound_Callback.
	*/
	if (Backend) Backend->Update();

	AudioLockClass lock(Mixer);

	/*
	** If the sample is loading, then for all intents and purposes the
	** sample is playing.
	*/
	if (LockedData.SampleTracker[handle].Loading) return(TRUE);

	/*
	** If the sample is not active, then it is not playing
	*/
	if (!LockedData.SampleTracker[handle].Active) return(FALSE);

	/*
	** If we made it this far, then the Sample is still playing if the
	** mixer says that it is.
	*/
	if (!Mixer.Is_Voice_Playing(handle)) {
		Stop_Sample(handle);
		return(FALSE);
	}
	return(TRUE);
}


/***********************************************************************************************
 * Is_Sample_Playing -- returns the play state of a sample                                     *
 *                                                                                             *
 * INPUT:    ptr to sample data                                                                *
 *                                                                                             *
 * OUTPUT:   TRUE if sample is currently playing                                               *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:11PM ST : Commented                                                            *
 *=============================================================================================*/
BOOL Is_Sample_Playing(void const * sample)
{
	int index;

	if (AudioDone) return (FALSE);

	if (!sample) {
		return FALSE;
	}

	AudioLockClass lock(Mixer);
	for (index = 0; index < MAX_SFX; index++) {
		if (LockedData.SampleTracker[index].Original == sample && Sample_Status(index)) {
			return (TRUE);
		}
	}
	return (FALSE);
}


/***********************************************************************************************
 * Stop_Sample_Playing -- stops a playing sample                                               *
 *                                                                                             *
 * INPUT:    ptr to sample data                                                                *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:13PM ST : Commented                                                            *
 *=============================================================================================*/
void Stop_Sample_Playing(void const * sample)
{
	int index;

	if (sample) {
		AudioLockClass lock(Mixer);
		for (index = 0; index < MAX_SFX; index++) {
			if (LockedData.SampleTracker[index].Original == sample) {
				Stop_Sample(index);
				break;
			}
		}
	}
}


/***********************************************************************************************
 * Get_Free_Sample_Handle -- finds a free slot in which to play a new sample                   *
 *                                                                                             *
 * INPUT:    priority of sample we want to play                                                *
 *                                                                                             *
 * OUTPUT:   Handle or -1 if none free                                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:14PM ST : Added function header                                                *
 *=============================================================================================*/
int Get_Free_Sample_Handle(int priority)
{
	int     id;

	Reap_Finished_Samples();
	AudioLockClass lock(Mixer);

	/*
	**      Find a free SFX holding buffer slot.
	*/
	for (id = MAX_SFX - 1; id >= 0; id--) {
		if (!LockedData.SampleTracker[id].Active && !LockedData.SampleTracker[id].Loading) {
			if (!StartingFileStream && LockedData.SampleTracker[id].IsScore) {
				StartingFileStream = TRUE;      // Ensures only one channel is kept free for scores.
				continue;
			}
			break;
		}
	}

	if (id < 0) {
		for (id = 0; id < MAX_SFX; id++) {
			if (LockedData.SampleTracker[id].Priority <= priority) break;
		}

		if (id == MAX_SFX) {
			return(-1);             // Cannot play!
		}
		Stop_Sample(id);                                                        // This sample gets clobbered.
	}

	if (id == -1) {
		return -1;
	}

	if (LockedData.SampleTracker[id].FileHandle != WW_ERROR) {
		Close_File(LockedData.SampleTracker[id].FileHandle);
		LockedData.SampleTracker[id].FileHandle = WW_ERROR;
	}

	if (LockedData.SampleTracker[id].Original && !LockedData.SampleTracker[id].IsScore) {
		LockedData.SampleTracker[id].Original = NULL;
	}
	LockedData.SampleTracker[id].IsScore = FALSE;
	return(id);
}


int Play_Sample(void const *sample, int priority, int volume, signed short panloc)
{
	return(Play_Sample_Handle(sample, priority, volume, panloc, Get_Free_Sample_Handle(priority)));
}


/***********************************************************************************************
 * Play_Sample_Handle -- Plays a digitized sample.                                             *
 *                                                                                             *
 *    Use this routine to play a previously loaded digitized sample.                           *
 *                                                                                             *
 * INPUT:   sample   -- Sample pointer as returned from Load_Sample.                           *
 *                                                                                             *
 *          priority -- Priority (0..255); a higher one may take the slot of a lower one.      *
 *                                                                                             *
 *          volume   -- The volume to play (0..255 with 255=loudest).                          *
 *                                                                                             *
 *          panloc   -- Ignored, as in the Win95 original (DirectSound buffers weren't         *
 *                      created with pan control).                                             *
 *                                                                                             *
 *          id       -- Tracker to play on (from Get_Free_Sample_Handle).                      *
 *                                                                                             *
 * OUTPUT:  The handle, or -1.                                                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/17/1992 JLB : Created.                                                                 *
 *   05/24/1992 JLB : Volume support -- Soundblaster Pro                                       *
 *   04/22/1994 JLB : Multiple sample playback rates.                                          *
 *   11/02/1995 ST  : Windows Direct Sound support                                             *
 *   10/14/2026 S09 : Mixer voice instead of a DirectSound buffer. The original first          *
 *                    returned 0 if any DirectDraw surface was locked (Any_Locked); that       *
 *                    check is gone.                                                           *
 *=============================================================================================*/
int Play_Sample_Handle(void const *sample, int priority, int volume, signed short , int id)
{
	SampleTrackerType               *st=NULL;       // Working pointer to sample tracker structure.

	if (AudioDone) return (0);

	if (!sample || LockedData.DigiHandle == -1) {
		return(-1);
	}

	if (id == -1 || (unsigned)id >= MAX_SFX) {
		return -1;
	}

	AudioLockClass lock(Mixer);
	st = &LockedData.SampleTracker[id];

	/*
	**      Fetch the control bytes from the start of the sample data.
	*/
	unsigned rate = AUD_RATE(sample);
	long data_size = AUD_SIZE(sample);
	unsigned flags = AUD_FLAGS(sample);
	unsigned compression = AUD_COMPRESSION(sample);

	/*
	** Fudge the sample rate to 22k
	*/
	if (rate <24000 && rate >20000) rate = 22050;

	/*
	** Stop whatever this tracker was playing.
	*/
	Mixer.Stop_Voice(id);

	/*
	**      Prepare the sample tracker structure for processing of this
	**      sample.  Fill the structure with data that can be determined
	**      before the sample is started.
	*/
	st->Active					= FALSE;
	st->Compression 			= (SCompressType) ((unsigned char)compression);
	st->Original            = sample;
	st->OriginalSize        = data_size + AUD_HEADER_SIZE;
	st->Priority            = (short)priority;
	st->DontTouch           = TRUE;
	st->Odd						= 0;
	st->Reducer             = 0;
	st->QueueBuffer 			= NULL;
	st->QueueSize           = 0;
	st->TrailerLen  			= 0;
	st->Remainder           = data_size;
	st->Source              = Audio_Add_Long_To_Pointer((void *)sample, AUD_HEADER_SIZE);

	/*
	** If the code in question using HMI based compression then we need
	** to set up for uncompressing it.
	*/
	if (st->Compression == SCOMP_SOS) {
		st->sosinfo.wChannels    = (flags & AUD_FLAG_STEREO) ? 2  : 1;
		st->sosinfo.wBitSize             = (flags & AUD_FLAG_16BIT)  ? 16 : 8;
		st->sosinfo.dwCompSize   = data_size;
		st->sosinfo.dwUnCompSize = data_size * ( st->sosinfo.wBitSize / 4 );
		sosCODECInitStream(&st->sosinfo);
	}

	/*
	** Remember the format of the sample.
	*/
	st->PlaybackRate 	= rate;
	st->Stereo			= flags & AUD_FLAG_STEREO;
	st->BitSize 		= flags & AUD_FLAG_16BIT;

	/*
	**  Set the volume of the sample.
	*/
	st->Volume = (volume << 7);
	int gain = Audio_Volume_To_Gain((int)((LockedData.SoundVolume*(unsigned)volume)/256));

	/*
	** Make sure the device is playing
	*/
	if (!Start_Primary_Sound_Buffer(FALSE)){
		st->DontTouch = FALSE;
		return(-1);
	}

	/*
	** Start the sample playing now. The mixer pulls the data through
	** Audio_Voice_Fill as it needs it.
	*/
	if (!Mixer.Start_Voice(id, (int)rate, (flags & AUD_FLAG_STEREO) ? 2 : 1, (flags & AUD_FLAG_16BIT) ? 16 : 8, gain)) {
		st->DontTouch = FALSE;
		return(-1);
	}

	st->Active=TRUE;
	st->Handle=(short)id;
	st->DontTouch = FALSE;
	return (st->Handle);
}


/***********************************************************************************************
 * Restore_Sound_Buffers -- restore the sound buffers                                          *
 *                                                                                             *
 *    Port: nothing to restore; DirectSound could lose buffers, SDL can't.                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:53PM ST : Created                                                              *
 *=============================================================================================*/
void Restore_Sound_Buffers ( void )
{
}


/***********************************************************************************************
 * Set_Sound_Vol -- sets the overall volume for sampled sounds                                 *
 *                                                                                             *
 *    Like the original, this only affects samples started afterwards.                        *
 *                                                                                             *
 * INPUT:    volume                                                                            *
 *                                                                                             *
 * OUTPUT:   the previous volume setting                                                       *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:19PM ST : Added function header                                                *
 *=============================================================================================*/
int Set_Sound_Vol(int volume)
{
	AudioLockClass lock(Mixer);
	int old;
 	old = LockedData.SoundVolume;
 	LockedData.SoundVolume = volume & 0xFF;
	return(old);
}


/***********************************************************************************************
 * Set_Score_Vol -- sets the overall volume for music scores                                   *
 *                                                                                             *
 *    Also changes the volume of the score that is playing.                                    *
 *                                                                                             *
 * INPUT:    volume                                                                            *
 *                                                                                             *
 * OUTPUT:   the previous volume setting                                                       *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:19PM ST : Added function header                                                *
 *=============================================================================================*/
int Set_Score_Vol(int volume)
{
	AudioLockClass lock(Mixer);
	int old;
	SampleTrackerType	*st;					//ptr to SampleTracker structure

	old = LockedData.ScoreVolume;
	LockedData.ScoreVolume = volume & 0xFF;

	for (int index=0 ; index<MAX_SFX ; index++){
		st = &LockedData.SampleTracker[index];

		if (st->IsScore && st->Active){
			Audio_Update_Voice_Volume(index);
		}
	}
	return(old);
}


/***********************************************************************************************
 * Fade_Sample -- Start a sample fading                                                        *
 *                                                                                             *
 *    The volume drops by 1/ticks of its start value on every maintenance tick (40 a second); *
 *    Sound_Callback stops the sample once it is silent.                                       *
 *                                                                                             *
 * INPUT:    Sample handle                                                                     *
 *           fade rate                                                                         *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:21PM ST : Added function header                                                *
 *=============================================================================================*/
void Fade_Sample(int handle, int ticks)
{
	if (Sample_Status(handle)) {
		if (!ticks || LockedData.SampleTracker[handle].Loading) {
			Stop_Sample(handle);
		} else {
			AudioLockClass lock(Mixer);
			SampleTrackerType * st;
			st = &LockedData.SampleTracker[handle];
			st->Reducer = (short) ((st->Volume / ticks)+1);
		}
	}
}


int Get_Digi_Handle(void)
{
	return(LockedData.DigiHandle);
}


/***************************************************************************
 * SAMPLE_LENGTH -- returns length of a sample in ticks                    *
 *                                                                         *
 * INPUT:  void const *sample - pointer to the sample to get length of.    *
 *                                                                         *
 * OUTPUT: long - length of the sample in ticks (60/sec)                   *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 PWG : Created.                                             *
 *=========================================================================*/
long Sample_Length(void const *sample)
{
	if (!sample) return(0);

	long time = AUD_UNCOMP_SIZE(sample);
	unsigned flags = AUD_FLAGS(sample);
	unsigned rate = AUD_RATE(sample);

	/*
	** If the sample is a 16 bit sample, then it will take only half
	** as long to play.
	*/
	if (flags & AUD_FLAG_16BIT) {
		time >>= 1;
	}

	/*
	** If the sample is a stereo sample, then it will take only half
	** as long to play.
	*/
	if (flags & AUD_FLAG_STEREO) {
		time >>= 1;
	}

	if (rate/60) {
		time /= (long)(rate/60);
	}
	return(time);
}


/***********************************************************************************************
 * Start_Primary_Sound_Buffer -- start the primary sound buffer playing                        *
 *                                                                                             *
 *    Port: resumes the backend. The original only started the buffer while the game had     *
 *    focus (GameInFocus); the port tracks that through Stop_Primary_Sound_Buffer, which the  *
 *    game calls on focus loss: until a forced start, a non-forced call fails, as it did      *
 *    out of focus.                                                                            *
 *                                                                                             *
 * INPUT:    forced -- start even if stopped by Stop_Primary_Sound_Buffer                      *
 *                                                                                             *
 * OUTPUT:   TRUE if the device is playing                                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/1/96 12:28PM ST : Created                                                              *
 *=============================================================================================*/
BOOL Start_Primary_Sound_Buffer (BOOL forced)
{
	if (!Backend) return (FALSE);
	if (DevicePaused && !forced) return (FALSE);
	if (DevicePaused) {
		Backend->Pause(false);
		DevicePaused = false;
	}
	return (TRUE);
}


/***********************************************************************************************
 * Stop_Primary_Sound_Buffer -- stops the primary sound buffer from playing.                   *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: This stops all sound playback                                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/1/96 12:28PM ST : Created                                                              *
 *=============================================================================================*/
void Stop_Primary_Sound_Buffer (void)
{
	if (Backend){
		Backend->Pause(true);
		DevicePaused = true;
	}

	for ( int index = 0; index < MAX_SFX; index++) {
		Stop_Sample(index);
	}
}


/*
** Port: the original stopped and restarted the Windows timer that drove
** maintenance_callback. The mixer drives it now. The game's only calls are
** commented out (CODE/CONQUER.CPP), but it still declares them.
*/
void Suspend_Audio_Thread(void)
{
}

void Resume_Audio_Thread(void)
{
}
