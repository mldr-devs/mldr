#!/bin/bash

SEEDS=(100 101 102 103 104 105 106 107 108 109)
N_WIFIS=(1 4 8 12 16 20)
DATA_RATE=120

for n in "${N_WIFIS[@]}"; do
  for seed in "${SEEDS[@]}"; do
    python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=1 --massive=0 --useWarmup --agentName="MLDR" --nWifi=$n --dataRate=$DATA_RATE --seed=$seed --simulationTime=100 --csvLogPath="$MLDR_DIR/scripts/throughput_MLDR_n${n}_r${DATA_RATE}_s${seed}_log.csv" --csvPath="$MLDR_DIR/scripts/throughput_MLDR_n${n}_r${DATA_RATE}_s${seed}.csv"
  done
done

for n in "${N_WIFIS[@]}"; do
  for seed in "${SEEDS[@]}"; do
    python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=1 --massive=0 --useWarmup --agentName="wifi" --nWifi=$n --dataRate=$DATA_RATE --seed=$seed --simulationTime=100 --csvLogPath="$MLDR_DIR/scripts/throughput_80211_n${n}_r${DATA_RATE}_s${seed}_log.csv" --csvPath="$MLDR_DIR/scripts/throughput_80211_n${n}_r${DATA_RATE}_s${seed}.csv"
  done
done

for n in "${N_WIFIS[@]}"; do
  for seed in "${SEEDS[@]}"; do
    python $MLDR_DIR/mldr/envs/run.py --urllc=0 --throughput=1 --massive=0 --useWarmup --agentName="wifi" --rtsCts --nWifi=$n --dataRate=$DATA_RATE --seed=$seed --simulationTime=100 --csvLogPath="$MLDR_DIR/scripts/throughput_80211_RTS_n${n}_r${DATA_RATE}_s${seed}_log.csv" --csvPath="$MLDR_DIR/scripts/throughput_80211_RTS_n${n}_r${DATA_RATE}_s${seed}.csv"
  done
done
