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
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                 Project Name : Command & Conquer                        *
 *                    File Name : SKIRMDLG.CPP                             *
 *                   Programmer : Bill R. Randolph                         *
 *                   Start Date : 04/29/95                                 *
 *-------------------------------------------------------------------------*
 * Skirmish game setup dialog. This code was part of NULLDLG.CPP (the      *
 * modem/null-modem dialogs) and was moved here in session S05; then the   *
 * modem (serial host) code paths were removed. Only skirmish == true is   *
 * supported now; the remaining !skirmish branches are dead UI code.       *
 * See docs/codebase/removed-code.md.                                      *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Com_Scenario_Dialog -- Serial game scenario selection dialog				*
 *   Find_Local_Scenario -- finds the file name of the scenario with       *
 *                          matching attributes                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "function.h"
#ifdef FIXIT_RANDOM_GAME
#include "time.h"
#endif

#ifdef FIXIT_CSII	//	checked - ajw 9/28/98
extern bool Is_Mission_126x126 (char *file_name);
extern bool Is_Mission_Counterstrike (char *file_name);
extern bool Is_Mission_Aftermath( char* file_name );
#endif

//extern char const *ForMisStr[];
extern char const *EngMisStr[];


#ifdef FIXIT_VERSION_3
bool Force_Scenario_Available( const char* szName );
bool bSpecialAftermathScenario( const char* szScenarioDescription );
#endif

#define PCOLOR_BROWN	PCOLOR_GREY


/***********************************************************************************************
 * Com_Scenario_Dialog -- Serial game scenario selection dialog										  *
 *                                                                         						  *
 *                                                                         						  *
 * INPUT:                                                                  						  *
 *		none.																												  *
 *                                                                         						  *
 * OUTPUT:                                                                 						  *
 *		true = success, false = cancel																			  *
 *                                                                         						  *
 * WARNINGS:                                                               						  *
 *		none.																												  *
 *                                                                         						  *
 * HISTORY:                                                                						  *
 *   02/14/1995 BR : Created.
 *   01/21/97 V.Grippi added check for CS before sending scenario file                                             						  *
 *=============================================================================================*/
int Com_Scenario_Dialog(bool skirmish)
{
	/*........................................................................
	Dialog & button dimensions
	........................................................................*/
	int d_dialog_w = 320 * RESFACTOR;									// dialog width
	int d_dialog_h = 200 * RESFACTOR;									// dialog height
	int d_dialog_x = ((320 * RESFACTOR - d_dialog_w) / 2);		// dialog x-coord
	int d_dialog_y = ((200 * RESFACTOR - d_dialog_h) / 2);		// dialog y-coord
	int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);		// center x-coord

	int d_txt6_h = 6 * RESFACTOR+1;										// ht of 6-pt text
	int d_margin1 = 5 * RESFACTOR;										// margin width/height
	int d_margin2 = 7 * RESFACTOR;										// margin width/height

	int d_name_w = 70 * RESFACTOR;
	int d_name_h = 9 * RESFACTOR;
	int d_name_x = d_dialog_x + (d_dialog_w / 4) - (d_name_w / 2);
	int d_name_y = d_dialog_y + d_margin2 + d_txt6_h + 1 * RESFACTOR;

#ifdef OLDWAY
	int d_gdi_w = 40 * RESFACTOR;
	int d_gdi_h = 9 * RESFACTOR;
	int d_gdi_x = d_dialog_cx - d_gdi_w;
	int d_gdi_y = d_name_y;

	int d_nod_w = 40 * RESFACTOR;
	int d_nod_h = 9 * RESFACTOR;
	int d_nod_x = d_dialog_cx;
	int d_nod_y = d_name_y;
#else
	int d_house_w = 60 *RESFACTOR;
	int d_house_h = (8 * 5 *RESFACTOR);
	int d_house_x = d_dialog_cx - (d_house_w / 2);
	int d_house_y = d_name_y;
