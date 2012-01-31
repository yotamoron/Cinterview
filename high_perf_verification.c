/*
 * This file is part of Cinterview.
 * 
 * Cinterview is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Cinterview is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cinterview.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "high_perf_calc.h"

#include <stdlib.h>

static void *plugin_init(void)
{
    /* YOUR CODE HERE */

    return NULL;
}

static verification_result_t plugin_verify(char *packet, void *priv_data)
{
    /* YOUR CODE HERE */

    return verification_result_ok;
}

static void plugin_close(void *priv_data)
{
    /* YOUR CODE HERE */

    return;
}

verification_plugin_t my_plugin = {
    .plugin_name = "MY PLUGIN",
    .init = plugin_init,
    .verify = plugin_verify,
    .close = plugin_close
};

