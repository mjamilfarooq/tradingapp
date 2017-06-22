#!/bin/bash



echo "Starting log $1 processing -- please wait"

oddstat(){

file=$3
grep -E "$1.*OnOrder" $2 >  $file
lines=`cut -d" " -f9 $file`

total=0
canceled=0
changed=0
error=0
executed=0

for i in $lines; do
	case $i in
	0) 
		total=$(($total+1));
		;;
	3)
		executed=$(($executed+1));
		;;
	4)
		canceled=$(($canceled+1));
		;;
	16)
		changed=$(($changed+1));
		;;
	233|255|245)
		error=$(($error+1));
		;;
	esac
done

echo "Order Info($1): Sent: $total Executed: $executed Canceled: $canceled Changed: $changed Error: $error"

}

oddexec(){
exec_file=`echo $1|sed 's/file/exec/g'`
cat $1|sed "s/\]\[/ /g" |cut -d" " -f2,9-|sort -k 2 > $exec_file

orders=( `cat $exec_file|cut -d" " -f2|uniq -c` )

i="1";
size=${#orders[@]}
#echo $size $exec_file
while [ $i -lt $size ]; do 
	nrecords=${orders[$i-1]};
	id="${orders[$i]}"
	order_info=( `grep "$id" $exec_file` )
	j="0";

	declare -a order_status
	price="${order_info[3]}"
	quantity="${order_info[4]}"
	acc_type=$2
	case "${order_info[5]}" in
	0)
		type="BUY"
		;;
	1)	
		type="SEL"
		;;
	4)
		type="SSL"
		;;
	*)
		type="ERR"
		;;
	esac


	ack="00:00:00.000000"
	rec="00:00:00.000000";
	exe="00:00:00.000000";
	canc="00:00:00.000000";
	chan="00:00:00.000000";
	err="00:00:00.000000";
	othr="00:00:00.000000";
	while [ $j -lt $nrecords ]; do
		time="${order_info[$(($j*6+0))]}"
		status="${order_info[$(($j*6+2))]}"		
		
		case $status in
		10)
			ack=$time
			;;
		0)	
			rec=$time
			;;
		3)	
			exe=$time
			;;
		4)
			canc=$time
			;;
		16)
			chan=$time
			;;
		255|245|233)
			err=$time
			ack=$time
			;;
		*)
			othr=$time
			;;
		esac
		j=$(($j+1))
	done
	echo -e $ack $id $acc_type $price $quantity $type $rec $exe $canc $chan $err	
	i=$(($i+2))
done  

}

triginfo(){
#grep "TRIGGER CHANGED" $1|sed "s/\]\[/ /g"|awk '/\,\+\,/ {print $2 " POS"} /,-,/ {print $2 " NEG"}'
#grep "selecting case" $1|sed "s/\]\[/ /g"|cut -d" " -f2 
grep "selecting case" $1|sed "s/\]\[/ /g"|awk '/\,\+\,/ {print $2 " POS " $NF} /\,\-\,/ {print $2 " NEG " $NF}'
}

sfile="/tmp/short-file.log"	#temporary file to store short order records
lfile="/tmp/long-file.log"	#temporary file to store short order records

oddstat short $1 $sfile
oddstat long $1 $lfile

oddexec $sfile short
oddexec $lfile long

triginfo $1
