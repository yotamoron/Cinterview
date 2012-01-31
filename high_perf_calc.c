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
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#if 0
#define LOG(fmt, args...)           \
    printf("\t%s():%d\t" fmt "\n", __func__, __LINE__, ##args)
#else
#define LOG(fmt, args...) do { } while(0)
#endif

#define MAXBUFF 512
#define NUM_OF_TRANSACTIONS 100000

#define SUFFIX "VON_NEUMANN"
#define SECURED "SECURED"
#define SEPARATE "SEPARATE"
#define SCRAP "SCRAP"
#define TRANS_TYPE_PREFIX "START_TRANSACTION"

#define EXTRA_SPACE(vr) (vr == verification_result_space ? " " : "")
#define BAD_SUFFIX(vr) (vr == verification_result_bad_suffix ? "DEADBEEF" : "")

#define NUM_OF_OPS 4

typedef struct packet_t {
    struct packet_t *next;
    char *payload;
    verification_result_t expected_result;
} packet_t;

typedef struct transaction_t {
    struct transaction_t *next;
    packet_t *packets;
} transaction_t;

static init_ctx_t ic = { 0 };
static char *priv_data = NULL;

static transaction_t *transactions = NULL;

typedef enum {
    proto_stages_handshake_to_server = 0,
    proto_stages_handshake_from_server = 1,
    proto_stages_op_to_server = 2,
    proto_stages_op_from_server = 3,
    proto_stages_teardown_to_server = 4,
    proto_stages_teardown_from_server = 5,
    proto_stages_max
} proto_stages_t;

typedef enum {
    trans_type_secured = 0,
    trans_type_separate = 1,
    trans_type_scrap = 2,
    trans_type_max,
} trans_type_t;

static char *trans_types[trans_type_max] = {
    [trans_type_secured] = SECURED,
    [trans_type_separate] = SEPARATE,
    [trans_type_scrap] = SCRAP
};
static char *code2str(verification_result_t vr)
{
    switch (vr) {
    default:
        return "UNKNOWN";

    case verification_result_ok:
        return "verification_result_ok";

    case  verification_result_space:
	    return "verification_result_space";

    case verification_result_overflow:
	    return "verification_result_overflow";

    case verification_result_bad_suffix:
	    return "verification_result_bad_suffix";
    }
}

static packet_t *handshake_to_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));
    trans_type_t curr_trans_type = *((trans_type_t *)data);
    char buf[MAXBUFF] = { 0 };
    char *extra_space = EXTRA_SPACE(vr);
    char *bad_suffix = BAD_SUFFIX(vr);

    sprintf(buf, TRANS_TYPE_PREFIX " %s %s" SUFFIX "%s", trans_types[curr_trans_type],
            extra_space, bad_suffix);

    p->payload = strdup(buf);
    p->expected_result = vr;
    LOG("p->payload = %s vr = %s", p->payload, code2str(vr));
    return p;
}

static packet_t *handshake_from_server(verification_result_t vr, void *data)
{
    char buf[MAXBUFF] = { 0 };
    packet_t *p = calloc(1, sizeof(packet_t));
    trans_type_t curr_trans_type = *((trans_type_t *)data);
    char *extra_space = EXTRA_SPACE(vr);
    char *bad_suffix = BAD_SUFFIX(vr);

    sprintf(buf, "%s %s" SUFFIX "%s", trans_types[curr_trans_type], 
            extra_space, bad_suffix);

    p->payload = strdup(buf);
    p->expected_result = vr;
    LOG("p->payload = %s vr = %s", p->payload, code2str(vr));
    return p;
}

static int get_res(char *op, int n1, int n2)
{
    if (!strcmp(op, "PLUS")) {
        return n1 + n2;
    }
    if (!strcmp(op, "MINUS")) {
        return n1 - n2;
    }
    if (!strcmp(op, "TIMES")) {
        return n1 * n2;
    }
    if (!strcmp(op, "DIVIDE")) {
        if (!n2) {
            return 0;
        }
        return (int)(n1 / n2);
    }

    return 0;
}

