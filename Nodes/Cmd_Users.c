#include <stdio.h>
#ifdef __GNUC__
/* In gcc access() is defined in unistd.h, while SAS/C has the
   prototype in stdio.h */
# include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <proto/exec.h>

#include "Terminal.h"
#include "UserNotificationHooks.h"
#include "ServerMemUtils.h"
#include "Logging.h"
#include "BasicIO.h"
#include "Languages.h"

#include "NiKomLib.h"
#include "NiKomStr.h"
#include "Nodes.h"
#include "NiKomFuncs.h"
#include "UserDataUtils.h"

#include "Cmd_Users.h"

extern struct System *Servermem;
extern int nodnr, inloggad, g_userDataSlot;
extern char outbuffer[],inmat[], *argument;


void Cmd_Status(void) {
  struct User *user;
  struct Mote *conf;
  int userId, cnt = 0, sumUnread = 0, showAllConf = FALSE, kb;
  struct tm *ts;
  struct UserGroup *listpek;
  char filnamn[100];
  struct UnreadTexts unreadTextsBuf, *unreadTexts;

  if(argument[0] == '-' && (argument[1] == 'a' || argument[1] == 'A')) {
    showAllConf = TRUE;
    argument = hittaefter(argument);
  }
  if(argument[0] == 0) {
    user = CURRENT_USER;
    unreadTexts = CUR_USER_UNREAD;
    userId = inloggad;
  } else {
    if((userId = parsenamn(argument)) == -1) {
      SendString("\r\n\nFinns ingen som heter s� eller har det numret\r\n\n");
      return;
    }
    if(!(user = GetLoggedInUser(userId, &unreadTexts))) {
      if(!(user = GetUserData(userId))) {
        return;
      }
      if(!ReadUnreadTexts(&unreadTextsBuf, userId)) {
        return;
      }
      unreadTexts = &unreadTextsBuf;
    }
  }
  if(SendStringCat("\r\n\n%s\r\n\n", CATSTR(MSG_USER_STATUS_HEADER), user->namn,userId)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_LEVEL), user->status)) { return; }
  if(!((user->flaggor & SKYDDAD)
       && CURRENT_USER->status < Servermem->cfg->st.sestatus
       && inloggad != userId)) {
    if(SendString("%-21s: %s\r\n", CATSTR(MSG_USER_STREET), user->gata)) { return; }
    if(SendString("%-21s: %s\r\n", CATSTR(MSG_USER_CITY), user->postadress)) { return; }
    if(SendString("%-21s: %s\r\n", CATSTR(MSG_USER_COUNTRY), user->land)) { return; }
    if(SendString("%-21s: %s\r\n", CATSTR(MSG_USER_PHONE), user->telefon)) { return; }
  }
  if(SendString("%-21s: %s\r\n", CATSTR(MSG_USER_MISCINFO), user->annan_info)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_LINES), user->rader)) { return; }
  ts = localtime(&user->forst_in);
  if(SendString("%-21s: %4d%02d%02d  %02d:%02d\r\n", CATSTR(MSG_USER_FIRST_LOGIN),
          ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
          ts->tm_min)) { return; }
  ts = localtime(&user->senast_in);
  if(SendString("%-21s: %4d%02d%02d  %02d:%02d\r\n", CATSTR(MSG_USER_LAST_LOGIN),
          ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday, ts->tm_hour,
          ts->tm_min)) { return; }
  if(SendString("%-21s: %d:%02d\r\n", CATSTR(MSG_USER_TOTAL_TIME), user->tot_tid / 3600,
          (user->tot_tid % 3600) / 60)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_LOGINS), user->loggin)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_READ), user->read)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_WRITTEN), user->skrivit)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_DL_FILES), user->download)) { return; }
  if(SendString("%-21s: %d\r\n", CATSTR(MSG_USER_UL_FILES), user->upload)) { return; }

  kb = user->downloadbytes > 90000;
  if(SendString("%-21s: %d %s\r\n", CATSTR(MSG_USER_DL_BYTES),
                kb ? user->downloadbytes / 1024 : user->downloadbytes,
                kb ? "KB" : "B"))
    { return; }
  kb = user->uploadbytes > 90000;
  if(SendString("%-21s: %d %s\r\n\n", CATSTR(MSG_USER_UL_BYTES),
                kb ? user->uploadbytes / 1024 : user->uploadbytes,
                kb ? "KB" : "B"))
    { return; }

  if(user->grupper) {
    SendString("%s:\r\n", CATSTR(MSG_USER_USER_GROUPS));
    ITER_EL(listpek, Servermem->grupp_list, grupp_node, struct UserGroup *) {
      if(!BAMTEST((char *)&user->grupper, listpek->nummer)) {
        continue;
      }
      if((listpek->flaggor & HEMLIGT)
         && !BAMTEST((char *)&CURRENT_USER->grupper, listpek->nummer)
         && CURRENT_USER->status < Servermem->cfg->st.medmoten) {
        continue;
      }
      if(SendString(" %s\r\n", listpek->namn)) { return; }
    }
    SendString("\n");
  }
  if((cnt = countmail(userId, user->brevpek))) {
    if(SendString("%4d %s\r\n", cnt, CATSTR(MSG_MAIL_MAILBOX))) { return; }
    sumUnread += cnt;
  }
  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(!MaySeeConf(conf->nummer, inloggad, CURRENT_USER)
       || !IsMemberConf(conf->nummer, userId, user)) {
      continue;
    }
    cnt = 0;
    switch(conf->type) {
    case MOTE_ORGINAL :
      cnt = CountUnreadTexts(conf->nummer, unreadTexts);
      break;
    case MOTE_FIDO:
      cnt = conf->texter - unreadTexts->lowestPossibleUnreadText[conf->nummer] + 1;
      break;
    default :
      break;
    }
    if(cnt > 0 || showAllConf) {
      if(SendString("%4d %s\r\n", cnt, conf->namn)) { return; }
      sumUnread += cnt;
    }
  }
  if(SendString("\r\n%s: %d\r\n\n", CATSTR(MSG_USER_UNREAD_TEXTS), sumUnread)) { return; }
  sprintf(filnamn, "NiKom:Users/%d/%d/Lapp", userId / 100, userId);
  if(!access(filnamn,0)) {
    sendfile(filnamn);
  }
}

