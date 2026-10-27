#!/usr/bin/env python3

import argparse
import multiprocessing as mp
import os
import sys
import time

sys.path.append("../lib/")

import libmericpy as mericpy


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

    workers = int(os.environ.get("MERIC_NUM_THREADS", os.cpu_count() or 1))

    with mp.Pool(workers) as pool:
        mericpy.init()

        try:
            mericpy.measureStart("SLEEP")
            time.sleep(args.sleep_ms / 1000.0)
            mericpy.measureStop("SLEEP")

            mericpy.measureStart("RUN")
            pool.map(do_work, [args.run_ms] * workers)
            mericpy.measureStop("RUN")
        finally:
            mericpy.close()


if __name__ == "__main__":
    main()
