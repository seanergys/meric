#!/usr/bin/env python3

import argparse
import sys
import time

from mpi4py import MPI

sys.path.append("../lib/")

import libmericmpipy as mericpy


def do_work(duration_ms):
    end_time = time.perf_counter() + duration_ms / 1000.0
    value = 1.0

    while time.perf_counter() < end_time:
        for index in range(1, 10000):
            value = (value * 1.0000001 + index) % 1000003.0

    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--sleep-ms", type=int, default=1000)
    parser.add_argument("-r", "--run-ms", type=int, default=1000)
    args = parser.parse_args()

    comm = MPI.COMM_WORLD
    mericpy.init()

    try:
        comm.Barrier()
        mericpy.measureStart("SLEEP")
        time.sleep(args.sleep_ms / 1000.0)
        mericpy.measureStop("SLEEP")
        comm.Barrier()

        mericpy.measureStart("RUN")
        do_work(args.run_ms)
        mericpy.measureStop("RUN")
        comm.Barrier()
    finally:
        mericpy.close()


if __name__ == "__main__":
    main()
