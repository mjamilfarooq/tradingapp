#!/bin/bash

tput civis      -- invisible

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap stp QUIT
enable_output=true
function ctrl_c() {
      #  echo "** Trapped CTRL-C"
	echo ""
	tput cnorm   -- normal
	exit
}

function stp() {
        echo "** Trapped CTRL-"
	echo ""
	clear
	tput cnorm   -- normal
	#exit
}
indexselect=1

ip="127.0.0.1"
port=6379
user1=$1
user2=$2
#symbollist=$3
userselect=$3
indexselect=$4

 if [ $indexselect -eq 1 ]; then
indexselect=12
fi

 if [ $indexselect -eq 2 ]; then
indexselect=13
fi

 if [ $indexselect -eq 4 ]; then
indexselect=14
fi

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
#symbol list detection


#added for sorting
IFS=',' read -ra SYM <<< "$symbollist"
declare -A hashmap
 for j in "${SYM[@]}"; do
readvalue="$(echo get ''$userselect':'${j}'' | redis-cli -h $ip -p $port)"
IFS=',' read -ra READSTR <<< "$readvalue"
hashmap[${j}]=${READSTR[$indexselect]}

if [ $indexselect -eq 3 ]; then
hashmap[${j}]=$((${READSTR[12]}+${READSTR[13]}))
fi

done


val="$(for k in "${!hashmap[@]}"
do
    echo $k '#' ${hashmap["$k"]}';'
done |
sort -rn -k3)"
#echo $val
some_variable="$( echo "$val" | sed 's/.#.*;//g' )"
#echo $some_variable
symbollist="$( echo "$some_variable" | tr '\n' ',' )"
#echo 'final:'${symbollist}
#read input_variable
#added for sorting
IFS=',' read -ra ADDR <<< "$symbollist"
 for i in "${ADDR[@]}"; do
val1="$(echo get ''$user1':'${i}'' | redis-cli -h $ip -p $port)"
#val1=get ''$user1':'${i}'' | redis-cli -h $ip -p $port
val2="$(echo get ''$user2':'${i}'' | redis-cli -h $ip -p $port)"
IFS=',' read -r -a array <<< "$val1"
IFS=',' read -r -a array2 <<< "$val2"
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
#totallive1=0
#else
totallive1=`expr ${array[12]} + ${array[13]}`
#fi
totallive2=`expr ${array2[12]} + ${array2[13]}`
    echo ''$user1'-'$user2' :: '${i}' :'${array[12]}','${array[13]}','$totallive1','${array[14]}' : '${array2[12]}','${array2[13]}','$totallive2','${array2[14]}'                            ' 
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
#read input_variable
done

tput cnorm   -- normal
