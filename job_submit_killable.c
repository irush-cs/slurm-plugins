/******************************************************************************
 *
 *   job_submit_killable.c
 *
 *   Copyright (C) 2018 Hebrew University of Jerusalem Israel, see
 *   LICENSE file.
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
 *****************************************************************************/

#include <sys/stat.h>
#include <grp.h>

#include <slurm/slurm.h>

#include "src/slurmctld/slurmctld.h"

#include "src/common/xstring.h"
#include "src/common/assoc_mgr.h"

const char plugin_name[]="killable";
const char plugin_type[]="job_submit/killable";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
//const uint32_t min_plug_version = 100;

s_p_options_t user_options[] = {
    {"User", S_P_STRING},
    {"Account", S_P_STRING},
    {"QOS", S_P_STRING},
    {NULL}
};

s_p_options_t primary_group_options[] = {
    {"PrimaryGroup", S_P_STRING},
    {"Account", S_P_STRING},
    {"QOS", S_P_STRING},
    {NULL}
};

static s_p_options_t killable_options[] = {
    {"User", S_P_LINE, NULL, NULL, user_options},
    {"PrimaryGroup", S_P_LINE, NULL, NULL, primary_group_options},
    {NULL}
};

int user_count = 0;
char** user_keys = NULL;
char** user_values = NULL;
char** user_qos = NULL;
int pgroup_count = 0;
char** pgroup_keys = NULL;
char** pgroup_values = NULL;
char** pgroup_qos = NULL;

extern int init (void) {

    char *conf_file = NULL;
    struct stat config_stat;
    s_p_hashtbl_t *options = NULL;
    s_p_hashtbl_t **users = NULL;
    s_p_hashtbl_t **pgroups = NULL;
    char buffer[1024];
    buffer[0] = 0;

    // read conf file
    conf_file = get_extra_conf_path("killable.conf");
    if (stat(conf_file, &config_stat) < 0)
        fatal("Can't stat conf file %s: %m", conf_file);

    options = s_p_hashtbl_create(killable_options);

    if (s_p_parse_file(options, NULL, conf_file, false) == SLURM_ERROR)
        fatal("Can't parse killable.conf %s: %m", conf_file);

    s_p_get_line(&users, &user_count, "User", options);
    s_p_get_line(&pgroups, &user_count, "PrimaryGroup", options);

    xfree(conf_file);

    user_keys = xmalloc(user_count * sizeof(char*));
    user_values = xmalloc(user_count * sizeof(char*));
    user_qos = xmalloc(user_count * sizeof(char*));
    pgroup_keys = xmalloc(user_count * sizeof(char*));
    pgroup_values = xmalloc(user_count * sizeof(char*));
    pgroup_qos = xmalloc(user_count * sizeof(char*));
    
    for (int i = 0; i < user_count; i++) {
        char* user;
        char* account;
        char* qos;
        user_keys[i] = NULL;
        user_values[i] = NULL;
        user_qos[i] = NULL;
        if (s_p_get_string(&user, "User", users[i])) {
            user_keys[i] = user;
            if (s_p_get_string(&account, "Account", users[i])) {
                user_values[i] = account;
            }
            if (s_p_get_string(&qos, "QOS", users[i])) {
                user_qos[i] = qos;
            }

            if (strlen(buffer) < sizeof(buffer) - 1) {
                if (buffer[0])
                    strcat(buffer, ",");
                strncat(buffer, user, sizeof(buffer) - strlen(buffer) - 2);
            }
        }
    }

    info("job_submit/killable: found %i killable user settings (%s)", user_count, buffer);
    buffer[0] = 0;

    for (int i = 0; i < pgroup_count; i++) {
        char* pgroup;
        char* account;
        char* qos;
        pgroup_keys[i] = NULL;
        pgroup_values[i] = NULL;
        pgroup_qos[i] = NULL;
        if (s_p_get_string(&pgroup, "PrimaryGroup", pgroups[i])) {
            pgroup_keys[i] = pgroup;
            if (s_p_get_string(&account, "Account", pgroups[i])) {
                pgroup_values[i] = account;
            }
            if (s_p_get_string(&qos, "QOS", pgroups[i])) {
                pgroup_qos[i] = qos;
            }

            if (strlen(buffer) < sizeof(buffer) - 1) {
                if (buffer[0])
                    strcat(buffer, ",");
                strncat(buffer, pgroup, sizeof(buffer) - strlen(buffer) - 2);
            }
        }
    }

    info("job_submit/killable: found %i killable primarygroup settings (%s)", pgroup_count, buffer);

    // FIXME, validate somehow?

    s_p_hashtbl_destroy(options);
    options = NULL;
    users = NULL;
    pgroups = NULL;

    return SLURM_SUCCESS;
}

