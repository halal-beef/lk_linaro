#!/bin/bash

case $1 in
	erd3830)
		rm -rf build-$1; make $1 -j16
		;;
	*)
		echo "-----------------------------------------------------------------"
		echo % usage : ./build.sh [board name] [none / user]
		echo % user mode is not entering ramdump mode when problem happened.
		echo % board list
		echo erd3830
		echo "-----------------------------------------------------------------"
		exit 0
		;;
esac
