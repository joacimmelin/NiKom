#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <time.h>
#include <stdio.h>
#ifdef __GNUC__
/* In gcc access() is defined in unistd.h, while SAS/C has the
   prototype in stdio.h */
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "NiKomStr.h"
#include "ServerFuncs.h"
#include "NiKomLib.h"
#include "Config.h"
#include "RexxUtils.h"
#include "StringUtils.h"
#include "CommandParser.h"
#include "UserData.h"
#include "UserDataUtils.h"

extern struct System *Servermem;

char usernamebuf[50];

void handlerexx(struct RexxMsg *mess) {
	if(!strcmp(mess->rm_Args[0],"USERINFO")) userinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"MEETINFO")) motesinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGMEET")) chgmote(mess);
	else if(!stricmp(mess->rm_Args[0],"SYSINFO")) sysinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"NIKPARSE")) nikparse(mess);
	else if(!stricmp(mess->rm_Args[0],"LASTLOGIN")) senaste(mess);
	else if(!stricmp(mess->rm_Args[0],"COMMANDINFO")) kominfo(mess);
	else if(!stricmp(mess->rm_Args[0],"FILEINFO")) filinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"AREAINFO")) areainfo(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGUSER")) chguser(mess);
	else if(!stricmp(mess->rm_Args[0],"CREATEFILE")) skapafil(mess);
	else if(!stricmp(mess->rm_Args[0],"READCONFIG")) rexxreadcfg(mess);
	else if(!stricmp(mess->rm_Args[0],"NODEINFO")) rexxnodeinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"NEXTFILE")) rexxnextfile(mess);
	else if(!stricmp(mess->rm_Args[0],"DELETEFILE")) rexxraderafil(mess);
	else if(!stricmp(mess->rm_Args[0],"DELNIKFILE")) rexxraderafil(mess);
	else if(!stricmp(mess->rm_Args[0],"CREATETEXT")) createtext(mess);
	else if(!stricmp(mess->rm_Args[0],"TEXTINFO")) textinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"NEXTUNREAD")) nextunread(mess);
	else if(!stricmp(mess->rm_Args[0],"CREATELETTER")) createletter(mess);
	else if(!stricmp(mess->rm_Args[0],"MEETRIGHT")) meetright(mess);
	else if(!stricmp(mess->rm_Args[0],"MEETMEMBER")) meetmember(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGMEETRIGHT")) chgmeetright(mess);
	else if(!stricmp(mess->rm_Args[0],"CHGFILE")) chgfile(mess);
	else if(!stricmp(mess->rm_Args[0],"KEYINFO")) keyinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"GETDIR")) getdir(mess);
	else if(!stricmp(mess->rm_Args[0],"DELOLDTEXTS")) rexxPurgeOldTexts(mess);
	else if(!stricmp(mess->rm_Args[0],"SENDNODEMESS")) rxsendnodemess(mess);
	else if(!stricmp(mess->rm_Args[0],"STATUSINFO")) rexxstatusinfo(mess);
	else if(!stricmp(mess->rm_Args[0],"AREARIGHT")) rexxarearight(mess);
	else if(!stricmp(mess->rm_Args[0],"SYSSETTINGS")) rexxsysteminfo(mess);
	else if(!stricmp(mess->rm_Args[0],"MOVEFILE")) movefile(mess);
	else if(!stricmp(mess->rm_Args[0],"MARKTEXTREAD")) rexxmarktextread(mess);
	else if(!stricmp(mess->rm_Args[0],"MARKTEXTUNREAD")) rexxmarktextunread(mess);
	else if(!stricmp(mess->rm_Args[0],"NEXTPATTERNFILE")) rexxnextpatternfile(mess);
	else if(!stricmp(mess->rm_Args[0],"CONSOLETEXT")) rexxconsoletext(mess);
	else if(!stricmp(mess->rm_Args[0],"CHECKUSERPASSWORD")) rexxcheckuserpassword(mess);
	else {
		mess->rm_Result1=10;
		mess->rm_Result2=1L;
	}
}

int writefiles(int area)
{
	struct Node *nod;
	BPTR fh;
	char datafil[110];
	int index;

	ObtainSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

	sprintf(datafil,"nikom:datocfg/areor/%d.dat",area);

	if(!(fh=Open(datafil,MODE_NEWFILE)))
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		return 1;
	}

	index=0;
	nod = ((struct List *)&Servermem->areor[area].ar_list)->lh_Head;
	while(nod->ln_Succ)
	{
		if(Write(fh,nod,sizeof(struct DiskFil)) != sizeof(struct DiskFil)) {
			Close(fh);
			ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
			return 3;
		}
		((struct Fil *)nod)->index = index++;
		nod = nod->ln_Succ;
	}
	Close(fh);

	ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

	return 0;
}

