
#include "high_perf_calc.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#if 1
#define LOG(fmt, args...)           \
    printf("\t%s():%d\t" fmt "\n", __func__, __LINE__, ##args)
#else
#define LOG(fmt, args...) do { } while(0)
#endif

#define SUFFIX "VON_NEUMANN"
#define MAXBUFF 512
#define NUM_OF_TRANSACTIONS 100

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

#define SECURED "SECURED"
#define SEPARATE "SEPARATE"
#define SCRAP "SCRAP"

static packet_t *handshake_to_server(verification_result_t vr, void *data)
{
#define TRANS_TYPE_PREFIX "START_TRANSACTION"
#define TRANS_TYPE_SECURED TRANS_TYPE_PREFIX " " SECURED " " SUFFIX
#define TRANS_TYPE_SEPARATE TRANS_TYPE_PREFIX " " SEPARATE " " SUFFIX
#define TRANS_TYPE_SCRAP TRANS_TYPE_PREFIX " " SCRAP " " SUFFIX
    packet_t *p = calloc(1, sizeof(packet_t));
    trans_type_t curr_trans_type = *((trans_type_t *)data);

    switch(curr_trans_type) {
    case trans_type_secured:
        p->payload = strdup(TRANS_TYPE_SECURED);
        break;
    case trans_type_separate:
        p->payload = strdup(TRANS_TYPE_SEPARATE);
        break;
    case trans_type_scrap:
        p->payload = strdup(TRANS_TYPE_SCRAP);
        break;
    default:
        printf("Unknown transcation type %d\n", curr_trans_type);
        exit(-1);
    }
    LOG("p->payload = %s", p->payload);
    p->expected_result = verification_result_ok;
    return p;
}

static packet_t *handshake_from_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));
    trans_type_t curr_trans_type = *((trans_type_t *)data);

    switch(curr_trans_type) {
    case trans_type_secured:
        p->payload = strdup(SECURED " " SUFFIX);
        break;
    case trans_type_separate:
        p->payload = strdup(SEPARATE " " SUFFIX);
        break;
    case trans_type_scrap:
        p->payload = strdup(SCRAP " " SUFFIX);
        break;
    default:
        printf("Unknown transcation type %d\n", curr_trans_type);
        exit(-1);
    }
    LOG("p->payload = %s", p->payload);
    p->expected_result = verification_result_ok;
    return p;
}

static packet_t *op_to_server(verification_result_t vr, void *data)
{
    static int curr_op = 0;
    packet_t *p = calloc(1, sizeof(packet_t));
    char buf[MAXBUFF] = { 0 };
#define NUM_OF_OPS 4
    static char *ops[NUM_OF_OPS] = {
        "PLUS",
        "MINUS",
        "TIMES",
        "DIVIDE"
    };
    sprintf(buf, "%s %d %d " SUFFIX, ops[curr_op], random() % 100, 
            random() % 100);
    curr_op = (curr_op + 1) % NUM_OF_OPS;
    p->payload = strdup(buf);
    LOG("p->payload = %s", p->payload);
    p->expected_result = verification_result_ok;
    return p;
}

static packet_t *op_from_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));

    p->payload = strdup("RESULT 100 " SUFFIX);
    LOG("p->payload = %s", p->payload);
    p->expected_result = verification_result_ok;
    return p;
}

static packet_t *teardown_to_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));

    p->payload = strdup("OK CLOSE " SUFFIX);
    LOG("p->payload = %s", p->payload);
    p->expected_result = verification_result_ok;
    return p;
}

static packet_t *teardown_from_server(verification_result_t vr, void *data)
{
    packet_t *p = calloc(1, sizeof(packet_t));

    p->payload = strdup("CLOSED " SUFFIX);
    LOG("p->payload = %s", p->payload);
    p->expected_result = verification_result_ok;
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

again:
    stage_to_fuck_up = (stage_to_fuck_up + 1 ) % proto_stages_max;
    verification_result = (verification_result + 1) % verification_result_max;
    if (!verification_result) {
        verification_result = !verification_result;
    }
    if ((stage_to_fuck_up != proto_stages_op_to_server || 
         stage_to_fuck_up != proto_stages_op_from_server) && 
        verification_result == verification_result_overflow) {
        goto again;
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
        int fuck_it_up = !(i % 5);
        transaction_t *t = create_single_transaction(fuck_it_up);

        t->next = transactions;
        transactions = t;
        curr_trans_type = (curr_trans_type + 1) % trans_type_max;
    }
}

static void register_cbs(void)
{
    int priv_data_size = 0;

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

  printf("The verification process took %d milliseconds\n", msec);
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
}
