/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Change user settings
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "change.h"
#include "dispfile.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "exitinfo.h"
#include "bye.h"
#include "term.h"
#include "ttyio.h"

int Chg_Language(int NewMode)
{
    FILE    *pLang;
    int	    iLang, iFoundLang = FALSE;
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));

    if (!NewMode)
	ReadExitinfo();

    while(TRUE) {
	sprintf(temp, "%s/etc/language.data", getenv("MBSE_ROOT"));
	if(( pLang = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    sprintf(temp, "\nFATAL: Can't open language file\n\n");
	    PUTSTR(temp);
	    Pause();
	    free(temp);
	    return 0;
	}
	fread(&langhdr, sizeof(langhdr), 1, pLang);

	colour(CFG.HiliteF, CFG.HiliteB);
	/* Select your preferred language */
	sprintf(temp, "\r\n%s\r\n\r\n", (char *) Language(378));
	PUTSTR(temp);

	iLang = 6;
	colour(9,0);
	while (fread(&lang, langhdr.recsize, 1, pLang) == 1)
	    if (lang.Available) {
		colour(13, 0);
		sprintf(temp, "(%s)", lang.LangKey);
		PUTSTR(temp);
   		colour(8,0);
   		sprintf(temp, " %c ", 46);
		PUTSTR(temp);
   		colour(3,0);
		sprintf(temp, "%-29s    ", lang.Name);
		PUTSTR(temp);

		iLang++;
		if ((iLang % 2) == 0) {
		    PUTCHAR('\r');
		    PUTCHAR('\n');
		}
	    }
	Enter(1);

	colour(CFG.HiliteF, CFG.HiliteB);
	/* Select language: */
        sprintf(temp, "\n%s", (char *) Language(379));
	PUTSTR(temp);

	alarm_on();
	iLang = toupper(Readkey());

	PUTCHAR(iLang);

	fseek(pLang, langhdr.hdrsize, 0);

	while (fread(&lang, langhdr.recsize, 1, pLang) == 1) {
	    strcpy(lang.LangKey,tu(lang.LangKey));
	    if ((lang.LangKey[0] == iLang) && (lang.Available)) {
		strcpy(CFG.current_language, lang.Filename);
		iFoundLang = TRUE;
		break;
	    }
	}
	
	fclose(pLang);

	if(!iFoundLang) {
	    Enter(2);
	    /* Invalid selection, please try again! */
	    pout(10, 0, (char *) Language(265));
	    Enter(2);
	} else {
	    exitinfo.iLanguage = iLang;
	    strcpy(CFG.current_language, lang.Filename);
	    Free_Language();
	    InitLanguage();

	    colour(10, 0);
	    /* Language now set to" */
	    sprintf(temp, "\r\n\r\n%s%s\r\n\r\n", (char *) Language(380), lang.Name);
	    PUTSTR(temp);

	    if (!NewMode) {
		Syslog('+', "Changed language to %s", lang.Name);
		WriteExitinfo();
		Pause();
	    }
	    break;
	}
    }

    free(temp);
    Enter(1);
    return iLang;
}



