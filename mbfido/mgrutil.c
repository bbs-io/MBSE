/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: AreaMgr and FileMgr utilities.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "sendmail.h"
#include "rollover.h"
#include "addpkt.h"
#include "mgrutil.h"




/*
 * Write Echomail groups list to tempfile
 */
void WriteMailGroups(FILE *fp, faddr *f)
{
	int	Count = 0, First = TRUE;
	char	*Group, *temp;
	FILE	*gp;
	faddr	*g;

	temp = calloc(128, sizeof(char));
	fprintf(fp, "Dear %s\r\r", nodes.Sysop);
	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	fprintf(fp, "The following is a list of mail groups at %s\r\r", ascfnode(f, 0x1f));

	if ((gp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		return;
	}
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);


//		     123456789012 1234567890123456789012345678901234567890123456789012345
	fprintf(fp, "Group        Description\r");
	fprintf(fp, "--------------------------------------------------------------------\r");

	while (TRUE) {
		Group = GetNodeMailGrp(First);
		if (Group == NULL)
			break;
		First = FALSE;

		fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
		while (fread(&mgroup, mgrouphdr.recsize, 1, gp) == 1) {
			g = bestaka_s(fido2faddr(mgroup.UseAka));
			if ((!strcmp(mgroup.Name, Group)) && 
			    (g->zone  == f->zone) && (g->net   == f->net) &&
			    (g->node  == f->node) && (g->point == f->point)) {
				fprintf(fp, "%-12s %s\r", mgroup.Name, mgroup.Comment);
				Count++;
				break;
			}
		}
	}
	
	fprintf(fp, "--------------------------------------------------------------------\r");
	fprintf(fp, "%d group(s)\r\r\r", Count);

	fclose(gp);
	free(temp);
}



/*
 * Write ticarea groups to tempfile
 */
void WriteFileGroups(FILE *fp, faddr *f)
{
	int	Count = 0, First = TRUE;
	char	*Group, *temp;
	FILE	*gp;
	faddr	*g;

	temp = calloc(128, sizeof(char));
	fprintf(fp, "Dear %s\r\r", nodes.Sysop);
	sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	fprintf(fp, "The following is a list of file groups at %s\r\r", ascfnode(f, 0x1f));

	if ((gp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		return;
	}
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);


//		     123456789012 1234567890123456789012345678901234567890123456789012345
	fprintf(fp, "Group        Description\r");
	fprintf(fp, "--------------------------------------------------------------------\r");

	while (TRUE) {
		Group = GetNodeFileGrp(First);
		if (Group == NULL)
			break;
		First = FALSE;

		fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
		while (fread(&fgroup, fgrouphdr.recsize, 1, gp) == 1) {
			g = bestaka_s(fido2faddr(fgroup.UseAka));
			if ((!strcmp(fgroup.Name, Group)) && 
			    (g->zone  == f->zone) && (g->net   == f->net) &&
			    (g->node  == f->node) && (g->point == f->point)) {
				fprintf(fp, "%-12s %s\r", fgroup.Name, fgroup.Comment);
				Count++;
				break;
			}
		}
	}
	
	fprintf(fp, "--------------------------------------------------------------------\r");
	fprintf(fp, "%d group(s)\r\r\r", Count);

	fclose(gp);
	free(temp);
}



char *GetBool(int Flag)
{
	if (Flag)
		return (char *)"Yes";
	else
		return (char *)"No";
}



/*
 * Shift all characters in Buf Cnt places to left
 */
void ShiftBuf(char *Buf, int Cnt)
{
	int	i;

	for (i = Cnt; i < strlen(Buf); i++)
		Buf[i - Cnt] = Buf[i];
	Buf[i - Cnt] = '\0';
}



/*
 * Remove spaces and = characters from begin of line
 */
void CleanBuf(char *Buf)
{
	while (strlen(Buf) && ((Buf[0] == ' ') || (Buf[0] == '=')))
		ShiftBuf(Buf, 1);
}



/*
 * Change AreaMgr and FileMgr password for a node
 */
void MgrPasswd(faddr *t, char *Buf, FILE *tmp, int Len)
{
	ShiftBuf(Buf, Len);
	CleanBuf(Buf);

	if ((strlen(Buf) < 3) || (strlen(Buf) > 15)) {
		fprintf(tmp, "A new password must be between 3 and 15 characters in length\n");
		Syslog('+', "XxxxMgr: Password length %d, not changed", strlen(Buf));
		return;
	}

	memset(&nodes.Apasswd, 0, sizeof(nodes.Apasswd));
	strncpy(nodes.Apasswd, tu(Buf), 15);
	fprintf(tmp, "AreaMgr and FileMgr password is now \"%s\"\n", nodes.Apasswd);
	Syslog('+', "XxxxMgr: Password \"%s\" for node %s", nodes.Apasswd, ascfnode(t, 0x1f));
	UpdateNode();
	SearchNodeFaddr(t);
}



/*
 * Change AreaMgr/FileMgr nodify flag for node
 */