int updatefile(int area,struct Fil *fil)
{
	BPTR fh;
	char datafil[110];

	ObtainSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);

	sprintf(datafil,"nikom:datocfg/areor/%d.dat",area);

	if(!(fh=Open(datafil,MODE_OLDFILE)))
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		return 1;
	}

	if(Seek(fh,fil->index * sizeof(struct DiskFil),OFFSET_BEGINNING)==-1)
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		Close(fh);
		return 2;
	}

	if(Write(fh,fil,sizeof(struct DiskFil)) != sizeof(struct DiskFil))
	{
		ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
		Close(fh);
		return 3;
	}

	Seek(fh,0,OFFSET_END);
	Close(fh);
	ReleaseSemaphore(&Servermem->semaphores[NIKSEM_FILEAREAS]);
	return 0;
}

void userinfo(struct RexxMsg *mess) {
  int userId, nodnr;
  char str[100];
  struct tm *ts;
  struct User *user;
  
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  if(!userexists(userId = atoi(mess->rm_Args[1]))) {
    SetRexxResultString(mess, "-1");
    return;
  }
  
  nodnr = 0; // Fix false warning about uninitialized variable.
  if(mess->rm_Args[2][0] != 'n'
     && mess->rm_Args[2][0] != 'N'
     && mess->rm_Args[2][0] != 'r'
     && mess->rm_Args[2][0] != 'R') {
    if(!(user = GetUserData(userId))) {
        SetRexxErrorResult(mess, 3);
        return;      
    }
  }
  switch(mess->rm_Args[2][0]) {
  case 'a' : case 'A' :
    strcpy(str, user->annan_info);
  case 'b' : case 'B' :
    ts = localtime(&user->forst_in);
    sprintf(str, "%4d%02d%02d %02d:%02d",
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min);
    break;
  case 'c' : case 'C' :
    strcpy(str, user->land);
    break;
  case 'd' : case 'D' :
    sprintf(str, "%ld", user->download);
    break;
  case 'e' : case 'E' :
    ts = localtime(&user->senast_in);
    sprintf(str, "%4d%02d%02d %02d:%02d",
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min);
    break;
  case 'f' : case 'F' :
    strcpy(str, user->telefon);
    break;
  case 'g' : case 'G' :
    strcpy(str, user->gata);
    break;
  case 'h' : case 'H' :
    sprintf(str, "%ld", user->downloadbytes);
    break;
  case 'i' : case 'I' :
    sprintf(str, "%ld", user->loggin);
    break;
  case 'j' : case 'J' :
    sprintf(str, "%ld", user->uploadbytes);
    break;
  case 'k' : case 'K' :
    strcpy(str, Servermem->languages[user->language]);
    break;
  case 'l' : case 'L' :
    sprintf(str, "%ld", user->read);
    break;
  case 'm' : case 'M' :
    strcpy(str, user->prompt);
    break;
  case 'n' : case 'N' :
    strcpy(str, getusername(userId));
    break;
  case 'o' : case 'O' :
    sprintf(str, "%ld", user->flaggor);
    break;
  case 'p' : case 'P' :
    strcpy(str, user->postadress);
    break;
  case 'q' : case 'Q' :
    sprintf(str, "%d", user->rader);
    break;
  case 'r' : case 'R' :
    sprintf(str, "%d", getuserstatus(userId));
    break;
  case 's' : case 'S' :
    sprintf(str, "%ld", user->skrivit);
    break;
  case 't' : case 'T' :
    sprintf(str, "%ld", user->tot_tid);
    break;
  case 'u' : case 'U' :
    sprintf(str, "%ld", user->upload);
    break;
  case 'x' : case 'X' :
    strcpy(str, user->losen);
    break;
  case 'y' : case 'Y' :
    sprintf(str, "%d,%d", (unsigned int)user->grupper / 65536, (unsigned int)user->grupper % 65536);
    break;
  case 'z' : case 'Z' :
    sprintf(str, "%ld", user->brevpek);
    break;
  default :
    str[0]=0;
    break;
  }
  SetRexxResultString(mess, str);
}


