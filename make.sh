#!/bin/bash

if [ -z $1 ]
then
	echo "1. Deploy"
	echo "2. Make"
	echo "3. Rebuild"
	echo "4. Make&test"

	read answer
else
	answer=$1
fi

#timestamp() {
#  date +%Y%m%d_%H%M%s
#}

case $answer in
    1) echo "Deploy"
       git pull
       ;;

    2)  echo "Make"
  	    if [ ! -d build ]; then
    		mkdir ./build
		fi
    	cd ./build
    	make -j4
        ;;

    3)  echo "Rebuild"
  	    if [ ! -f build ]; then
    		mkdir ./build
		fi
		rm -rf ./build/*
    	cd ./build
    	cmake ..
    	make -j4
        ;;
    4)  echo "Make&test"
  	    if [ ! -d build ]; then
    		mkdir ./build
		fi
    	rm ./build/tests/connector
    	cd ./build
    	make -j4
    	cd ..
    	./build/tests/connector
        ;;

esac