void Chg_Password()
{
    char    *temp1, *temp2, *args[16];

    temp1 = calloc(PATH_MAX, sizeof(char));
    temp2 = calloc(PATH_MAX, sizeof(char));

    ReadExitinfo();
    DisplayFile((char *)"password");

    Enter(1);
    /* Old password: */
    language(15, 0, 120);
    colour(CFG.InputColourF, CFG.InputColourB);
    Getpass(temp1);

    if (!strcmp(exitinfo.Password, temp1)) {
	while (TRUE) {
	    Enter(1);
	    /* New password: */
	    language(9, 0, 121);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    Getpass(temp1);
	    if((strlen(temp1)) >= CFG.password_length) {
		Enter(1);
		/* Confirm new password: */
		language(9, 0, 122);
		colour(CFG.InputColourF, CFG.InputColourB);
		Getpass(temp2);
		if(( strcmp(temp1,temp2)) != 0) {
		    /* Passwords do not match! */
		    Enter(2);
		    language(12, 0, 123);
		    Enter(1);
		} else {
		    break;
		}
	    } else {
		colour(12, 0);
		/* Your password must contain at least %d characters! Try again.*/
		sprintf(temp2, "\r\n%s%d %s\r\n\r\n", (char *) Language(42), CFG.password_length, (char *) Language(43));
		PUTSTR(temp2);
	    }
	}

	Syslog('+', "%s/bin/mbpasswd -n %s ******", getenv("MBSE_ROOT"), exitinfo.Name);
	sprintf(temp1, "%s/bin/mbpasswd", getenv("MBSE_ROOT"));
	memset(args, 0, sizeof(args));
	args[0] = temp1;
	args[1] = (char *)"-n";
	args[2] = exitinfo.Name;
	args[3] = temp2;
	args[4] = NULL;

	if (execute(args, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") != 0) {
	    WriteError("Failed to set new Unix password");
	} else {
	    memset(&exitinfo.Password, 0, sizeof(exitinfo.Password));
	    strcpy(exitinfo.Password, temp2);
	    exitinfo.tLastPwdChange = time(NULL);
	    Enter(1);
	    /* Password Change Successful */
	    language(10, 0, 124);
	    Syslog('+', "User changed his password");
	    WriteExitinfo();
	}
    } else {
	Enter(1);
	/* Old password incorrect! */
	language(12, 0, 125);
    }

    free(temp1);
    free(temp2);
    Enter(2);
    Pause();
}



/*
 * Function to check if User Handle exists and returns a 0 or 1
 */
int CheckHandle(char *);
int CheckHandle(char *Name)
{
    FILE    *fp;
    int     Status = FALSE;
    struct  userhdr uhdr;
    struct  userrec u;
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp,"rb")) != NULL) {
        fread(&uhdr, sizeof(uhdr), 1, fp);

        while (fread(&u, uhdr.recsize, 1, fp) == 1) {
            if ((strcasecmp(u.sHandle, Name)) == 0) {
                Status = TRUE;
                break;
            }
        }
        fclose(fp);
    }

    free(temp);
    return Status;
}



/*
 * Function will allow a user to change his handle
 */
void Chg_Handle()
{
    char    *Handle, *temp;

    Handle = calloc(81, sizeof(char));
    temp   = calloc(81, sizeof(char));

    ReadExitinfo();
    Syslog('+', "Old handle \"%s\"", exitinfo.sHandle);

    while (TRUE) {
	Enter(1);
	/* Enter a handle (Enter to Quit): */
	pout(9, 0, (char *) Language(412));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 34);

	if ((strcmp(temp, "")) == 0) {
	    free(Handle);
	    free(temp);
	    return;
	}
	strcpy(Handle, tlcap(temp));

	if (CheckHandle(Handle) || CheckUnixNames(Handle)) {
	    pout(12, 0, (char *)"\r\nThat handle is already been used\r\n");
	} else if (CheckName(Handle)) {
	    pout(12, 0, (char *)"\r\nThat name is already been used\r\n");
	} else if((strcasecmp(Handle, "sysop")) == 0) {
	    pout(12, 0, (char *)"\r\nYou cannot use Sysop as a handle\r\n");
	} else if(strcmp(temp, "") != 0) {
	    Setup(exitinfo.sHandle, temp);
	    pout(10, 0, (char *)"\r\nHandle Changed!\r\n\r\n");
	    Syslog('+', "New handle \"%s\"", exitinfo.sHandle);
	    break;
	}
    }

    WriteExitinfo();
    free(temp);
    free(Handle);
}



/*
 * Toggle hotkeys
 */
void Chg_Hotkeys()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.HotKeys) {
	exitinfo.HotKeys = FALSE;
	/* Hotkeys are now OFF */
	pout(10, 0, (char *) Language(146));
    } else {
	exitinfo.HotKeys = TRUE;
	/* Hotkeys are now ON */
	pout(10, 0, (char *) Language(145));
    }

    Enter(2);
    sleep(2);
    Syslog('+', "Hotkeys changed to %s", exitinfo.HotKeys?"True":"False");
    WriteExitinfo();
}



/*
 * Toggle Mail Check
 */