void motesinfo(struct RexxMsg *mess) {
	int nummer;
	struct Mote *motpek;
	char str[100];
	struct tm *ts;
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	motpek=getmotpek(nummer=atoi(mess->rm_Args[1]));
	if(!motpek) {
          SetRexxResultString(mess, "-1");
          return;
	}

	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%ld",motpek->skapat_av);
			break;
		case 'c' : case 'C' :
			sprintf(str,"%d",motpek->charset);
			break;
		case 'd' : case 'D' :
			sprintf(str,motpek->dir);
			break;
		case 'f' : case 'F' :
			strcpy(str,motpek->tagnamn);
			break;
		case 'g' : case 'G' :
			sprintf(str,"%d,%d",(unsigned int)motpek->grupper/65536,(unsigned int)motpek->grupper%65536);
			break;
		case 'h' : case 'H' :
			sprintf(str,"%ld",motpek->texter);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%ld",motpek->lowtext);
			break;
		case 'm' : case 'M' :
			sprintf(str,"%ld",motpek->mad);
			break;
		case 'n' : case 'N' :
			strcpy(str,motpek->namn);
			break;
		case 'o' : case 'O' :
			strcpy(str,motpek->origin);
			break;
		case 'p' : case 'P' :
			sprintf(str,"%ld",motpek->sortpri);
			break;
		case 'r' : case 'R' :
			sprintf(str,"%ld",motpek->renumber_offset);
			break;
		case 's' : case 'S' :
			sprintf(str,"%d",motpek->status);
			break;
		case 't' : case 'T' :
			ts=localtime(&motpek->skapat_tid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour,ts->tm_min);
			break;
		case 'y' : case 'Y' :
			sprintf(str,"%d",motpek->type);
			break;
		default :
			str[0]=0;
			break;
	}
        SetRexxResultString(mess, str);
}

