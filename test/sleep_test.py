import time
import sys
sys.path.append("../lib/")

import libmericpy as mericpy

def main():
    mericpy.init()
    
    mericpy.measureStart("SLEEP")
    time.sleep(5) # 5 seconds
    mericpy.measureStop("SLEEP")

    mericpy.close()

if __name__ == "__main__":
    main()