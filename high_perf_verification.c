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

typedef struct {
    int i;
} priv_data_t;

static verification_result_t verify_packet(char *packet, void *priv_data)
{
    return verification_result_ok;
}

static void init_priv_data(void *priv_data)
{
    priv_data_t *dpd = priv_data;

    dpd->i = 0;
}

void init(init_ctx_t *ic)
{
    ic->vp = verify_packet;
    ic->ipd = init_priv_data;
    ic->priv_data_size = sizeof(priv_data_t);
}