void chgmote(struct RexxMsg *mess)
{
	int nummer, ok = 0;
	struct Mote *motpek;

	if((!mess->rm_Args[1]) || (!mess->rm_Args[2]) || (!mess->rm_Args[3]))
	{
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	motpek=getmotpek(nummer=atoi(mess->rm_Args[1]));
	if(!motpek) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			ok = 1;
			motpek->skapat_av = atoi(mess->rm_Args[3]);
			break;
		case 'c' : case 'C' :
			ok = 1;
			motpek->charset = atoi(mess->rm_Args[3]);
			break;
		case 'd' : case 'D' :
			ok = 1;
			strncpy(motpek->dir, mess->rm_Args[3], 79);
			break;
		case 'f' : case 'F' :
			ok = 1;
			strncpy(motpek->tagnamn, mess->rm_Args[3], 49);
			break;
		case 'h' : case 'H' :
			ok = 1;
			motpek->texter = atoi(mess->rm_Args[3]);
			break;
		case 'l' : case 'L' :
			ok = 1;
			motpek->lowtext = atoi(mess->rm_Args[3]);
			break;
		case 'm' : case 'M' :
			ok = 1;
			motpek->mad = atoi(mess->rm_Args[3]);
			break;
		case 'n' : case 'N' :
			ok = 1;
			strncpy(motpek->namn, mess->rm_Args[3], 40);
			break;
		case 'o' : case 'O' :
			ok = 1;
			strncpy(motpek->origin, mess->rm_Args[3], 69);
			break;
		case 'p' : case 'P' :
			motpek->sortpri = atoi(mess->rm_Args[3]);
			ok = 1;
			break;
		case 'r' : case 'R' :
			ok = 1;
			motpek->renumber_offset = atoi(mess->rm_Args[3]);
			break;
		case 's' : case 'S' :
			ok = 1;
			motpek->status = atoi(mess->rm_Args[3]);
			break;
		case 'y' : case 'Y' :
			ok = 1;
			motpek->type = atoi(mess->rm_Args[3]);
			break;
		default :
			if(!(mess->rm_Result2=(long)CreateArgstring("-2",2)))
				printf("Kunde inte allokera en ArgString\n");
			mess->rm_Result1=0;
			return;
			break;
	}

	if(ok)
		writemeet(motpek);

	if(!(mess->rm_Result2=(long)CreateArgstring("1",strlen("1"))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void writemeet(struct Mote *motpek)
{
	BPTR fh;
	if(!(fh=Open("NiKom:DatoCfg/M�ten.dat",MODE_OLDFILE))) {
		printf("\r\n\nKunde inte �ppna M�ten.dat\r\n");
		return;
	}
	if(Seek(fh,motpek->nummer*sizeof(struct Mote),OFFSET_BEGINNING)==-1) {
		printf("\r\n\nKunde inte s�ka i Moten.dat!\r\n\n");
		Close(fh);
		return;
	}
	if(Write(fh,(void *)motpek,sizeof(struct Mote))==-1) {
		printf("\r\n\nFel vid skrivandet av M�ten.dat\r\n");
	}
	Close(fh);
}

static int findLangId(char *langStr) {
  int i;
  if(langStr == NULL) {
    return 0;
  }
  for(i = 0; i < NUM_LANGUAGES; i++) {
    if(stricmp(Servermem->languages[i], langStr) == 0) {
      return i;
    }
  }
  return 0;
}

void nikparse(struct RexxMsg *mess) {
  char str[10];
  int parseRes, nikparseRes;
  struct Kommando *cmds[50];

  if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  switch(mess->rm_Args[2][0]) {
  case 'k' : case 'K' :
    parseRes = ParseCommand(mess->rm_Args[1], findLangId(mess->rm_Args[3]), NULL, cmds, NULL);
    switch(parseRes) {
    case -2:
      nikparseRes = 212;
      break;
    case -1:
      nikparseRes = -3;
      break;
    case 0:
      nikparseRes = -1;
      break;
    case 1:
      nikparseRes = cmds[0]->nummer;
      break;
    default:
      nikparseRes = -2;
      break;
    }
    sprintf(str, "%d", nikparseRes);
    break;
  case 'm' : case 'M' :
    sprintf(str, "%d", parsemot(mess->rm_Args[1]));
    break;
  case 'n' : case 'N' :
    sprintf(str, "%d", parsenamn(mess->rm_Args[1]));
    break;
  case 'a' : case 'A' :
    sprintf(str, "%d", parsearea(mess->rm_Args[1]));
    break;
  case 'y' : case 'Y' :
    sprintf(str, "%d", parsenyckel(mess->rm_Args[1]));
    break;
  default :
    str[0] = '\0';
    break;
  }
  SetRexxResultString(mess, str);
}

void senaste(struct RexxMsg *mess) {
	int nummer;
	struct tm *ts;
	char str[100];
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	if((nummer=atoi(mess->rm_Args[1]))<0 || nummer>=MAXSENASTE) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!Servermem->senaste[nummer].utloggtid) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!userexists(Servermem->senaste[nummer].anv)) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%ld",Servermem->senaste[nummer].anv);
			break;
		case 'd' : case 'D' :
			sprintf(str,"%d",Servermem->senaste[nummer].dl);
			break;
		case 'g' : case 'G' :
			ts=localtime(&Servermem->senaste[nummer].utloggtid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%d",Servermem->senaste[nummer].read);
			break;
		case 's' : case 'S' :
			sprintf(str,"%d",Servermem->senaste[nummer].write);
			break;
		case 't' : case 'T' :
			sprintf(str,"%d",Servermem->senaste[nummer].tid_inloggad);
			break;
		case 'u' : case 'U' :
			sprintf(str,"%d",Servermem->senaste[nummer].ul);
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void sysinfo(struct RexxMsg *mess) {
	int nummer=0, i, j, tmp = -1;
	long tmp1;
	struct Mote *letpek;
	char str[100], *args1;
	if(!mess->rm_Args[1]) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	switch(mess->rm_Args[1][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%ld",((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer);
			break;
		case 'h' : case 'H' :
			sprintf(str,"%ld",Servermem->info.hightext);
			break;
		case 'k' : case 'K' :
			sprintf(str,"%d",Servermem->cfg->noOfCommands);
			break;
		case 'l' : case 'L' :
			sprintf(str,"%ld",Servermem->info.lowtext);
			break;
		case 'm' : case 'M' :
			letpek=(struct Mote *)Servermem->mot_list.mlh_Head;
			for(;letpek->mot_node.mln_Succ;letpek=(struct Mote *)letpek->mot_node.mln_Succ)
				if(letpek->nummer > nummer) nummer=letpek->nummer;
			sprintf(str,"%d",nummer);
			break;
		case 'n' : case 'N' :
			sprintf(str,"%d",Servermem->cfg->noOfFileKeys);
			break;
		case 'o' : case 'O' :
			sprintf(str,"%d",Servermem->info.areor);
			break;
		case 'T' : case 't' :
			while(Servermem->info.bps[++tmp] > 0);

			sprintf(str,"%d",tmp);
			break;

		case 'b' : case 'B' :
			args1 = &mess->rm_Args[1][0];
			args1++;
			if(args1 == NULL)
			{
				strcpy(str,"-2");
				return;
			}

			while(Servermem->info.bps[++tmp] > 0 && tmp < 50);

			for(i=0;i<tmp;i++)
			{
				for(j=0;j<i;j++)
				{
					if(i != j)
					{
						if(Servermem->info.bps[i] < Servermem->info.bps[j])
						{
							swapbps(&Servermem->info.bps[i], &Servermem->info.bps[j]);
							swapbps(&Servermem->info.antbps[i], &Servermem->info.antbps[j]);
						}
					}
				}
			}

			tmp1 = atoi((char *)args1);
			if(tmp1 < 1 || tmp1 > tmp)
			{
				strcpy(str,"-1");
				break;
			}

			sprintf(str,"%ld %ld",Servermem->info.bps[tmp1-1], Servermem->info.antbps[tmp1-1]);
			break;
		case 'd' : case 'D' :
			sprintf(str, "%d", (int) MAXNOD);
			break;

		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void swapbps(long *bps, long *bps2)
{
	long tmp;

	tmp = *bps;
	*bps = *bps2;
	*bps2 = tmp;
}

void rexxreadcfg(struct RexxMsg *mess) {
  SetRexxResultString(mess, ReReadConfigs() ? "1" : "0");
}

int parsenamn(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,found=-1,nummer;
	char *faci,*skri2;
	struct ShortUser *letpek;
	if(skri[0]==0 || skri[0]==' ') return(-3);
	if(IzDigit(skri[0])) {
		nummer=atoi(skri);
		if(userexists(nummer)) return(nummer);
		else return(-1);
	}
	if(matchar(skri,"Sysop")) return(0);
	letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;
	while(letpek->user_node.mln_Succ && going)
	{
		faci=letpek->namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci))
		{
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0)
				{
					found=letpek->nummer;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci))
				{
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		letpek=(struct ShortUser *)letpek->user_node.mln_Succ;
	}
	return(found);
}

int matchar(skrivet,facit)
char *skrivet,*facit;
{
	int mat=TRUE,count=0;
	char tmpskrivet,tmpfacit;
	if(facit!=NULL) {
		while(mat && skrivet[count]!=' ' && skrivet[count]!=0) {
			if(skrivet[count]=='*') { count++; continue; }
			tmpskrivet=ToUpper(skrivet[count]);
			tmpfacit=ToUpper(facit[count]);
			if(tmpskrivet!=tmpfacit) mat=FALSE;
			count++;
		}
	}
	return(mat);
}

int parsemot(char *skri) {
	struct Mote *motpek=(struct Mote *)Servermem->mot_list.mlh_Head;
	int going2=TRUE,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') return(-3);
	for(;motpek->mot_node.mln_Succ;motpek=(struct Mote *)motpek->mot_node.mln_Succ) {
		faci=motpek->namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0) return((int)motpek->nummer);
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
	}
	return(found);
}

int parsearea(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==Servermem->info.areor) going=FALSE;
		faci=Servermem->areor[count].namn;
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

int parsenyckel(skri)
char *skri;
{
	int going=TRUE,going2=TRUE,count=0,found=-1;
	char *faci,*skri2;
	if(skri[0]==0 || skri[0]==' ') {
		found=-3;
		going=FALSE;
	}
	while(going) {
		if(count==Servermem->cfg->noOfFileKeys) going=FALSE;
		faci=Servermem->cfg->fileKeys[count];
		skri2=skri;
		going2=TRUE;
		if(matchar(skri2,faci)) {
			while(going2) {
				skri2=FindNextWord(skri2);
				faci=FindNextWord(faci);
				if(skri2[0]==0) {
					found=count;
					going2=going=FALSE;
				}
				else if(faci[0]==0) going2=FALSE;
				else if(!matchar(skri2,faci)) {
					faci=FindNextWord(faci);
					if(faci[0]==0 || !matchar(skri2,faci)) going2=FALSE;
				}
			}
		}
		count++;
	}
	return(found);
}

struct Fil *parsefil(skri,area)
char *skri;
int area;
{
	int quoted = FALSE;
	char tmpstr[42], *pt;
	struct Fil *letpek;
	if(skri[0]==0 || skri[0]==' ') return(NULL);
	if(skri[0]=='"') {
		quoted = TRUE;
		strcpy(tmpstr,&skri[1]);
		if((pt = strchr(tmpstr,'"'))) *pt = 0;
	}
	else strcpy(tmpstr,skri);
	letpek=(struct Fil *)Servermem->areor[area].ar_list.mlh_TailPred;
	while(letpek->f_node.mln_Pred) {
		if(quoted) {
			if(!stricmp(tmpstr,letpek->namn)) return(letpek);
		} else {
			if(matchar(tmpstr,letpek->namn)) return(letpek);
		}
		letpek=(struct Fil *)letpek->f_node.mln_Pred;
	}
	return(NULL);
}

char *getusername(int nummer) {
	struct ShortUser *letpek;
	int found=FALSE;
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			if(letpek->namn[0]) sprintf(usernamebuf,"%s #%d",letpek->namn,nummer);
			else strcpy(usernamebuf,"<Raderad Anv�ndare>");
			found=TRUE;
			break;
		}
	}
	if(!found) strcpy(usernamebuf,"<Felaktigt anv�ndarnummer>");
	return(usernamebuf);
}

int userexists(int nummer) {
	struct ShortUser *letpek;
	int found=FALSE;
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			found=TRUE;
			break;
		}
	}
	return(found);
}

