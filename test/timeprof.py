import time
import sys
sys.path.append("../lib/")

import libtimeprofpy as timeprof

def main():
    
    timeprof.regionStart("SLEEP")
    time.sleep(5) # 5 seconds
    timeprof.regionStop("SLEEP")

    timeprof.evaluate()
    print(timeprof.getLastRegionDuration())

if __name__ == "__main__":
    main()