#endif

	int d_color_w = 10 * RESFACTOR;
	int d_color_h = 9 * RESFACTOR;
	int d_color_y = d_name_y;
	int d_color_x = d_dialog_x + ((d_dialog_w / 4) * 3) - (d_color_w * 3);

	int d_playerlist_w = 118 * RESFACTOR;
	int d_playerlist_h = (6 * 6 * RESFACTOR) + 3 * RESFACTOR;			// 6 rows high
	int d_playerlist_x = d_dialog_x + d_margin1 + d_margin1 + 5*RESFACTOR;
	int d_playerlist_y = d_color_y + d_color_h + d_margin2 + 2*RESFACTOR/*KO + d_txt6_h*/;

	int d_scenariolist_w = 162 * RESFACTOR;
	int d_scenariolist_h = (6 * 6 * RESFACTOR) + 3 * RESFACTOR;		// 6 rows high

	if (skirmish) {
		d_scenariolist_h *= 2;
	}


	int d_scenariolist_x = d_dialog_x + d_dialog_w - d_margin1 - d_margin1 - d_scenariolist_w - 5*RESFACTOR;
	int d_scenariolist_y = d_color_y + d_color_h + d_margin2 + 2*RESFACTOR;

	if (skirmish) {
		d_scenariolist_x = d_dialog_x + (d_dialog_w-d_scenariolist_w)/2;
	}

	int d_count_w = 25 * RESFACTOR;
	int d_count_h = d_txt6_h;
	int d_count_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR;		// (fudged)
	int d_count_y = d_playerlist_y + d_playerlist_h + (d_margin1 * 2) - 2 * RESFACTOR;

	if (skirmish) {
		d_count_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 2*RESFACTOR;
	}

	int d_level_w = 25 * RESFACTOR;
	int d_level_h = d_txt6_h;
	int d_level_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR;		// (fudged)
	int d_level_y = d_count_y + d_count_h;

	int d_credits_w = 25 * RESFACTOR;
	int d_credits_h = d_txt6_h;
	int d_credits_x = d_playerlist_x + (d_playerlist_w / 2) + 20 * RESFACTOR;	// (fudged)
	int d_credits_y = d_level_y + d_level_h;

	int d_aiplayers_w = 25*RESFACTOR;
	int d_aiplayers_h = d_txt6_h;
	int d_aiplayers_x = d_playerlist_x + (d_playerlist_w / 2) + 20*RESFACTOR;	// (fudged)
	int d_aiplayers_y = d_credits_y + d_credits_h;

	int d_options_w = 106 * RESFACTOR;
	int d_options_h = (5 * 6* RESFACTOR) + 4*RESFACTOR;
	int d_options_x = d_dialog_x + d_dialog_w - 149 * RESFACTOR;
	int d_options_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 2*RESFACTOR;

	int d_message_w = d_dialog_w - (d_margin1 * 2) - 20*RESFACTOR;
	int d_message_h = (8 * d_txt6_h) + 3 * RESFACTOR;		// 4 rows high
	int d_message_x = d_dialog_x + d_margin1 + 10*RESFACTOR;
	int d_message_y = d_options_y + d_options_h + 2*RESFACTOR;

	int d_send_w = d_dialog_w - (d_margin1 * 2) - 20*RESFACTOR;
	int d_send_h = 9 * RESFACTOR;
	int d_send_x = d_dialog_x + d_margin1 + 10*RESFACTOR;
	int d_send_y = d_message_y + d_message_h;

	int d_ok_w = 45 * RESFACTOR;
	int d_ok_h = 9 * RESFACTOR;
	int d_ok_x = d_dialog_x + (d_dialog_w / 6) - (d_ok_w / 2);
	int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - RESFACTOR*6;

	int d_cancel_w = 45 * RESFACTOR;
	int d_cancel_h = 9 * RESFACTOR;
	int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
	int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin1 - RESFACTOR*6;

	int d_load_w = 45 * RESFACTOR;
	int d_load_h = 9 * RESFACTOR;
	int d_load_x = d_dialog_x + ((d_dialog_w * 5) / 6) - (d_load_w / 2);
	int d_load_y = d_dialog_y + d_dialog_h - d_load_h - d_margin1 - RESFACTOR*6;


	/*........................................................................
	Button Enumerations
	........................................................................*/
	enum {
		BUTTON_NAME = 100,
#ifdef OLDWAY
		BUTTON_GDI,
		BUTTON_NOD,
#else
		BUTTON_HOUSE,
#endif
		BUTTON_CREDITS,
		BUTTON_AIPLAYERS,
		BUTTON_OPTIONS,
		BUTTON_PLAYERLIST,
		BUTTON_SCENARIOLIST,
		BUTTON_COUNT,
		BUTTON_LEVEL,
		BUTTON_OK,
		BUTTON_LOAD,
		BUTTON_CANCEL,
		BUTTON_DIFFICULTY,
	};

	/*........................................................................
	Redraw values: in order from "top" to "bottom" layer of the dialog
	........................................................................*/
	typedef enum {
		REDRAW_NONE = 0,
		REDRAW_PARMS,
		REDRAW_MESSAGE,
		REDRAW_COLORS,
		REDRAW_BUTTONS,
		REDRAW_BACKGROUND,
		REDRAW_ALL = REDRAW_BACKGROUND
	} RedrawType;

	/*........................................................................
	Dialog variables
	........................................................................*/
	RedrawType display = REDRAW_ALL;		// redraw level
	bool process = true;						// process while true
	KeyNumType input;

	int playertabs[] = {77*RESFACTOR};	// tabs for player list box
	int optiontabs[] = {8};					// tabs for player list box
	char namebuf[MPLAYER_NAME_MAX] = {0};		// buffer for player's name
	bool transmit;								// 1 = re-transmit new game options
	int cbox_x[] = {
							d_color_x,
							d_color_x + d_color_w,
							d_color_x + (d_color_w * 2),
							d_color_x + (d_color_w * 3),
							d_color_x + (d_color_w * 4),
							d_color_x + (d_color_w * 5),
							d_color_x + (d_color_w * 6),
							d_color_x + (d_color_w * 7)
							};
	bool changed = false;							// 1 = user has changed an option

	int rc;
	int i;
	unsigned long transmittime = 0;
	static bool first_time = true;
	bool gameoptions = Session.Type == GAME_SKIRMISH;

	CCFileClass loadfile ("SAVEGAME.NET");
	bool load_game = false;					// 1 = load a saved game
	NodeNameType *who;					// node to add to Players
	char *item;								// for filling in lists
	RemapControlType * scheme = GadgetClass::Get_Color_Scheme();
	bool messages_have_focus = true;		// Gadget focus starts on the message system

	Set_Logic_Page(SeenBuff);

	CDTimerClass<SystemTimerClass> kludge_timer;		// Timer to allow a wait after client joins
																	// game before game can start
	bool ok_button_added = false;

	/*........................................................................
	Buttons
	........................................................................*/
	GadgetClass * commands;										// button list

	EditClass name_edt(BUTTON_NAME, namebuf, MPLAYER_NAME_MAX, TPF_TEXT, d_name_x, d_name_y, d_name_w, d_name_h, EditClass::ALPHANUMERIC);

#ifdef OLDWAY
	TextButtonClass gdibtn(BUTTON_GDI, TXT_ALLIES, TPF_BUTTON, d_gdi_x, d_gdi_y, d_gdi_w, d_gdi_h);
	TextButtonClass nodbtn(BUTTON_NOD, TXT_SOVIET, TPF_BUTTON, d_nod_x, d_nod_y, d_nod_w, d_nod_h);
#else
	char housetext[25] = "";
	Fancy_Text_Print("", 0, 0, 0, 0, TPF_TEXT);
	DropListClass housebtn(BUTTON_HOUSE, housetext, sizeof(housetext),
		TPF_TEXT,
		d_house_x, d_house_y, d_house_w, d_house_h,
		MFCD::Retrieve("BTN-UP.SHP"),
		MFCD::Retrieve("BTN-DN.SHP"));