int getuserstatus(int nummer) {
	struct ShortUser *letpek;
	int status=-1;
	for(letpek=(struct ShortUser *)Servermem->user_list.mlh_Head;letpek->user_node.mln_Succ;letpek=(struct ShortUser *)letpek->user_node.mln_Succ) {
		if(letpek->nummer==nummer) {
			status=letpek->status;
			break;
		}
	}
	return(status);
}

void kominfo(struct RexxMsg *mess) {
  int cmdId, found = FALSE;
  char str[100];
  struct Kommando *cmd;
  if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  cmdId = atoi(mess->rm_Args[1]);
  ITER_EL(cmd, Servermem->cfg->kom_list, kom_node, struct Kommando *) {
    if(cmd->nummer == cmdId) {
      found = TRUE;
      break;
    }
  }
  if(!found) {
    SetRexxResultString(mess, "-1");
    return;
  }
  switch(mess->rm_Args[2][0]) {
  case 'a' : case 'A' :
    sprintf(str, "%d", cmd->argument);
    break;
  case 'd' : case 'D' :
    sprintf(str, "%ld", cmd->mindays);
    break;
  case 'l' : case 'L' :
    sprintf(str, "%ld", cmd->minlogg);
    break;
  case 'n' : case 'N' :
    strcpy(str, cmd->langCmd[findLangId(mess->rm_Args[3])].name);
    if(str[strlen(str) - 1] == '\n') {
      str[strlen(str) - 1] = '\0';
    }
    break;
  case 'o' : case 'O' :
    sprintf(str, "%d", cmd->langCmd[findLangId(mess->rm_Args[3])].words);
    break;
  case 's' : case 'S' :
    sprintf(str, "%d", cmd->status);
    break;
  case 'x' : case 'X' :
    strcpy(str, cmd->losen);
    break;
  defualt :
    str[0] = '\0';
    break;
  }
  SetRexxResultString(mess, str);
}

