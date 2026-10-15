/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**
**	STUB (session S10b): movie playback, per ADR-0005 ("stub out the cutscene movies";
**	they only matter for the campaign). See docs/codebase/stubs.md and ADR-0022.
**	Permanent unless a later decision brings movies back.
**
**	The game plays VQA movies through the VQA player library (WINVQ/VQA32, target
**	vqa32wp). That library needs a VQ decoder (UnVQ_4x2, asm), the SOS audio codec
**	(VQA_sosCODEC*, AudioUnzap: object files that are not in the source release),
**	DirectSound, multimedia timers and MCGA/VGA retrace routines, none of which exist
**	any more. Instead of stubbing those leaves, the game no longer links vqa32wp at all
**	and this file provides the small part of its API the game calls:
**
**	  VQA_Alloc        returns NULL: "no player". Every caller (CONQUER.CPP Play_Movie,
**	                   INTRO.CPP Open_Movie) then skips the movie, so playing a movie is
**	                   a no-op that returns at once, as if it had finished.
**	  VQA_Open         fails with VQAERR_OPEN (in case a caller ever gets a handle).
**	  VQA_Play         returns VQAERR_EOF: "finished".
**	  VQA_Init, VQA_Close, VQA_Free, VQA_PauseAudio, VQA_ResumeAudio: nothing to do.
**	  VQA_DefaultConfig clears the configuration (INIT.CPP then fills in its fields).
**
**	The MPEG (DVD) movie path in CONQUER.CPP is switched off at compile time instead:
**	MPEGMOVIE is no longer defined (CODE/DEFINES.H), so MpgPlay & co. are not referenced.
**
**	The VQA library still builds as its own target (compile coverage); it is just not
**	linked into the game.
*/

#include <windows.h>
#include <VQA32/VQAPLAY.H>		// the VQA player API (WINVQ/INCLUDE/VQA32/VQAPLAY.H)

void VQA_DefaultConfig(VQAConfig * config)
{
	if (config) memset(config, 0, sizeof(*config));
}

VQAHandle * VQA_Alloc(void)
{
	return NULL;
}

void VQA_Free(VQAHandle *)
{
}

void VQA_Init(VQAHandle *, long (*)(VQAHandle *, long, void *, long))
{
}

long VQA_Open(VQAHandle *, char const *, VQAConfig *)
{
	return VQAERR_OPEN;
}

void VQA_Close(VQAHandle *)
{
}

long VQA_Play(VQAHandle *, long)
{
	return VQAERR_EOF;
}

void VQA_PauseAudio(void)
{
}

void VQA_ResumeAudio(void)
{
}