void Chg_MailCheck()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.MailScan) {
	exitinfo.MailScan = FALSE;
	/* New Mail check is now OFF */
	pout(10, 0, (char *) Language(367));
    } else {
	exitinfo.MailScan = TRUE;
	/* New Mail check is now ON */
	pout(10, 0, (char *) Language(366));
    }

    Enter(2);
    sleep(2);
    Syslog('+', "New Mail Check changed to %s", exitinfo.MailScan ?"True":"False");
    WriteExitinfo();
}



/*
 * Toggle New Files Check
 */
void Chg_FileCheck()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.ieFILE) {
	exitinfo.ieFILE = FALSE;
	/* New Files check is now OFF */
	pout(10, 0, (char *) Language(371));
    } else {
	exitinfo.ieFILE = TRUE;
	/* New Files check is now ON */
	pout(10, 0, (char *) Language(370));
    }

    Enter(2);
    sleep(2);
    Syslog('+', "Check New Files changed to %s", exitinfo.ieFILE ?"True":"False");
    WriteExitinfo();
}



/*
 * Choose Message Editor
 */
void Chg_FsMsged()
{
    int	    z;
    char    temp[81];

    ReadExitinfo();
    Enter(2);

    /*                               Now using the */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(372));
    /*                 Line/Fullscreen/External    */
    colour(LIGHTCYAN, BLACK);
    sprintf(temp, " %s ", Language(387 + (exitinfo.MsgEditor & 3)));
    PUTSTR(temp);
    /*                                      Editor */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(390));
    Enter(1);

    if (strlen(CFG.externaleditor))
	/* Select: 1) Line editor, 2) Fullscreen editor, 3) External editor */
	pout(WHITE, BLACK, (char *)Language(373));
    else
	/* Select: 1) Line editor, 2) Fullscreen editor */
	pout(WHITE, BLACK, (char *)Language(438));
    alarm_on();
    z = toupper(Readkey());

    if (z == Keystroke(373, 0)) {
	exitinfo.MsgEditor = LINEEDIT;
	Syslog('+', "User selected line editor");
    } else if (z == Keystroke(373, 1)) {
	exitinfo.MsgEditor = FSEDIT;
	Syslog('+', "User selected fullscreen editor");
    } else if ((z == Keystroke(373, 2) && strlen(CFG.externaleditor))) {
	exitinfo.MsgEditor = EXTEDIT;
	Syslog('+', "User selected external editor");
    }

    Enter(2);

    /*                               Now using the */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(372));
    /*                 Line/Fullscreen/External    */
    colour(LIGHTCYAN, BLACK);
    sprintf(temp, " %s ", Language(387 + (exitinfo.MsgEditor & 3)));
    PUTSTR(temp);
    /*                                      Editor */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(390));

    Enter(2);
    sleep(2);
    WriteExitinfo();
}



/*
 * Toggle Fullscreen Editor Shotcut keys
 */
void Chg_FsMsgedKeys()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.FSemacs) {
	exitinfo.FSemacs = FALSE;
	/* Fullscreen Editor shortcut keys set to Wordstar */
	pout(10, 0, (char *) Language(473));
    } else {
	exitinfo.FSemacs = TRUE;
	/* Fullscreen Editor shortcut keys set to Emacs */
	pout(10, 0, (char *) Language(472));
    }
    Enter(2);
    sleep(2);
    Syslog('+', "FS editor shortcut keys changed to %s", exitinfo.FSemacs?"Emacs":"Wordstar");
    WriteExitinfo();
}



/*
 * Function to toggle DoNotDisturb Flag
 */
void Chg_Disturb()
{
    ReadExitinfo();
    Enter(2);

    if(exitinfo.DoNotDisturb) {
	exitinfo.DoNotDisturb = FALSE;
	/* Do not disturb turned OFF */
	pout(10, 0, (char *) Language(416));
    } else {
	exitinfo.DoNotDisturb = TRUE;
	/* Do not disturb turned ON */
	pout(10, 0, (char *) Language(417));
    }

    Enter(2);
    Syslog('+', "Do not disturb now %s", exitinfo.DoNotDisturb?"True":"False");
    UserSilent(exitinfo.DoNotDisturb);
    sleep(2);
    WriteExitinfo();
}