void filinfo(struct RexxMsg *mess) {
	int area;
	struct Fil *filpek;
	struct tm *ts;
	char str[256];
	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	area=atoi(mess->rm_Args[3]);
	if(area<0 || area>=Servermem->info.areor || !Servermem->areor[area].namn) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!(filpek=parsefil(mess->rm_Args[1],area))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'b' : case 'B' :
			strcpy(str,filpek->beskr);
			break;
		case 'd' : case 'D' :
			sprintf(str,"%d",filpek->downloads);
			break;
		case 'e' : case 'E' :
			ts=localtime(&filpek->senast_dl);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'f' : case 'F' :
			sprintf(str,"%ld",filpek->flaggor);
			break;
		case 'i' : case 'I' :
			sprintf(str,"%ld",filpek->dir);
			break;
		case 'n' : case 'N' :
			strcpy(str,filpek->namn);
			break;
		case 'r' : case 'R' :
			sprintf(str,"%d",filpek->status);
			break;
		case 's' : case 'S' :
			sprintf(str,"%ld",filpek->size);
			break;
		case 't' : case 'T' :
			ts=localtime(&filpek->tid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		case 'u' : case 'U' :
			sprintf(str,"%ld",filpek->uppladdare);
			break;
		case 'l' : case 'L' :
			if(filpek->flaggor & FILE_LONGDESC)
				sprintf(str,"%slongdesc/%s.long",Servermem->areor[area].dir[filpek->dir],filpek->namn);
			else
			{
				if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
					printf("Kunde inte allokera en ArgString\n");
				mess->rm_Result1=0;
				return;
			}
			break;
		default:
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void areainfo(struct RexxMsg *mess) {
	int nummer,dir;
	char str[100];
	struct tm *ts;
	if((!mess->rm_Args[1]) || (!mess->rm_Args[2])) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	if((nummer=atoi(mess->rm_Args[1]))<0 || nummer>=Servermem->info.areor) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-2",strlen("-2"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!Servermem->areor[nummer].namn[0]) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	switch(mess->rm_Args[2][0]) {
		case 'a' : case 'A' :
			sprintf(str,"%ld",Servermem->areor[nummer].skapad_av);
			break;
		case 'd' : case  'D' :
			dir=atoi(&mess->rm_Args[2][1]);
			if(dir<0 || dir>15) {
				str[0]=0;
				break;
			}
			strcpy(str,Servermem->areor[nummer].dir[dir]);
			break;
		case 'f' : case 'F' :
			sprintf(str,"%ld",Servermem->areor[nummer].flaggor);
			break;
		case 'g' : case 'G' :
			sprintf(str,"%d,%d",(unsigned int)Servermem->areor[nummer].grupper/65536,(unsigned int)Servermem->areor[nummer].grupper%65536);
			break;
		case 'm' : case 'M' :
			sprintf(str,"%d",Servermem->areor[nummer].mote);
			break;
		case 'n' : case 'N' :
			strcpy(str,Servermem->areor[nummer].namn);
			break;
		case 't' : case 'T' :
			ts=localtime(&Servermem->areor[nummer].skapad_tid);
			sprintf(str,"%4d%02d%02d %02d:%02d",
                                ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                                ts->tm_hour, ts->tm_min);
			break;
		default :
			str[0]=0;
			break;
	}
	if(!(mess->rm_Result2=(long)CreateArgstring(str,strlen(str))))
		printf("Kunde inte allokera en ArgString\n");
	mess->rm_Result1=0;
}

void chguser(struct RexxMsg *mess) {
  int userId, i, otherUserId;
  struct ShortUser *shortUser;
  struct User chguseruser;
  char temp[20];

  if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3]) {
    SetRexxErrorResult(mess, 1);
    return;
  }
  if(!userexists(userId = atoi(mess->rm_Args[1]))) {
    SetRexxResultString(mess, "-1");
    return;
  }
  for(i = 0; i < MAXNOD; i++) {
    if(Servermem->inloggad[i] == userId) {
      break;
    }
  }

  if(i == MAXNOD) {
    if(!ReadUser(userId, &chguseruser)) {
      SetRexxErrorResult(mess, 3);
      return;
    }
  }

  switch(mess->rm_Args[3][0]) {
  case 'a' : case 'A' :
    if(i == MAXNOD) strncpy(chguseruser.annan_info, mess->rm_Args[2], 60);
    else strncpy(Servermem->inne[i].annan_info, mess->rm_Args[2], 60);
    break;
  case 'c' : case 'C' :
    if(i == MAXNOD) strncpy(chguseruser.land, mess->rm_Args[2], 40);
    else strncpy(Servermem->inne[i].land, mess->rm_Args[2], 40);
    break;
  case 'd' : case 'D' :
    if(i == MAXNOD) chguseruser.download = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].download = atoi(mess->rm_Args[2]);
    break;
  case 'f' : case 'F' :
    if(i == MAXNOD) strncpy(chguseruser.telefon, mess->rm_Args[2], 20);
    else strncpy(Servermem->inne[i].telefon, mess->rm_Args[2], 20);
    break;
  case 'g' : case 'G' :
    if(i == MAXNOD) strncpy(chguseruser.gata, mess->rm_Args[2], 40);
    else strncpy(Servermem->inne[i].gata, mess->rm_Args[2], 40);
    break;
  case 'h' : case 'H' :
    if(i == MAXNOD) chguseruser.downloadbytes = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].downloadbytes = atoi(mess->rm_Args[2]);
    break;
  case 'j' : case 'J' :
    if(i == MAXNOD) chguseruser.uploadbytes = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].uploadbytes = atoi(mess->rm_Args[2]);
    break;
  case 'i' : case 'I' :
    if(i == MAXNOD) chguseruser.loggin = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].loggin = atoi(mess->rm_Args[2]);
    break;
  case 'l' : case 'L' :
    if(i == MAXNOD) chguseruser.read = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].read = atoi(mess->rm_Args[2]);
    break;
  case 'm' : case 'M' :
    if(i == MAXNOD) strncpy(chguseruser.prompt, mess->rm_Args[2], 5);
    else strncpy(Servermem->inne[i].prompt, mess->rm_Args[2], 5);
    break;
  case 'n' : case 'N' :
    if((otherUserId = parsenamn(mess->rm_Args[2])) == -1 || otherUserId == userId) {
      if(i == MAXNOD) strncpy(chguseruser.namn, mess->rm_Args[2], 40);
      else strncpy(Servermem->inne[i].namn, mess->rm_Args[2], 40);
      ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
        if(shortUser->nummer == userId) {
          strncpy(shortUser->namn, mess->rm_Args[2], 40);
          break;
        }
      }
    }
    break;
  case 'o' : case 'O' :
    if(i == MAXNOD) chguseruser.flaggor = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].flaggor = atoi(mess->rm_Args[2]);
    break;
  case 'p' : case 'P' :
    if(i == MAXNOD) strncpy(chguseruser.postadress, mess->rm_Args[2], 40);
    else strncpy(Servermem->inne[i].postadress, mess->rm_Args[2], 40);
    break;
  case 'q' : case 'Q' :
    if(i == MAXNOD) chguseruser.rader = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].rader = atoi(mess->rm_Args[2]);
    break;
  case 'r' : case 'R' :
    if(i == MAXNOD) chguseruser.status = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].status = atoi(mess->rm_Args[2]);
    ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
      if(shortUser->nummer == userId) {
        shortUser->status = atoi(mess->rm_Args[2]);
        break;
      }
    }
    break;
  case 's' : case 'S' :
    if(i == MAXNOD) chguseruser.skrivit = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].skrivit = atoi(mess->rm_Args[2]);
    break;
  case 't' : case 'T' :
    if(i == MAXNOD) chguseruser.tot_tid = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].tot_tid = atoi(mess->rm_Args[2]);
    break;
  case 'u' : case 'U' :
    if(i == MAXNOD) chguseruser.upload = atoi(mess->rm_Args[2]);
    else Servermem->inne[i].upload = atoi(mess->rm_Args[2]);
    break;
  case 'x' : case 'X' :
    if(Servermem->cfg->cfgflags & NICFG_CRYPTEDPASSWORDS) {
      CryptPassword(mess->rm_Args[2], temp);
    } else {
      strncpy(temp, mess->rm_Args[2], 15);
    }
    if(i == MAXNOD) strncpy(chguseruser.losen, temp, 15);
    else strncpy(Servermem->inne[i].losen, temp, 15);
    break;
  default :
    break;
  }
  if(i == MAXNOD && !WriteUser(userId,&chguseruser)) {
    SetRexxErrorResult(mess, 3);
  } else {
    SetRexxResultString(mess, "0");
  }
}

