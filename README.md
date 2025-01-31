# SLURM plugins

A collection of SLURM plugins used at the Hebrew University of Jerusalem,
School of Computer Science and Engineering.

# Table of Contents

* [Compilation](#compilation)
* [job_submit_limit_interactive](#job_submit_limit_interactive)
* [job_submit_info](#job_submit_info)
* [proepilogs/TaskProlog-lmod.sh](#proepilogstaskprolog-lmodsh)
* [spank_lmod](#spank_lmod)
* [job_submit_default_options](#job_submit_default_options)
* [job_submit_valid_partitions](#job_submit_valid_partitions)
* [job_submit_meta_partitions](#job_submit_meta_partitions)
* [job_submit_killable](#job_submit_killable)
* [spank_killable](#spank_killable)
* [job_submit_cpuonly](#job_submit_cpuonly)
* [job_submit_gres_groups](#job_submit_gres_groups)

# Compilation

Some plugins here are written in `c` and needs to be compiled. For that, basic
compilation utilities needs to already be installed (compiler, make,
etc.). Slurm development files (e.g. slurm-dev, slurm-devel, slurmdb-dev, etc.)
should also be installed. Also, the plugins uses some slurm header files which
are not necessarily available through slurm*-dev packages, so the slurm sources
should be available as well.

For the compilation to work the `C_INCLUDE_PATH` (or `CPATH`) environment
variable should be set with the proper directories. E.g. if the slurm sources
are in `/slurm/source/` directory then:
```
export C_INCLUDE_PATH="/slurm/source"
```
If slurm is built manually (i.e. no slurm-dev distribution package is used), and
the build is in e.g. `/slurm/build`, then:
```
export C_INCLUDE_PATH="/slurm/source:/slurm/build"
```

With the `C_INCLUDE_PATH` set, run `make` to compile the plugins:
```
make
```

The compiled plugins (*.so files) will reside in a
`build.<os>-<arch>-<version>` directory.  To install, they need to be copied to
the slurm plugin directory. The slurm plugin directory can be obtained by
running:
```
scontrol show config | grep PluginDir
```

# job\_submit\_limit\_interactive

Limits interactive jobs. Interactive jobs are jobs that run outside `sbatch`,
i.e. with `srun` or `salloc` directly. `srun` inside `sbatch` are not
considered interactive.

This plugin uses licenses. When an interactive job starts, it adds
`interactive` licenses per the number of nodes the job will run
on (up to 9). Modification of an interactive job is disabled.

The accounting system should be enabled and track interactive licenses.
`slurm.conf` should contain e.g.

```
Licenses=interactive:1000
AccountingStorageTres=license/interactive
JobSubmitPlugins=job_submit/limit_interactive
```

Each association should have the number of allowed interactive jobs, e.g. to
give user `userA` using account `accountA` on cluster `clusterA` 2 interactive
jobs, one might run:
> sacctmgr update user userA account=accountA cluster=clusterA set grptres=license/interactive=2

The `limit_interactive.conf` configuration file can be used to configure the
plugin. Available options are `Partition` and `DefaultLimit`.
* Partition - if set, forces this partition for all interactive jobs. This
  allows adding additional constraints on interactive jobs.
* DefaultLimit - currently not used. But useful for automating the creation of
  new users/associations with default limits. 

# job\_submit\_info

Used for developments. Writes parts of the job\_descriptor data to a file in
`/tmp`.

# proepilogs/TaskProlog-lmod.sh

purge all lmod modules and loads the defaults (from /etc/lmod/lmodrc and
~/.lmodrc).

This only updates the environment variable, so aliases and other goodies aren't
passed.

Useful to avoid unwanted modules/PATHs to be loaded on the submission node and
passed to the execution nodes.

# spank\_lmod

This plugins is the second half of TaskProlog-lmod.sh. It makes sure that all
the modules were purged before distributing to the nodes. This is required as
some environment variables will be retained if the module doesn't exists in the
compute node (so the purge won't unset them).

The plugin also adds the --module option so that modules can be loaded through
srun/sbatch directly without needing additional bash script wrapper.

The plugin uses the TaskProlog-lmod.sh script, so it needs it as a parameter in
the plugstack.conf file. e.g.

> optional spank_lmod.so /etc/slurm/TaskProlog/TaskProlog-lmod.sh

# job\_submit\_default\_options

Sets some default options to jobs.

The `default_options.conf` configuration file is used to set the default
values. Currently only "cluster_features" is supported (for the
--cluster-constraint option).

# job\_submit\_valid\_partitions

Based on SLURM's `job_submit/all_partitions` plugin. Makes additional checks
before adding all partitions. Checks AllowAccounts, DenyAccounts and
MaxTime. This is to avoid unintended Reasons such as AccountNotAllowed or
PartitionTimeLimit.

By default, if a partition is already set for the job, the plugin does
nothing. If a `valid_partitions.conf` file exists and contains:
```
Force=yes
```
then the plugin will also filter out unwanted partition from the ones the user
specified. This is relevant when the `job_submit/limit_interactive` plugin
specifies several partitions which are not accessible to all.

# job\_submit\_meta\_partitions

Create meta partitions which are replaced on submit. This is useful if there
are several partitions with things in common, so users won't have to list all
of them explicitly.

The `meta_partitions.conf` configuration file should be used to configure the
meta partitions. Each line has two keys:
* MetaPartition - The name of the partition as should be specified by the user
* Partitions - The partitions to replace with

For example:
```
MetaPartition=short Partitions=short-low,short-high
MetaPartition=long Partitions=long-low,long-high
```

Can be used so that users will specify `-p short` instead of `-p
short-low,short-high`.

# job\_submit\_killable

This plugin is used to set some sort of auto-account/qos/partition when a
certain flag is passed. This is used for a special preemptable accounts, which
is why it's named "killable".

If the --killable flag was set (using the `spank_killable` plugin), this plugin
will set the account, qos and partition according to the `killable.conf`
configuration file. The configuration can be either per user, or per primary
group. For example:
```
User=user1 Account=killable-1
User=user2 QOS=qos-1 Partition=partition-1
PrimaryGroup=group2 Account=killable-2 QOS=qos-2
User=*default Account=killable-3 QOS=qos-1 Partition=partition-2
```
The `*default` user is special and will set the account, qos or partition
unless they were set by other explicit line.

# spank\_killable

This plugin does nothing more than to add the `--killable` flag which the
`job_submit_killable` plugin uses.

# job\_submit\_cpuonly

This plugin adds a `cpuonly` feature to jobs that don't request any `gpu`
gres. It is used in heterogeneous clusters with both CPU nodes and GPU nodes,
when CPU-only jobs shouldn't run on the GPU nodes.

For this to work properly, a `cpuonly` feature needs to be available on all
nodes without a `gpu` gres. The script `verify-cpuonly.sh` can be used to
verify the nodes are indeed set up appropriately.

Currently users who want to circumvent this could use `scontrol update job`

# job\_submit\_gres\_groups

This plugin is used to group together several gres types (same gres, different
types) such that e.g. if user can use up to 5 gpus of type A or B, this can be
controlled using slurm's GrpTRES accounting.

The plugin will update the job gres fields such that for each type of gres in a
group, an equivalent gres group will be added, and for each gres group (either
added explicitly by the user or implicitly by the plugin) the proper un-typed
gres will be added (so that gpu binding will occur).

The groups are configured a `gres_groups.conf`, one line per gres that should
be in a group. For example:
```
GRES=gpu:a10 Group=gg:g1
GRES=gpu:a20 Group=gg:g1
GRES=gpu:a30 Group=gg:g2
GRES=gpu:a40 Group=gg:g4
```
