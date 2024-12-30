/******************************************************************************
 *
 *   job_submit_gres_groups.c
 *
 *   Copyright (C) 2024 Hebrew University of Jerusalem Israel, see LICENSE
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
 *****************************************************************************/

#define _GNU_SOURCE
#include <string.h>

#include <errno.h>
#include <sys/stat.h>

#include <slurm/slurm.h>

#include "src/slurmctld/slurmctld.h"
#include "src/common/xstring.h"

const char plugin_name[]="gres_groups";
const char plugin_type[]="job_submit/gres_groups";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;

s_p_options_t group_options[] = {
    {"Gres", S_P_STRING},
    {"Group", S_P_STRING},
    {NULL}
};

static s_p_options_t gres_groups_options[] = {
    {"Gres", S_P_LINE, NULL, NULL, group_options},
    {NULL}
};

typedef struct gg_tres {
    long count;
    char* tres;
} gg_tres_t;

// per gres line, gres is e.g. "gpu:a10"
int gres_count = 0;
char** gres_keys = NULL;
int* gres_groups = NULL;
int* gres_names = NULL;

// group, group is e.g. "gg:g3"
int group_count = 0;
char** group_keys = NULL;
int* group_names = NULL;

// name of the gres without type, e.g. "gpu"
int name_count = 0;
char** name_keys = NULL;

extern int init (void) {

    char *conf_file = NULL;
    struct stat config_stat;
    s_p_hashtbl_t *options = NULL;
    s_p_hashtbl_t **greses = NULL;

    // read conf file
    conf_file = get_extra_conf_path("gres_groups.conf");
    if (stat(conf_file, &config_stat) < 0)
        fatal("job_submit/gres_groups: Can't stat conf file %s: %m", conf_file);

    options = s_p_hashtbl_create(gres_groups_options);

    if (s_p_parse_file(options, NULL, conf_file, false) == SLURM_ERROR)
        fatal("job_submit/gres_groups: Can't parse gres_groups.conf %s: %m", conf_file);

    s_p_get_line(&greses, &gres_count, "GRES", options);

    xfree(conf_file);

    gres_keys = xmalloc(gres_count * sizeof(char*));
    gres_groups = xmalloc(gres_count * sizeof(int));
    gres_names = xmalloc(gres_count * sizeof(int));

    group_keys = xmalloc(gres_count * sizeof(char*));
    group_names = xmalloc(gres_count * sizeof(int));

    // set to -1, as 0 is a valid value
    for (int g = 0; g < gres_count; g++) {
        group_names[g] = -1;
    }

    name_keys = xmalloc(gres_count * sizeof(char*));

    // go over gres lines
    for (int i = 0; i < gres_count; i++) {
        char* gres;
        char* group;
        if (s_p_get_string(&gres, "Gres", greses[i])) {
            gres_keys[i] = gres;
            if (s_p_get_string(&group, "Group", greses[i])) {

                // find group or "create" it
                int found = -1;
                for (int g = 0; g < group_count; g++) {
                    if (strcmp(group, group_keys[g]) == 0) {
                        found = g;
                        break;
                    }
                }
                if (found == -1) {
                    group_keys[group_count] = group;
                    found = group_count++;
                } else {
                    xfree(group);
                }
                gres_groups[i] = found;

                // find name or "create" it
                found = -1;
                char* name = xstrdup(gres);
                *strchrnul(name, ':') = 0;
                for (int n = 0; n < name_count; n++) {
                    if (strcmp(name, name_keys[n]) == 0) {
                        found = n;
                        break;
                    }
                }
                if (found == -1) {
                    name_keys[name_count] = name;
                    found = name_count++;
                } else {
                    xfree(name);
                }
                gres_names[i] = found;

                // verify only one name per group
                if (group_names[gres_groups[i]] == -1) {
                    group_names[gres_groups[i]] = gres_names[i];
                } else if (group_names[gres_groups[i]] != gres_names[i]) {
                    fatal("job_submit/gres_groups: Gres group must have the same gres: %s has %s and %s",
                          group_keys[gres_groups[i]],
                          name_keys[group_names[gres_groups[i]]],
                          name_keys[gres_names[i]]
                          );
                }

            } else {
                fatal("job_submit/gres_groups: Gres \"%s\" without a group", gres);
            }
        }
    }

    info("job_submit/gres_groups: found %i GRES in %i groups", gres_count, group_count);
    if (get_log_level() >= LOG_LEVEL_DEBUG) {
        for (int i = 0; i < gres_count; i++) {
            debug("job_submit/gres_groups: gres: %s, name: %s, group: %s", gres_keys[i], name_keys[gres_names[i]], group_keys[gres_groups[i]]);
        }
    }

    s_p_hashtbl_destroy(options);
    options = NULL;
    greses = NULL;

    return SLURM_SUCCESS;
}

