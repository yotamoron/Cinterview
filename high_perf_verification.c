
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
