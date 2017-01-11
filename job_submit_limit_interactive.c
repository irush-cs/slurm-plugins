
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>

#include "src/slurmctld/slurmctld.h"
#include "src/slurmctld/licenses.h"

#include "src/common/xstring.h"

const char plugin_name[]="Limit interactive jobs";
const char plugin_type[]="job_submit/limit_interactive";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
//const uint32_t min_plug_version = 100;

static s_p_options_t limit_interactive_options[] = {
	{"Partition", S_P_STRING},
	{NULL}
};

char* limit_partition = NULL;

extern int init (void) {
    char *conf_file = NULL;
    struct stat config_stat;
    s_p_hashtbl_t *tbl = NULL;

    // read conf file
    conf_file = get_extra_conf_path("limit_interactive.conf");
    if (stat(conf_file, &config_stat) < 0)
        fatal("Can't stat conf file %s: %m", conf_file);

    tbl = s_p_hashtbl_create(limit_interactive_options);

    if (s_p_parse_file(tbl, NULL, conf_file, false) == SLURM_ERROR)
        fatal("Can't parse limit_interactive.conf %s: %m", conf_file);

    s_p_get_string(&limit_partition, "Partition", tbl);

    s_p_hashtbl_destroy(tbl);
    xfree(conf_file);

    // validate
    // FIXME, validate limit_interactive is a/are real partition(s)

    return SLURM_SUCCESS;
}

extern int fini (void) {
    xfree(limit_partition);
    limit_partition = NULL;
    return SLURM_SUCCESS;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {
    // NOTE: no job id actually exists yet (=NO_VAL)

    // salloc: argc = 0, script = NULL
    // srun: argc > 0, script = NULL
    // sbatch: argc = 0, script != NULL

    if (job_desc->script == NULL) {
        info("limit_interactive: no script, adding interactive license");

        // also limit number of nodes
        int nlic = 1;
        if (job_desc->max_nodes > 1) {
            nlic = job_desc->max_nodes;
        }
        if (nlic > 9)
            nlic = 9;
        else if (nlic < 1)
            nlic = 1;
        job_desc->max_nodes = nlic;
        char* licstr = xstrdup("interactive:1");
        licstr[12] = ('0' + nlic);

        if (limit_partition) {
            if (job_desc->partition != NULL) {
                xfree(job_desc->partition);
            }
            job_desc->partition = xstrdup(limit_partition);
        }

        char* tmp_str;
        if (job_desc->licenses == NULL) {
            job_desc->licenses = xstrdup(licstr);
        } else {
            bool have_license = false;
            tmp_str = xstrdup(job_desc->licenses);
            char *last;
            char *token = strtok_r(tmp_str, ",;", &last);
            while (token) {
                if (strncmp(token, "interactive", 11) == 0) {
                    uint32_t num = 1;
                    for (int i = 0; token[i]; i++) {
                        if (token[i] == ':') {
                            token[i++] = '\0';
                            char* end_num;
                            num = (uint32_t)strtol(&token[i], &end_num,10);
                        }
                    }
                    if (strcmp(token, "interactive") == 0 && num >= nlic) {
                        have_license = true;
                        break;
                    }
                }
		token = strtok_r(NULL, ",;", &last);
            }
            xfree(tmp_str);

            if (!have_license) {
                tmp_str = xmalloc(strlen(job_desc->licenses) + 1 + 13 + 1);
                strcpy(tmp_str, job_desc->licenses);
                strcat(tmp_str, ",");
                strcat(tmp_str, licstr);
                xfree(job_desc->licenses);
                job_desc->licenses = tmp_str;
            }
        }
        xfree(licstr);
    }
    return SLURM_SUCCESS;
}

extern int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {

    debug("limit_interactive: job_modify: licenses: %s -> %s", job_ptr->licenses, job_desc->licenses);

    // get current licenses
    bool have_interactive = false;
    if (job_ptr->license_list) {
        have_interactive = license_get_total_cnt_from_list(job_ptr->license_list, "interactive") > 0;
    }
    if (!have_interactive) {
        debug("limit_interactive: job_modify: not interactive, ignoring");
        return SLURM_SUCCESS;
    }

    // for now, can't change anything on interactive jobs..
    return ESLURM_NOT_SUPPORTED;

    // for now, can't change max_nodes
    if (job_desc->max_nodes != NO_VAL) {
        return ESLURM_NOT_SUPPORTED;
    }

    // add interactive to wanted licenses, if NULL, no change is needed
    bool need_interactive = true;
    if (job_desc->licenses) {
        char *tmp_str = xstrdup(job_desc->licenses);
        char *last;
        char *token = strtok_r(tmp_str, ",;", &last);
        while (token) {
            if (strncmp(token, "interactive", 11) == 0) {
                uint32_t num = 1;
                for (int i = 0; token[i]; i++) {
                    if (token[i] == ':') {
                        token[i++] = '\0';
                        char* end_num;
                        num = (uint32_t)strtol(&token[i], &end_num,10);
                    }
                }
                if (strcmp(token, "interactive") == 0 && num > 0) {
                    need_interactive = false;
                    break;
                }
            }
            token = strtok_r(NULL, ",;", &last);
        }
        xfree(tmp_str);

        if (need_interactive) {
            // for now, just fail
            return ESLURM_NOT_SUPPORTED;

            tmp_str = xmalloc(strlen(job_desc->licenses) + 1 + 13 + 1);
            strcpy(tmp_str, job_desc->licenses);
            strcat(tmp_str, ",interactive:1");
            info("limit_interactive: job_modify: licenses: %s -> (%s -> %s)", job_ptr->licenses, job_desc->licenses, tmp_str);
            xfree(job_desc->licenses);
            job_desc->licenses = tmp_str;
        }
    }

    return SLURM_SUCCESS;
}