extern int fini (void) {
    for (int i = 0 ; i < gres_count; i++) {
        xfree(gres_keys[i]);
        gres_keys[i] = NULL;
    }
    xfree(gres_keys);
    gres_keys = NULL;
    xfree(gres_groups);
    gres_groups = NULL;
    xfree(gres_names);
    gres_names = NULL;
    gres_count = 0;

    for (int i = 0; i < group_count; i++) {
        xfree(group_keys[i]);
        group_keys[i] = NULL;
    }
    xfree(group_keys);
    group_keys = NULL;
    group_count = 0;
    xfree(group_names);
    group_names = 0;

    for (int i = 0; i < name_count; i++) {
        xfree(name_keys[i]);
        name_keys[i] = NULL;
    }
    xfree(name_keys);
    name_keys = NULL;
    name_count = 0;

    return SLURM_SUCCESS;
}

/*
  if "token" is exactly the tres "name" with count, returns the count
  returns -1 on error
  returns 0 otherwise
*/
inline static long _is_tres_token(char* token, char* name) {
    if (
        // exactly <name>
        strcmp(token, name) == 0

        // or <name>:
        || (
            strncmp(token, name, strlen(name)) == 0 &&
            (strlen(token) == strlen(name) + 1) &&
            token[strlen(name)] == ':'
            )
        ) {
        return 1;

    } else if (
               // or <name>:<digit>
               strncmp(token, name, strlen(name)) == 0 &&
               (strlen(token) > strlen(name) + 1) &&
               token[strlen(name) + 1] >= '0' &&
               token[strlen(name) + 1] <= '9'
               ) {
        errno = 0;
        long res = strtol(token + strlen(name) + 1, NULL, 10);
        if (errno != 0) {
            return -1;
        }
        return res;
    }
    return 0;
}

static void _parse_tres(const char* tres, gg_tres_t*** out_result, int* out_count) {
    char *tmp_str = xstrdup(tres);
    size_t rsize = 5;
    gg_tres_t** result = xmalloc(sizeof(gg_tres_t*) * rsize);
    int count = 0;

    char *last;
    char *token = strtok_r(tmp_str, ",", &last);
    while (token) {
        gg_tres_t* tres = xmalloc(sizeof(gg_tres_t));
        char* rcolon = strrchr(token, ':');

        // has at least one colon
        if (rcolon) {
            // last is number
            if (rcolon[1] >= '0' && rcolon[1] <= '9') {
                // FIXME, check strtol errors
                tres->count = strtol(rcolon + 1, NULL, 10);
                tres->tres = xstrdup(token);
                tres->tres[rcolon - token] = 0;

                // last is not number
            } else {
                tres->count = 1;
                tres->tres = xstrdup(token);
            }

            // no colon
        } else {
            tres->tres = xstrdup(token);
            tres->count = 1;
        }

        if (count == rsize) {
            rsize *= 2;
            result = xrealloc(result, sizeof(gg_tres_t*) * rsize);
        }

        result[count++] = tres;

        token = strtok_r(NULL, ",", &last);
    }

    xfree(tmp_str);
    *out_result = result;
    *out_count = count;
}

