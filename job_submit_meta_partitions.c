/******************************************************************************
 *
 *   job_submit_meta_partitions.c
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

#include <slurm/slurm.h>

#include "src/slurmctld/slurmctld.h"

#include "src/common/xstring.h"

const char plugin_name[]="Meta partitions";
const char plugin_type[]="job_submit/meta_partitions";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
//const uint32_t min_plug_version = 100;

s_p_options_t partition_options[] = {
    {"MetaPartition", S_P_STRING},
    {"Partitions", S_P_STRING},
    {NULL}
};

static s_p_options_t meta_partitions_options[] = {
    {"MetaPartition", S_P_LINE, NULL, NULL, partition_options},
    {NULL}
};

int meta_count = 0;
char** meta_keys = NULL;
char** meta_values = NULL;

extern int init (void) {

    char *conf_file = NULL;
    struct stat config_stat;
    s_p_hashtbl_t *options = NULL;
    s_p_hashtbl_t **metas = NULL;
    char buffer[1024];
    buffer[0] = 0;

    // read conf file
    conf_file = get_extra_conf_path("meta_partitions.conf");
    if (stat(conf_file, &config_stat) < 0)
        fatal("Can't stat conf file %s: %m", conf_file);

    options = s_p_hashtbl_create(meta_partitions_options);

    if (s_p_parse_file(options, NULL, conf_file, false) == SLURM_ERROR)
        fatal("Can't parse meta_partitions.conf %s: %m", conf_file);

    s_p_get_line(&metas, &meta_count, "MetaPartition", options);

    xfree(conf_file);

    meta_keys = xmalloc(meta_count * sizeof(char*));
    meta_values = xmalloc(meta_count * sizeof(char*));
    
    for (int i = 0; i < meta_count; i++) {
        char* metapartition;
        char* partitions;
        s_p_get_string(&metapartition, "MetaPartition", metas[i]);
        s_p_get_string(&partitions, "Partitions", metas[i]);
        meta_keys[i] = metapartition;
        meta_values[i] = partitions;
        if (strlen(buffer) < sizeof(buffer) - 1) {
            if (buffer[0])
                strcat(buffer, ",");
            strncat(buffer, metapartition, sizeof(buffer) - strlen(buffer) - 2);
        }
    }

    info("job_submit/meta_partitions: found %i meta partitions (%s)", meta_count, buffer);

    // FIXME, validate somehow?

    s_p_hashtbl_destroy(options);
    options = NULL;
    metas = NULL;

    return SLURM_SUCCESS;
}

extern int fini (void) {
    for (int i = 0 ; i < meta_count; i++) {
        xfree(meta_keys[i]);
        meta_keys[i] = NULL;
        xfree(meta_values[i]);
        meta_values[i] = NULL;
    }
    xfree(meta_keys);
    xfree(meta_values);
    meta_count = 0;
    return SLURM_SUCCESS;
}

static int _update_partition(struct job_descriptor *job_desc) {
    if (job_desc->partition) {
        for (int i = 0; i < meta_count; i++) {
            if (strcmp(meta_keys[i], job_desc->partition) == 0) {
                info("meta_partitions: job %i, %s -> %s", job_desc->job_id, job_desc->partition, meta_values[i]);
                xfree(job_desc->partition);
                job_desc->partition = xstrdup(meta_values[i]);
            }
        }
    }

    return SLURM_SUCCESS;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {
    return _update_partition(job_desc);
}

extern int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {
    return _update_partition(job_desc);
}
