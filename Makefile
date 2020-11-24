
CFLAGS += -fPIC -Wall -ggdb -O0

PLUGINS = job_submit_limit_interactive \
          job_submit_info \
	  job_submit_default_options \
	  job_submit_valid_partitions \
	  job_submit_meta_partitions \
	  job_submit_killable \
	  job_submit_cpuonly \
          spank_lmod \
          spank_killable


BUILDDIR = $(shell echo build.`uname -s`-`uname -m`-`./slurm-version.sh`)

all:

define _compile
all: $(1).so
.PHONY: $(1) $(1).so
$(1): $(1).so
$(1).so: $(BUILDDIR)/$(1).so

$(BUILDDIR)/$(1).so: $(1).c
	mkdir -p $(BUILDDIR)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -shared $$< -o $$@
	chmod a-x $$@

endef
$(foreach pi,$(PLUGINS),$(eval $(call _compile,$(pi))))


clean:
	rm -rf $(BUILDDIR)
