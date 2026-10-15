// Unit test for the portable file layer (CODE/compat/ra_file.cpp, session S10b, ADR-0022)
// that RawFileClass uses instead of the Win32 file API: case-insensitive name resolution on
// case-sensitive file systems, Win32 CreateFile semantics for each access mode, positioned
// read/write on one handle, and DOS date/time conversion.
//
// Works in a scratch directory under the current directory (ctest runs it in the build tree).

#include "ra_test.h"
#include "ra_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#  include <direct.h>
#  define MAKE_DIR(p) _mkdir(p)
#else
#  include <unistd.h>
#  define MAKE_DIR(p) mkdir(p, 0755)
#endif

static void Write_File(char const * path, char const * text)
{
	FILE * f = fopen(path, "wb");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

static void Setup(void)
{
	static bool done = false;
	if (done) return;
	done = true;
	MAKE_DIR("s10b_files");
	MAKE_DIR("s10b_files/Data");
	MAKE_DIR("s10b_files/Data/movies");
	Write_File("s10b_files/Data/redalert.mix", "MIX");
	Write_File("s10b_files/Data/movies/Intro.vqa", "VQA");
}

RA_TEST(resolve_exact_and_case_insensitive)
{
	Setup();
	char out[512];
	RA_CHECK(RA_Resolve_Path("s10b_files/Data/redalert.mix", out, sizeof(out), true));
	RA_CHECK(strcmp(out, "s10b_files/Data/redalert.mix") == 0);

	// The game's upper-case DOS spelling with backslashes.
	RA_CHECK(RA_Resolve_Path("S10B_FILES\\DATA\\REDALERT.MIX", out, sizeof(out), true));
#if !defined(_WIN32)
	RA_CHECK(strcmp(out, "s10b_files/Data/redalert.mix") == 0);
	RA_CHECK(RA_Resolve_Path("s10b_files\\DATA\\MOVIES\\INTRO.VQA", out, sizeof(out), true));
	RA_CHECK(strcmp(out, "s10b_files/Data/movies/Intro.vqa") == 0);
	// A drive letter is dropped.
	RA_CHECK(RA_Resolve_Path("C:S10B_FILES\\DATA\\REDALERT.MIX", out, sizeof(out), true));
	RA_CHECK(strcmp(out, "s10b_files/Data/redalert.mix") == 0);
#endif
}

RA_TEST(resolve_missing)
{
	Setup();
	char out[512];
	RA_CHECK(!RA_Resolve_Path("S10B_FILES\\DATA\\NOSUCH.MIX", out, sizeof(out), true));
#if !defined(_WIN32)
	// A new file keeps its spelling, in the real (differently cased) directory.
	RA_CHECK(RA_Resolve_Path("S10B_FILES\\DATA\\SAVEGAME.001", out, sizeof(out), false));
	RA_CHECK(strcmp(out, "s10b_files/Data/SAVEGAME.001") == 0);
	// A missing directory cannot be created implicitly.
	RA_CHECK(!RA_Resolve_Path("S10B_FILES\\NODIR\\X.TXT", out, sizeof(out), false));
#endif
	RA_CHECK(!RA_Resolve_Path("S10B_FILES\\DATA\\REDALERT.MIX", out, 4, true));	// too small
}

RA_TEST(exists_and_open_read)
{
	Setup();
	RA_CHECK(RA_File_Exists("S10B_FILES\\DATA\\REDALERT.MIX"));
	RA_CHECK(!RA_File_Exists("S10B_FILES\\DATA"));				// directories are not files
	RA_CHECK(!RA_File_Exists("S10B_FILES\\DATA\\NOSUCH.MIX"));

	int h = RA_File_Open("S10B_FILES\\DATA\\REDALERT.MIX", RA_FILE_READ);
	RA_CHECK(h != -1);
	char buf[8] = {0};
	RA_CHECK_EQ(3L, RA_File_Read(h, buf, sizeof(buf)));
	RA_CHECK(memcmp(buf, "MIX", 3) == 0);
	RA_CHECK_EQ(0L, RA_File_Read(h, buf, sizeof(buf)));			// end of file
	RA_CHECK_EQ(3L, RA_File_Size(h));
	RA_CHECK(RA_File_Close(h));

	RA_CHECK_EQ(-1, RA_File_Open("S10B_FILES\\DATA\\NOSUCH.MIX", RA_FILE_READ));
	RA_CHECK_EQ(-1, RA_File_Open("S10B_FILES\\DATA", RA_FILE_READ));
	RA_CHECK_EQ(-1, RA_File_Open("S10B_FILES\\DATA\\REDALERT.MIX", 0));
}

RA_TEST(write_seek_read_one_handle)
{
	Setup();
	// READ|WRITE truncates (CREATE_ALWAYS), as the original did.
	int h = RA_File_Open("S10B_FILES\\DATA\\RW.BIN", RA_FILE_READ | RA_FILE_WRITE);
	RA_CHECK(h != -1);
	RA_CHECK_EQ(6L, RA_File_Write(h, "ABCDEF", 6));
	RA_CHECK_EQ(6L, RA_File_Size(h));					// unbuffered, like a Win32 handle
	RA_CHECK_EQ(2L, RA_File_Seek(h, 2, SEEK_SET));
	char buf[4] = {0};
	RA_CHECK_EQ(2L, RA_File_Read(h, buf, 2));			// read right after write, no flush
	RA_CHECK(memcmp(buf, "CD", 2) == 0);
	RA_CHECK_EQ(4L, RA_File_Seek(h, 0, SEEK_CUR));
	RA_CHECK_EQ(5L, RA_File_Seek(h, -1, SEEK_END));
	RA_CHECK_EQ(-1L, RA_File_Seek(h, -100, SEEK_SET));	// before the start fails
	RA_CHECK_EQ(10L, RA_File_Seek(h, 10, SEEK_SET));	// past the end is allowed
	RA_CHECK(RA_File_Close(h));

	h = RA_File_Open("S10B_FILES\\DATA\\RW.BIN", RA_FILE_WRITE);	// truncates
	RA_CHECK(h != -1);
	RA_CHECK_EQ(0L, RA_File_Size(h));
	RA_CHECK(RA_File_Close(h));

	RA_CHECK(RA_File_Delete("S10B_FILES\\DATA\\RW.BIN"));
	RA_CHECK(!RA_File_Exists("S10B_FILES\\DATA\\RW.BIN"));
	RA_CHECK(!RA_File_Delete("S10B_FILES\\DATA\\RW.BIN"));
	RA_CHECK(!RA_File_Close(-1));
	RA_CHECK_EQ(-1L, RA_File_Read(-1, buf, 1));
}

RA_TEST(dos_date_time)
{
	// 1996-10-31 12:34:56 UTC = 846765296: date (16<<9)|(10<<5)|31, time (12<<11)|(34<<5)|28.
	unsigned long dt = RA_Dos_Date_Time_From_Unix(846765296LL);
	RA_CHECK_EQ((unsigned long)(((16u << 9) | (10u << 5) | 31u) << 16 | ((12u << 11) | (34u << 5) | 28u)), dt);
	RA_CHECK_EQ(846765296LL, RA_Unix_From_Dos_Date_Time(dt));
	// Leap day, and the DOS epoch (earlier times clamp to it).
	unsigned long leap = RA_Dos_Date_Time_From_Unix(951782400LL);		// 2000-02-29 00:00:00
	RA_CHECK_EQ((unsigned long)((20u << 9) | (2u << 5) | 29u) << 16, leap);
	RA_CHECK_EQ((unsigned long)((0u << 9) | (1u << 5) | 1u) << 16, RA_Dos_Date_Time_From_Unix(0));
	RA_CHECK_EQ(-1LL, RA_Unix_From_Dos_Date_Time(0));					// month 0 is invalid

	Setup();
	// Windows needs write access to change the time (SetFileTime did too).
	int h = RA_File_Open("S10B_FILES\\DATA\\DT.BIN", RA_FILE_WRITE);
	RA_CHECK(h != -1);
	RA_CHECK(RA_File_Set_Dos_Date_Time(h, dt));
	RA_CHECK_EQ(dt, RA_File_Get_Dos_Date_Time(h));
	RA_CHECK(RA_File_Close(h));
	RA_CHECK(RA_File_Delete("S10B_FILES\\DATA\\DT.BIN"));
}

RA_TEST_MAIN()
