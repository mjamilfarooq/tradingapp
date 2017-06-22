#!/bin/sh

syntax_msg () {
	echo "syntax: compile.sh build-directory target-directory";
}

if [ "$#" -ne 2 ]; then
	syntax_msg;
	exit 1;
else 
	echo "build directory "$1" specified";
	echo "target directory "$2" specified ";

	if [ -d $2 ]; then
		echo "Deleting target directory";
		rm -rf $2 && echo "target directory successfully deleted";
	fi
fi

if [ -d $1 ]; then
	echo "build directory already exist -- do you want to remove it? ";
	read user;

	case  "$user" in
		yes|y|Y|YES)
		rm -rf "$1" && echo "$1 -- deleted successfully";
		mkdir "$1" && echo "creating new directory $1";
		;;
		
		no|n|N|NO)
		;;
		
		*)
		echo "exiting -->  incorrect selection -- select yes|no|y|n|YES|NO";
		exit 1
		;;
	esac
else
	mkdir $1
fi



cd $1 && echo "changing directory to $1";
cmake -DCMAKE_CXX_FLAGS=-pg -DCMAKE_INSTALL_PREFIX="../$2" -DCMAKE_BUILD_TYPE=Debug .. && make && make install && cd .. && tar cvzf "$2.tar.gz" $2;

if [ "$?" -ne 0 ]; then
	echo "STEM compilation has failed -- exiting";
	exit 1;
fi

