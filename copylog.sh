#!/bin/sh
if [ ! -d serverlogs ]; then
	echo "serverlogs directory doesn't exist -- creating one";
	mkdir serverlogs;
fi

#if [ $# -ne 1 ]; then
#	echo "must provide destination file name for serverlogs/{file-name}";
#	exit 1;
#fi


FN=`date +%Y%m%d-%H`.log

scp acm@172.25.172.52:~/deploy/scripts/stem.log serverlogs/$FN
if [ $? -eq 0 ]; then
	echo "file copied from server successfully";
else 
	echo "log copy from vpn has failed";
	exit 1;	
fi

tar cvzf serverlogs/$1.tar.gz serverlogs/$FN;