void Cmd_ChangeUser(void) {
  int userId, tmp, isCorrect, needsWrite;
  struct User *user, *userWrite;
  struct ShortUser *shortUser;
  if(argument[0]) {
    if(CURRENT_USER->status < Servermem->cfg->st.anv) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_USER_CHANGE_NO_OTHER));
      return;
    }
    if((userId = parsenamn(argument)) == -1) {
      SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
      return;
    } else if(userId == -3) {
      LogEvent(SYSTEM_LOG, ERROR,
               "-3 returned from parsenamn() in Cmd_ChangeUser(). Argument = '%s'",
               argument);
      DisplayInternalError();
      return;
    }
    SendStringCat("\r\n\n%s\r\n", CATSTR(MSG_USER_CHANGING_USER), getusername(userId));
  } else {
    userId = inloggad;
    SendString("\r\n\n%s\r\n", CATSTR(MSG_USER_CHANGING_SELF));
  }
  if(!(user = GetUserData(userId))) {
    return;
  }

  for(;;) {
    SendString("\r\n%s : (%s) ", CATSTR(MSG_USER_NAME), user->namn);
    if(GetString(40,NULL)) {
      return;
    }
    if(inmat[0] == '\0') {
      break;
    }
    if((tmp = parsenamn(inmat)) != -1 && tmp != userId) {
      SendString("\r\n\n%s\r\n", CATSTR(MSG_USER_ALREADY_EXISTS));
    } else {
      strncpy(user->namn, inmat, 41);
      break;
    }
  }

  if(CURRENT_USER->status >= Servermem->cfg->st.chgstatus) {
    if(MaybeEditNumberChar(CATSTR(MSG_USER_LEVEL), &user->status, 3, 0, 100)) { return; }
  }
  if(MaybeEditString(CATSTR(MSG_USER_STREET), user->gata, 40)) { return; }
  if(MaybeEditString(CATSTR(MSG_USER_CITY), user->postadress, 40)) { return; }
  if(MaybeEditString(CATSTR(MSG_USER_COUNTRY), user->land, 40)) { return; }
  if(MaybeEditString(CATSTR(MSG_USER_PHONE), user->telefon, 20)) { return; }
  if(MaybeEditString(CATSTR(MSG_USER_MISCINFO), user->annan_info, 60)) { return; }
  if(MaybeEditPassword(CATSTR(MSG_USER_PASSWORD), CATSTR(MSG_USER_PASSWORD_CONFIRM), user->losen, 15)) { return; }
  if(MaybeEditString(CATSTR(MSG_USER_PROMPT), user->prompt, 5)) { return; }
  if(MaybeEditNumberChar(CATSTR(MSG_USER_LINES), &user->rader, 5, 0, 127)) { return; }

  if(CURRENT_USER->status>=Servermem->cfg->st.anv) {
    if(MaybeEditNumber(CATSTR(MSG_USER_READ), (int *)&user->read, 8, 0, INT_MAX)) { return; }
    if(MaybeEditNumber(CATSTR(MSG_USER_WRITTEN), (int *)&user->skrivit, 8, 0, INT_MAX)) { return; }
    if(MaybeEditNumber(CATSTR(MSG_USER_DL_FILES), (int *)&user->download, 8, 0, INT_MAX)) { return; }
    if(MaybeEditNumber(CATSTR(MSG_USER_UL_FILES), (int *)&user->upload, 8, 0, INT_MAX)) { return; }
    if(MaybeEditNumber(CATSTR(MSG_USER_DL_BYTES), (int *)&user->downloadbytes, 8, 0, INT_MAX)) { return; }
    if(MaybeEditNumber(CATSTR(MSG_USER_UL_BYTES), (int *)&user->uploadbytes, 8, 0, INT_MAX)) { return; }
    if(MaybeEditNumber(CATSTR(MSG_USER_LOGINS), (int *)&user->loggin, 8, 0, INT_MAX)) { return; }
  }
  if(GetYesOrNo("\r\n\n", CATSTR(MSG_COMMON_IS_CORRECT), NULL, NULL,
                CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO), "\r\n\n",
                TRUE, &isCorrect)) {
    return;
  }
  if(!isCorrect) {
    return;
  }

  if((shortUser = FindShortUser(userId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "While changing user %d, can't find ShortUser entry.", userId);
    DisplayInternalError();
    return;
  }
  strncpy(shortUser->namn, user->namn, 41);
  shortUser->status = user->status;

  if(!(userWrite = GetUserDataForUpdate(userId, &needsWrite))) {
    return;
  }
  
  strncpy(userWrite->namn, user->namn, 41);
  userWrite->status = user->status;
  strncpy(userWrite->gata, user->gata, 41);
  strncpy(userWrite->postadress, user->postadress, 41);
  strncpy(userWrite->land, user->land, 41);
  strncpy(userWrite->telefon, user->telefon, 21);
  strncpy(userWrite->annan_info, user->annan_info, 61);
  strncpy(userWrite->losen, user->losen, 16);
  strncpy(userWrite->prompt, user->prompt, 6);
  userWrite->rader = user->rader;
  userWrite->read = user->read;
  userWrite->skrivit = user->skrivit;
  userWrite->download = user->download;
  userWrite->upload = user->upload;
  userWrite->uploadbytes = user->uploadbytes;
  userWrite->downloadbytes = user->downloadbytes;
  userWrite->loggin = user->loggin;

  if(needsWrite) {
    WriteUser(userId, userWrite, FALSE);
  }
}

