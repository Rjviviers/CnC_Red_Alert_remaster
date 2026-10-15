// Unit test for the start-up helpers (CODE/port/ra_boot.cpp, session S10c, ADR-0023): the
// -data option, making paths absolute, the search list, the required-file check and its
// message, and the first-run REDALERT.INI.
//
// Works in a scratch directory under the current directory (ctest runs it in the build tree).
// The "data" files it creates are empty placeholders for name lookups only; nothing here reads
// game data.

#include "ra_test.h"
#include "ra_boot.h"
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

static void Touch(char const * path)
{
	FILE * f = fopen(path, "wb");
	if (f) fclose(f);
}

static void Setup(void)
{
	static bool done = false;
	if (done) return;
	done = true;
	MAKE_DIR("s10c_boot");
	MAKE_DIR("s10c_boot/data");
	MAKE_DIR("s10c_boot/game");
	MAKE_DIR("s10c_boot/empty");
	Touch("s10c_boot/data/redalert.mix");		// lower case on purpose (ADR-0022)
	Touch("s10c_boot/game/Main.Mix");
	Touch("s10c_boot/game/REDALERT.MIX");		// a second copy: the data folder must win
	Touch("s10c_boot/data/expand2.mix");
}

RA_TEST(data_option_forms)
{
	char a0[] = "ra95", a1[] = "-DATA=/x/Red Alert", a2[] = "-CD.", a3[] = "--data", a4[] = "rel/dir", a5[] = "-480";
	char * argv[] = {a0, a1, a2, a3, a4, a5, NULL};
	int argc = 6;
	char out[256];
	bool empty = true;
	RA_CHECK(RA_Boot_Take_Data_Option(&argc, argv, out, sizeof(out), &empty));
	RA_CHECK(!empty);
	RA_CHECK(strcmp(out, "rel/dir") == 0);			// the last one wins, case kept
	RA_CHECK_EQ(3, argc);								// both options removed
	RA_CHECK(strcmp(argv[0], "ra95") == 0);
	RA_CHECK(strcmp(argv[1], "-CD.") == 0);
	RA_CHECK(strcmp(argv[2], "-480") == 0);
	RA_CHECK(argv[3] == NULL);

	char b0[] = "ra95", b1[] = "-data=";
	char * argv2[] = {b0, b1, NULL};
	argc = 2;
	RA_CHECK(RA_Boot_Take_Data_Option(&argc, argv2, out, sizeof(out), &empty));
	RA_CHECK(empty);
	RA_CHECK_EQ(1, argc);

	char c0[] = "ra95", c1[] = "-datafile", c2[] = "--data";	// not the option; option without a value
	char * argv3[] = {c0, c1, c2, NULL};
	argc = 3;
	RA_CHECK(RA_Boot_Take_Data_Option(&argc, argv3, out, sizeof(out), &empty));
	RA_CHECK(empty);
	RA_CHECK_EQ(2, argc);
	RA_CHECK(strcmp(argv3[1], "-datafile") == 0);

	char d0[] = "ra95", d1[] = "-attract";
	char * argv4[] = {d0, d1, NULL};
	argc = 2;
	RA_CHECK(!RA_Boot_Take_Data_Option(&argc, argv4, out, sizeof(out), &empty));
	RA_CHECK_EQ(2, argc);
}

RA_TEST(make_absolute)
{
	char out[512];
#if defined(_WIN32)
	RA_CHECK(RA_Boot_Make_Absolute("data", "C:\\games", out, sizeof(out)));
	RA_CHECK(strcmp(out, "C:\\games/data") == 0);
	RA_CHECK(RA_Boot_Make_Absolute("D:\\RA\\", "C:\\games", out, sizeof(out)));
	RA_CHECK(strcmp(out, "D:\\RA") == 0);
#else
	RA_CHECK(RA_Boot_Make_Absolute("data", "/home/p", out, sizeof(out)));
	RA_CHECK(strcmp(out, "/home/p/data") == 0);
	RA_CHECK(RA_Boot_Make_Absolute("./data/", "/home/p/", out, sizeof(out)));
	RA_CHECK(strcmp(out, "/home/p/data") == 0);
	RA_CHECK(RA_Boot_Make_Absolute("/opt/ra//", "/home/p", out, sizeof(out)));
	RA_CHECK(strcmp(out, "/opt/ra") == 0);
	RA_CHECK(RA_Boot_Make_Absolute(".", "/home/p", out, sizeof(out)));
	RA_CHECK(strcmp(out, "/home/p") == 0);
#endif
	RA_CHECK(!RA_Boot_Make_Absolute("", "/home/p", out, sizeof(out)));
	RA_CHECK(!RA_Boot_Make_Absolute("data", "/home/p", out, 4));
}

RA_TEST(executable_dir)
{
	char out[1024];
	RA_CHECK(RA_Boot_Executable_Dir(NULL, out, sizeof(out)));
	RA_CHECK(RA_Boot_Is_Directory(out));
}

