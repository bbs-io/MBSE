/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Limits.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_limits.h"



int	LimUpdated = 0;


/*
 * Count nr of LIMIT records in the database.
 * Creates the database if it doesn't exist.
 */
int CountLimits(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			LIMIThdr.hdrsize = sizeof(LIMIThdr);
			LIMIThdr.recsize = sizeof(LIMIT);
			fwrite(&LIMIThdr, sizeof(LIMIThdr), 1, fil);

			/*
			 *  Create default limits
			 */
			memset(&LIMIT, 0, sizeof(LIMIT));
			LIMIT.Security = 0;
			LIMIT.Time = 5;
			LIMIT.DownK = 1;
			LIMIT.DownF = 1;
			sprintf(LIMIT.Description, "Twit");
			LIMIT.Available = TRUE;
			fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 5;
                        LIMIT.Time = 15;
                        LIMIT.DownK = 100;
                        LIMIT.DownF = 2;
                        sprintf(LIMIT.Description, "New User");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 20;
                        LIMIT.Time = 60;
                        LIMIT.DownK = 10240;
                        LIMIT.DownF = 25;
                        sprintf(LIMIT.Description, "Normal User");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 50;
                        LIMIT.Time = 90;
                        LIMIT.DownK = 20480;
                        LIMIT.DownF = 100;
                        sprintf(LIMIT.Description, "V.I.P. User");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 80;
                        LIMIT.Time = 120;
                        LIMIT.DownK = 40960;
                        sprintf(LIMIT.Description, "Fellow Sysop or Point");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 100;
                        LIMIT.Time = 180;
                        LIMIT.DownK = 40960;
                        sprintf(LIMIT.Description, "Co-Sysop");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

                        memset(&LIMIT, 0, sizeof(LIMIT));
                        LIMIT.Security = 32000;
                        LIMIT.Time = 240;
                        LIMIT.DownK = 40960;
                        sprintf(LIMIT.Description, "Sysop");
                        LIMIT.Available = TRUE;
                        fwrite(&LIMIT, sizeof(LIMIT), 1, fil);

			fclose(fil);
			chmod(ffile, 0640);
			return 7;
		} else
			return -1;
	}

	fread(&LIMIThdr, sizeof(LIMIThdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - LIMIThdr.hdrsize) / LIMIThdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenLimits(void);
int OpenLimits(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/limits.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&LIMIThdr, sizeof(LIMIThdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = LIMIThdr.recsize;
			if (oldsize != sizeof(LIMIT)) {
				LimUpdated = 1;
				Syslog('+', "Updated %s, format changed");
			} else
				LimUpdated = 0;
			LIMIThdr.hdrsize = sizeof(LIMIThdr);
			LIMIThdr.recsize = sizeof(LIMIT);
			fwrite(&LIMIThdr, sizeof(LIMIThdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&LIMIT, 0, sizeof(LIMIT));
			while (fread(&LIMIT, oldsize, 1, fin) == 1) {
				fwrite(&LIMIT, sizeof(LIMIT), 1, fout);
				memset(&LIMIT, 0, sizeof(LIMIT));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseLimits(int);
void CloseLimits(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX], temp[20];
	FILE	*fi, *fo;
	st_list	*lim = NULL, *tmp;

	sprintf(fin, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/limits.temp", getenv("MBSE_ROOT"));

	if (LimUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&LIMIThdr, LIMIThdr.hdrsize, 1, fi);
			fwrite(&LIMIThdr, LIMIThdr.hdrsize, 1, fo);

			while (fread(&LIMIT, LIMIThdr.recsize, 1, fi) == 1)
				if (!LIMIT.Deleted) {
					sprintf(temp, "%014ld", LIMIT.Security);
					fill_stlist(&lim, temp, ftell(fi) - LIMIThdr.recsize);
				}
			sort_stlist(&lim);

			for (tmp = lim; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&LIMIT, LIMIThdr.recsize, 1, fi);
				fwrite(&LIMIT, LIMIThdr.recsize, 1, fo);
			}

			tidy_stlist(&lim);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"limits.data\"");
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendLimits(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&LIMIT, 0, sizeof(LIMIT));
		fwrite(&LIMIT, sizeof(LIMIT), 1, fil);
		fclose(fil);
		LimUpdated = 1;
		return 0;
	} else
		return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditLimRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Limits");

	sprintf(mfile, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(LIMIThdr) + ((Area -1) * sizeof(LIMIT));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&LIMIT, sizeof(LIMIT), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&LIMIT, crc, sizeof(LIMIT));

	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "8.1 EDIT SECURITY LIMIT");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.  Access Level");
	mvprintw( 8, 6, "2.  Maximum Time");
	mvprintw( 9, 6, "3.  Download Kb.");
	mvprintw(10, 6, "4.  Download Files");
	mvprintw(11, 6, "5.  Description");
	mvprintw(12, 6, "6.  Available");
	mvprintw(13, 6, "7.  Deleted");

	for (;;) {
		set_color(WHITE, BLACK);
		show_int( 7,25,    LIMIT.Security);
		show_int( 8,25,    LIMIT.Time);
		show_int( 9,25,    LIMIT.DownK);
		show_int(10,25,    LIMIT.DownF);
		show_str(11,25,40, LIMIT.Description);
		show_bool(12,25,   LIMIT.Available);
		show_bool(13,25,   LIMIT.Deleted);

		j = select_menu(7);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&LIMIT, crc1, sizeof(LIMIT));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&LIMIT, sizeof(LIMIT), 1, fil);
					fclose(fil);
					LimUpdated = 1;
					working(1, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_INT(  7,25,   LIMIT.Security,   "The ^Security^ level for this limit")
		case 2:	E_INT(  8,25,   LIMIT.Time,       "The maxmimum ^Time online^ per day for this limit, zero to disable")
		case 3:	E_INT(  9,25,   LIMIT.DownK,      "The ^Kilobytes^ download limit per day, 0 = don't care")
		case 4:	E_INT( 10,25,   LIMIT.DownF,      "The ^nr of files^ to download per day, 0 = don't care")
		case 5:	E_STR( 11,25,40,LIMIT.Description,"A short ^description^ for this limit")
		case 6:	E_BOOL(12,25,   LIMIT.Available,  "Is this record ^avaiable^")
		case 7: E_BOOL(13,25,   LIMIT.Deleted,    "Is this level ^Deleted^")
		}
	}

	return 0;
}



