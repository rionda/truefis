#! /bin/sh
DATASET="chess-e20M-${SGE_TASK_ID}.dat"
ORIG_RES="/home/matteo/myres/truefis/largeres/FI/chess_f32.res"
MIN_FREQ="30"
DS_SIZE="20000000"
HOLDOUT_SIZE="10000000"
ITEMS="75"
DELTA="10"
FREQS="500 550 600 650 700 750 775 800"
