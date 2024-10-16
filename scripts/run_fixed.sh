#!/bin/bash

SEEDS=(100 101 102 103 104 105 106 107 108 109)

for seed in "${SEEDS[@]}"; do
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_ampdu_cw0_rts_s${seed}.xml" --simulationTime=100 --ampdu --cw=0 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_noAmpdu_cw0_rts_s${seed}.xml" --simulationTime=100 --no-ampdu --cw=0 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_ampdu_cw6_rts_s${seed}.xml" --simulationTime=100 --ampdu --cw=6 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_noAmpdu_cw6_rts_s${seed}.xml" --simulationTime=100 --no-ampdu --cw=6 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_ampdu_cw0_noRts_s${seed}.xml" --simulationTime=100 --ampdu --cw=0 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_noAmpdu_cw0_noRts_s${seed}.xml" --simulationTime=100 --no-ampdu --cw=0 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_ampdu_cw6_noRts_s${seed}.xml" --simulationTime=100 --ampdu --cw=6 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=4 --dataRate=20 --seed=$seed --flowmonPath="$MLDR_DIR/scripts/latency_noAmpdu_cw6_noRts_s${seed}.xml" --simulationTime=100 --no-ampdu --cw=6 --no-rtsCts
done

for seed in "${SEEDS[@]}"; do
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_ampdu_cw0_rts_s${seed}.csv" --simulationTime=100 --ampdu --cw=0 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_noAmpdu_cw0_rts_s${seed}.csv" --simulationTime=100 --no-ampdu --cw=0 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_ampdu_cw6_rts_s${seed}.csv" --simulationTime=100 --ampdu --cw=6 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_noAmpdu_cw6_rts_s${seed}.csv" --simulationTime=100 --no-ampdu --cw=6 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_ampdu_cw0_noRts_s${seed}.csv" --simulationTime=100 --ampdu --cw=0 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_noAmpdu_cw0_noRts_s${seed}.csv" --simulationTime=100 --no-ampdu --cw=0 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_ampdu_cw6_noRts_s${seed}.csv" --simulationTime=100 --ampdu --cw=6 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=12 --dataRate=110 --seed=$seed --csvPath="$MLDR_DIR/scripts/throughput_noAmpdu_cw6_noRts_s${seed}.csv" --simulationTime=100 --no-ampdu --cw=6 --no-rtsCts
done

for seed in "${SEEDS[@]}"; do
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_ampdu_cw0_rts_s${seed}.txt" --simulationTime=100 --ampdu --cw=0 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_noAmpdu_cw0_rts_s${seed}.txt" --simulationTime=100 --no-ampdu --cw=0 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_ampdu_cw6_rts_s${seed}.txt" --simulationTime=100 --ampdu --cw=6 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_noAmpdu_cw6_rts_s${seed}.txt" --simulationTime=100 --no-ampdu --cw=6 --rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_ampdu_cw0_noRts_s${seed}.txt" --simulationTime=100 --ampdu --cw=0 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_noAmpdu_cw0_noRts_s${seed}.txt" --simulationTime=100 --no-ampdu --cw=0 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_ampdu_cw6_noRts_s${seed}.txt" --simulationTime=100 --ampdu --cw=6 --no-rtsCts
    python $MLDR_DIR/mldr/envs/run.py --agentName="wifi" --nWifi=250  --distance=500 --interPacketInterval=0.05 --packetSize=256 --fuzzTime=50 --scenario=adhoc --seed=$seed --thrPath="$MLDR_DIR/scripts/massive_noAmpdu_cw6_noRts_s${seed}.txt" --simulationTime=100 --no-ampdu --cw=6 --no-rtsCts
done