void Cmd_DeleteUser(void) {
  struct ShortUser *shortUser;
  int userId, isCorrect;
  if(!argument[0]) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_USER_DELETE_SYNTAX));
    return;
  }
  if((userId = parsenamn(argument)) == -1) {
    SendString("\r\n\n%s\r\n\n", CATSTR(MSG_COMMON_NO_SUCH_USER));
    return;
  }
  SendStringCat("\r\n\n%s", CATSTR(MSG_USER_DELETE_CONFIRM), getusername(userId));
  if(GetYesOrNo(NULL, NULL, NULL, NULL, CATSTR(MSG_COMMON_YES), CATSTR(MSG_COMMON_NO),
                "\r\n\n", FALSE, &isCorrect)) {
    return;
  }
  if(!isCorrect) {
    return;
  }

  if((shortUser = FindShortUser(userId)) == NULL) {
    LogEvent(SYSTEM_LOG, ERROR,
             "Error finding ShortUser entry for user %d in Cmd_DeleteUser()",
             userId);
    DisplayInternalError();
    return;
  }

  Remove((struct Node *)shortUser);
  sprintf(inmat, "%d", userId);
  argument = inmat;
  sendrexx(15);
}

int Cmd_ListUsers(void) {
  struct ShortUser *listpek;
  int backwards=FALSE,status=-1,going=TRUE,more=0,less=100,cnt=2;
  char pattern[40]="",*argpek=argument,*tpek=NULL;
  puttekn("\r\n\n",-1);
  while(argpek[0] && going) {
    if(argpek[0]=='-') {
      if(argpek[1]=='b' || argpek[1]=='B') backwards=TRUE;
      else if(argpek[1]=='s' || argpek[1]=='S') {
        while(argpek[cnt] && argpek[cnt]!=' ') {
          if(argpek[cnt]=='-') { tpek=&argpek[cnt+1]; break; }
          cnt++;
        }
        if(tpek) {
          if(argpek[2]!='-') more=atoi(&argpek[2]);
          if(tpek[0] && tpek[0]!=' ') less=atoi(tpek);
        } else status=atoi(&argpek[2]);
      }
    } else {
      strncpy(pattern,argpek,39);
      going=FALSE;
    }
    argpek=hittaefter(argpek);
  }
  if(backwards) {
    for(listpek=(struct ShortUser *)Servermem->user_list.mlh_TailPred;listpek->user_node.mln_Pred;listpek=(struct ShortUser *)listpek->user_node.mln_Pred) {
      if(status!=-1 && status!=listpek->status) continue;
      if(listpek->status>less || listpek->status<more) continue;
      if(!namematch(pattern,listpek->namn)) continue;
      sprintf(outbuffer,"%s #%ld\r\n",listpek->namn,listpek->nummer);
      if(puttekn(outbuffer,-1)) break;
    }
    return(0);
  }
  for(listpek=(struct ShortUser *)Servermem->user_list.mlh_Head;listpek->user_node.mln_Succ;listpek=(struct ShortUser *)listpek->user_node.mln_Succ) {
    if(status!=-1 && status!=listpek->status) continue;
    if(listpek->status>less || listpek->status<more) continue;
    if(!namematch(pattern,listpek->namn)) continue;
    sprintf(outbuffer,"%s #%ld\r\n",listpek->namn,listpek->nummer);
    if(puttekn(outbuffer,-1)) break;
  }
  return(0);
}

void Cmd_ChangeLanguage(void) {
  AskUserForLanguage(CURRENT_USER);
}