void Chg_Location()
{
    char    temp[81];

    ReadExitinfo();
    Syslog('+', "Old location \"%s\"", exitinfo.sLocation);

    while (TRUE) {
	/* Old Location: */
	Enter(1);
	/* Old location: */
	pout(15, 0, (char *) Language(73));
	pout(9, 0, exitinfo.sLocation);
	Enter(2);
	/* Please enter your location: */
	pout(14, 0, (char *) Language(49));

	colour(CFG.InputColourF, CFG.InputColourB);
	if (CFG.iCapLocation) {
	    GetnameNE(temp, 24);
	} else {
	    GetstrC(temp, 80);
	}

	if((strcmp(temp, "")) == 0)
	    break;

	if(( strlen(temp)) < CFG.CityLen) {
	    Enter(1);
	    /* Please enter a longer location (min */
	    colour(12, 0);
	    sprintf(temp, "%s%d)", (char *) Language(74), CFG.CityLen);
	    PUTSTR(temp);
	    Enter(1);
	} else {
	    Setup(exitinfo.sLocation,temp);
	    break;
	}
    }

    Syslog('+', "New location \"%s\"", exitinfo.sLocation);
    WriteExitinfo();
}



void Chg_Address()
{
    int	    i;
    char    temp[41];
    
    ReadExitinfo();
    Syslog('+', "Old address \"%s\"", exitinfo.address[0]);
    Syslog('+', "            \"%s\"", exitinfo.address[1]);
    Syslog('+', "            \"%s\"", exitinfo.address[2]);

    while (TRUE) {
	Enter(1);
	/* Old address: */
	pout(WHITE, BLACK, (char *) Language(476));
	Enter(1);
	colour(LIGHTBLUE, BLACK);
	PUTSTR(exitinfo.address[0]);
	Enter(1);
	PUTSTR(exitinfo.address[1]);
	Enter(1);
	PUTSTR(exitinfo.address[2]);
	Enter(2);
	/* Your address, maximum 3 lines (only visible for the sysop): */
	pout(YELLOW, BLACK, (char *) Language(474));
	Enter(1);

	for (i = 0; i < 3; i++ ) {
	    colour(YELLOW, BLACK);
	    printf("%d: ", i+1);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    alarm_on();
	    GetstrC(temp, 40);
	    if (strcmp(temp, ""))
		Setup(exitinfo.address[i], temp);
	}

	if (strlen(exitinfo.address[0]) || strlen(exitinfo.address[1]) || strlen(exitinfo.address[2]))
	    break;

	Enter(1);
	/* You need to enter your address here */
	pout(LIGHTRED, BLACK, (char *)Language(475));
	Enter(1);
    }

    Syslog('+', "New address \"%s\"", exitinfo.address[0]);
    Syslog('+', "            \"%s\"", exitinfo.address[1]);
    Syslog('+', "            \"%s\"", exitinfo.address[2]);
    WriteExitinfo();
}



/*
 * Toggle Graphics
 */
void Chg_Graphics()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.GraphMode) {
	exitinfo.GraphMode = FALSE;
	/* Ansi Mode turned OFF */
	pout(15, 0, (char *) Language(76));
    } else {
	exitinfo.GraphMode = TRUE;
	/* Ansi Mode turned ON */
	pout(15, 0, (char *) Language(75));
    }

    Syslog('+', "Graphics mode now %s", exitinfo.GraphMode?"On":"Off");
    Enter(2);
    TermInit(exitinfo.GraphMode, 80, exitinfo.iScreenLen);
    WriteExitinfo();
    sleep(2);
}



void Chg_VoicePhone()
{
    char	temp[81];

    ReadExitinfo();
    Syslog('+', "Old voice phone \"%s\"", exitinfo.sVoicePhone);

    while (TRUE) {
	Enter(1);
	/* Please enter you Voice Number */
	pout(10, 0, (char *) Language(45));
	Enter(1);
	pout(10, 0, (char *)": ");
	colour(CFG.InputColourF, CFG.InputColourB);
	GetPhone(temp, 16);

	if (strlen(temp) < 6) {
	    Enter(1);
	    /* Please enter a proper phone number */
	    pout(12, 0, (char *) Language(47));
	    Enter(1);
	} else {
	    strcpy(exitinfo.sVoicePhone, temp);
	    break;
	}
    }

    Syslog('+', "New voice phone \"%s\"", exitinfo.sVoicePhone);
    WriteExitinfo();
}



