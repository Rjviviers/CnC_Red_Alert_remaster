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
 *                    File Name : SOUNDINT.H                               *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : June 23, 1995                            *
 *                                                                         *
 *                  Last Update : June 23, 1995   [PWG]                    *
 *                                                                         *
 * This file is the include file for the Westwood Sound Sytem defines and  *
 * routines that are handled in an interrupt.
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
** Port (S09): this header no longer depends on DirectSound or Win32 types.
** The DirectSound secondary buffer of each sample tracker was replaced by a
** voice of the portable software mixer (audio_mixer.h); voice N belongs to
** SampleTracker[N]. Fields that only made sense for DirectSound (PlayBuffer,
** DestPtr, MoreSource, OneShot, Service, Restart, Pitch, Flags and the
** per-tracker critical section) were removed. See docs/codebase/audio.md.
** Nothing in the game includes this header; it is internal to WIN32LIB/AUDIO.
*/
#ifndef SOUNDINT_H
#define SOUNDINT_H

#include "sound.h"

/*
** Defines for true and false.  These are included because we do not allow
** the sound int to include any of the westwood standard headers.  If we
** did, there might be too much temptation to call another library function.
** this would be bad, because then that function would not be locked.
*/
#ifndef FALSE
#define	FALSE		0
#endif
#ifndef TRUE
#define	TRUE		1
#endif

/*
** Define the different type of sound compression avaliable to the westwood
** library.
*/
typedef enum {
	SCOMP_NONE=0,			// No compression -- raw data.
	SCOMP_WESTWOOD=1,		// Special sliding window delta compression.
	SCOMP_SONARC=33,		// Sonarc frame compression.
	SCOMP_SOS=99			// SOS frame compression.
} SCompressType;

/*
**	This is the safety overrun margin for the sonarc compressed
** data frames.  This value should be equal the maximum 'order' times
**	the maximum number of bytes per sample.  It should be evenly divisible
**	by 16 to aid paragraph alignment.
*/
#define	SONARC_MARGIN				32

/*
** Port (S09): size in bytes of the AUD file header (AUDHeaderType in audio.h):
** rate (2), compressed size (4), uncompressed size (4), flags (1), compression (1).
** The port reads and writes these fields byte by byte (AUD_* below), so it
** doesn't depend on the size of 'long' or on struct packing.
*/
#define	AUD_HEADER_SIZE			12

/*
** Port (S09): every compressed frame starts with this 8-byte header:
** compressed size (2), uncompressed size (2), magic number 0x0000DEAF (4).
*/
#define	AUD_FRAME_HEADER_SIZE	8
#define	AUD_FRAME_MAGIC			0x0000DEAFL

/*
** This is the maximum size that a sonarc block can be (moved here from
** SOUNDIO.CPP). The decompression staging buffer (LockedData.UncompBuffer)
** holds one compressed frame and is UNCOMP_BUFFER_SIZE bytes long.
*/
#define	LARGEST_SONARC_BLOCK		2048
#define	UNCOMP_BUFFER_SIZE		(LARGEST_SONARC_BLOCK + 50)

/*
** Port (S09): number of times per second maintenance_callback runs. It now
** runs from the mixer, in output time, instead of from a Windows timer.
*/
#define	MAINTENANCE_RATE			40


/*
** Define the sample control structure which helps us to handle feeding
** data to the sound interrupt.
*/
typedef struct {
	/*
	**	This flags whether this sample structure is active or not.
	*/
	unsigned Active;

	/*
	**	This flags whether the sample is loading or has been started.
	*/
	unsigned Loading;

	/*
	**	This semaphore ensures that simultaneous update of this structure won't
	**	occur.  This is necessary since both interrupt and regular code can modify
	**	this structure.  (Port: the audio lock does this job now; the flag is
	** kept because the streaming code still sets it.)
	*/
	unsigned DontTouch;

	/*
	**	If this sample is really to be considered a score rather than
	**	a sound effect, then special rules apply.  These largely fall into
	**	the area of volume control.
	*/
	unsigned IsScore;

	/*
	**	This is the original sample pointer. It is used to control the sample based on
	**	pointer rather than handle. The handle method is necessary when more than one
	**	sample could be playing simultaneously. The pointer method is necessary when
	**	the dealing with a sample that may have stopped behind the programmer's back and
	**	this occurance is not otherwise determinable.
	*/
	void const *Original;
	long OriginalSize;

	/*
	** Format of the sample last started on this tracker: rate in Hz,
	** AUD_FLAG_16BIT, AUD_FLAG_STEREO.
	*/
	int	PlaybackRate;
	int	BitSize;
	int	Stereo;

	/*
	**	Pointer to the sound data that has not yet been copied
	**	to the mixer.
	*/
	void *Source;

	/*
	**	This is the number of bytes remaining in the source data as
	**	pointed to by the "Source" element.
	*/
	long Remainder;

	/*
	**	Samples maintain a priority which is used to determine
	**	which sounds live or die when the maximum number of
	**	sounds are being played.
	*/
	int Priority;

	/*
	**	This is the handle as returned by Play_Sample.
	*/
	short int Handle;

	/*
	**	This is the current volume of the sample as it is being played
	** (0..255 shifted left by 7, as in the original).
	*/
	int Volume;
	int Reducer;		// Amount to reduce volume per tick.

	/*
	**	This is the compression that the sound data is using.
	*/
	SCompressType Compression;
	short int TrailerLen;						// Number of trailer bytes in buffer.
	unsigned char Trailer[SONARC_MARGIN];		// Maximum number of 'order' samples needed.

	/*
	**	Streaming control handlers.
	*/
	int (*Callback)(short int id, short int *odd, void **buffer, long *size);
	void	*QueueBuffer;	// Pointer to continued sample data.
	long	QueueSize;		// Size of queue buffer attached.
	short int	Odd;				// Block number tracker (0..StreamBufferCount-1).
	int	FilePending;	// Number of buffers already filled ahead.
	long	FilePendingSize;	// Number of bytes in last filled buffer.

	/*
	**	The file variables are used when streaming directly off of the
	**	hard drive.
	*/
	int	FileHandle;		// Streaming file handle (ERROR = not in use).
	void	*FileBuffer;	// Temporary streaming buffer (allowed to be freed).

	/*
	** The following structure is used if the sample if compressed using
	** the sos 16 bit compression Codec.
	*/
	_SOS_COMPRESS_INFO sosinfo;

} SampleTrackerType;


