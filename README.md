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

# job\_submit\_default_options

Sets some default options to jobs.

The `default_options.conf` configuration file is used to set the default
values. Currently only "cluster_features" is supported (for the
--cluster-constraint option).
