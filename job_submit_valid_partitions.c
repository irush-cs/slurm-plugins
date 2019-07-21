/*****************************************************************************\
 *  job_submit_valid_partitions.c - Set default partition in job submit request
 *  to all valid partitions in a cluster.
 *****************************************************************************
 *  This file is based on job_submit_all_partitions.c from SLURM:
 *  http://slurm.schedmd.com.
 *
 *  Updated by Yair Yarom <irush@cs.huji.ac.il>
 *
 ******************************************************************************
 *  Copyright (C) 2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris Jette <jette1@llnl.gov>
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "slurm/slurm_errno.h"
#include "src/common/slurm_xlator.h"
#include "src/slurmctld/locks.h"
#include "src/slurmctld/slurmctld.h"
#include "src/common/assoc_mgr.h"

/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting the type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  SLURM uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "auth" for SLURM authentication) and <method> is a
 * description of how this plugin satisfies that application.  SLURM will
 * only load authentication plugins if the plugin_type string has a prefix
 * of "auth/".
 *
 * plugin_version - an unsigned 32-bit integer containing the Slurm version
 * (major.minor.micro combined into a single number).
 */
const char plugin_name[]       	= "Job submit valid_partitions plugin";
const char plugin_type[]       	= "job_submit/valid_partitions";
const uint32_t plugin_version   = SLURM_VERSION_NUMBER;

/* Set a job's default partition to all partitions in the cluster */
extern int job_submit(struct job_descriptor *job_desc, uint32_t submit_uid,
                      char **err_msg)
{
	/* Locks: Read partition */
	ListIterator part_iterator;
	struct part_record *part_ptr;
	const char* account = NULL;
	slurmdb_user_rec_t user;
	int i;

	if (job_desc->partition)	/* job already specified partition */
		return SLURM_SUCCESS;

	/* Get account or default account */
	if (job_desc->account) {
		account = job_desc->account;
	} else {
		memset(&user, 0, sizeof(slurmdb_user_rec_t));
		user.uid = job_desc->user_id;
#if SLURM_VERSION_NUMBER < SLURM_VERSION_NUM(19,5,0)
		if (assoc_mgr_fill_in_user(acct_db_conn, &user, accounting_enforce, NULL) != SLURM_ERROR) {
#else
                if (assoc_mgr_fill_in_user(acct_db_conn, &user, accounting_enforce, NULL, false) != SLURM_ERROR) {
#endif
			account = user.default_acct;
		}
	}

	part_iterator = list_iterator_create(part_list);
	while ((part_ptr = (struct part_record *) list_next(part_iterator))) {
		if (!(part_ptr->state_up & PARTITION_SUBMIT))
			continue;	/* nobody can submit jobs here */

		/* Check if in AllowAccounts */
		if (account && part_ptr->allow_accounts) {
			for (i = 0; part_ptr->allow_account_array[i]; i++) {
				if (xstrcmp(account, part_ptr->allow_account_array[i]) == 0)
					break;
			}
			if (!part_ptr->allow_account_array[i])
				continue;
		}

		/* Check if not in DenyAccounts */
		if (account && part_ptr->deny_accounts) {
			for (i = 0; part_ptr->deny_account_array[i]; i++) {
				if (xstrcmp(account, part_ptr->deny_account_array[i]) == 0)
					break;
			}
			if (part_ptr->deny_account_array[i])
				continue;
		}

		/* Check time_limit doesn't exceeds MaxTime */
		if (part_ptr->max_time != INFINITE && job_desc->time_limit != NO_VAL) {
			if (job_desc->time_limit == INFINITE || job_desc->time_limit > part_ptr->max_time)
				continue;
		}

		if (job_desc->partition)
			xstrcat(job_desc->partition, ",");
		xstrcat(job_desc->partition, part_ptr->name);
	}
	list_iterator_destroy(part_iterator);
	//info("Set partition of submitted job to %s", job_desc->partition);

    /*
     * If job_desc->partition is empty, than even the default partition is not
     * good enough. As such better tell the user his job won't run
     */
	return job_desc->partition ? SLURM_SUCCESS : ESLURM_PARTITION_NOT_AVAIL;
}

extern int job_modify(struct job_descriptor *job_desc,
                      struct job_record *job_ptr, uint32_t submit_uid)
{
	return SLURM_SUCCESS;
}