#endif
	ColorListClass playerlist(BUTTON_PLAYERLIST, d_playerlist_x, d_playerlist_y, d_playerlist_w, d_playerlist_h, TPF_TEXT, MFCD::Retrieve("BTN-UP.SHP"), MFCD::Retrieve("BTN-DN.SHP"));
	ListClass scenariolist(BUTTON_SCENARIOLIST, d_scenariolist_x, d_scenariolist_y, d_scenariolist_w, d_scenariolist_h, TPF_TEXT, MFCD::Retrieve("BTN-UP.SHP"), MFCD::Retrieve("BTN-DN.SHP"));
	GaugeClass countgauge(BUTTON_COUNT, d_count_x, d_count_y, d_count_w, d_count_h);

	char staticcountbuff[35];
	StaticButtonClass staticcount(0, "     ", TPF_TEXT, d_count_x+d_count_w+3*RESFACTOR, d_count_y);

	GaugeClass levelgauge(BUTTON_LEVEL, d_level_x, d_level_y, d_level_w, d_level_h);

	char staticlevelbuff[35];
	StaticButtonClass staticlevel(0, "     ", TPF_TEXT, d_level_x+d_level_w+3*RESFACTOR, d_level_y);

	GaugeClass creditsgauge(BUTTON_CREDITS, d_credits_x, d_credits_y, d_credits_w, d_credits_h);

	char staticcreditsbuff[35];
	StaticButtonClass staticcredits(0, "         ", TPF_TEXT, d_credits_x+d_credits_w+3*RESFACTOR, d_credits_y);

	GaugeClass aiplayersgauge(BUTTON_AIPLAYERS, d_aiplayers_x, d_aiplayers_y, d_aiplayers_w, d_aiplayers_h);

	char staticaibuff[35];
	StaticButtonClass staticai(0, "     ", TPF_TEXT, d_aiplayers_x+d_aiplayers_w+3*RESFACTOR, d_aiplayers_y);

	CheckListClass optionlist(BUTTON_OPTIONS, d_options_x, d_options_y, d_options_w, d_options_h, TPF_TEXT, MFCD::Retrieve("BTN-UP.SHP"), MFCD::Retrieve("BTN-DN.SHP"));
	TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_BUTTON, d_ok_x, d_ok_y, d_ok_w, d_ok_h);
	TextButtonClass loadbtn(BUTTON_LOAD, TXT_LOAD_BUTTON, TPF_BUTTON, d_load_x, d_load_y, d_load_w, d_load_h);
	TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_BUTTON, d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

	SliderClass difficulty(BUTTON_DIFFICULTY, d_name_x, optionlist.Y + optionlist.Height + d_margin1 + d_margin1, d_dialog_w - (d_name_x-d_dialog_x)*2, 8*RESFACTOR, true);
	if (Rule.IsFineDifficulty) {
		difficulty.Set_Maximum(5);
		difficulty.Set_Value(2);
	} else {
		difficulty.Set_Maximum(3);
		difficulty.Set_Value(1);
	}

	/*
	------------------------- Build the button list --------------------------
	*/
	commands = &name_edt;
	staticcount.Add_Tail(*commands);
	staticcredits.Add_Tail(*commands);
	staticai.Add_Tail(*commands);
	staticlevel.Add_Tail(*commands);
	if (!skirmish) {
		playerlist.Add_Tail(*commands);
	} else {
		difficulty.Add_Tail(*commands);
	}
	scenariolist.Add_Tail(*commands);
	countgauge.Add_Tail(*commands);
	levelgauge.Add_Tail(*commands);
	creditsgauge.Add_Tail(*commands);
	aiplayersgauge.Add_Tail(*commands);
	optionlist.Add_Tail(*commands);
	if (Session.Type == GAME_SKIRMISH){
		okbtn.Add_Tail(*commands);
	}
	cancelbtn.Add_Tail(*commands);
	if (!skirmish && loadfile.Is_Available()) {
#ifdef FIXIT_MULTI_SAVE
		//Load button added only when other player has arrived
		//loadbtn.Add_Tail(*commands);
#endif
	} else {
		cancelbtn.X = loadbtn.X;
	}
#ifdef OLDWAY
	gdibtn.Add_Tail(*commands);
	nodbtn.Add_Tail(*commands);
#else
	housebtn.Add_Tail(*commands);
