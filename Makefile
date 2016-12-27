
CPPFLAGS += -I/cs/system/irush/yard/slurm/slurm/linux-15.08.10-1 -I/cs/system/irush/yard/slurm/slurm/slurm.git
CFLAGS += -fPIC -Wall -ggdb -O0

PLUGINS = job_submit_limit_interactive \
          job_submit_info

BUILDDIR = $(shell echo build.`uname -s`-`uname -m`)

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
