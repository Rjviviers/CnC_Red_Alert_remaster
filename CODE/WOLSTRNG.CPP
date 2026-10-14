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
//	S05: Westwood Online was removed. Only the strings still used by the game are kept here
//	(draw proposals and the expansion mission menus); the rest are in the repository history.

#include "function.h"
#include "WolStrng.h"

//#undef ENGLISH
//#define GERMAN

#ifdef ENGLISH
#pragma message( "...Building English version..." )

const char TXT_WOL_PROPOSE_DRAW[]			= "Propose a Draw";
const char TXT_WOL_RETRACT_DRAW[]			= "Retract Draw Proposal";
const char TXT_WOL_ACCEPT_DRAW[]			= "Accept Proposed Draw";
const char TXT_WOL_PROPOSE_DRAW_CONFIRM[]	= "Are you sure you want to propose a draw?";
const char TXT_WOL_ACCEPT_DRAW_CONFIRM[]	= "Are you sure you want to accept a draw?";

const char TXT_WOL_DRAW_PROPOSED_LOCAL[]	= "You have proposed that the game be declared a draw.";

const char TXT_WOL_DRAW_PROPOSED_OTHER[]	= "%s has proposed that the game be declared a draw.";

const char TXT_WOL_DRAW_RETRACTED_LOCAL[]	= "You have retracted your offer of a draw.";

const char TXT_WOL_DRAW_RETRACTED_OTHER[]	= "%s has retracted the offer of a draw.";
const char TXT_WOL_DRAW[]					= "The Game is a Draw";

const char TXT_WOL_CS_MISSIONS[]				= "Counterstrike Missions";

const char TXT_WOL_AM_MISSIONS[]				= "Aftermath Missions";

#else

#ifdef GERMAN
#pragma message( "...Building German version..." )

const char TXT_WOL_PROPOSE_DRAW[]			= "Unentschieden vorschlagen";
const char TXT_WOL_RETRACT_DRAW[]			= "Unentschieden-Vorschlag zurÅckziehen";
const char TXT_WOL_ACCEPT_DRAW[]			= "Unentschieden-Vorschlag akzeptieren";
const char TXT_WOL_PROPOSE_DRAW_CONFIRM[]	= "Sind Sie sicher, da\251 Sie ein Unentschieden vorschlagen mîchten?";
const char TXT_WOL_ACCEPT_DRAW_CONFIRM[]	= "Sind Sie sicher, da\251 Sie ein Unentschieden akzeptieren mîchten?";
const char TXT_WOL_DRAW_PROPOSED_LOCAL[]	= "Sie haben vorgeschlagen, da\251 das Spiel fÅr unentschieden erklÑrt wird.";
const char TXT_WOL_DRAW_PROPOSED_OTHER[]	= "%s hat vorgeschlagen, da\251 das Spiel fÅr unentschieden erklÑrt wird.";
const char TXT_WOL_DRAW_RETRACTED_LOCAL[]	= "Sie haben Ihr Unentschieden-Angebot zurÅckgezogen.";
const char TXT_WOL_DRAW_RETRACTED_OTHER[]	= "%s hat das Unentschieden-Angebot zurÅckgezogen.";
const char TXT_WOL_DRAW[]					= "Das Spiel ist unentschieden";
const char TXT_WOL_CS_MISSIONS[]				= "Gegenangriff-Missionen";
const char TXT_WOL_AM_MISSIONS[]				= "Vergeltungsschlag-Missionen";

#else	//	FRENCH

#pragma message( "...Building French version..." )

const char TXT_WOL_PROPOSE_DRAW[]			= "Proposer une fin avec ÇgalitÇ";
const char TXT_WOL_RETRACT_DRAW[]			= "Annuler la proposition de fin avec ÇgalitÇ";
const char TXT_WOL_ACCEPT_DRAW[]			= "Accepter la proposition de fin avec ÇgalitÇ";
const char TXT_WOL_PROPOSE_DRAW_CONFIRM[]	= "Voulez-vous vraiment proposer une fin avec ÇgalitÇ ?";
const char TXT_WOL_ACCEPT_DRAW_CONFIRM[]	= "Voulez-vous vraiment accepter une fin avec ÇgalitÇ ?" ;
const char TXT_WOL_DRAW_PROPOSED_LOCAL[]	= "Vous proposez de terminer la partie sans vainqueur ni perdant.";
const char TXT_WOL_DRAW_PROPOSED_OTHER[]	= "%s a proposÇ de terminer la partie sans vainqueur ni perdant.";
const char TXT_WOL_DRAW_RETRACTED_LOCAL[]	= "Vous avez annulÇ votre proposition de terminer la partie sans vainqueur ni perdant.";
const char TXT_WOL_DRAW_RETRACTED_OTHER[]	= "%s a annulÇ sa proposition de terminer la partie sans vainqueur ni perdant.";
const char TXT_WOL_DRAW[]					= "Match nul";
const char TXT_WOL_CS_MISSIONS[]				= "Missions extraites de Missions Taãga";
const char TXT_WOL_AM_MISSIONS[]				= "Missions extraites de Missions M.A.D.";

#endif

#endif