RA_TEST(search_list_and_find)
{
	Setup();
	RABootSearchList list;
	RA_Boot_Search_Init(&list);
	RA_CHECK(!RA_Boot_Search_Add(&list, "", "empty"));
	RA_CHECK(RA_Boot_Search_Add(&list, "s10c_boot/data", "data folder"));
	RA_CHECK(RA_Boot_Search_Add(&list, "s10c_boot/game/", "game folder"));
	RA_CHECK(!RA_Boot_Search_Add(&list, "s10c_boot/game", "duplicate"));
	RA_CHECK_EQ(2, list.Count);

	char found[512];
	RA_CHECK_EQ(0, RA_Boot_Find(&list, "REDALERT.MIX", found, sizeof(found)));	// data folder first
#if !defined(_WIN32)
	RA_CHECK(strcmp(found, "s10c_boot/data/redalert.mix") == 0);
#endif
	RA_CHECK_EQ(1, RA_Boot_Find(&list, "MAIN.MIX", NULL, 0));
	RA_CHECK_EQ(-1, RA_Boot_Find(&list, "EXPAND.MIX", NULL, 0));
	RA_CHECK_EQ(0, RA_Boot_Find(&list, RA_BOOT_AFTERMATH_MIX, NULL, 0));
	RA_CHECK_EQ(-1, RA_Boot_Find(&list, "data", NULL, 0));		// a folder is not a file

	char search[2048];
	RA_CHECK(RA_Boot_Search_String(&list, search, sizeof(search)));
	RA_CHECK(strcmp(search, "s10c_boot/data;s10c_boot/game") == 0);
	RA_CHECK(!RA_Boot_Search_String(&list, search, 10));
}

RA_TEST(required_files_present)
{
	Setup();
	RABootSearchList list;
	RA_Boot_Search_Init(&list);
	RA_Boot_Search_Add(&list, "s10c_boot/data", "data folder");
	RA_Boot_Search_Add(&list, "s10c_boot/game", "game folder");
	char report[4096] = "x";
	RA_CHECK(RA_Boot_Check_Required(&list, report, sizeof(report)));
	RA_CHECK(report[0] == '\0');
}

RA_TEST(required_files_missing_report)
{
	Setup();
	RABootSearchList list;
	RA_Boot_Search_Init(&list);
	RA_Boot_Search_Add(&list, "s10c_boot/empty", "data folder, from -data");
	RA_Boot_Search_Add(&list, "s10c_boot/nosuch", "start-up folder");
	char report[4096];
	RA_CHECK(!RA_Boot_Check_Required(&list, report, sizeof(report)));
	RA_CHECK(strstr(report, "REDALERT.MIX") != NULL);
	RA_CHECK(strstr(report, "MAIN.MIX") != NULL);
	RA_CHECK(strstr(report, "1. s10c_boot/empty (data folder, from -data)\n") != NULL);
	RA_CHECK(strstr(report, "2. s10c_boot/nosuch (start-up folder) - does not exist") != NULL);
	RA_CHECK(strstr(report, "-data=<folder>") != NULL);

	// A small buffer is truncated, never overrun.
	char small[32];
	memset(small, 'z', sizeof(small));
	RA_CHECK(!RA_Boot_Check_Required(&list, small, sizeof(small)));
	RA_CHECK_EQ((size_t)31, strlen(small));
}

RA_TEST(first_run_config)
{
	Setup();
	RA_File_Delete("s10c_boot/game/REDALERT.INI");
	RA_CHECK_EQ(1, RA_Boot_Ensure_Config("s10c_boot/game/REDALERT.INI"));
	RA_CHECK_EQ(0, RA_Boot_Ensure_Config("s10c_boot/game/REDALERT.INI"));
#if !defined(_WIN32)
	RA_CHECK_EQ(0, RA_Boot_Ensure_Config("S10C_BOOT/GAME/redalert.ini"));		// found case-insensitively
#endif

	FILE * f = fopen("s10c_boot/game/REDALERT.INI", "rb");
	RA_CHECK(f != NULL);
	char text[2048] = "";
	if (f) {
		size_t n = fread(text, 1, sizeof(text) - 1, f);
		text[n] = '\0';
		fclose(f);
	}
	RA_CHECK(strcmp(text, RA_Boot_Default_Config) == 0);
	RA_CHECK(strstr(text, "[Data]\n") != NULL);
	RA_CHECK(strstr(text, "\nPath=\n") != NULL);
	RA_CHECK(strstr(text, "[Options]\n") != NULL);
	RA_CHECK(strstr(text, "\nResolution=0\n") != NULL);
	RA_CHECK(strstr(text, "PlayIntro") == NULL);

	RA_CHECK_EQ(-1, RA_Boot_Ensure_Config("s10c_boot/nosuch/REDALERT.INI"));
}

RA_TEST_MAIN()
