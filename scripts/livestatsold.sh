#!/bin/bash
clear
tput civis      -- invisible

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

function ctrl_c() {
      #  echo "** Trapped CTRL-C"
	echo "Exiting ..."
	tput cnorm   -- normal
	exit
}
ip="127.0.0.1"
port=6379
pair=$1
user1=$2
user2=$3
symbollist=$4
enable_auto_symbol_list=true
echo "Account 1 '$user1'" 
echo "Account 2 '$user2'"
symbolcount=0
IFS=',' read -ra ADDR <<< "$symbollist"
 for i in "${ADDR[@]}"; do
    echo 'symbol '${i}'' 
    symbolcount=$((symbolcount+1))
  done
echo 'symbol count '${symbolcount}''


while [ 1 ]
do
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
    echo ''$pair'('$user1'-'$user2')::'${i}':'${sign}','${array3[2]}' '${array[7]}','${array[9]}','${array[12]}','${array[13]}','$totallive1','${array[14]}' : '${array2[7]}','${array2[9]}','${array2[12]}','${array2[13]}','$totallive2','${array2[14]}'                                                                                   ' 
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
done

tput cnorm   -- normal
