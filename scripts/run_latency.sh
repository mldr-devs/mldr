#!/bin/bash

SEEDS=(100 101 102 103 104 105 106 107 108 109)
N_WIFI=4
DATA_RATE=20

for seed in "${SEEDS[@]}"; do
  python $MLDR_DIR/mldr/envs/run.py --urllc=1 --throughput=0 --massive=0 --useWarmup --agentName="MLDR" --nWifi=$N_WIFI --dataRate=$DATA_RATE --seed=$seed --simulationTime=100 --csvLogPath="$MLDR/scripts/latency_MLDR_n${N_WIFI}_r${DATA_RATE}_s${seed}.csv" --flowmonPath="$MLDR/scripts/latency_MLDR_n${N_WIFI}_r${DATA_RATE}_s${seed}.xml"
done

for seed in "${SEEDS[@]}"; do
  python $MLDR_DIR/mldr/envs/run.py --urllc=1 --throughput=0 --massive=0 --useWarmup --agentName="wifi" --nWifi=$N_WIFI --dataRate=$DATA_RATE --seed=$seed --simulationTime=100 --csvLogPath="$MLDR/scripts/latency_80211_n${N_WIFI}_r${DATA_RATE}_s${seed}.csv" --flowmonPath="$MLDR/scripts/latency_80211_n${N_WIFI}_r${DATA_RATE}_s${seed}.xml"
done

for seed in "${SEEDS[@]}"; do
  python $MLDR_DIR/mldr/envs/run.py --urllc=1 --throughput=0 --massive=0 --useWarmup --agentName="wifi" --rtsCts --nWifi=$N_WIFI --dataRate=$DATA_RATE --seed=$seed --simulationTime=100 --csvLogPath="$MLDR/scripts/latency_80211_RTS_n${N_WIFI}_r${DATA_RATE}_s${seed}.csv" --flowmonPath="$MLDR/scripts/latency_80211_RTS_n${N_WIFI}_r${DATA_RATE}_s${seed}.xml"
done
