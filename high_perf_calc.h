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

#ifndef __HIGH_PERF_CALC_H__
#define __HIGH_PERF_CALC_H__

typedef enum {
	/* Packet has been verified */
	verification_result_ok = 0,

	/* There’s a space issue somewhere */
	verification_result_space = 1,

	/* A number is smaller than 0 or bigger than 65535 */
	verification_result_overflow = 2,

	/* A packet’s suffix missing or corrupted */
	verification_result_bad_suffix = 3,

	verification_result_max
} verification_result_t;

/* Prototype of the function that will actually do the function verification */
typedef verification_result_t (*verify_packet_t)(char *packet, void *priv_data);

/* 
 * Prototype of the function that will initialize the private data before 
 * starting the verification process
 */
typedef void (*init_priv_data_t)(void *priv_data);

typedef struct {
    init_priv_data_t ipd;
    verify_packet_t vp;
    int priv_data_size;
} init_ctx_t;

void init(init_ctx_t *ic);

#endif /* __HIGH_PERF_CALC_H__ */
