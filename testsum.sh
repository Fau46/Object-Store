#!/bin/bash
OUT="testout.log"

for ((i=0; i<6; i++)) do
  tot[$i]=$((0));
  connect[$i]=$((0));
  disconnect[$i]=$((0));
  store[$i]=$((0));
  retrieve[$i]=$((0));
  delete[$i]=$((0));
done

connect[0]=$(grep -c 'Operazione connect' $OUT)     #numero di client che ha effettuato la connect
connect[1]=${connect[0]}                            #totale connect effettuate
connect[2]=$(grep -c 'Operazione connect: 1' $OUT); #connect andate a buon fine
connect[3]=$((${connect[1]}-${connect[2]}))         #connect fallite 
connect[5]=$((${connect[2]}*100/${connect[1]}));    #percentuale di connessioni andate a buon fine

disconnect[0]=$(grep -c 'Operazione disconnect' $OUT)     #numero di client che ha effettuato la disconnect
disconnect[1]=${disconnect[0]}                            #totale disconnect effettuate
disconnect[2]=$(grep -c 'Operazione disconnect: 1' $OUT); #disconnect andate a buon fine
disconnect[3]=$((${disconnect[1]}-${disconnect[2]}))      #disconnect fallite 
disconnect[5]=$((${disconnect[2]}*100/${disconnect[1]})); #percentuale di connessioni andate a buon fine


test1=$(grep 'TEST: 1' $OUT)
store[0]=$(wc -l <<< "$test1")             #numero di client che hanno fatto la store
while IFS=";" read -r ok ko tot; do
store[2]=$((${store[2]}+${ok##*:}))        #ok store
store[3]=$((${store[3]}+${ko##*:}))        #ko store
done <<< "$test1"
store[1]=$((${store[2]}+${store[3]}))      #store totali
store[5]=$((${store[2]}*100/${store[1]}))  #percentuale ok store

test2=$(grep 'TEST: 2' $OUT)
retrieve[0]=$(wc -l <<< "$test2")               #numero di client che hanno fatto la retrieve
while IFS=";" read -r ok ko wrong tot; do
  retrieve[2]=$((${retrieve[2]}+${ok##*:}))     #ok retrieve
  retrieve[3]=$((${retrieve[3]}+${ko##*:}))     #ko retrieve
  retrieve[4]=$((${retrieve[4]}+${wrong##*:}))  #wrong retrieve
done <<< "$test2"
retrieve[1]=$((${retrieve[2]}+${retrieve[3]}+${retrieve[4]}))  #retrieve totali
retrieve[5]=$((${retrieve[2]}*100/${retrieve[1]}))             #percentuale ok retrieve

test3=$(grep 'TEST: 3' $OUT)
delete[0]=$(wc -l <<< "$test3")               #numero di client che hanno fatto la delete
while IFS=";" read -r ok ko tot; do
  delete[2]=$((${delete[2]}+${ok##*:}))       #ok delete
  delete[3]=$((${delete[3]}+${ko##*:}))       #ko delete
done <<< "$test3"
delete[1]=$((${delete[2]}+${delete[3]}))      #delete totali
delete[5]=$((${delete[2]}*100/${delete[1]}))  #percentuale ok delete

tot[0]=$((${store[0]}+${retrieve[0]}+${delete[0]}))                                   #totale client lanciati
tot[1]=$((${store[1]}+${retrieve[1]}+${delete[1]}+${connect[1]}+${disconnect[1]}))    #totale operazioni effettuate
tot[2]=$((${store[2]}+${retrieve[2]}+${delete[2]}+${connect[2]}+${disconnect[2]}))    #totale operazioni andate a buon fine
tot[3]=$((${store[3]}+${retrieve[3]}+${delete[3]}+${connect[3]}+${disconnect[3]}))    #totale operazioni fallite
tot[4]=$((${retrieve[4]}))                                                            #totale retrieve errate
tot[5]=$((${tot[2]}*100/${tot[1]}))                                                   #percentuale ok generale

clear #pulisco lo stdout

echo "*****************************************************************"
echo "*                                                               *"
echo "*                     Statistiche testum.sh                     *"  
echo "*                                                               *"
echo "*****************************************************************"
echo

echo "************* CLIENT ** OP. *** OK **** KO **** WRONG ** % ******"
echo -e "* Store      \t${store[0]} \t${store[1]} \t${store[2]} \t${store[3]} \t / \t${store[5]}% \t* \n";
echo -e "* Retrieve   \t${retrieve[0]} \t${retrieve[1]} \t${retrieve[2]} \t${retrieve[3]} \t${retrieve[4]} \t${retrieve[5]}% \t* \n";
echo -e "* Delete     \t${delete[0]} \t${delete[1]} \t${delete[2]} \t${delete[3]} \t / \t${delete[5]}% \t* \n";
echo -e "* Connect    \t${connect[0]} \t${connect[1]} \t${connect[2]} \t${connect[3]} \t / \t${connect[5]}% \t* \n";
echo -e "* Disconnect \t${disconnect[0]} \t${disconnect[1]} \t${disconnect[2]} \t${disconnect[3]} \t / \t${disconnect[5]}% \t* \n";
echo -e "* Totale      \t${tot[0]} \t${tot[1]} \t${tot[2]} \t${tot[3]} \t${tot[4]} \t${tot[5]}% \t* \n";
echo "*****************************************************************"

killall -USR1 server
killall -SIGTERM server