void skapafil(struct RexxMsg *mess) {
	struct Fil *fil;
	char filnamn[100];
	long tid;
	struct FileInfoBlock *fib;
	BPTR lock;
	int cnt=6,nyckel,area,x,valres;

	if(!mess->rm_Args[1] || !mess->rm_Args[2] || !mess->rm_Args[3] || !mess->rm_Args[4] || !mess->rm_Args[5]) {
		mess->rm_Result1=1;
		mess->rm_Result2=0;
		return;
	}
	if((area=atoi(mess->rm_Args[2]))<0 || area>=Servermem->info.areor || !Servermem->areor[area].namn[0]) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-1",strlen("-1"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	valres = valnamn(mess->rm_Args[1],area);
	if(valres) {
		sprintf(filnamn,"%d",valres);
		if(!(mess->rm_Result2=(long)CreateArgstring(filnamn,strlen(filnamn))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	for(x=0;x<16;x++) {
		sprintf(filnamn,"%s%s",Servermem->areor[area].dir[x],mess->rm_Args[1]);
		if(!access(filnamn,0)) break;
	}
	if(x==16) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-3",strlen("-3"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	sprintf(filnamn,"%s%s",Servermem->areor[area].dir[x],mess->rm_Args[1]);

	if(!userexists(atoi(mess->rm_Args[3]))) {
		if(!(mess->rm_Result2=(long)CreateArgstring("-4",strlen("-4"))))
		printf("Kunde inte allokera en ArgString\n");
		mess->rm_Result1=0;
		return;
	}
	if(!(fil=(struct Fil *)AllocMem(sizeof(struct Fil),MEMF_CLEAR | MEMF_PUBLIC))) {
		printf("Kunde inte allokera minne f�r filen!\n");
		mess->rm_Result1=2;
		mess->rm_Result2=0;
		return;
	}
	fil->tid=fil->validtime=time(&tid);
	if(!(lock=Lock(filnamn,ACCESS_READ))) {
		printf("Kunde inte f� ett lock f�r filen!\n");
		mess->rm_Result1=3;
		mess->rm_Result2=0;
		FreeMem(fil,sizeof(struct Fil));
		return;
	}
	if(!(fib=(struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock),MEMF_CLEAR))) {
		printf("Kunde inte allokera minne f�r en FileInfoBlock-struktur!\n");
		mess->rm_Result1=4;
		mess->rm_Result2=0;
		FreeMem(fil,sizeof(struct Fil));
		UnLock(lock);
		return;
	}
	if(!Examine(lock,fib)) {
		printf("Kunde inte g�ra Examine() p� filen!\n");
		mess->rm_Result1=5;
		mess->rm_Result2=0;
		FreeMem(fil,sizeof(struct Fil));
		UnLock(lock);
		FreeMem(fib,sizeof(struct FileInfoBlock));
		return;
	}
	fil->size=fib->fib_Size;
	UnLock(lock);
	FreeMem(fib,sizeof(struct FileInfoBlock));
	fil->uppladdare=atoi(mess->rm_Args[3]);
	strncpy(fil->namn,mess->rm_Args[1],40);
	fil->status=atoi(mess->rm_Args[5]);
	strncpy(fil->beskr,mess->rm_Args[4],70);
	fil->dir=x;
	if(Servermem->cfg->cfgflags & NICFG_VALIDATEFILES) fil->flaggor = FILE_NOTVALID;
	for(cnt=6;cnt<16 && mess->rm_Args[cnt];cnt++)
		if((nyckel=parsenyckel(mess->rm_Args[cnt]))!=-1) BAMSET(fil->nycklar,nyckel);
	AddTail((struct List *)&Servermem->areor[area].ar_list,(struct Node *)fil);
	if(writefiles(area)) {
		printf("Kunde inte �ppna datafilen\n");
		mess->rm_Result1=6;
		mess->rm_Result2=0;
		return;
	}
	mess->rm_Result1=0;
	if(!(mess->rm_Result2=(long)CreateArgstring("0",strlen("0"))))
		printf("Kunde inte allokera en ArgString\n");
}

int valnamn(char *namn,int area) {
	if(strlen(namn)>25) return(-6);
	if(strpbrk(namn," #?*/:()[]\"")) return(-5);
	if(parsefil(namn,area)) return(-2);
	return(0);
}