void Chg_DataPhone()
{
    char	temp[81];

    ReadExitinfo();
    Syslog('+', "Old data phone \"%s\"", exitinfo.sDataPhone);

    while (1) {
	Enter(1);
	/* Please enter you Data Number */
	pout(10, 0, (char *) Language(48));
	Enter(1);
	pout(10, 0, (char *)": ");
	colour(CFG.InputColourF, CFG.InputColourB);
	GetPhone(temp, 16);

	if( strlen(temp) < 6) {
	    Enter(1);
	    /* Please enter a proper phone number */
	    pout(12, 0, (char *) Language(47));
	    Enter(1);
	} else {
	    strcpy(exitinfo.sDataPhone, temp);
	    break;
	}
    }

    Syslog('+', "New data phone \"%s\"", exitinfo.sDataPhone);
    WriteExitinfo();
}



void Chg_News()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.ieNEWS) {
	exitinfo.ieNEWS = FALSE;
	/* News bulletins turned OFF */
	pout(10, 0, (char *) Language(79));
    } else {
	exitinfo.ieNEWS = TRUE;
	/* News bulletins turned ON */
	pout(10, 0, (char *) Language(78));
    }

    Enter(2);
    Syslog('+', "News bullentins now %s", exitinfo.ieNEWS?"True":"False");
    sleep(2);
    WriteExitinfo();
}



void Chg_ScreenLen()
{
    char	*temp;

    ReadExitinfo();
    temp = calloc(81, sizeof(char));
    Syslog('+', "Old screenlen %d", exitinfo.iScreenLen);

    Enter(1);
    /* Please enter your Screen Length? [24]: */
    pout(13, 0, (char *) Language(64));
    colour(CFG.InputColourF, CFG.InputColourB);
    Getnum(temp, 2);

    if((strcmp(temp, "")) == 0) {
	exitinfo.iScreenLen = 24;
	sprintf(temp, "\r\n%s\r\n\r\n", (char *) Language(80));
    } else {
	exitinfo.iScreenLen = atoi(temp);
	sprintf(temp, "\r\n%s%d\r\n\r\n", (char *) Language(81), exitinfo.iScreenLen);
    }
    PUTSTR(temp);

    TermInit(exitinfo.GraphMode, 80, exitinfo.iScreenLen);
    Syslog('+', "New screenlen %d", exitinfo.iScreenLen);
    WriteExitinfo();
    Pause();
    free(temp);
}



/*
 * Check users Date of Birth, if it is ok, we calculate his age.
 */
int Test_DOB(char *DOB)
{
    int	    tyear, year, month, day;
    char    temp[40], temp1[40];

    /*
     * If Ask Date of Birth is off, assume users age is
     * zero, and this check is ok.
     */
    if (!CFG.iDOB) {
        UserAge = 0;
        return TRUE;
    }

    /*
     *  First check length of string 
     */
    if (strlen(DOB) != 10) {
	Syslog('!', "Date format length %d characters", strlen(DOB));
	/* Please enter the correct date format */
	language(14, 0, 83);
	return FALSE;
    }
	
    /*
     * Split the date into pieces
     */
    strcpy(temp1, DOB);
    strcpy(temp, strtok(temp1, "-"));
    day = atoi(temp);
    strcpy(temp, strtok(NULL, "-"));
    month = atoi(temp);
    strcpy(temp, strtok(NULL, ""));
    year = atoi(temp);
    tyear = l_date->tm_year + 1900;

    if (((tyear - year) < 10) || ((tyear - year) > 95)) {
	Syslog('!', "DOB: Year error: %d", tyear - year);
	return FALSE;
    }
    if ((month < 1) || (month > 12)) {
	Syslog('!', "DOB: Month error: %d", month);
	return FALSE;
    }
    if ((day < 1) || (day > 31)) {
	Syslog('!', "DOB: Day error: %d", day);
	return FALSE;
    }

    UserAge = tyear - year;
    if ((l_date->tm_mon + 1) < month)
	UserAge--;
    if (((l_date->tm_mon + 1) == month) && (l_date->tm_mday < day))
	UserAge--; 
    Syslog('B', "DOB: Users age %d year", UserAge);
    return TRUE;
}



