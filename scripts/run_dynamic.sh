#!/bin/bash

SEED=100
MIN_WIFI=5
N_WIFI=50
INTERVAL_STA=5
INTERVAL_TIME=100
SIMULATION_TIME=1000

python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=1 --massive=0 --agentName="MLDR" --nWifi=$N_WIFI --minWifi=$MIN_WIFI --intervalSta=$INTERVAL_STA --intervalTime=$INTERVAL_TIME --dataRate=120 --seed=$SEED --simulationTime=$SIMULATION_TIME --csvLogPath="$MLDR_DIR/scripts/dynamic_MLDR_m${MIN_WIFI}_n${N_WIFI}_s${SEED}.csv"
python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=1 --massive=0 --agentName="wifi" --nWifi=$N_WIFI --minWifi=$MIN_WIFI --intervalSta=$INTERVAL_STA --intervalTime=$INTERVAL_TIME --dataRate=120 --seed=$SEED --simulationTime=$SIMULATION_TIME --csvLogPath="$MLDR_DIR/scripts/dynamic_80211_m${MIN_WIFI}_n${N_WIFI}_s${SEED}.csv"
python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=1 --massive=0 --agentName="wifi" --rtsCts --nWifi=$N_WIFI --minWifi=$MIN_WIFI --intervalSta=$INTERVAL_STA --intervalTime=$INTERVAL_TIME --dataRate=120 --seed=$SEED --simulationTime=$SIMULATION_TIME --csvLogPath="$MLDR_DIR/scripts/dynamic_80211_RTS_m${MIN_WIFI}_n${N_WIFI}_s${SEED}.csv"