extern int fini (void) {
    for (int i = 0 ; i < user_count; i++) {
        xfree(user_keys[i]);
        user_keys[i] = NULL;
        if (user_values[i]) {
            xfree(user_values[i]);
            user_values[i] = NULL;
        }
        if (user_qos[i]) {
            xfree(user_qos[i]);
            user_qos[i] = NULL;
        }
    }
    for (int i = 0 ; i < pgroup_count; i++) {
        xfree(pgroup_keys[i]);
        pgroup_keys[i] = NULL;
        if (pgroup_values[i]) {
            xfree(pgroup_values[i]);
            pgroup_values[i] = NULL;
        }
        if (pgroup_qos[i]) {
            xfree(pgroup_qos[i]);
            pgroup_qos[i] = NULL;
        }
    }
    xfree(user_keys);
    xfree(user_values);
    xfree(user_qos);
    user_count = 0;
    xfree(pgroup_keys);
    xfree(pgroup_values);
    xfree(pgroup_qos);
    pgroup_count = 0;
    return SLURM_SUCCESS;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {
    bool is_killable = false;
    slurmdb_user_rec_t user;
    bool found_account = false;
    bool found_qos = false;
    char* default_account = NULL;
    char* default_qos = NULL;

    for (int i = 0; i < job_desc->spank_job_env_size; i++) {
        if (strcmp(job_desc->spank_job_env[i], "_SLURM_SPANK_OPTION_killable_killable=(null)") == 0) {
            is_killable = true;
        }
    }
        
    if (is_killable) {
        memset(&user, 0, sizeof(slurmdb_user_rec_t));
        user.uid = job_desc->user_id;
        if (assoc_mgr_fill_in_user(acct_db_conn, &user, accounting_enforce, NULL) == SLURM_ERROR) {
            error("job_submit/killable: can't get user name");
            return SLURM_FAILURE;
        }

        // first try specific user
        for (int i = 0; i < user_count; i++) {
            if (strcmp(user_keys[i], user.name) == 0) {
                if (user_values[i]) {
                    if (job_desc->account) {
                        xfree(job_desc->account);
                    }
                    job_desc->account = xstrdup(user_values[i]);
                    found_account = true;
                }
                if (user_qos[i]) {
                    if (job_desc->qos) {
                        xfree(job_desc->qos);
                    }
                    job_desc->qos = xstrdup(user_qos[i]);
                    found_qos = true;
                }
            }
            if (!found_account && strcmp(user_keys[i], "*default") == 0) {
                if (user_values[i]) {
                    default_account = xstrdup(user_values[i]);
                }
                if (user_qos[i]) {
                    default_qos = xstrdup(user_qos[i]);
                }
            }
        }

        // then try primary group
        if (!found_account) {
            struct passwd pwd, *result = NULL;
            char buffer[4096];
            if (getpwuid_r(user.uid, &pwd, buffer, sizeof(buffer), &result) != 0 || result == NULL) {
                error("job_submit/killable: can't get user %i pw", user.uid);
                return SLURM_FAILURE;
            }
            struct group gr, *gresult = NULL;
            gid_t gid = pwd.pw_gid;
            if (getgrgid_r(gid, &gr, buffer, sizeof(buffer), &gresult) != 0 || gresult == NULL) {
                error("job_submit/killable: can't get group %i gr", gid);
                return SLURM_FAILURE;
            }
                
            for (int i = 0; i < pgroup_count; i++) {
                if (strcmp(pgroup_keys[i], gr.gr_name) == 0) {
                    if (pgroup_values[i]) {
                        if (job_desc->account) {
                            xfree(job_desc->account);
                        }
                        job_desc->account = xstrdup(pgroup_values[i]);
                        found_account = true;
                    }
                    if (pgroup_qos[i]) {
                        if (job_desc->qos) {
                            xfree(job_desc->qos);
                        }
                        job_desc->qos = xstrdup(pgroup_values[i]);
                        found_qos = true;
                    }
                }
            }
        }

        // then try the user=*default
        if (!found_account && default_account) {
            if (job_desc->account) {
                xfree(job_desc->account);
            }
            job_desc->account = xstrdup(default_account);
        }
        if (!found_qos && default_qos) {
            if (job_desc->qos) {
                xfree(job_desc->qos);
            }
            job_desc->qos = xstrdup(default_qos);
        }
        info("job_submit/killable: killable, setting account/qos: %s/%s", job_desc->account, job_desc->qos);

        if (default_account) {
            xfree(default_account);
            default_account = NULL;
        }
        if (default_qos) {
            xfree(default_qos);
            default_qos = NULL;
        }
    } else {
        info("job_submit/killable: job not killable");
    }



    return SLURM_SUCCESS;
}

extern int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {
    //return ESLURM_NOT_SUPPORTED;
    return SLURM_SUCCESS;
}