void MgrNotify(faddr *t, char *Buf, FILE *tmp)
{
	/*
	 *  First strip leading garbage
	 */
	ShiftBuf(Buf, 7);
	CleanBuf(Buf);

	if (!strncasecmp(Buf, "on", 2))
		nodes.Notify = TRUE;
	else if (!strncasecmp(Buf, "off", 3))
		nodes.Notify = FALSE;
	else
		return;

	UpdateNode();
	SearchNodeFaddr(t);
	Syslog('+', "XxxxMgr: Notify %s", GetBool(nodes.Notify));
	fprintf(tmp, "AreaMgr and FileMgr Notify is %s\n", GetBool(nodes.Notify));
}



/*
 * Create uplink areamgr request. One netmail per request.
 * More is possible, cmd is then: "+area1\+area2\-area3"
 * Return values:
 *  0   - Ok
 *  1   - Node not in setup
 *  2   - No Uplink mgr program in setup
 *  3   - No uplink password in setup
 *  4   - Can't add mail to outbound
 */
int UplinkRequest(faddr *t, int FileMgr, char *cmd)
{
    FILE	*qp;
    time_t	Now;
    struct tm	*tm;
    fidoaddr	Orig, Dest;
    faddr	From;
    unsigned	flags = M_PVT;
    char	ext[4], *mgrname, *bymgr, *subj;
    int		i;

    From = *bestaka_s(t);
    memset(&Orig, 0, sizeof(Orig));
    Orig.zone  = From.zone;
    Orig.net   = From.net;
    Orig.node  = From.node;
    Orig.point = From.point;
    sprintf(Orig.domain, "%s", From.domain);

    memset(&Dest, 0, sizeof(Dest));
    Dest.zone  = t->zone;
    Dest.net   = t->net;
    Dest.node  = t->node;
    Dest.point = t->point;
    sprintf(Dest.domain, "%s", t->domain);

    if (!SearchNode(Dest)) {
	Syslog('+', "Can't find node %s in setup", aka2str(Dest));
	return 1;
    }

    if (FileMgr) {
	if (strlen(nodes.UplFmgrPgm) == 0) {
	    Syslog('!', "FileMgr program not defined in setup of node %s", aka2str(Dest));
	    return 2;
	}
	mgrname = xstrcpy(nodes.UplFmgrPgm);
	bymgr = xstrcpy((char *)"FileMgr");
    } else {
	if (strlen(nodes.UplAmgrPgm) == 0) {
	    Syslog('!', "AreaMgr program not defined in setup of node %s", aka2str(Dest));
	    return 2;
	}
	mgrname = xstrcpy(nodes.UplAmgrPgm);
	bymgr = xstrcpy((char *)"AreaMgr");
    }

    if (strlen(nodes.Apasswd) == 0) {
	Syslog('!', "No %s password set for node %s", mgrname, aka2str(Dest));
	return 3;
    }
    subj = xstrcpy(nodes.Apasswd);

    Syslog('-', "  Netmail from %s to %s", aka2str(Orig), ascfnode(t, 0x1f));

    Now = time(NULL) - (gmt_offset((time_t)0) * 60);
    flags |= (nodes.Crash)  ? M_CRASH    : 0;
    flags |= (nodes.Hold)   ? M_HOLD     : 0;

    /*
     *  Increase counters, update record and reload.
     */
    StatAdd(&nodes.MailSent, 1L);
    UpdateNode();
    SearchNode(Dest);

    memset(&ext, 0, sizeof(ext));
    if (nodes.PackNetmail)
	sprintf(ext, (char *)"qqq");
    else if (nodes.Crash)
	sprintf(ext, (char *)"ccc");
    else if (nodes.Hold)
	sprintf(ext, (char *)"hhh");
    else
	sprintf(ext, (char *)"nnn");

    if ((qp = OpenPkt(Orig, Dest, (char *)ext)) == NULL)
	return 4;

    if (AddMsgHdr(qp, &From, t, flags, 0, Now, mgrname, bymgr, subj)) {
	fclose(qp);
	return 4;
    }

    if (Dest.point)
	fprintf(qp, "\001TOPT %d\r", Dest.point);
    if (Orig.point)
	fprintf(qp, "\001FMPT %d\r", Orig.point);

    fprintf(qp, "\001INTL %d:%d/%d %d:%d/%d\r", Dest.zone, Dest.net, Dest.node, Orig.zone, Orig.net, Orig.node);

    /*
     * Add MSGID, REPLY and PID
     */
    fprintf(qp, "\001MSGID: %s %08lx\r", aka2str(Orig), sequencer());
    fprintf(qp, "\001PID: MBSE-FIDO %s\r", VERSION);
    fprintf(qp, "\001TZUTC: %s\r", gmtoffset(Now));

    for (i = 0; i < strlen(cmd); i++)
	putc(cmd[i], qp);
    putc('\r', qp);
    fprintf(qp, TearLine());

    tm = gmtime(&Now);
    fprintf(qp, "\001Via %s @%d%02d%02d.%02d%02d%02d.02.UTC %s\r",
		ascfnode(bestaka_s(t), 0x1f), tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, 
		tm->tm_hour, tm->tm_min, tm->tm_sec, VERSION);

    putc(0, qp);
    fclose(qp);

    free(mgrname);
    free(bymgr);
    free(subj);
    return 0;
}


