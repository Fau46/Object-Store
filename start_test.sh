#!/bin/bash

OUTLOG="testout.log"

n=50 #numero di client da lanciare

# reset dei file
echo -n "" > $OUTLOG


# avvio test
for ((i=0; i<n; i++)); do
  ./client "fausto$i" "1" &>> $OUTLOG &
  PID[$i]=$!
done


for pid in ${PID[*]}; do
wait $pid
done


for ((i=0; i<n; i++)); do
  if [ $i -lt 30 ]; then
    ./client "fausto$i" "2" &>> $OUTLOG &
  else
    ./client "fausto$i" "3" &>> $OUTLOG &
  fi
  
  PID[$i]=$!
done

for pid in ${PID[*]}; do
wait $pid
done

#avvio la lettura del file di log
./testsum.sh