#endif


	/*
	----------------------------- Various Inits ------------------------------
	*/
	/*........................................................................
	Init player name & house
	........................................................................*/
	Session.ColorIdx = Session.PrefColor;			// init my preferred color
	strcpy (namebuf, Session.Handle);				// set my name
	name_edt.Set_Text(namebuf,MPLAYER_NAME_MAX);
	name_edt.Set_Color(&ColorRemaps[(Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx]);

#ifdef OLDWAY
	if (Session.House==HOUSE_GOOD) {
		gdibtn.Turn_On();
	} else {
		nodbtn.Turn_On();
	}
#else
	for (HousesType house = HOUSE_USSR; house <= HOUSE_FRANCE; house++) {
		housebtn.Add_Item(Text_String(HouseTypeClass::As_Reference(house).Full_Name()));
	}
	housebtn.Set_Selected_Index(Session.House - HOUSE_USSR);
	housebtn.Set_Read_Only (true);
#endif

	/*........................................................................
	Init scenario values, only the first time through
	........................................................................*/
	Special.IsCaptureTheFlag = Rule.IsMPCaptureTheFlag;
	if (first_time) {
		Session.Options.Credits = Rule.MPDefaultMoney;	// init credits & credit buffer
		Session.Options.Bases = Rule.IsMPBasesOn;			// init scenario parameters
		Session.Options.Tiberium = Rule.IsMPTiberiumGrow;
		Session.Options.Goodies = Rule.IsMPCrates;
		Session.Options.AIPlayers = 0;
		Special.IsShadowGrow = Rule.IsMPShadowGrow;
		Session.Options.UnitCount = (SessionClass::CountMax[Session.Options.Bases] + SessionClass::CountMin[Session.Options.Bases]) / 2;
		first_time = false;
	}

	/*........................................................................
	Init button states
	........................................................................*/
	playerlist.Set_Tabs(playertabs);
	playerlist.Set_Selected_Style(ColorListClass::SELECT_NORMAL);

	optionlist.Set_Tabs(optiontabs);
	optionlist.Set_Read_Only(0);

	optionlist.Add_Item(Text_String(TXT_BASES));
	optionlist.Add_Item(Text_String(TXT_ORE_SPREADS));
	optionlist.Add_Item(Text_String(TXT_CRATES));
	optionlist.Add_Item(Text_String(TXT_SHADOW_REGROWS));
	if (!skirmish) {
		optionlist.Add_Item(Text_String(TXT_CAPTURE_THE_FLAG));
	}

	optionlist.Check_Item(0, Session.Options.Bases);
	optionlist.Check_Item(1, Session.Options.Tiberium);
	optionlist.Check_Item(2, Session.Options.Goodies);
	optionlist.Check_Item(3, Special.IsShadowGrow);
	if (!skirmish) {
		optionlist.Check_Item(4, Special.IsCaptureTheFlag);
	}

	countgauge.Set_Maximum(SessionClass::CountMax[Session.Options.Bases] - SessionClass::CountMin[Session.Options.Bases]);
	countgauge.Set_Value(Session.Options.UnitCount - SessionClass::CountMin[Session.Options.Bases]);

	levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
	levelgauge.Set_Value(BuildLevel - 1);

	creditsgauge.Set_Maximum(Rule.MPMaxMoney);
	creditsgauge.Set_Value(Session.Options.Credits);

	int maxp = Rule.MaxPlayers - 2;
//	int maxp = Rule.MaxPlayers - (skirmish ? 1 : 2);
	aiplayersgauge.Set_Maximum(maxp);

	if (skirmish) {
		if ( Session.Options.AIPlayers > 7 ) {
			Session.Options.AIPlayers = 7;
		}
		Session.Options.AIPlayers = max(Session.Options.AIPlayers, 1);
	}else{
		if ( Session.Options.AIPlayers > 6 ) {
			Session.Options.AIPlayers = 6;
		}
	}

	aiplayersgauge.Set_Value(Session.Options.AIPlayers - (skirmish ? 1 : 0));

	/*........................................................................
	Init other scenario parameters
	........................................................................*/
	Special.IsTGrowth = //Session.Options.Tiberium;
		Rule.IsTGrowth = //Session.Options.Tiberium;
			Special.IsTSpread = //Session.Options.Tiberium;
				Rule.IsTSpread = Session.Options.Tiberium;
	transmit = true;

	/*........................................................................
	Clear the Players vector
	........................................................................*/
	Clear_Vector(&Session.Players);

	/*........................................................................
	Init scenario description list box
	........................................................................*/
	for (i = 0; i < Session.Scenarios.Count(); i++) {
		int j;
		for (j = 0; EngMisStr[j] != NULL;  j++) {
			if (!strcmp(Session.Scenarios[i]->Description(), EngMisStr[j])) {
#ifdef FIXIT_CSII	//	ajw Added Aftermath installed checks (before, it was assumed).
			//	Add mission if it's available to us.
			if( !( ( Is_Mission_Counterstrike((char *)(Session.Scenarios[i]->Get_Filename())) && !Is_Counterstrike_Installed() ) ||
					( Is_Mission_Aftermath((char *)(Session.Scenarios[i]->Get_Filename())) && !Is_Aftermath_Installed() ) ) )
#endif
#if defined(GERMAN) || defined(FRENCH)
				scenariolist.Add_Item(EngMisStr[j+1]);
#else
				scenariolist.Add_Item(EngMisStr[j]);
#endif
				break;
			}
		}
		if (EngMisStr[j] == NULL) {
#ifdef FIXIT_CSII	//	ajw Added Aftermath installed checks (before, it was assumed). Added officialness check.
			//	Add mission if it's available to us.
			if( !Session.Scenarios[i]->Get_Official() ||
				!( ( Is_Mission_Counterstrike((char *)(Session.Scenarios[i]->Get_Filename())) && !Is_Counterstrike_Installed() ) ||
					( Is_Mission_Aftermath((char *)(Session.Scenarios[i]->Get_Filename())) && !Is_Aftermath_Installed() ) ) )
#endif
			scenariolist.Add_Item(Session.Scenarios[i]->Description());
		}
	}

	Session.Options.ScenarioIndex = 0;		// 1st scenario is selected

	/*........................................................................
	Init random-number generator, & create a seed to be used for all random
	numbers from here on out
	........................................................................*/
#ifdef FIXIT_RANDOM_GAME
	srand(time(NULL));
	Seed = rand();
#else
//	randomize();
//	Seed = rand();
#endif

	/*........................................................................
	Init the message display system
	........................................................................*/
	if (!skirmish) {
		Session.Messages.Init (d_message_x + 1, d_message_y + 1, 8,
			MAX_MESSAGE_LENGTH, d_txt6_h, d_send_x + 1, d_send_y + 1, 1,
			20, MAX_MESSAGE_LENGTH - 5, d_message_w);
		Session.Messages.Add_Edit ((Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx,
									TPF_TEXT, NULL, '_', d_message_w);
	}

	/*........................................................................
	Init version number clipping system
	........................................................................*/
	VerNum.Init_Clipping();
	Load_Title_Page(true);
	CCPalette.Set();

	/*
	---------------------------- Processing loop -----------------------------
	*/


	while (process) {
		if (!skirmish){
			if (!ok_button_added && gameoptions && kludge_timer == 0){
				okbtn.Add_Tail(*commands);
				ok_button_added = true;
#ifdef FIXIT_VERSION_3
				if( loadfile.Is_Available() )
				{
					loadbtn.Add_Tail( *commands );
				}
#else
#ifdef FIXIT_MULTI_SAVE
				if ( loadfile.Is_Available() && PlayingAgainstVersion > VERSION_RED_ALERT_104 ) {
					loadbtn.Add_Tail ( *commands );
				}
#endif
#endif
				if (display < REDRAW_BUTTONS) display = REDRAW_BUTTONS;
			}
		}

		/*
		** Kludge to make sure we redraw the message input line when it loses focus.
		** If we dont do this then the cursor doesnt disappear.
		*/
		if (!skirmish) {
			if (messages_have_focus) {
				if (name_edt.Has_Focus()) {
					if (display < REDRAW_MESSAGE) display = REDRAW_MESSAGE;
					messages_have_focus = false;
				}
			} else {
				if (!name_edt.Has_Focus()) {
					messages_have_focus = true;
					if (display < REDRAW_MESSAGE)	display = REDRAW_MESSAGE;
					Session.Messages.Set_Edit_Focus();
				}
			}
		}

		/*
		........................ Invoke game callback .........................
		*/
		Call_Back();

		/*
		** If we have just received input focus again after running in the background then
		** we need to redraw.
		*/
		if (AllSurfaces.SurfacesRestored) {
			AllSurfaces.SurfacesRestored=FALSE;
			display = REDRAW_ALL;
		}

		/*
		...................... Refresh display if needed ......................
		*/
		if (display) {
			if (housebtn.IsDropped) {
				housebtn.Collapse();
				display = REDRAW_BACKGROUND;
			}
			Hide_Mouse();

			/*
			.................. Redraw backgound & dialog box ...................
			*/
			if (display >= REDRAW_BACKGROUND) {
				Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

				// init font variables

				Fancy_Text_Print(TXT_NONE, 0, 0, (RemapControlType *)NULL, TBLACK, TPF_CENTER | TPF_TEXT);

				/*...............................................................
				Dialog & Field labels
				...............................................................*/
				Fancy_Text_Print(TXT_YOUR_NAME, d_name_x + (d_name_w / 2), d_name_y - d_txt6_h, scheme, TBLACK, TPF_CENTER | TPF_TEXT);
#ifdef OLDWAY
				Fancy_Text_Print(TXT_SIDE_COLON, d_gdi_x + d_gdi_w, d_gdi_y - d_txt6_h, scheme, TBLACK, TPF_CENTER | TPF_TEXT);
#else
				Fancy_Text_Print(TXT_SIDE_COLON, d_house_x + (d_house_w / 2), d_house_y - d_txt6_h, scheme, TBLACK, TPF_CENTER | TPF_TEXT);
#endif
				Fancy_Text_Print(TXT_COLOR_COLON, d_dialog_x + ((d_dialog_w / 4) * 3), d_color_y - d_txt6_h, scheme, TBLACK, TPF_CENTER | TPF_TEXT);
				if (!skirmish) {
					Fancy_Text_Print(TXT_PLAYERS, d_playerlist_x + (d_playerlist_w / 2), d_playerlist_y - d_txt6_h, scheme, TBLACK, TPF_CENTER | TPF_TEXT);
				} else {
					Fancy_Text_Print(TXT_EASY, difficulty.X, difficulty.Y-8*RESFACTOR, scheme, TBLACK, TPF_TEXT);
					Fancy_Text_Print(TXT_HARD, difficulty.X + difficulty.Width, difficulty.Y-8*RESFACTOR, scheme, TBLACK, TPF_RIGHT|TPF_TEXT);
					Fancy_Text_Print(TXT_NORMAL, difficulty.X + difficulty.Width/2, difficulty.Y-8*RESFACTOR, scheme, TBLACK, TPF_CENTER|TPF_TEXT);
				}
				Fancy_Text_Print(TXT_SCENARIOS, d_scenariolist_x + (d_scenariolist_w / 2), d_scenariolist_y - d_txt6_h, scheme, TBLACK, TPF_CENTER | TPF_TEXT);
				Fancy_Text_Print(TXT_COUNT, d_count_x - 2, d_count_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
				Fancy_Text_Print(TXT_LEVEL, d_level_x - 2, d_level_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
				Fancy_Text_Print(TXT_CREDITS_COLON, d_credits_x - 2, d_credits_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
				Fancy_Text_Print(TXT_AI_PLAYERS_COLON, d_aiplayers_x - 2*RESFACTOR, d_aiplayers_y, scheme, TBLACK, TPF_TEXT | TPF_RIGHT);
			}

			/*..................................................................
			Draw the color boxes
			..................................................................*/
			if (display >= REDRAW_COLORS) {
				for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
					LogicPage->Fill_Rect (cbox_x[i] + 1, d_color_y + 1,
						cbox_x[i] + 1 + d_color_w - 2, d_color_y + 1 + d_color_h - 2,
						ColorRemaps[i].Box);
//						(i == PCOLOR_DIALOG_BLUE) ? ColorRemaps[PCOLOR_REALLY_BLUE].Box : ColorRemaps[i].Box);

					if (i == Session.ColorIdx) {
						Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_DOWN, false);
					} else {
						Draw_Box(cbox_x[i], d_color_y, d_color_w, d_color_h, BOXSTYLE_RAISED, false);
					}
				}
			}

			/*..................................................................
			Draw the message system; erase old messages first
			..................................................................*/
			if (display >= REDRAW_MESSAGE && !skirmish) {
				Draw_Box(d_message_x, d_message_y, d_message_w, d_message_h, BOXSTYLE_BOX, true);
				Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h, BOXSTYLE_BOX, true);
				Session.Messages.Draw();
			}

			//..................................................................
			// Update game parameter labels
			//..................................................................
			if (display >= REDRAW_PARMS) {

//				LogicPage->Fill_Rect(d_count_x + d_count_w + 2, d_count_y, d_count_x + d_count_w + 35 * RESFACTOR, d_aiplayers_y + d_aiplayers_h+RESFACTOR, BLACK);

				sprintf(staticcountbuff, "%d", Session.Options.UnitCount);
				staticcount.Set_Text(staticcountbuff);
				staticcount.Draw_Me();
//				Fancy_Text_Print("%d ", d_count_x + d_count_w + 3 * RESFACTOR, d_count_y, scheme, BLACK, TPF_TEXT, Session.Options.UnitCount);

				if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
					sprintf(staticlevelbuff, "%d ", BuildLevel);
				} else {
					sprintf(staticlevelbuff, "**");
				}
				staticlevel.Set_Text(staticlevelbuff);
				staticlevel.Draw_Me();
//				Fancy_Text_Print(txt, d_level_x + d_level_w + 3 * RESFACTOR, d_level_y, scheme, BLACK, TPF_TEXT);

				sprintf(staticcreditsbuff, "%d", Session.Options.Credits);
				staticcredits.Set_Text(staticcreditsbuff);
				staticcredits.Draw_Me();
//				Fancy_Text_Print("%d", d_credits_x + d_credits_w + 2 * RESFACTOR, d_credits_y, scheme, BLACK, TPF_TEXT, Session.Options.Credits);

				sprintf(staticaibuff, "%d", Session.Options.AIPlayers);
				staticai.Set_Text(staticaibuff);
				staticai.Draw_Me();
//				Fancy_Text_Print("%d", d_aiplayers_x + d_aiplayers_w + 2*RESFACTOR, d_aiplayers_y, scheme, BLACK, TPF_TEXT, Session.Options.AIPlayers);
			}

			/*
			.......................... Redraw buttons ..........................
			*/
			if (display >= REDRAW_BUTTONS) {
				commands->Flag_List_To_Redraw();
				commands->Draw_All();
			}

			Show_Mouse();
			display = REDRAW_NONE;
		}

		/*
		........................... Get user input ............................
		*/
		messages_have_focus = Session.Messages.Has_Edit_Focus();
		bool droplist_is_dropped = housebtn.IsDropped;
		input = commands->Input();

		/*
		** Sort out the input focus between the name edit box and the message system
		*/
		if (!skirmish) {
			if (messages_have_focus) {
				if (!name_edt.Has_Focus()) {
					Session.Messages.Set_Edit_Focus();
				} else {
					messages_have_focus = false;
					display = REDRAW_MESSAGE;
				}
			}
		}

		/*
		** Redraw everything if the droplist collapsed
		*/
		if (droplist_is_dropped && !housebtn.IsDropped) {
			display = REDRAW_BACKGROUND;
		}

		if (input & KN_BUTTON) {
			if (housebtn.IsDropped) {
				housebtn.Collapse();
				display = REDRAW_BACKGROUND;
			}
		}


		/*
		---------------------------- Process input ----------------------------
		*/
		switch (input) {
			/*------------------------------------------------------------------
			User clicks on a color button
			------------------------------------------------------------------*/
			case KN_LMOUSE:
				if (Keyboard->MouseQX > cbox_x[0] &&
						Keyboard->MouseQX < (cbox_x[MAX_MPLAYER_COLORS - 1] + d_color_w) &&
						Keyboard->MouseQY > d_color_y &&
						Keyboard->MouseQY < (d_color_y + d_color_h)) {

					Session.PrefColor = (PlayerColorType)((Keyboard->MouseQX - cbox_x[0]) / d_color_w);
					Session.ColorIdx = Session.PrefColor;
					if (display < REDRAW_COLORS) display = REDRAW_COLORS;

					name_edt.Set_Color (&ColorRemaps[(Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx]);
					name_edt.Flag_To_Redraw();
					Session.Messages.Set_Edit_Color((Session.ColorIdx == PCOLOR_DIALOG_BLUE) ? PCOLOR_REALLY_BLUE : Session.ColorIdx);
					strcpy (Session.Handle, namebuf);
					transmit = true;
					changed = true;
					if (housebtn.IsDropped) {
						housebtn.Collapse();
						display = REDRAW_BACKGROUND;
					}
				}
				break;

			/*------------------------------------------------------------------
			User edits the name field; retransmit new game options
			------------------------------------------------------------------*/
			case (BUTTON_NAME | KN_BUTTON):
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				strcpy (Session.Handle, namebuf);
				transmit = true;
				changed = true;
				break;

#ifdef OLDWAY
			/*------------------------------------------------------------------
			House Buttons: set the player's desired House
			------------------------------------------------------------------*/
			case (BUTTON_GDI | KN_BUTTON):
				Session.House = HOUSE_GOOD;
				gdibtn.Turn_On();
				nodbtn.Turn_Off();
				strcpy (Session.Handle, namebuf);
				transmit = true;
				break;

			case (BUTTON_NOD | KN_BUTTON):
				Session.House = HOUSE_BAD;
				gdibtn.Turn_Off();
				nodbtn.Turn_On();
				strcpy (Session.Handle, namebuf);
				transmit = true;
				break;
#else
			case (BUTTON_HOUSE | KN_BUTTON):
				Session.House = HousesType(housebtn.Current_Index()+HOUSE_USSR);
				strcpy (Session.Handle, namebuf);
				display = REDRAW_BACKGROUND;
				transmit = true;
				break;
#endif

			/*------------------------------------------------------------------
			New Scenario selected.
			------------------------------------------------------------------*/
#ifdef FIXIT_VERSION_3		//	All scenarios now allowable as downloads. ajw
			case (BUTTON_SCENARIOLIST | KN_BUTTON):
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				if (scenariolist.Current_Index() != Session.Options.ScenarioIndex)
				{
					Session.Options.ScenarioIndex = scenariolist.Current_Index();
					strcpy (Session.Handle, namebuf);
					transmit = true;
				}
				break;

#else	//	FIXIT_VERSION_3		Whoever duplicated Netdlg into Nulldlg should be shot. Wasn't it enough?

				Abandon all hope ye who hit enter here.

			case (BUTTON_SCENARIOLIST | KN_BUTTON):
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				if (scenariolist.Current_Index() != Session.Options.ScenarioIndex) {
#ifdef FIXIT_CSII	//	checked - ajw
					if ( !skirmish && (PlayingAgainstVersion != VERSION_RED_ALERT_107 && PlayingAgainstVersion != VERSION_RED_ALERT_108 && PlayingAgainstVersion < VERSION_AFTERMATH_CS) &&
#else
					if ( !skirmish && PlayingAgainstVersion < VERSION_RED_ALERT_107 &&
#endif
								Session.Scenarios[scenariolist.Current_Index()]->Get_Expansion()){
						scenariolist.Set_Selected_Index (Session.Options.ScenarioIndex);
						Session.Messages.Add_Message(NULL, 0,
							(char *)Text_String(TXT_NO_CS_SCENARIOS), PCOLOR_BROWN, TPF_TEXT, 1200);
						Sound_Effect(VOC_SYS_ERROR);
						if (display < REDRAW_MESSAGE) display = REDRAW_MESSAGE;
#ifdef FIXIT_CSII	//	checked - ajw
					} else
					if ( !skirmish && PlayingAgainstVersion < VERSION_AFTERMATH_CS &&
							Is_Mission_126x126((char *)Session.Scenarios[scenariolist.Current_Index()]->Get_Filename() ) ){
						scenariolist.Set_Selected_Index (Session.Options.ScenarioIndex);
						Session.Messages.Add_Message(NULL, 0,
							(char *)Text_String(TXT_NO_CS_SCENARIOS), PCOLOR_BROWN, TPF_TEXT, 1200);
						Sound_Effect(VOC_SYS_ERROR);
						if (display < REDRAW_MESSAGE) display = REDRAW_MESSAGE;
#endif
					}else{
						Session.Options.ScenarioIndex = scenariolist.Current_Index();
						transmit = 1;
					}

					strcpy (Session.Handle, namebuf);
					transmit = true;
				}
				break;
#endif

			/*------------------------------------------------------------------
			User adjusts max # units
			------------------------------------------------------------------*/
			case (BUTTON_COUNT | KN_BUTTON):
				Session.Options.UnitCount = countgauge.Get_Value() + SessionClass::CountMin[Session.Options.Bases];
				if (display < REDRAW_PARMS) display = REDRAW_PARMS;
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				transmit = true;
				break;

			/*------------------------------------------------------------------
			User adjusts build level
			------------------------------------------------------------------*/
			case (BUTTON_LEVEL | KN_BUTTON):
				BuildLevel = levelgauge.Get_Value() + 1;
				if (BuildLevel > MPLAYER_BUILD_LEVEL_MAX)	// if it's pegged, max it out
					BuildLevel = MPLAYER_BUILD_LEVEL_MAX;
				if (display < REDRAW_PARMS) display = REDRAW_PARMS;
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				transmit = true;
				break;

			/*------------------------------------------------------------------
			User adjusts max # units
			------------------------------------------------------------------*/
			case (BUTTON_CREDITS | KN_BUTTON):
				Session.Options.Credits = creditsgauge.Get_Value();
				Session.Options.Credits = ((Session.Options.Credits + 250) / 500) * 500;
				if (display < REDRAW_PARMS) display = REDRAW_PARMS;
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				transmit = true;
				break;

			//..................................................................
			//	User adjusts # of AI players
			//..................................................................
			case (BUTTON_AIPLAYERS | KN_BUTTON):
				{
					Session.Options.AIPlayers = aiplayersgauge.Get_Value();
					int humans = 2;			// Two humans.
					if (skirmish) {
						Session.Options.AIPlayers += 1;	// Always one forced AI player.
						humans = 1;			// One human.
	//						if (Session.Options.AIPlayers == 0) {
	//							Session.Options.AIPlayers = 1;
	//							aiplayersgauge.Set_Value(0);
	//						}
					}
					if (Session.Options.AIPlayers+humans >= Rule.MaxPlayers) {	// if it's pegged, max it out
						Session.Options.AIPlayers = Rule.MaxPlayers - humans;
						aiplayersgauge.Set_Value(Session.Options.AIPlayers - (skirmish ? 1 : 0));
					}
					transmit = true;
					if (display < REDRAW_PARMS) display = REDRAW_PARMS;

					if (housebtn.IsDropped) {
						housebtn.Collapse();
						display = REDRAW_BACKGROUND;
					}

					break;
				}

			//------------------------------------------------------------------
			// Toggle-able options:
			// If 'Bases' gets toggled, we have to change the range of the
			// UnitCount slider.
			// Also, if Tiberium gets toggled, we have to set the flags
			// in SpecialClass.
			//------------------------------------------------------------------
			case (BUTTON_OPTIONS | KN_BUTTON):
				if (!skirmish && Special.IsCaptureTheFlag != optionlist.Is_Checked(4) && !Special.IsCaptureTheFlag) {
					optionlist.Check_Item(0, true);
				}
				if (Session.Options.Bases != optionlist.Is_Checked(0)) {
					Session.Options.Bases = optionlist.Is_Checked(0);
					if (Session.Options.Bases) {
						Session.Options.UnitCount = Fixed_To_Cardinal (
							SessionClass::CountMax[1] -
							SessionClass::CountMin[1],
							Cardinal_To_Fixed(
								SessionClass::CountMax[0]-SessionClass::CountMin[0],
								Session.Options.UnitCount-SessionClass::CountMin[0])) +
							SessionClass::CountMin[1];
					} else {
						if (!skirmish) optionlist.Check_Item(4, false);
						Session.Options.UnitCount = Fixed_To_Cardinal (
							SessionClass::CountMax[0] -
							SessionClass::CountMin[0],
							Cardinal_To_Fixed(
								SessionClass::CountMax[1]-SessionClass::CountMin[1],
								Session.Options.UnitCount - SessionClass::CountMin[1])) +
							SessionClass::CountMin[0];
					}
					countgauge.Set_Maximum(
						SessionClass::CountMax[Session.Options.Bases] -
						SessionClass::CountMin[Session.Options.Bases]);
					countgauge.Set_Value(Session.Options.UnitCount -
						SessionClass::CountMin[Session.Options.Bases]);
				}
				Session.Options.Tiberium = optionlist.Is_Checked(1);
				Special.IsTGrowth = Session.Options.Tiberium;
				Rule.IsTGrowth = Session.Options.Tiberium;
				Special.IsTSpread = Session.Options.Tiberium;
				Rule.IsTSpread = Session.Options.Tiberium;

				Session.Options.Goodies = optionlist.Is_Checked(2);
				Special.IsShadowGrow = optionlist.Is_Checked(3);
				if (!skirmish) {
					Special.IsCaptureTheFlag = optionlist.Is_Checked(4);
				}

				transmit = true;
				if (display < REDRAW_PARMS) display = REDRAW_PARMS;
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				break;

			/*------------------------------------------------------------------
			OK: exit loop with true status
			------------------------------------------------------------------*/
			case (BUTTON_LOAD | KN_BUTTON):
			case (BUTTON_OK | KN_BUTTON):
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				//
				// make sure we got a game options packet from the other player
				//
				if (gameoptions) {
					rc = true;
					process = false;

					// force transmitting of game options packet one last time

					transmit = true;
					transmittime = 0;
				} else {
					WWMessageBox().Process (TXT_ONLY_ONE,TXT_OOPS,NULL);
					display = REDRAW_ALL;
				}
				if (input==(BUTTON_LOAD | KN_BUTTON))
					load_game = true;
				break;

			/*------------------------------------------------------------------
			CANCEL: send a SIGN_OFF, bail out with error code
			------------------------------------------------------------------*/
			case (KN_ESC):
			case (BUTTON_CANCEL | KN_BUTTON):
				if (housebtn.IsDropped) {
					housebtn.Collapse();
					display = REDRAW_BACKGROUND;
				}
				process = false;
				rc = false;
				break;

			/*------------------------------------------------------------------
			Default: manage the inter-player messages
			------------------------------------------------------------------*/
			default:
				if (!skirmish) {
					if (Session.Messages.Manage()) {
						if (display < REDRAW_MESSAGE) display = REDRAW_MESSAGE;
					}

					/*...............................................................
					Service keyboard input for any message being edited.
					...............................................................*/
					i = Session.Messages.Input(input);

					/*...............................................................
					If 'Input' returned 1, it means refresh the message display.
					(We have to redraw the edit line, to erase the cursor.)
					...............................................................*/
					if (i==1) {
						Hide_Mouse();
						Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h,
							BOXSTYLE_BOX, true);
						Session.Messages.Draw();
						Show_Mouse();
					} else if (i==2) {
					/*...............................................................
					If 'Input' returned 2, it means redraw the message display.
					Rather than setting 'display', which would redraw all msgs,
					we only need to erase & redraw the edit box here.
					...............................................................*/
						Hide_Mouse();
						Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h, BOXSTYLE_BOX, true);
						Session.Messages.Draw();
						Show_Mouse();
					}
				}
				break;
		}

		/*---------------------------------------------------------------------
		Detect editing of the name buffer, transmit new values to players
		---------------------------------------------------------------------*/
		if (strcmp (namebuf, Session.Handle)) {
			strcpy (Session.Handle, namebuf);
			transmit = true;
			changed = true;
		}

	}

	/*------------------------------------------------------------------------
	Prepare to load the scenario
	------------------------------------------------------------------------*/
	if (rc) {
		Session.NumPlayers = skirmish ? 1 : 2;

		Scen.Scenario = Session.Options.ScenarioIndex;
		strcpy (Scen.ScenarioName, Session.Scenarios[Session.Options.ScenarioIndex]->Get_Filename());

		/*.....................................................................
		Add both players to the Players vector; the local system is always
		index 0.
		.....................................................................*/
		who = new NodeNameType;
		strcpy(who->Name, namebuf);
		who->Player.House = Session.House;
		who->Player.Color = Session.ColorIdx;
		who->Player.ProcessTime = -1;
		Session.Players.Add (who);

		/*
		**	Fetch the difficulty setting when in skirmish mode.
		*/
		if (skirmish) {
			int diff = difficulty.Get_Value() * (Rule.IsFineDifficulty ? 1 : 2);
			switch (diff) {
				case 0:
					Scen.CDifficulty = DIFF_HARD;
					Scen.Difficulty = DIFF_EASY;
					break;

				case 1:
					Scen.CDifficulty = DIFF_HARD;
					Scen.Difficulty = DIFF_NORMAL;
					break;

				case 2:
					Scen.CDifficulty = DIFF_NORMAL;
					Scen.Difficulty = DIFF_NORMAL;
					break;

				case 3:
					Scen.CDifficulty = DIFF_EASY;
					Scen.Difficulty = DIFF_NORMAL;
					break;

				case 4:
					Scen.CDifficulty = DIFF_EASY;
					Scen.Difficulty = DIFF_HARD;
					break;
			}
		} else {
			Scen.CDifficulty = DIFF_NORMAL;
			Scen.Difficulty = DIFF_NORMAL;
		}
	}

	/*------------------------------------------------------------------------
	Clear all lists
	------------------------------------------------------------------------*/
	while (scenariolist.Count()) {
		scenariolist.Remove_Item(scenariolist.Get_Item(0));
	}

	/*------------------------------------------------------------------------
	Clean up the list boxes
	------------------------------------------------------------------------*/
	while (playerlist.Count()>0) {
		item = (char *)playerlist.Get_Item(0);
		delete [] item;
		playerlist.Remove_Item(item);
	}

	/*------------------------------------------------------------------------
	Remove the chat edit box
	------------------------------------------------------------------------*/
	Session.Messages.Remove_Edit();

	/*------------------------------------------------------------------------
	Restore screen
	------------------------------------------------------------------------*/
	Hide_Mouse();
	Load_Title_Page(true);
	Show_Mouse();

	/*------------------------------------------------------------------------
	Save any changes made to our options
	------------------------------------------------------------------------*/
	if (changed) {
		Session.Write_MultiPlayer_Settings();
	}

	if (load_game && !skirmish) {
		if (!Load_Game (-1)) {
			WWMessageBox().Process (TXT_ERROR_LOADING_GAME);
			rc = false;
		}
		Frame++;
	}

	return(rc);
}


