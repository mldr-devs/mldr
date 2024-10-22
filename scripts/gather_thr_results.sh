#!/bin/bash

FILENAME="thr_results.csv"
N_WIFIS=(1 4 8 12 16 20)
AGENTS=("MLDR" "80211" "80211_RTS")

echo "agent,dataRate,distance,nWifi,nWifiReal,seed,warmupEnd,fairness,latency,plr,throughput" > $FILENAME

for agent in "${AGENTS[@]}"; do
  for n in "${N_WIFIS[@]}"; do
    for file in throughput_${agent}_n${n}_* ; do
      if [[ $file == *_log.csv ]]; then
        continue
      fi
      if [[ "$agent" == "80211_RTS" ]]; then
        sed -e "s/wifi/80211_RTS/g" $file >> $FILENAME
      elif [[ "$agent" == "80211" ]]; then
        sed -e "s/wifi/80211/g" $file >> $FILENAME
      else
        cat $file >> $FILENAME
      fi
    done
  done
done