void EditLimits(void)
{
	int	records, i, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountLimits();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenLimits() == -1) {
		working(2, 0, 0);
		return;
	}

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 7, "8.1 LIMITS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/limits.temp", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&LIMIThdr, sizeof(LIMIThdr), 1, fil);
				x = 5;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= records; i++) {
					offset = sizeof(LIMIThdr) + ((i - 1) * LIMIThdr.recsize);
					fseek(fil, offset, 0);
					fread(&LIMIT, LIMIThdr.recsize, 1, fil);
					if (i == 11) {
						x = 45;
						y = 7;
					}
					if (LIMIT.Available)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTBLUE, BLACK);
					sprintf(temp, "%3d.  %-6ld %-40s", i, LIMIT.Security, LIMIT.Description);
					temp[37] = '\0';
					mvprintw(y, x, temp);
					y++;
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseLimits(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendLimits() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditLimRec(atoi(pick));
	}
}



void InitLimits(void)
{
    CountLimits();
    OpenLimits();
    CloseLimits(TRUE);
}



char *PickLimits(int nr)
{
	static	char Lim[21] = "";
	int	records, i, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return Lim;
	}

	records = CountLimits();
	if (records == -1) {
		working(2, 0, 0);
		return Lim;
	}


	clr_index();
	set_color(WHITE, BLACK);
	sprintf(temp, "%d.  LIMITS SELECT", nr);
	mvprintw( 5, 4, temp);
	set_color(CYAN, BLACK);
	if (records != 0) {
		sprintf(temp, "%s/etc/limits.data", getenv("MBSE_ROOT"));
		if ((fil = fopen(temp, "r")) != NULL) {
			fread(&LIMIThdr, sizeof(LIMIThdr), 1, fil);
			x = 2;
			y = 7;
			set_color(CYAN, BLACK);
			for (i = 1; i <= records; i++) {
				offset = sizeof(LIMIThdr) + ((i - 1) * LIMIThdr.recsize);
				fseek(fil, offset, 0);
				fread(&LIMIT, LIMIThdr.recsize, 1, fil);
				if (i == 11) {
					x = 42;
					y = 7;
				}
				if (LIMIT.Available)
					set_color(CYAN, BLACK);
				else
					set_color(LIGHTBLUE, BLACK);
				sprintf(temp, "%3d.  %-6ld %-40s", i, LIMIT.Security, LIMIT.Description);
				temp[37] = '\0';
				mvprintw(y, x, temp);
				y++;
			}
			strcpy(pick, select_pick(records, 20));

			if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
				offset = sizeof(LIMIThdr) + ((atoi(pick) - 1) * LIMIThdr.recsize);
				fseek(fil, offset, 0);
				fread(&LIMIT, LIMIThdr.recsize, 1, fil);
				sprintf(Lim, "%ld", LIMIT.Security);
			}
			fclose(fil);
		}
	}
	return Lim;
}



