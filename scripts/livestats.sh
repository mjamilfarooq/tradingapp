#!/bin/sh
trap ctrl_c INT
idtoexit=0
tput civis      -- invisible
function ctrl_c() {
      #  echo "** Trapped CTRL-C"
	echo "Exiting ..."
	tput cnorm   -- normal
	kill $idtoexit	
	exit
}

positionRecords (){
#echo $1
#echo $2
#clear
tput reset
#positionRecordsNum="$(echo 'LRANGE '$1';'$2' 0 -1' | redis-cli -h $ip -p $port)"
#echo $positionRecordsNum
echo 'average_at_open,open_order_id,open_price,open_size,average_at_close,close_order_id,close_price,close_size,pnl,sign,is_complete'
echo $1
echo 'LRANGE '$1';'$2' 0 -1' | redis-cli -h $ip -p $port
echo ""
echo $3
echo 'LRANGE '$3';'$2' 0 -1' | redis-cli -h $ip -p $port
sleep 4
#for i in {1..71}
#do 
#printf "\033[A" 
#done
#tput cup 0 0
}

statsOrders ()
{ 
rediskeys="$(echo keys ''$user1',*' | redis-cli -h $ip -p $port)" #| wc -l

# first user
#echo ' '
#echo ' '
echo '	'$user1':                                      '
IFS=' ' read -r -a array <<< "$rediskeys"
#echo 'keys '$user',*' | redis-cli -h $ip -p $port| sed 's/^/get /' | redis-cli -h $ip -p $port
#for i in "${rediskeys[@]}"; do
#echo ''$i' \n'
#done
stringarray=($rediskeys)
for i in "${stringarray[@]}"
do
#echo ''$i' \n'
redisVal="$(echo get ''$i'' | redis-cli -h $ip -p $port)"
#echo ''$i' '$redisVal''
IFS=',' read -r -a splitarray <<< "$redisVal"
if [ -z "${splitarray[4]}" ] && [ -z "${splitarray[7]}" ] && [ "${splitarray[0]}" == "$redissymbol" ]
then
#echo ${splitarray[4]}
echo '	'$redisVal''
#read input
fi
#echo ' '
done
# ......................................


#second user
rediskeys2="$(echo keys ''$user2',*' | redis-cli -h $ip -p $port)"
#echo ' '
#echo ' '
echo '	'$user2':                                      '
IFS=' ' read -r -a array2 <<< "$rediskeys2"
stringarray2=($rediskeys2)
for i in "${stringarray2[@]}"
do
redisVal2="$(echo get ''$i'' | redis-cli -h $ip -p $port)"
IFS=',' read -r -a splitarray2 <<< "$redisVal2"
if [ -z "${splitarray2[4]}" ] && [ -z "${splitarray2[7]}" ] && [ "${splitarray2[0]}" == "$redissymbol" ]
then
echo '	'$redisVal2''
fi
done
echo '                                                                         '
# ......................................
}