#ifdef FIXIT_CSII	//	checked - ajw 9/28/98
extern bool Is_Mission_Aftermath (char *file_name);
#endif

/***********************************************************************************************
 * Find_Local_Scenario -- finds the file name of the scenario with matching attributes         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to Scenario description                                                       *
 *           ptr to Scenario filename to fix up                                                *
 *           length of file for trivial rejection of scenario files                            *
 *           ptr to digest. Digests must match.                                                *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   true if scenario is available locally                                             *
 *                                                                                             *
 * WARNINGS: We need to reject files that don't match exactly because scenarios with the same  *
 *           description can exist on both machines but have different contents. For example   *
 *           there will be lots of scenarios called 'my map' and 'crap' and 'aaaaaa'.          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/23/96 12:36PM ST : Created                                                             *
 *=============================================================================================*/
bool Find_Local_Scenario (char *description, char *filename, unsigned int length, char *digest, bool official)
{
//FILE *fp;
//fp = fopen("findscen.txt","wt");
//debugprint("looking for local scenario: description = %s, name=%s, length=%d, digest=%s, official=%d\n", description, filename, length, digest, official);

	char digest_buffer[32];
	/*
	** Scan through the scenario list looking for scenarios with matching descriptions.
	*/
	for (int index = 0; index < Session.Scenarios.Count(); index++) {

//debugprint( "Checking against scenario: %s\n", Session.Scenarios[index]->Description());
		if (!strcmp (Session.Scenarios[index]->Description(), description)) {
//debugprint("found matching description.\n");
			CCFileClass file (Session.Scenarios[index]->Get_Filename());

			/*
			** Possible rejection on the basis of availability.
			*/
			if (file.Is_Available()) {
//debugprint("file is available.\n");
				/*
				** Possible rejection on the basis of size.
				*/
				if (file.Size() == length) {
//debugprint("length matches.\n");
					/*
					** We don't know the digest for 'official' scenarios so assume its correct
					*/
					if (!official) {
//debugprint("!official.\n");
						/*
						** Possible rejection on the basis of digest
						*/
						INIClass ini;
						ini.Load(file);
						ini.Get_String ("Digest", "1", "No digest here mate. Nope.", digest_buffer, sizeof (digest_buffer) );
					}
//debugprint("digest = %s, digest_buffer = %s.\n", digest, digest_buffer);
#ifdef FIXIT_CSII	//	checked - ajw 9/28/98. But don't know why this happens. Because of autodownload?
					/*
					** If this is an aftermath scenario then ignore the digest and return success.
					*/
					if ( Is_Mission_Aftermath ((char*)Session.Scenarios[index]->Get_Filename()) ) {
//debugprint("a 1match!\n");
						strcpy (filename, Session.Scenarios[index]->Get_Filename());
						return (true);
					}
#endif

					/*
					** This must be the same scenario. Copy the name and return true.
					*/
					if (official || !strcmp (digest, digest_buffer)) {
//debugprint("a match!\n");
						strcpy (filename, Session.Scenarios[index]->Get_Filename());
						return (true);
					}
				}
			}
//			else
//				debugprint("file not available '%s'.\n", Session.Scenarios[index]->Get_Filename());

		}
	}
//debugprint("failed match.\n");
	/*
	** Couldnt find the scenario locally. Return failure.
	*/
	return (false);
}