void Chg_DOB()
{
    char	*temp;

    if (!CFG.iDOB)
	return;

    temp  = calloc(81, sizeof(char));
    ReadExitinfo();
    Syslog('+', "Old DOB %s", exitinfo.sDateOfBirth);

    while (TRUE) {
	Enter(1);
	/* Please enter your Date of Birth DD-MM-YYYY: */
	pout(3, 0, (char *) Language(56));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetDate(temp, 10);
	if (Test_DOB(temp)) {
	    Setup(exitinfo.sDateOfBirth, temp);
	    break;
	}
    }

    Syslog('+', "New DOB %s", exitinfo.sDateOfBirth);
    WriteExitinfo();
    free(temp);
}



/*
 * Change default protocol.
 */
void Chg_Protocol()
{
    FILE    *pProtConfig;
    int	    iProt, iFoundProt = FALSE, precno = 0;
    char    *temp, Prot[2];

    temp = calloc(PATH_MAX, sizeof(char));
    ReadExitinfo();
    Set_Protocol(exitinfo.sProtocol);
    Syslog('+', "Old protocol %s", sProtName);

    while(TRUE) {
	sprintf(temp, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	
	if ((pProtConfig = fopen(temp, "r")) == NULL) {
	    WriteError("$Can't open %s", temp);
	    /* Protocol: Can't open protocol file. */
	    Enter(1);
	    PUTSTR((char *) Language(262));
	    Enter(2);
	    Pause();
	    free(temp);
	    fclose(pProtConfig);
	    return;
	}
	fread(&PROThdr, sizeof(PROThdr), 1, pProtConfig);

	Enter(1);
	/* Select your preferred protocol */
	pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(263));
	Enter(2);

	while (fread(&PROT, PROThdr.recsize, 1, pProtConfig) == 1) {
	    if (PROT.Available && Access(exitinfo.Security, PROT.Level)) {
		colour(LIGHTBLUE, BLACK);
		PUTCHAR('(');
		colour(WHITE, BLACK);
		PUTSTR(PROT.ProtKey);
		colour(LIGHTBLUE, BLACK);
		sprintf(temp, ")  %-20s Efficiency %3d %%\r\n", PROT.ProtName, PROT.Efficiency);
		PUTSTR(temp);
	    }
	}
	
	Enter(1);
        pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(264));

        alarm_on();
        iProt = toupper(Readkey());

        PUTCHAR(iProt);
        sprintf(Prot, "%c", iProt);

        fseek(pProtConfig, PROThdr.hdrsize, 0);
        while (fread(&PROT, PROThdr.recsize, 1, pProtConfig) == 1) {
	    if ((strncmp(PROT.ProtKey, Prot, 1) == 0) && PROT.Available && Access(exitinfo.Security, PROT.Level)) {
	        strcpy(sProtName, PROT.ProtName);
	        strcpy(sProtUp, PROT.ProtUp);
	        strcpy(sProtDn, PROT.ProtDn);
	        strcpy(sProtAdvice, PROT.Advice);
	        uProtInternal = PROT.Internal;
	        iProtEfficiency = PROT.Efficiency;
	        iFoundProt = TRUE;
	    } else
	        precno++;
	}

        fclose(pProtConfig);

        if (iProt == 13) {
	    free(temp);
	    return;
	} else {
	    if (!iFoundProt) {
	        Enter(2);
	        pout(10, 0, (char *) Language(265));
	        Enter(2);
	        /* Loop for new attempt */
	    } else {
	        Setup(exitinfo.sProtocol, sProtName);
	        Enter(1);
	        /* Protocol now set to: */
	        pout(10, 0, (char *) Language(266));
		PUTSTR(sProtName);
	        Enter(2);
	        Pause();
	        break;
	    }
	}
    }

    Syslog('+', "New protocol %s", sProtName);
    WriteExitinfo();
    free(temp);
}



