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
    
    /* ... Any more ? ... */
    
    verification_result_max
} verification_result_t;

/* Any initiation to be done */
typedef void *(*verification_plugin_init_t)(void);

/* Called on each packet for verification */
typedef verification_result_t (*verification_plugin_verify_packet_t)(char *packet, void *priv_data);

/* Close and free whatever needed */
typedef void (*verification_plugin_close_t)(void *priv_data);

typedef struct verification_plugin_t {
    char                                *plugin_name;
    verification_plugin_init_t          init;
    verification_plugin_verify_packet_t verify;
    verification_plugin_close_t         close;
} verification_plugin_t;

#endif /* __HIGH_PERF_CALC_H__ */