typedef struct LockedData {
	int			 		DigiHandle; 			// = -1;
	int 					ServiceSomething;		// = FALSE;
	long 					MagicNumber; 			// = 0xDEAF;
	void 					*UncompBuffer;			// = NULL;
	long 					StreamBufferSize; 	// = (SECONDARY_BUFFER_SIZE/4)+128;
	short 				StreamBufferCount; 	// = 16;
	SampleTrackerType SampleTracker[MAX_SFX];
	unsigned int		SoundVolume;
	unsigned int		ScoreVolume;
	int					_int;
} LockedDataType;

extern LockedDataType LockedData;

void Init_Locked_Data(void);
long Simple_Copy(void ** source, long * ssize, void ** alternate, long * altsize, void **dest, long size);
long Sample_Copy(SampleTrackerType *st, void ** source, long * ssize, void ** alternate, long * altsize, void * dest, long size, SCompressType scomp, void * trailer, short int *trailersize);
void maintenance_callback(void);
void Audio_Promote_Stream_Queue(SampleTrackerType *st);
long Audio_Voice_Fill(void *user, int voice, void *dest, long size);
void Audio_Update_Voice_Volume(int index);
int Audio_Volume_To_Gain(int volume);
int Convert_HMI_To_Direct_Sound_Volume(int volume);
void *Audio_Add_Long_To_Pointer(void const *ptr, long size);

class AudioMixerClass;
AudioMixerClass &Audio_Mixer(void);		// SOUNDIO.CPP: voice N = SampleTracker[N].

/*
** Port (S09): IMA ADPCM tables shared by OLSOSDEC.CPP (general decoder) and
** SOSCODEC.CPP (fast 16-bit mono decoder), and the fast decoder's generated
** lookup tables (exposed for tests).
*/
extern short const SOS_Index_Adjust[16];
extern unsigned short const SOS_Step_Table[89];
long const *SOS_Fast_Diff_Table(void);
unsigned short const *SOS_Fast_Index_Table(void);

/*
** Port (S09): little-endian field access for AUD file and frame headers.
*/
inline unsigned Aud_Get_U16(void const *ptr) {
	unsigned char const *p = (unsigned char const *)ptr;
	return (unsigned)p[0] | ((unsigned)p[1] << 8);
}
inline unsigned long Aud_Get_U32(void const *ptr) {
	unsigned char const *p = (unsigned char const *)ptr;
	return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}
inline void Aud_Put_U32(void *ptr, unsigned long value) {
	unsigned char *p = (unsigned char *)ptr;
	p[0] = (unsigned char)value;
	p[1] = (unsigned char)(value >> 8);
	p[2] = (unsigned char)(value >> 16);
	p[3] = (unsigned char)(value >> 24);
}
#define AUD_RATE(hdr)			((unsigned)Aud_Get_U16((char const *)(hdr) + 0))
#define AUD_SIZE(hdr)			((long)Aud_Get_U32((char const *)(hdr) + 2))
#define AUD_UNCOMP_SIZE(hdr)	((long)Aud_Get_U32((char const *)(hdr) + 6))
#define AUD_FLAGS(hdr)			(((unsigned char const *)(hdr))[10])
#define AUD_COMPRESSION(hdr)	(((unsigned char const *)(hdr))[11])

extern "C" {
	void __cdecl Audio_Mem_Set(void const *ptr, unsigned char value, long size);
	long  __cdecl Decompress_Frame(void * source, void * dest, long size);
	int	__cdecl sosCODEC_Lock(void);
	int	__cdecl sosCODEC_Unlock(void);
}

#endif
