
#include <stdlib.h>

#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>

#include "src/slurmctld/slurmctld.h"

#include "src/common/xstring.h"

const char plugin_name[]="Limit interactive jobs";
const char plugin_type[]="job_submit/limit_interactive";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
//const uint32_t min_plug_version = 100;

extern int init (void) {
    return SLURM_SUCCESS;
}

extern int fini (void) {
    return SLURM_SUCCESS;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {
    // NOTE: no job id actually exists yet (=NO_VAL)

    // salloc: argc = 0, script = NULL
    // srun: argc > 0, script = NULL
    // sbatch: argc = 0, script != NULL

    if (job_desc->script == NULL) {
        //*err_msg = xstrdup("Interactive jobs are not allowed");
        //return SLURM_ERROR;
        //return ESLURM_JOB_SCRIPT_MISSING;
        //return ESLURM_NOT_SUPPORTED;
        //ESLURM_DISABLED,

        char* tmp_str;
        if (job_desc->licenses == NULL) {
            job_desc->licenses = xstrdup("interactive:1");
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
                    if (strcmp(token, "interactive") == 0 && num > 0) {
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
                strcat(tmp_str, ",interactive:1");
                xfree(job_desc->licenses);
                job_desc->licenses = tmp_str;
            }
        }
    }
    return SLURM_SUCCESS;
}

int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {
    //if (job_desc->time_limit == INFINITE) {
    //    info("Bad replacement time limit for %u", job_desc->job_id);
    //    return ESLURM_INVALID_TIME_LIMIT;
    //}

    return SLURM_SUCCESS;
}
