/******************************************************************************
 *
 *   job_submit_default_options.c
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
#include <sys/types.h>
#include <unistd.h>

#include <slurm/slurm.h>

#include "src/slurmctld/slurmctld.h"

#include "src/common/xstring.h"

const char plugin_name[]="Set default options for jobs";
const char plugin_type[]="job_submit/default_options";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
//const uint32_t min_plug_version = 100;

static s_p_options_t default_options_options[] = {
	{"cluster_features", S_P_STRING},
	{NULL}
};

char* cluster_features = NULL;

extern int init (void) {

    char *conf_file = NULL;
    struct stat config_stat;
    s_p_hashtbl_t *tbl = NULL;

    // read conf file
    conf_file = get_extra_conf_path("default_options.conf");
    if (stat(conf_file, &config_stat) < 0)
        fatal("Can't stat conf file %s: %m", conf_file);

    tbl = s_p_hashtbl_create(default_options_options);

    if (s_p_parse_file(tbl, NULL, conf_file, false) == SLURM_ERROR)
        fatal("Can't parse default_options.conf %s: %m", conf_file);

    s_p_get_string(&cluster_features, "cluster_features", tbl);

    s_p_hashtbl_destroy(tbl);
    xfree(conf_file);
    
    // validate
    // FIXME, validate cluster_features is a/are real features

    return SLURM_SUCCESS;
}

extern int fini (void) {
    xfree(cluster_features);
    cluster_features = NULL;
    return SLURM_SUCCESS;
}

extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid, char **err_msg) {
    if (cluster_features != NULL && job_desc->cluster_features == NULL) {
        info("default_options: cluster_features = %s", cluster_features);
        job_desc->cluster_features = xstrdup(cluster_features);
    }

    return SLURM_SUCCESS;
}

extern int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {
    return SLURM_SUCCESS;
}