static packet_t *op_to_server(verification_result_t vr, void *data)
{
    static int curr_op = 0;
    static int curr_overflow = 1;
    packet_t *p = calloc(1, sizeof(packet_t));
    char buf[MAXBUFF] = { 0 };
    char *extra_space = EXTRA_SPACE(vr);
    char *bad_suffix = BAD_SUFFIX(vr);
    int n1 = random() % 100;
    int n2 = random() % 100;
    int *res = (int *) data;
    static char *ops[NUM_OF_OPS] = {
        "PLUS",
        "MINUS",
        "TIMES",
        "DIVIDE"
    };

    if (vr == verification_result_overflow) {
        n2 += (65355 * curr_overflow);
        curr_overflow *= -1;
    }

    sprintf(buf, "%s %d %d %s" SUFFIX"%s", ops[curr_op], n1, n2, extra_space, 
            bad_suffix); 
    *res = get_res(ops[curr_op], n1, n2);
    curr_op = (curr_op + 1) % NUM_OF_OPS;
    p->payload = strdup(buf);
    LOG("p->payload = %s vr = %s", p->payload, code2str(vr));
    p->expected_result = vr; 
    return p;
}

static packet_t *op_from_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));
    char buf[MAXBUFF] = { 0 };
    char res_buf[MAXBUFF] = { 0 };
    char *extra_space = EXTRA_SPACE(vr);
    char *bad_suffix = BAD_SUFFIX(vr);
    int *res = (int *) data;

    sprintf(res_buf, "%d", *res);
    sprintf(buf, "RESULT %s %s" SUFFIX "%s", *res < 0 ? "ERROR" : res_buf, 
            extra_space, bad_suffix);
    p->payload = strdup(buf);
    LOG("p->payload = %s vr = %s", p->payload, code2str(vr));
    p->expected_result = vr; 
    return p;
}

static packet_t *teardown_to_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));
    char buf[MAXBUFF] = { 0 };
    char *extra_space = EXTRA_SPACE(vr);
    char *bad_suffix = BAD_SUFFIX(vr);

    sprintf(buf, "OK CLOSE %s" SUFFIX "%s", extra_space, bad_suffix);
    p->payload = strdup(buf);
    LOG("p->payload = %s vr = %s", p->payload, code2str(vr));
    p->expected_result = vr; 
    return p;
}

static packet_t *teardown_from_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));
    char buf[MAXBUFF] = { 0 };
    char *extra_space = EXTRA_SPACE(vr);
    char *bad_suffix = BAD_SUFFIX(vr);

    sprintf(buf, "CLOSED %s" SUFFIX "%s", extra_space, bad_suffix);
    p->payload = strdup(buf);
    LOG("p->payload = %s vr = %s", p->payload, code2str(vr));
    p->expected_result = vr; 
    return p;
}

typedef packet_t *(*proto_stages_cb_t)(verification_result_t vr, void *data);

typedef struct {
    proto_stages_cb_t proto_stages_cb;
    void **spec_data;
} proto_stages_cb_desc_t;

static proto_stages_cb_desc_t proto_stages_descs[proto_stages_max] = {
    [proto_stages_handshake_to_server] = {
        .proto_stages_cb = handshake_to_server,
    },
    [proto_stages_handshake_from_server] = {
        .proto_stages_cb = handshake_from_server,
    },
    [proto_stages_op_to_server] = {
        .proto_stages_cb = op_to_server,
    },
    [proto_stages_op_from_server] = {
        .proto_stages_cb = op_from_server,
    },
    [proto_stages_teardown_to_server] = {
        .proto_stages_cb = teardown_to_server,
    },
    [proto_stages_teardown_from_server] = {
        .proto_stages_cb = teardown_from_server,
    }
};

static void get_needed_error(int *on_verification_result, 
        int *on_stage)
{
    static int stage_to_fuck_up = proto_stages_handshake_to_server;
    static int verification_result = verification_result_space;

    stage_to_fuck_up = (stage_to_fuck_up + 1 ) % proto_stages_max;
    verification_result = (verification_result + 1) % verification_result_max;
    if (!verification_result) {
        verification_result = !verification_result;
    }
    if (stage_to_fuck_up == proto_stages_op_to_server) {
        verification_result = verification_result_overflow;
    } else if (verification_result == verification_result_overflow) {
        verification_result++;
    }
    *on_verification_result = verification_result;
    *on_stage = stage_to_fuck_up;
}

