/*
**	Command & Conquer Red Alert(tm) - multiplayer port. GPLv3 or later, see LICENSE.md.
**
**	STUBS (session S10b) for assembly routines that S07 did not port because nothing
**	needs their result. See docs/codebase/stubs.md. Permanent: no later session is
**	expected to replace them. Built on every host (no TASM anywhere since ADR-0010).
**
**	  CPUType, VendorID, Detect_MMX_Availability   (CODE/CPUID.ASM)
**	      Only read by Get_CPU_Type (CODE/GETCPU.CPP), which nothing calls. The values
**	      are CPUID.ASM's own "no CPUID instruction" defaults: type 0, no MMX. VendorID
**	      is declared as a single char (the start of the asm's string), so here it is
**	      one NUL: Get_CPU_Type would copy an empty vendor name.
**	  Force_VM_Page_In                              (WIN32LIB/MEM/VMPAGEIN.ASM)
**	      Touched every 4 KB page of a buffer so that Windows 95 paged it in before a
**	      time-critical slide show (CODE/EGOS.CPP). A performance hint only: no-op.
*/

extern "C" {

char CPUType = 0;
char VendorID = 0;

int __cdecl Detect_MMX_Availability(void)
{
	return 0;
}

void __cdecl Force_VM_Page_In(void *, int)
{
}

}
