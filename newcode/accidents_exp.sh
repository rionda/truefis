#! /bin/sh
DATASET="accidents-e20M-${SGE_TASK_ID}.dat"
ORIG_RES="/home/matteo/myres/truefis/largeres/FI/accidents_f075.res"
MIN_FREQ="075"
DS_SIZE="20000000"
HOLDOUT_SIZE="10000000"
ITEMS="468"
DELTA="10"
FREQS="17 20 30 40 50 60 70 80" 
