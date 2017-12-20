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

