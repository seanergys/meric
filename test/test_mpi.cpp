#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <mpi.h>

#include <meric.h>
#include "work.h"

void help(const std::string name, const int ret)
{
    std::cerr << "MPI+OpenMP application of two MERIC regions - SLEEP (idle), and RUN (compute bound)\n"
              << "Usage: mpirun -np <N> " << name << " [-s sleep_ms] [-r run_ms]\n";
    exit(ret);
}

int main(int argc, char **argv)
{
    int sleep_ms = 1000;
    int run_ms = 1000;

    int opt;
    while ((opt = getopt(argc, argv, "hs:r:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                help(std::string(argv[0]), 0);
            case 's':
                sleep_ms = atoi(optarg);
                break;
            case 'r':
                run_ms = atoi(optarg);
                break;
            default:
                help(std::string(argv[0]), 1);
        }
    }

    MPI_Init(&argc, &argv);
    MERIC_Init();

    MERIC_MeasureStart("SLEEP");
    usleep(sleep_ms * 1000);
    MERIC_MeasureStop("SLEEP");

    MERIC_MeasureStart("RUN");
    #pragma omp parallel
    {
        do_work(run_ms / 1000.0);
    }
    MERIC_MeasureStop("RUN");

    MERIC_Close();
    MPI_Finalize();

    return 0;
}
