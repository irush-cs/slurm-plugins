/******************************************************************************
 *
 *   spank_killable.c
 *
 *   Copyright (C) 2018 Hebrew University of Jerusalem Israel, see LICENSE
 *   file.
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

#include <stdlib.h>

#include <slurm/spank.h>

SPANK_PLUGIN(killable, 1);

static int _opt_process (int val, const char *optarg, int remote);
static int is_killable = 0;

struct spank_option spank_option_array[] = {
    { "killable", NULL, "run a killable job", 0, 0, (spank_opt_cb_f)_opt_process },
    SPANK_OPTIONS_TABLE_END
};

static int _opt_process(int val, const char *optarg, int remote) {
    //slurm_info("spank_killable: setting killable");
    is_killable = 1;
    return 0;
}

int slurm_spank_init(spank_t spank, int ac, char **av) {
    int i, j, rc = ESPANK_SUCCESS;
    is_killable = 0;
    //slurm_info("spank_killable: init");

    for (i = 0; spank_option_array[i].name; i++) {
        j = spank_option_register(spank, &spank_option_array[i]);
        if (j != ESPANK_SUCCESS) {
            slurm_error("Could not register Spank option %s",
                        spank_option_array[i].name);
            rc = j;
        }
    }
    return rc;
}

int slurm_spank_init_post_opt(spank_t spank, int ac, char **av) {
    // doesn't really work..
    return setenv("SPANK_KILLABLE", is_killable ? "1" : "0", 1);
}
