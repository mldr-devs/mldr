#!/bin/bash

SEEDS=(100 101 102 103 104 105 106 107 108 109)
N_WIFI=250
DISTANCE=500
INTER_PACKET_INTERVAL=0.05
PACKET_SIZE=256

for seed in "${SEEDS[@]}"; do
  python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=0 --massive=1 --scenario=adhoc --useWarmup --agentName="MLDR" --nWifi=$N_WIFI --distance=$DISTANCE --interPacketInterval=$INTER_PACKET_INTERVAL --seed=$seed --simulationTime=100 --fuzzTime=50 --packetSize=$PACKET_SIZE --csvLogPath="$MLDR_DIR/scripts/massive_MLDR_n${N_WIFI}_d${DISTANCE}_s${seed}.csv" --thrPath="$MLDR_DIR/scripts/massive_MLDR_n${N_WIFI}_d${DISTANCE}_s${seed}.txt"
done

for seed in "${SEEDS[@]}"; do
  python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=0 --massive=1 --scenario=adhoc --useWarmup --agentName="wifi" --nWifi=$N_WIFI --distance=$DISTANCE --interPacketInterval=$INTER_PACKET_INTERVAL --seed=$seed --simulationTime=100 --fuzzTime=50 --packetSize=$PACKET_SIZE --csvLogPath="$MLDR_DIR/scripts/massive_80211_n${N_WIFI}_d${DISTANCE}_s${seed}.csv" --thrPath="$MLDR_DIR/scripts/massive_80211_n${N_WIFI}_d${DISTANCE}_s${seed}.txt"
done

for seed in "${SEEDS[@]}"; do
  python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=0 --massive=1 --scenario=adhoc --useWarmup --agentName="wifi" --rtsCts --nWifi=$N_WIFI --distance=$DISTANCE --interPacketInterval=$INTER_PACKET_INTERVAL --seed=$seed --simulationTime=100 --fuzzTime=50 --packetSize=$PACKET_SIZE --csvLogPath="$MLDR_DIR/scripts/massive_80211_RTS_n${N_WIFI}_d${DISTANCE}_s${seed}.csv" --thrPath="$MLDR_DIR/scripts/massive_80211_RTS_n${N_WIFI}_d${DISTANCE}_s${seed}.txt"
done
