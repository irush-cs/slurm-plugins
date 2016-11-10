
#include <slurm/slurm.h>
#include <slurm/slurm_errno.h>

#include "src/slurmctld/slurmctld.h"

#include <stdio.h>
#include <time.h>

const char plugin_name[]="some job info";
const char plugin_type[]="job_submit/info";
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
    char buf[1024];
    snprintf(buf, sizeof(buf), "/net/mantis/mnt/local/tmp/slurm/%zi.out", time(NULL));
    FILE* out = fopen(buf, "w");
    //if (out == NULL)
    //    return SLURM_ERROR;

    snprintf(buf, sizeof(buf), "account: %s\n", job_desc->account);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "acctg_freq: %s\n", job_desc->acctg_freq);
    fwrite(buf, 1, strlen(buf), out);

    //snprintf(buf, sizeof(buf), "admin_comment: %s", job_desc->admin_comment);
    //fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "alloc_node: %s\n", job_desc->alloc_node);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "alloc_resp_port: %i\n", job_desc->alloc_resp_port);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "alloc_sid: %i\n", job_desc->alloc_sid);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "time_limit (min): %i\n", job_desc->time_limit);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "time_min (min): %i\n", job_desc->time_min);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "argc: %i\n", job_desc->argc);
    fwrite(buf, 1, strlen(buf), out);

    for(int i = 0; i < job_desc->argc; i++) {
        snprintf(buf, sizeof(buf), "argv[%i]: %s\n", i, job_desc->argv[i]);
        fwrite(buf, 1, strlen(buf), out);
    }

    snprintf(buf, sizeof(buf), "qos: %s\n", job_desc->qos);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "std_err: %s\n", job_desc->std_err);
    fwrite(buf, 1, strlen(buf), out);
    
    snprintf(buf, sizeof(buf), "std_in: %s\n", job_desc->std_in);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "std_out: %s\n", job_desc->std_out);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "licenses: %s\n", job_desc->licenses);
    fwrite(buf, 1, strlen(buf), out);

    snprintf(buf, sizeof(buf), "job_id_str: %s\n", job_desc->job_id_str);
    fwrite(buf, 1, strlen(buf), out);
    
    snprintf(buf, sizeof(buf), "script: %s\n", job_desc->script);
    fwrite(buf, 1, strlen(buf), out);


//	char *array_inx;	/* job array index values */
//	void *array_bitmap;	/* NOTE: Set by slurmctld */
//	time_t begin_time;	/* delay initiation until this time */
//	uint32_t bitflags;      /* bitflags */
//	char *burst_buffer;	/* burst buffer specifications */
//	uint16_t ckpt_interval;	/* periodically checkpoint this job */
//	char *ckpt_dir;	 	/* directory to store checkpoint images */
//	char *clusters;		/* cluster names used for multi-cluster jobs */
//	char *comment;		/* arbitrary comment (used by Moab scheduler) */
//	uint16_t contiguous;	/* 1 if job requires contiguous nodes,
//				 * 0 otherwise,default=0 */
//	uint16_t core_spec;	/* specialized core/thread count,
//				 * see CORE_SPEC_THREAD */
//	char *cpu_bind;		/* binding map for map/mask_cpu - This
//				 * currently does not matter to the
//				 * job allocation, setting this does
//				 * not do anything for steps. */
//	uint16_t cpu_bind_type;	/* see cpu_bind_type_t - This
//				 * currently does not matter to the
//				 * job allocation, setting this does
//				 * not do anything for steps. */
//	uint32_t cpu_freq_min;  /* Minimum cpu frequency  */
//	uint32_t cpu_freq_max;  /* Maximum cpu frequency  */
//	uint32_t cpu_freq_gov;  /* cpu frequency governor */
//	time_t deadline;	/* deadline */
//	uint32_t delay_boot;	/* delay boot for desired node state */
//	char *dependency;	/* synchronize job execution with other jobs */
//	time_t end_time;	/* time by which job must complete, used for
//				 * job update only now, possible deadline
//				 * scheduling in the future */
//	char **environment;	/* environment variables to set for job,
//				 *  name=value pairs, one per line */
//	uint32_t env_size;	/* element count in environment */
//	char *exc_nodes;	/* comma separated list of nodes excluded
//				 * from job's allocation, default NONE */
//	char *features;		/* required feature specification,
//				 * default NONE */
//	char *gres;		/* comma separated list of required generic
//				 * resources, default NONE */
//	uint32_t group_id;	/* group to assume, if run as root. */
//	uint16_t immediate;	/* 1 if allocate to run or fail immediately,
//				 * 0 if to be queued awaiting resources */
//	uint32_t job_id;	/* job ID, default set by SLURM */
//	uint16_t kill_on_node_fail; /* 1 if node failure to kill job,
//				     * 0 otherwise,default=1 */
//	uint16_t mail_type;	/* see MAIL_JOB_ definitions above */
//	char *mail_user;	/* user to receive notification */
//	char *mcs_label;	/* mcs_label if mcs plugin in use */
//	char *mem_bind;		/* binding map for map/mask_cpu */
//	uint16_t mem_bind_type;	/* see mem_bind_type_t */
//	char *name;		/* name of the job, default "" */
//	char *network;		/* network use spec */
//	uint32_t nice;		/* requested priority change,
//				 * NICE_OFFSET == no change */
//	uint32_t num_tasks;	/* number of tasks to be started,
//				 * for batch only */
//	uint8_t open_mode;	/* out/err open mode truncate or append,
//				 * see OPEN_MODE_* */
//	uint16_t other_port;	/* port to send various notification msg to */
//	uint8_t overcommit;	/* over subscribe resources, for batch only */
//	char *partition;	/* name of requested partition,
//				 * default in SLURM config */
//	uint16_t plane_size;	/* plane size when task_dist =
//				   SLURM_DIST_PLANE */
//	uint8_t power_flags;	/* power management flags,
//				 * see SLURM_POWER_FLAGS_ */
//	uint32_t priority;	/* relative priority of the job,
//				 * explicitly set only for user root,
//				 * 0 == held (don't initiate) */
//	uint32_t profile;	/* Level of acct_gather_profile {all | none} */

