#!/bin/bash


#if [ $# -eq 0 ]; then
#echo 'keys *:*' | redis-cli | sed 's/^/get /' | redis-cli >file.csv
#exit
#fi

user_list=""
output_file="file.csv"
ip="127.0.0.1"
port=6379

while getopts "h?f:u:i:p:" opt; do
    case "$opt" in
    help|\?)
        echo "Usage: $0 -f -u -i -p"
	echo "-f filepath"
        echo "-u userlist e.g ROOT1,ROOT2,ROOT3"
        echo "-i ip [optional default 6379]"
        echo "-p port [optional default 127.0.0.1]"
        exit 0
        ;;
    u)  user_list=$OPTARG
        ;;
    f)  output_file=$OPTARG
        ;;
    i)  ip=$OPTARG
        ;;
    p)  port=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

echo "output_file=$output_file,user_list=$user_list,ip='$ip',port='$port'"

echo "Copying following user's positions from redis to file : $user_list"
if [ -z "$user_list" ]
then
  echo "user list is null."
  echo 'keys *:*' | redis-cli -h $ip -p $port| sed 's/^/get /' | redis-cli -h $ip -p $port
  echo 'RecordType,AccountName,SymbolorUnderlier,Root,ExpirationDate,Strike,Quantity,PutorCall,Price'>${output_file}
   echo 'keys *:*' | redis-cli -h $ip -p $port| sed 's/^/get /' | redis-cli -h $ip -p $port>>${output_file}
   #to discard 11 and 12 collumn of the positions file
   cut --complement -f 11-12 -d, ${output_file} > temp.csv
   mv temp.csv ${output_file}
else
  echo 'RecordType,Reserved,AccountName,SymbolorUnderlier,Root,ExpirationDate,Strike,Quantity,PutorCall,Price'>${output_file}
  IFS=',' read -ra ADDR <<< "$user_list"
  for i in "${ADDR[@]}"; do
    echo 'keys '${i}':*' | redis-cli -h $ip -p $port| sed 's/^/get /' | redis-cli -h $ip -p $port>>${output_file}
    echo "Redis data written to file for user '$i'"
  done
  #to discard 11 and 12 collumn of the positions file
  cut --complement -f 11-12 -d, ${output_file} > temp.csv
  mv temp.csv ${output_file}
fi


#if [ $# -gt 0 ]; then
#filename=$1
#for arg in "$@" ; do
# if [[ "$arg" = "$0" ]] ; then
#        echo 'skiping oth\n'
# fi
# if [[ "$arg" = "$1" ]] ; then
#	echo 'RecordType,AccountName,SymbolorUnderlier,Root,ExpirationDate,Strike,Quantity,PutorCall,Price'>${filename}
# else
#	echo 'keys '${arg}':*' | redis-cli | sed 's/^/get /' | redis-cli >>${filename}
#fi
#done

#else
#echo 'keys *:*' | redis-cli | sed 's/^/get /' | redis-cli >file.csv
#fi