int bbs_limits_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[PATH_MAX];
	FILE	*no;

	sprintf(temp, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	addtoc(fp, toc, 8, 1, page, (char *)"BBS user limits");

	fread(&LIMIThdr, sizeof(LIMIThdr), 1, no);

	fprintf(fp, "\n");
	fprintf(fp, "     Access   Max.   Down   Down\n");
	fprintf(fp, "      Level   time    Kb.  files Active Description\n");
	fprintf(fp, "     ------ ------ ------ ------ ------ ------------------------------\n");

	while ((fread(&LIMIT, LIMIThdr.recsize, 1, no)) == 1) {
		fprintf(fp, "     %6ld %6ld %6ld %6d %s    %s\n", LIMIT.Security, LIMIT.Time, LIMIT.DownK, 
			LIMIT.DownF, getboolean(LIMIT.Available), LIMIT.Description);
	}	

	fclose(no);
	return page;
}



int limit_users_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[PATH_MAX];
	FILE	*no, *us;
	int	line = 0, j;

	sprintf(temp, "%s/etc/limits.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((us = fopen(temp, "r")) == NULL) {
		fclose(no);
		return page;
	}
	
	page = newpage(fp, page);
	addtoc(fp, toc, 1, 0, page, (char *)"Access limits and users");

	fread(&LIMIThdr, sizeof(LIMIThdr), 1, no);
	fread(&usrconfighdr, sizeof(usrconfighdr), 1, us);

	while (fread(&LIMIT, LIMIThdr.recsize, 1, no) == 1) {
		if (LIMIT.Available) {
			if (line > 52) {
				page = newpage(fp, page);
				line = 0;
			}
			fprintf(fp, "\n\n");
			fprintf(fp, "    Level %ld - %s\n\n", LIMIT.Security, LIMIT.Description);
			line += 4;
			j = 2;
			fseek(us, usrconfighdr.hdrsize, SEEK_SET);

			while (fread(&usrconfig, usrconfighdr.recsize, 1, us) == 1) {
				if ((!usrconfig.Deleted) && strlen(usrconfig.sUserName) &&
				    (usrconfig.Security.level == LIMIT.Security)) {
					if (j == 2) {
						j = 0;
						fprintf(fp, "     %-35s", usrconfig.sUserName);
					 } else {
						fprintf(fp, "     %s\n", usrconfig.sUserName);
						line++;
						if (line > 56) {
							page = newpage(fp, page);
							line = 0;
						}
					}
					j++;
				}
			}
			if (j != 2)
				fprintf(fp, "\n");
		}
	}

	fclose(us);
	fclose(no);
	return page;
}