void Set_Protocol(char *Protocol)
{
    FILE    *pProtConfig;
    int	    precno = 0;
    char    *temp;

    memset(&sProtName, 0, sizeof(sProtName));
    temp = calloc(PATH_MAX, sizeof(char));

    sprintf(temp, "%s/etc/protocol.data", getenv("MBSE_ROOT"));

    if (( pProtConfig = fopen(temp, "rb")) == NULL) {
	WriteError("$Can't open %s", temp);
        Enter(1);
        /* Protocol: Can't open protocol file. */
        pout(LIGHTRED, BLACK, (char *) Language(262));
        Enter(2);
        Pause();
        free(temp);
        return;
    }

    fread(&PROThdr, sizeof(PROThdr), 1, pProtConfig);

    while (fread(&PROT, PROThdr.recsize, 1, pProtConfig) == 1) {
	if (((strcmp(PROT.ProtName, Protocol)) == 0) && PROT.Available) {
	    strcpy(sProtName, PROT.ProtName);
	    strcpy(sProtUp, PROT.ProtUp);
	    strcpy(sProtDn, PROT.ProtDn);
	    strcpy(sProtAdvice, PROT.Advice);
	    uProtInternal = PROT.Internal;
	    iProtEfficiency = PROT.Efficiency;
	} else
	    precno++;
    }

    free(temp);
    fclose(pProtConfig);
}



void Chg_OLR_ExtInfo()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.OL_ExtInfo) {
	exitinfo.OL_ExtInfo = FALSE;
	/* Offline Reader: Extended Info turned OFF */
	pout(GREEN, BLACK, (char *) Language(16));
    } else {
	exitinfo.OL_ExtInfo = TRUE;
	/* Offline Reader: Extended Info turned ON */
	pout(GREEN, BLACK, (char *) Language(15));
    }

    Enter(2);
    Syslog('+', "OLR Extended Info now %s", exitinfo.OL_ExtInfo?"True":"False");
    sleep(2);
    WriteExitinfo();
}



/*
 * Change character set.
 */
void Chg_Charset()
{
    int	    i;
    char    *temp;

    temp = calloc(81, sizeof(char));
    ReadExitinfo();
    Syslog('+', "Old character set %s", getftnchrs(exitinfo.Charset));

    while(TRUE) {
	Enter(1);
        /* Select your preferred character set */
        pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(23));
	Enter(2);

        colour(LIGHTBLUE, BLACK);
	for (i = (FTNC_NONE + 1); i <= FTNC_MAXCHARS; i++) {
	    colour(LIGHTBLUE, BLACK);
	    sprintf(temp, "%2d ", i);
	    PUTSTR(temp);
	    colour(LIGHTCYAN, BLACK);
	    sprintf(temp, "%-9s ", getftnchrs(i));
	    PUTSTR(temp);
	    colour(LIGHTMAGENTA, BLACK);
	    sprintf(temp, "%s\r\n", getchrsdesc(i));
	    PUTSTR(temp);
	}

	Enter(1);
	/* Select character set  (Enter to Quit): */
        pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(24));

	Getnum(temp, 2);

	if (((strcmp(temp, "")) == 0) && (exitinfo.Charset != FTNC_NONE)) {
	    free(temp);
	    return;
	}
	
	i = atoi(temp);

	if ((i > FTNC_NONE) && (i <= FTNC_MAXCHARS)) {
	    exitinfo.Charset = i;
	    Syslog('+', "New character set %s", getftnchrs(exitinfo.Charset));
	    WriteExitinfo();
	    setlocale(LC_CTYPE, getlocale(exitinfo.Charset));
	    Syslog('b', "setlocale(LC_CTYPE, NULL) returns \"%s\"", printable(setlocale(LC_CTYPE, NULL), 0));
	    Enter(2);
	    colour(LIGHTGREEN, BLACK);
	    /* Character set now set to: */
	    sprintf(temp, "%s%s", (char *) Language(25), getftnchrs(i));
	    PUTSTR(temp);
	    free(temp);
	    Enter(2);
	    Pause();
	    return;
	}

	Enter(2);
	/* Invalid selection, please try again! */
	pout(LIGHTRED, BLACK, (char *) Language(265));
	Enter(2);
    }
}

