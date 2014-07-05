#! /bin/sh
# Finding the True Frequent Itemsets 
#
# Copyright 2014 Matteo Riondato <matteo@cs.brown.edu> and Fabio Vandin
# <vandinfa@imada.sdu.dk>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

. ./conf.sh

USE_ADDIT_KNOWL=$1
# We keep only the decimal part
DELTA=`echo $2 | cut -d "." -f 2`
MIN_FREQ=`echo $3 | cut -d "." -f 2`
GAP=`echo $4 | cut -d "." -f 2`

DATASET=$5
echo -n "Getting dataset stats..." >&2
DS_STATS=`grep ${DATASET} ${SCRIPTS_BASE}/datasetsinfo.py`
if [ "${DS_STATS:-empty}" = "empty" ]; then
	DS_STATS=`${PYTHON3} ${SCRIPTS_BASE}/getDatasetInfo.py ${DATASET}`
fi
BASEDATASETNAME=`echo ${DS_STATS} | cut -d ":" -f 1 | cut -d "'" -f 2 | rev | cut -d . -f 2- |rev`
BASE_STATS=`echo ${DS_STATS} | cut -d ":" -f 2- | rev | cut -d "," -f 2- | rev`
SIZE_COMMAND="stats=${BASE_STATS}; print(stats['size'])"
SIZE=`${PYTHON3} -c "${SIZE_COMMAND}"`

LOWER_DELTA=`echo "scale=10; d = 1 - e(l(1 - 0.${DELTA})/2); print d" | bc -l | cut -d . -f 2`
EPSILON=`${PYTHON3} ${SCRIPTS_BASE}/epsilon.py ${USE_ADDIT_KNOWL} 0.${LOWER_DELTA} ${DATASET} | tail -1 | cut -f 1 | cut -d "." -f 2`
LOWER_SUPP=`echo "scale=scale(${EPSILON}); supp=${SIZE} * (0.${MIN_FREQ} - 0.${EPSILON} ); print supp" | bc | cut -d. -f 1`  
if [ ${LOWER_SUPP} -le 0 ]; then
    echo "LOWER_SUPP=${LOWER_SUPP} less than 0. USE_ADDIT_KNOWL=${USE_ADDIT_KNOWL} MIN_FREQ=${MIN_FREQ} EPSILON=${EPSILON}" >&2
  exit 1
fi
echo "done" >&2

# Only mine the dataset if there is no results file containing the frequent
# itemsets w.r.t. a frequency less than ${MIN_FREQ}.
echo -n "Getting FIs..." >&2
for RES in `ls ${RESULTS_BASE}/${BASEDATASETNAME}_t*.res 2> /dev/null || echo ""`; do
	FREQ=`basename ${RES} | cut -d "_" -f 2 | cut -d "t" -f 2 | cut -d "." -f 1`
	# Floating point comparison
	MINUS=`echo 0.${FREQ} - 0.${MIN_FREQ} | bc | cut -d "." -f 1`
	if [ ${MINUS:-empty} = "-" ]; then
		echo -n "found results for freq=${FREQ}..." >&2
		RESULTS_FILE=${RES}
		break
	fi
done

if [ ${RESULTS_FILE:-empty} = "empty" ]; then
	echo -n "must mine the dataset..." >&2
	RESULTS_FILE=${RESULTS_BASE}/${BASEDATASETNAME}_t${MIN_FREQ}.res
	sh ${SCRIPTS_BASE}/minedb-gra.sh ${LOWER_SUPP} ~/myres/realfis/samples/dat/${DATASETNAME} ${RESULTS_FILE}
fi
echo "done" >&2

# Compute the True FIs
echo "Getting TFIs..." >&2
${PYTHON3} ${SCRIPTS_BASE}/getTrueFIsVC.py ${USE_ADDIT_KNOWL} 0.${DELTA} 0.${MIN_FREQ} 0.${GAP} ${DATASET} ${RESULTS_FILE}

