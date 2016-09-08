
#include "hdeem.h"
#include <stdio.h>

int main(int argc, char **argv)
{
        hdeem_bmc_data_t bmc;

        int rc = hdeem_init(&bmc);
        if (rc) {
                printf("error in init\n");
        }
        rc = hdeem_start(&bmc);
        if (rc) {
                printf("error in start\n");
        }

        hdeem_stats_reading_t stats;
        rc = hdeem_get_stats(&bmc, &stats);
        if (rc) {
                printf("error in stats\n");
        }

        time_t hdeem = stats.read_time_blade.tv_sec;
        printf("%ld\n", hdeem);

        hdeem_stats_free(&stats);

        hdeem_stop(&bmc);
        hdeem_close(&bmc);

        return 0;
}