//	uint16_t reboot;	/* force node reboot before startup */
//	char *resp_host;	/* NOTE: Set by slurmctld */
//	char *req_nodes;	/* comma separated list of required nodes
//				 * default NONE */
//	uint16_t requeue;	/* enable or disable job requeue option */
//	char *reservation;	/* name of reservation to use */

//	uint16_t shared;	/* 2 if the job can only share nodes with other
//				 *   jobs owned by that user,
//				 * 1 if job can share nodes with other jobs,
//				 * 0 if job needs exclusive access to the node,
//				 * or NO_VAL to accept the system default.
//				 * SHARED_FORCE to eliminate user control. */
//	char **spank_job_env;	/* environment variables for job prolog/epilog
//				 * scripts as set by SPANK plugins */
//	uint32_t spank_job_env_size; /* element count in spank_env */
//	uint32_t task_dist;	/* see enum task_dist_state */
//	uint32_t user_id;	/* set only if different from current UID,
//				 * can only be explicitly set by user root */
//	uint16_t wait_all_nodes;/* 0 to start job immediately after allocation
//				 * 1 to start job after all nodes booted
//				 * or NO_VAL to use system default */
//	uint16_t warn_flags;	/* flags  related to job signals
//				 * (eg. KILL_JOB_BATCH) */
//	uint16_t warn_signal;	/* signal to send when approaching end time */
//	uint16_t warn_time;	/* time before end to send signal (seconds) */
//	char *work_dir;		/* pathname of working directory */
//
//	/* job constraints: */
//	uint16_t cpus_per_task;	/* number of processors required for
//				 * each task */
//	uint32_t min_cpus;	/* minimum number of processors required,
//				 * default=0 */
//	uint32_t max_cpus;	/* maximum number of processors required,
//				 * default=0 */
//	uint32_t min_nodes;	/* minimum number of nodes required by job,
//				 * default=0 */
//	uint32_t max_nodes;	/* maximum number of nodes usable by job,
//				 * default=0 */
//	uint16_t boards_per_node; /* boards per node required by job  */
//	uint16_t sockets_per_board;/* sockets per board required by job */
//	uint16_t sockets_per_node;/* sockets per node required by job */
//	uint16_t cores_per_socket;/* cores per socket required by job */
//	uint16_t threads_per_core;/* threads per core required by job */
//	uint16_t ntasks_per_node;/* number of tasks to invoke on each node */
//	uint16_t ntasks_per_socket;/* number of tasks to invoke on
//				    * each socket */
//	uint16_t ntasks_per_core;/* number of tasks to invoke on each core */
//	uint16_t ntasks_per_board;/* number of tasks to invoke on each board */
//	uint16_t pn_min_cpus;    /* minimum # CPUs per node, default=0 */
//	uint64_t pn_min_memory;  /* minimum real memory per node OR
//				  * real memory per CPU | MEM_PER_CPU,
//				  * default=0 (no limit) */
//	uint32_t pn_min_tmp_disk;/* minimum tmp disk per node,
//				  * default=0 */
//
///*
// * The following parameters are only meaningful on a Blue Gene
// * system at present. Some will be of value on other system. Don't remove these
// * they are needed for LCRM and others that can't talk to the opaque data type
// * select_jobinfo.
// */
//	uint16_t geometry[HIGHEST_DIMENSIONS];	/* node count in various
//						 * dimensions, e.g. X, Y, and Z */
//	uint16_t conn_type[HIGHEST_DIMENSIONS];	/* see enum connection_type */
//	uint16_t rotate;	/* permit geometry rotation if set */
//	char *blrtsimage;       /* BlrtsImage for block */
//	char *linuximage;       /* LinuxImage for block */
//	char *mloaderimage;     /* MloaderImage for block */
//	char *ramdiskimage;     /* RamDiskImage for block */
//
///* End of Blue Gene specific values */
//	uint32_t req_switch;    /* Minimum number of switches */
//	dynamic_plugin_data_t *select_jobinfo; /* opaque data type,
//					   * SLURM internal use only */
//	uint64_t *tres_req_cnt; /* used internally in the slurmctld,
//				   DON'T PACK */
//	uint32_t wait4switch;   /* Maximum time to wait for minimum switches */
//	char *wckey;            /* wckey for job */
//} job_desc_msg_t;
//

    fflush(out);
    fclose(out);

    return SLURM_SUCCESS;
}

int job_modify(struct job_descriptor *job_desc, struct job_record *job_ptr, uint32_t submit_uid) {
    //if (job_desc->time_limit == INFINITE) {
    //    info("Bad replacement time limit for %u", job_desc->job_id);
    //    return ESLURM_INVALID_TIME_LIMIT;
    //}

    return SLURM_SUCCESS;
}
