/* $Id$
 * (c) 2000-2001 IC&S, The Netherlands
 *
 * imapd.c
 * 
 * main prg for imap daemon
 */

#include <stdio.h>
#include "imap4.h"
#include "serverservice.h"
#include "debug.h"
#include "misc.h"
#include "dbmysql.h"

#define PNAME "dbmail/imap4"


int main()
{
  int sock,d;
  char *newuser,*newgroup,*port,*bindip,*defchld,*maxchld,*daem;

  /* open logs */
  openlog(PNAME, LOG_PID, LOG_MAIL);

  /* open db connection */
  if (db_connect() != 0)
    trace(TRACE_FATAL, "IMAPD: cannot connect to dbase\n");

  /* read options from config */
  port   = db_get_config_item("IMAPD_BIND_PORT",CONFIG_MANDATORY);
  bindip = db_get_config_item("IMAPD_BIND_IP",CONFIG_MANDATORY);
  defchld = db_get_config_item("IMAPD_DEFAULT_CHILD",CONFIG_MANDATORY);
  maxchld = db_get_config_item("IMAPD_MAX_CHILD",CONFIG_MANDATORY);
  daem = db_get_config_item("IMAPD_DAEMONIZES", CONFIG_EMPTY);

  if (!port || !bindip)
    trace(TRACE_FATAL, "IMAPD: port and/or ip not specified in configuration file!\r\n");

  if (!defchld || !maxchld)
    trace(TRACE_FATAL, "IMAPD: default/maximum number of children not "
	  "specified in configuration file!\r\n");

  if (daem)
    {
      d = (strcmp(daem, "yes") == 0);
      free(daem);
      daem = NULL;
    }
  else
    d = 0;

  /* open socket */
  sock = SS_MakeServerSock(bindip, port);

  free(port);
  free(bindip);
  port = NULL;
  bindip = NULL;

  if (sock == -1)
    {
      db_disconnect();
      trace(TRACE_FATAL, "IMAPD: Error creating serversocket: %s\n", SS_GetErrorMsg());
      return 1;
    }
  
  /* drop priviledges */
  newuser = db_get_config_item("IMAPD_EFFECTIVE_USER",CONFIG_MANDATORY);
  newgroup = db_get_config_item("IMAPD_EFFECTIVE_GROUP",CONFIG_MANDATORY);

  if ((newuser!=NULL) && (newgroup!=NULL))
  {
    if (drop_priviledges (newuser, newgroup) != 0)
      trace(TRACE_FATAL,"IMAPD: could not set uid %s, gid %s\n",newuser,newgroup);
    
    free(newuser);
    free(newgroup);
    newuser = NULL;
    newgroup = NULL;
  }
  else
    {
      db_disconnect();
      trace(TRACE_FATAL,"IMAPD: newuser and newgroup should not be NULL\n");
    }

  db_disconnect();

  /* get started */
  trace(TRACE_MESSAGE, "IMAPD: server ready to run\n");
  if (SS_WaitAndProcess(sock, atoi(defchld), atoi(maxchld), d, imap_process, imap_login) == -1)
    {
      trace(TRACE_FATAL,"IMAPD: Fatal error while processing clients: %s\n",SS_GetErrorMsg());
      return 1;
    }

  SS_CloseServer(sock);

  free(defchld);
  free(maxchld);
  defchld = NULL;
  maxchld = NULL;

  return 0; 
}

