/******************************************************************************
 *
 *   spank_lmod.c
 *
 *   Copyright (C) 2017 Hebrew University of Jerusalem Israel, see LICENSE
 *   file.
 *
 *   Author: Yair Yarom <irush@cs.huji.ac.il>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc., 59
 *   Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *
 *  Some functions are based on slurm code:src/slurmd/slurmstepd/task.c
 *
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2009 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Mark A. Grondona <mgrondona@llnl.gov>.
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <slurm/spank.h>

SPANK_PLUGIN(lmod, 1);

static int _opt_process (int val, const char *optarg, int remote);
static char* modules;

struct spank_option spank_option_array[] = {
    { "modules", "module", "lmod modules to load", 1, 0, (spank_opt_cb_f)_opt_process },
    SPANK_OPTIONS_TABLE_END
};

static int _run_script_and_set_env(const char *path);

int slurm_spank_init(spank_t spank, int ac, char **av) {
    int i, j, rc = ESPANK_SUCCESS;
    modules = 0;

    for (i = 0; spank_option_array[i].name; i++) {
        j = spank_option_register(spank, &spank_option_array[i]);
        if (j != ESPANK_SUCCESS) {
            slurm_error("Could not register Spank option %s",
                        spank_option_array[i].name);
            rc = j;
        }
    }
    return rc;
}

int slurm_spank_init_post_opt(spank_t spank, int ac, char **av) {
    int rc = ESPANK_SUCCESS;

    switch (spank_context()) {
      case S_CTX_REMOTE:
          if (modules) {
              rc = spank_setenv(spank, "SPANK_MODULES", modules, 1);
              free(modules);
              modules = NULL;
          }
          break;
      case S_CTX_LOCAL:
          if (ac) {
              return _run_script_and_set_env(av[0]);
          }
          break;
      default:
          break;
    }

    return rc;
}

static int _opt_process(int val, const char *optarg, int remote) {
    modules = strdup(optarg);
    return 0;
}

static void _proc_stdout(char *buf) {
    int end_buf = 0;
    char *buf_ptr, *name_ptr, *val_ptr;
    char *end_line, *equal_ptr;

    buf_ptr = buf;
    while (buf_ptr[0]) {
        end_line = strchr(buf_ptr, '\n');
        if (!end_line) {
            end_line = buf_ptr + strlen(buf_ptr);
            end_buf = 1;
        }
        if (!strncmp(buf_ptr, "export ",7)) {
            name_ptr = buf_ptr + 7;
            while (isspace(name_ptr[0]))
                name_ptr++;
            equal_ptr = strchr(name_ptr, '=');
            if (!equal_ptr || (equal_ptr > end_line))
                goto rwfail;
            val_ptr = equal_ptr + 1;
            while (isspace(equal_ptr[-1]))
                equal_ptr--;
            equal_ptr[0] = '\0';
            end_line[0] = '\0';
            slurm_debug("export name:%s:val:%s:", name_ptr, val_ptr);
            if (setenv(name_ptr, val_ptr, 1)) {
                slurm_error("Unable to set %s environment variable",
                            buf_ptr);
            }
            equal_ptr[0] = '=';
            if (end_buf)
                end_line[0] = '\0';
            else
                end_line[0] = '\n';
        } else if (!strncmp(buf_ptr, "unset ", 6)) {
            name_ptr = buf_ptr + 6;
            while (isspace(name_ptr[0]))
                name_ptr++;
            if ((name_ptr[0] == '\n') || (name_ptr[0] == '\0'))
                goto rwfail;
            while (isspace(end_line[-1]))
                end_line--;
            end_line[0] = '\0';
            slurm_debug(" unset name:%s:", name_ptr);
            unsetenv(name_ptr);
            if (end_buf)
                end_line[0] = '\0';
            else
                end_line[0] = '\n';
        }

    rwfail:		 /* process rest of script output */
        if (end_buf)
            break;
        buf_ptr = end_line + 1;
    }
    return;
}


/*
 * Run a task prolog script.  Also read the stdout of the script and set
 * 	environment variables in the task's environment as specified
 *	in the script's standard output.
 * name IN: class of program ("system prolog", "user prolog", etc.)
 * path IN: pathname of program to run
 * job IN/OUT: pointer to associated job, can update job->env
 *	if prolog
 * RET 0 on success, -1 on failure.
 */
static int
_run_script_and_set_env(const char *path) {
    int status, rc;
    pid_t cpid;
    int pfd[2];
    char buf[4096];
    FILE *f;

    if (path == NULL || path[0] == '\0')
        return ESPANK_SUCCESS;

    // slurm_debug("[job %u] attempting to run %s [%s]", job->jobid, name, path);

    if (access(path, R_OK | X_OK) < 0) {
        slurm_error("Could not run %s: %m", path);
        return ESPANK_ERROR;
    }
    if (pipe(pfd) < 0) {
        slurm_error("executing %s: pipe: %m", path);
        return ESPANK_ERROR;
    }
    if ((cpid = fork()) < 0) {
        slurm_error("executing %s: fork: %m", path);
        return ESPANK_ERROR;
    }
    if (cpid == 0) {
        char *argv[3];

        argv[0] = strdup(path);
        argv[1] = NULL;
        argv[2] = "purge";
        if (dup2(pfd[1], 1) == -1)
            slurm_error("couldn't do the dup: %m");
        close(2);
        close(0);
        close(pfd[0]);
        close(pfd[1]);
        setpgid(0, 0);
        execv(path, argv);
        slurm_error("execv(%s): %m", path);
        exit(127);
    }

    close(pfd[1]);
    f = fdopen(pfd[0], "r");
    if (f == NULL) {
        slurm_error("Cannot open pipe device: %m");
        return ESPANK_ERROR;
    }
    while (feof(f) == 0) {
        if (fgets(buf, sizeof(buf) - 1, f) != NULL) {
            _proc_stdout(buf);
        }
    }
    fclose(f);

    while (1) {
        rc = waitpid(cpid, &status, 0);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            slurm_error("waidpid: %m");
            return 0;
        } else  {
            killpg(cpid, SIGKILL);  /* kill children too */
            return status;
        }
    }

    /* NOTREACHED */
}
