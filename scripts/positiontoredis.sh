#!/bin/bash

if [ $# -eq 0 ]; then
echo "Usage: $0 "
echo "-c [comma seperated list of symbols to clear entries from redis]"
echo "-f [filepath to read values from file to redis]"
exit
fi

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
input_file=""
isclear=0
isall=0
clear_vr=""
ip="127.0.0.1"
port=6379
while getopts "h?c:i:p:f:" opt; do
    case "$opt" in
    help|\?)
        echo "Usage: $0 -c -f -i -p"
        echo "-f filepath containing positions"
        echo "-c to clear positions of certain symbols from redis eg AAPL,AAB"
	echo "-c -a to clear positions of all symbols from redis "
        echo "-i ip [optional default 6379]"
        echo "-p port [optional default 127.0.0.1]"
        exit 0
        ;;
    c)  clear_vr=$OPTARG
	isclear=1
	;;
    f)  input_file=$OPTARG
        ;;
    a)  isall=1
        ;;
    i)  ip=$OPTARG
        ;;
    p)  port=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

echo "clear=$isclear,clear_vr=$clear_vr, all=$isall, input_file='$input_file' ,ip='$ip',port='$port'"

if [ "$isclear" = 1 ]; then

if [ "$clear_vr" = "-a" ]; then
echo "Deleting all positions in redis "
echo 'keys *:*' | redis-cli -h $ip -p $port| sed 's/^/del /' | redis-cli -h $ip -p $port #clears positions
echo 'keys *,*' | redis-cli -h $ip -p $port| sed 's/^/del /' | redis-cli -h $ip -p $port #clears order info
echo 'keys *#*' | redis-cli -h $ip -p $port| sed 's/^/del /' | redis-cli -h $ip -p $port #clears trade case info
else
echo "Deleting symbol list $clear_vr"
IFS=',' read -ra ADDR <<< "$clear_vr"
for i in "${ADDR[@]}"; do
    echo 'keys *:'$i'' | redis-cli -h $ip -p $port| sed 's/^/del /' | redis-cli -h $ip -p $port
    #echo "$i"
done

fi
exit
fi

if [ -z "$input_file" ]
then
  echo "input file path is null."
else
  #echo "'$input_file' is NOT null."
	while IFS= read -r line; do
  	echo "Reading line : $line"
	IFS=', ' read -r -a array <<< "$line"
	echo "User  : ${array[2]}"
	echo "Symbol: ${array[3]}"
	c=${array[2]}':'${array[3]}
	#echo $c
	echo 'set '${c}' '${line}'' | redis-cli -h $ip -p $port
	done < "$input_file"
fi     