stats ()
{ 
#symbols list detection
if [ "$enable_auto_symbol_list" = true ] ; then
#for auto symbol list
keys1="$(echo keys ''$user1':*' | redis-cli -h $ip -p $port)"
keys2="$(echo keys ''$user2':*' | redis-cli -h $ip -p $port)"
#echo 'account 1 keys :: '$keys1''
#echo 'account 2 keys :: '$keys2''
temp1="$(echo $keys1 | sed 's/'$user1'://g')"
temp2="$(echo $keys2 | sed 's/'$user2'://g')"
temp3=''$temp1' '$temp2''
#echo $temp1
#echo $temp2
#echo 'joining list'
#echo $temp3

#echo 'sorting list'
IFS=' ' read -ra KEYLIST <<< "$temp3"
sorted_unique_ids=$(echo "${KEYLIST[@]}" | tr ' ' '\n' | sort -u | tr '\n' ',')
#echo $sorted_unique_ids
last="$(echo "${sorted_unique_ids: -1}")"
#echo $last
if [ "$last" == "," ]; then
  #sorted_unique_ids=${sorted_unique_ids::-1}
size=${#sorted_unique_ids} 
size=$((size-1))
#echo ${sorted_unique_ids:0:$size}
symbollist="$(echo ${sorted_unique_ids:0:$size})"
fi
#echo 'symbol list '$symbollist''
#read input_variable
#for auto symbol list
fi
#symbols list detection

IFS=',' read -ra ADDR <<< "$symbollist"
 for i in "${ADDR[@]}"; do
val1="$(echo get ''$user1':'${i}'' | redis-cli -h $ip -p $port)"
#val1=get ''$user1':'${i}'' | redis-cli -h $ip -p $port
val2="$(echo get ''$user2':'${i}'' | redis-cli -h $ip -p $port)"
val3="$(echo LRANGE ''$pair'#'${i}'' -1 -1 | redis-cli -h $ip -p $port)"
IFS=',' read -r -a array <<< "$val1"
IFS=',' read -r -a array2 <<< "$val2"
IFS=',' read -r -a array3 <<< "$val3"
if [ -z "${array[12]}" ]
then
	array[12]=0
fi
if [ -z "${array[13]}" ]
then
	array[13]=0
fi
if [ -z "${array2[12]}" ]
then
	array2[12]=0
fi
if [ -z "${array2[13]}" ]
then
	array2[13]=0
fi
if [ -z "${array[14]}" ]
then
	array[14]=0
fi
if [ -z "${array2[14]}" ]
then
	array2[14]=0
fi
if [ -z "${array[7]}" ]
then
	array[7]=0
fi
if [ -z "${array[9]}" ]
then
	array[9]=0.0000
fi
if [ -z "${array2[7]}" ]
then
	array2[7]=0
fi
if [ -z "${array2[9]}" ]
then
	array2[9]=0.0000
fi
if [ "${array3[1]}" == "TRIGGER_STATE_POSITIVE" ] 
then
  sign="+ve"
elif [ "${array3[1]}" == "TRIGGER_STATE_NEGATIVE" ]
then
  sign="-ve"
else
  sign=" "
fi
#totallive1=0
#else
totallive1=`expr ${array[12]} + ${array[13]}`
#fi
totallive2=`expr ${array2[12]} + ${array2[13]}`
symbollength=${#i} 
if [ $symbollength -eq 2 ]
then
i=$i' '
fi
    echo ''$pair'('$user1'-'$user2')::'${i}':'${sign}','${array3[2]}'('${array3[3]}','${array3[4]}','${array3[5]}','${array3[6]}','${array3[7]}') '${array[7]}','${array[9]}','${array[12]}','${array[13]}','$totallive1','${array[14]}' : '${array2[7]}','${array2[9]}','${array2[12]}','${array2[13]}','$totallive2','${array2[14]}'                                                                                 ' 
if [ "${i}" == "${redissymbol}" ] && [ $1 = true ]
then
statsOrders
fi  
done
#date
#echo 2*$i
#i=$((i+1))
IFS=',' read -ra APPR <<< "$symbollist"
for i in "${APPR[@]}"; do
    printf "\033[A" 
   
  done
sleep 0.5
#printf "\033[A"
#printf "\033[A"
tput cup 3 0
#read input_variable
} # Function declaration must precede call.

clear
ip="127.0.0.1"
port=6379
pair=$1
user1=$2
user2=$3
symbollist=$4
enable_auto_symbol_list=true
echo "Pair '$pair'"
echo "Account 1 '$user1'" 
echo "Account 2 '$user2'"
symbolcount=0
redissymbol=""

status="normal"
if [ $# -lt 3 ]; then
echo 'Help'
echo "Usage: $0 PairName Account1 Account2"
echo "To switch to detailed live order mode press S"
echo "Enter the name of the symbol and press enter"
echo "To switch back to normal mode press B"
echo "To switch symbol in detailed live order mode press V"
exit
fi

while true;
do


case $status in
normal)
#echo "in false"

clear
echo "Pair '$pair'"
echo "Account 1 '$user1'" 
echo "Account 2 '$user2'"

while [ 1 ]
do
stats false
done &

main=$!
#echo "# main is $main" >&2
idtoexit=$main

# livinfree's neat dd trick from that other thread vino pointed out
#tput smso
#echo "Press any key to return \c"
#tput rmso
result=""
while [ "$result" != "s" ] && [ "$result" != "p" ];
do
oldstty=`stty -g`
stty -icanon -echo min 1 time 0
result=`dd bs=1 count=1 2>/dev/null` 
stty "$oldstty"
#echo $result
done
#/home/akela/Desktop/new.sh $main
kill $main
wait $main 2>/dev/null



if [ "$result" == "s" ] 
then
status="symbol"
fi

if [ "$result" == "p" ] 
then
status="positionRecord"
fi
;;





symbol)
#echo "in true"
clear
echo "Enter the symbol:"
read str
redissymbol="$(echo "${str^^}")"
echo 'Symbol read is:'$redissymbol''
clear
echo "Pair '$pair'"
echo "Account 1 '$user1'" 
echo "Account 2 '$user2'"


while : ; do
stats true 
done &

main2=$!
#echo "# main is $main2" >&2
idtoexit=$main2

# livinfree's neat dd trick from that other thread vino pointed out
#tput smso
#echo "Press any key to return \c"
#tput rmso
result2=""
while [ "$result2" != "b" ] && [ "$result2" != "r" ];
do
oldstty=`stty -g`
stty -icanon -echo min 1 time 0
result2=`dd bs=1 count=1 2>/dev/null` 
stty "$oldstty"
#echo $result2
done

kill $main2
wait $main2 2>/dev/null


if [ "$result2" == "b" ] 
then
status="normal"
fi
if [ "$result2" == "r" ] 
then
status="symbol"
fi
;;
orange|tangerine)
        echo $'Eeeks! I don\'t like those!\nGo away!'
        exit 1
;;
positionRecord)
clear
#echo "Enter the user:"
#read pos_acc
echo "Enter the symbol:"
read pos_sym
Capitalized_pos_sym="$(echo "${pos_sym^^}")"
while : ; do
positionRecords $user1 $Capitalized_pos_sym $user2
done &

main3=$!
#echo "# main is $main3" >&2
idtoexit=$main3

result3=""
while [ "$result3" != "b" ] && [ "$result3" != "r" ];
do
oldstty=`stty -g`
stty -icanon -echo min 1 time 0
result3=`dd bs=1 count=1 2>/dev/null` 
stty "$oldstty"
#echo $result
done
#/home/akela/Desktop/new.sh $main3
kill $main3
wait $main3 2>/dev/null
if [ "$result3" == "b" ] 
then
status="normal"
fi
if [ "$result3" == "r" ] 
then
status="positionRecord"
fi
;;
*)
echo "Unknown Command?"
esac

done
