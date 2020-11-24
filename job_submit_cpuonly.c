/******************************************************************************
 *
 *   job_submit_cpuonly.c
 *
 *   Copyright (C) 2020 Hebrew University of Jerusalem Israel, see
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

#include <slurm/slurm.h>

#include "src/slurmctld/slurmctld.h"
#include "src/common/xmalloc.h"
#include "src/common/xstring.h"

const char plugin_name[]="cpuonly";
const char plugin_type[]="job_submit/cpuonly";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
//const uint32_t min_plug_version = 100;

extern int init (void) {
    return SLURM_SUCCESS;
}

extern int fini (void) {
    return SLURM_SUCCESS;
}

inline static bool _has_gpu_gres(const char* gres) {
    if (gres) {
        char *tmp_str;
        tmp_str = xstrdup(gres);
        char *last;
        char *token = strtok_r(tmp_str, ",", &last);
        while (token) {
            if (strcmp(token, "gpu") == 0 || strncmp(token, "gpu:", 4) == 0) {
                xfree(tmp_str);
                return true;
            }
            token = strtok_r(NULL, ",", &last);
        }
        xfree(tmp_str);
    }
    return false;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {

    bool wants_gpu = false;
    bool wants_cpuonly = false;

    // check if wants gpu
    wants_gpu = _has_gpu_gres(job_desc->tres_per_node);
    if (!wants_gpu) wants_gpu = _has_gpu_gres(job_desc->tres_per_job);
    if (!wants_gpu) wants_gpu = _has_gpu_gres(job_desc->tres_per_task);
    if (!wants_gpu) wants_gpu = _has_gpu_gres(job_desc->tres_per_socket);

    debug("job_submit/cpuonly: %s gpu", (wants_gpu ? "wants" : "doesn't want"));

    if (wants_gpu) {
        return SLURM_SUCCESS;
    }

    if (job_desc->features) {
        // I'm not going to check the entire syntax with ORs, ANDs, and
        // parenthesis...
        if (strcmp(job_desc->features, "cpuonly") == 0) {
            wants_cpuonly = true;
        }
    }

    debug("job_submit/cpuonly: %s cpuonly", (wants_cpuonly ? "wants" : "probably didn't want"));

    if (!wants_cpuonly) {
        info("job_submit/cpuonly: adding cpuonly");
        if (job_desc->features) {
            char* tmp_str = xmalloc(strlen(job_desc->features) + strlen("cpuonly&()") + 1);
            strcpy(tmp_str, "cpuonly&(");
            strcat(tmp_str, job_desc->features);
            strcat(tmp_str, ")");
            xfree(job_desc->features);
            job_desc->features = tmp_str;
        } else {
            job_desc->features = xstrdup("cpuonly");
        }
    }

    return SLURM_SUCCESS;
}

int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {
    // For now, allow to bypass by updating the job
    return SLURM_SUCCESS;
}