static transaction_t *create_single_transaction(int fuck_it_up)
{
    transaction_t *t = calloc(1, sizeof(transaction_t));
    packet_t **next_packet = &(t->packets);
    int i = 0;
    int on_verification_result = 0;
    int on_stage = proto_stages_max;

    if (fuck_it_up) {
        /* Need to get the stage and the code to fuck things up */
        get_needed_error(&on_verification_result, &on_stage);
    }
    for (; i < proto_stages_max; i++) {
        verification_result_t vr = i == on_stage ? 
            on_verification_result : verification_result_ok;
        packet_t *p = 
            proto_stages_descs[i].proto_stages_cb(vr,
                    proto_stages_descs[i].spec_data);

        *next_packet = p;
        next_packet = &(p->next);
        if (vr != verification_result_ok) {
            break;
        }
    }

    return t;
}

static void create_transactions(void)
{
    int i = 0;
    int res = 0;
    trans_type_t curr_trans_type = trans_type_secured;

    proto_stages_descs[proto_stages_handshake_to_server].spec_data = 
    proto_stages_descs[proto_stages_handshake_from_server].spec_data = 
        (void *)&curr_trans_type;

    proto_stages_descs[proto_stages_op_to_server].spec_data = 
    proto_stages_descs[proto_stages_op_from_server].spec_data = 
        (void *)&res;
    for (i = 0; i < NUM_OF_TRANSACTIONS; i++) {
        int fuck_it_up = !(i % 2);
        transaction_t *t = create_single_transaction(fuck_it_up);

        t->next = transactions;
        transactions = t;
        curr_trans_type = (curr_trans_type + 1) % trans_type_max;
    }
}

static void register_cbs(void)
{
    init(&ic);

    if (!ic.vp) {
        printf("No verification callback, exiting\n");
        exit(-1);
    }

    if (ic.ipd) {
        if (!ic.priv_data_size || ic.priv_data_size < 0) {
            printf("WARNING: init_priv_data registered but priv_data size is"
                   " 0 or smaller than 0\n");
        } else {
            priv_data = malloc(ic.priv_data_size);
        }
    }
}

static void run_verification(void)
{
    transaction_t *t = transactions;

    while (t) {
        packet_t *p = t->packets;

        if (ic.ipd) {
            ic.ipd(priv_data);
        }
        while (p) {
            verification_result_t vr = ic.vp(p->payload, priv_data);

            if (vr != p->expected_result) {
                printf("On payload \"%s\": expected \"%s\" but received "
                        "\"%s\" -- VERIFICATION FAILED\n", p->payload, 
                        code2str(p->expected_result), code2str(vr));
                return;
            }
            p = p->next;
        }
        t = t->next;
    }

    printf("VERIFICATION SUCCESSFUL\n");

    return;
}

static void free_all(void)
{
    while (transactions) {
        transaction_t *transaction_to_free = transactions;
        packet_t *p = transaction_to_free->packets;

        transactions = transactions->next;
        while (p) {
            packet_t *packet_to_free = p;
            
            p = p->next;
            LOG("Free %s", packet_to_free->payload);
            free(packet_to_free->payload);
            free(packet_to_free);
        }
    }
}

static void print_time_diff(struct timeval *before, struct timeval *after)
{
  long msec = 0;

  msec = (after->tv_sec - before->tv_sec) * 1000;
  msec += (after->tv_usec - before->tv_usec) / 1000;

  printf("The verification process took %lu milliseconds\n", msec);
}

int main()
{
    struct timeval time_before_test, time_after_test;

    printf("Welcome to the High Performance Calculator\n");
    printf("Preparing transactions ...\n");

    create_transactions();

    register_cbs();

    gettimeofday(&time_before_test, NULL);
    run_verification();
    gettimeofday(&time_after_test, NULL);

    print_time_diff(&time_before_test, &time_after_test);
    free_all();

    return 0;
}
