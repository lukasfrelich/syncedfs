/**********************************************************************\
*                Copyright (C) Michael Kerrisk, 2010.                  *
*                                                                      *
* This program is free software. You may use, modify, and redistribute *
* it under the terms of the GNU Affero General Public License as       *
* published by the Free Software Foundation, either version 3 or (at   *
* your option) any later version. This program is distributed without  *
* any warranty. See the file COPYING for details.                      *
\**********************************************************************/

/* create_pid_file.h

   Header file for create_pid_file.c.
*/
#ifndef CREATE_PID_FILE_H   /* Prevent accidental double inclusion */
#define CREATE_PID_FILE_H

#define CPF_CLOEXEC 1

int createPidFile(const char *progName, const char *pidFile, int flags);
int deletePidFile(int fd, const char *pidFile);

#endif