static void _free_tres(gg_tres_t*** tres, int count) {
    for (int t = 0; t < count; t++) {
        xfree((*tres)[t]->tres);
        xfree((*tres)[t]);
        (*tres)[t] = NULL;
    }
    xfree(*tres);
    *tres = NULL;
}

/*
  update *tres to have the tres in *keys with counts in *count
  return true on success
*/
inline static bool _set_tres(char** tres, int n_keys, char** keys, long* count) {
    char *tmp_str = xstrdup(*tres);
    char *last;
    char *token = strtok_r(tmp_str, ",", &last);
    char result[1024];
    char buffer[1024];
    result[0] = result[sizeof(result) - 1] = 0;
    buffer[0] = buffer[sizeof(buffer) - 1] = 0;
    bool *done = xcalloc(n_keys, sizeof(bool));

    // go over *tres, add token to result if token not in keys, add token from
    // keys otherwise
    while (token) {
        // per tres, compare to keys
        for (int k = 0; k < n_keys; k++) {
            long current = 0;
            if (count[k] && (current = _is_tres_token(token, keys[k]))) {
                // try to avoid changing gres:type to gres:type:1
                done[k] = true;
                if (current != count[k]) {
                    snprintf(buffer, sizeof(buffer) - 1, "%s:%li,", keys[k], count[k]);
                    strncat(result, buffer, sizeof(result) - 1);
                } else {
                    strncat(result, token, sizeof(result) - 1);
                    strncat(result, ",", sizeof(result) - 1);
                }
            } else {
                strncat(result, token, sizeof(result) - 1);
                strncat(result, ",", sizeof(result) - 1);
            }
        }

        token = strtok_r(NULL, ",", &last);
    }

    // add all remaining keys
    for (int k = 0; k < n_keys; k++) {
        if (count[k] && !done[k]) {
            snprintf(buffer, sizeof(buffer) - 1, "%s:%li,", keys[k], count[k]);
            strncat(result, buffer, sizeof(result) - 1);
        }
    }

    xfree(done);
    xfree(tmp_str);

    // too many treses
    if (strlen(result) >= sizeof(result) - 1) {
        return false;
    }

    if (result[strlen(result) - 1] == ',') {
        result[strlen(result) - 1] = 0;
    }

    if (strcmp(result, *tres) != 0) {
        debug("job_submit/gres_groups: updating gres \"%s\" -> \"%s\"", *tres, result);
        xfree(*tres);
        *tres = xstrdup(result);
    }

    return true;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {
    char** tres_pers[4];
    char* tres_pers_names[4];
    char buffer[1024];
    buffer[0] = 0;
    buffer[sizeof(buffer) - 1] = 0;

    tres_pers[0] = &job_desc->tres_per_job;
    tres_pers[1] = &job_desc->tres_per_node;
    tres_pers[2] = &job_desc->tres_per_task;
    tres_pers[3] = &job_desc->tres_per_socket;
    tres_pers_names[0] = "tres_per_job";
    tres_pers_names[1] = "tres_per_node";
    tres_pers_names[2] = "tres_per_task";
    tres_pers_names[3] = "tres_per_socket";

    for (int i = 0; i < 4; i++) {
        char** tres_per = tres_pers[i];

        if (*tres_per == NULL) {
            debug2("job_submit/gres_groups: %s: %s", tres_pers_names[i], *tres_per ? *tres_per : "NULL");
            continue;
        }
        debug("job_submit/gres_groups: %s: %s", tres_pers_names[i], *tres_per);

        gg_tres_t** tres;
        int tres_count;
        _parse_tres(*tres_per, &tres, &tres_count);

        // don't allow to repeat tres (slurm takes the last, unless it's 0), we
        // just bail out
        for (int t = 0; t < tres_count; t++) {
            for (int t2 = t + 1; t2 < tres_count; t2++) {
                if (strcmp(tres[t]->tres, tres[t2]->tres) == 0) {
                    snprintf(buffer, sizeof(buffer) - 1, "GRES %s appears more than once", tres[t]->tres);
                    info("job_submit/gres_groups: %s", buffer);
                    *err_msg = xstrdup(buffer);
                    _free_tres(&tres, tres_count);
                    return ESLURM_DUPLICATE_GRES;
                }
            }
        }

        // don't allow direct name if is grouped
        // e.g. gpu:2 -> fail
        for (int g = 0; g < gres_count; g++) {
            for (int t = 0; t < tres_count; t++) {
                if (strcmp(name_keys[gres_names[g]], tres[t]->tres) == 0) {
                    snprintf(buffer, sizeof(buffer) - 1, "Can't have un-typed %s (either specify type, e.g. %s, or group e.g. %s)",
                             name_keys[gres_names[g]],
                             gres_keys[g],
                             group_keys[gres_groups[g]]
                             );
                    info("job_submit/gres_groups: %s", buffer);
                    *err_msg = xstrdup(buffer);
                    _free_tres(&tres, tres_count);
                    return ESLURM_INVALID_GRES;
                }
            }
        }

        // can't have both name and its group specified
        // e.g. can't have both gpu:a10 and gg:g3
        for (int g = 0; g < gres_count; g++) {
            bool have_gres = false;
            bool have_group = false;
            for (int t = 0; t < tres_count; t++) {
                if (strcmp(tres[t]->tres, gres_keys[g]) == 0) {
                    have_gres = true;
                }
                if (strcmp(tres[t]->tres, group_keys[gres_groups[g]]) == 0) {
                    have_group = true;
                }
            }
            if (have_gres && have_group) {
                snprintf(buffer, sizeof(buffer) - 1, "Can't have both %s and %s", gres_keys[g], group_keys[gres_groups[g]]);
                info("job_submit/gres_groups: %s", buffer);
                *err_msg = xstrdup(buffer);
                _free_tres(&tres, tres_count);
                return ESLURM_INVALID_GRES;
            }
        }

        // count explicit groups
        long* groups = xcalloc(group_count, sizeof(long));
        for (int gr = 0; gr < group_count; gr++) {
            for (int t = 0; t < tres_count; t++) {
                if (strcmp(tres[t]->tres, group_keys[gr]) == 0) {
                    groups[gr] = tres[t]->count;
                }
            }
            debug2("job_submit/gres_groups: %s: %s: %li", tres_pers_names[i], group_keys[gr], groups[gr]);
        }

        // add group counter for explicit name
        // e.g. gpu:a10:n -> gg:g3 += n
        for (int g = 0; g < gres_count; g++) {
            for (int t = 0; t < tres_count; t++) {
                if (strcmp(tres[t]->tres, gres_keys[g]) == 0) {
                    groups[gres_groups[g]] += tres[t]->count;
                    debug2("job_submit/gres_groups: %s: %s: %s: %li -> %li",
                           tres_pers_names[i],
                           gres_keys[g],
                           group_keys[gres_groups[g]],
                           groups[gres_groups[g]] - tres[t]->count,
                           groups[gres_groups[g]]);
                }
            }
        }

        // add name so slurm will actually ask for the resources
        // e.g. gg:g3:n -> gpu += n
        long* names = xcalloc(name_count, sizeof(long));
        for (int gr = 0; gr < group_count; gr++) {
            if (groups[gr]) {
                names[group_names[gr]] += groups[gr];
                debug2("job_submit/gres_groups: %s: %s: %s: %li -> %li",
                       tres_pers_names[i],
                       group_keys[gr],
                       name_keys[group_names[gr]],
                       names[group_names[gr]] - groups[gr],
                       names[group_names[gr]]);
            }
        }

        // update tres
        bool success = _set_tres(tres_per, group_count, group_keys, groups);
        if (success) {
            success = _set_tres(tres_per, name_count, name_keys, names);
        }

        _free_tres(&tres, tres_count);
        xfree(names);
        xfree(groups);

        if (!success) {
            *err_msg = xstrdup("Can't set gres properly");
            return ESLURM_UNSUPPORTED_GRES;
        }
    }

    return SLURM_SUCCESS;
}

int job_modify(struct job_descriptor *job_desc, job_record_t *job_ptr, uint32_t modify_uid) {
    // For now, disallow all
    return SLURM_ERROR;
}
