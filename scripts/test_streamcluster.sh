#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    echo "Usage: $0 <action>"
	echo ""
	echo "action:"
	echo " clean: cleanup previous test-runs"
	echo " compile: compile Streamcluster"
	echo " test: run the test"
    exit 1
}

BASEDIR="$(dirname $0)/.."
APPSDIR="$BASEDIR/apps"

function clean() {

	rm -rf $APPSDIR
}

function run_test() {
	echo "Running tests .. "

	cd $BASEDIR
	SMLTDIR=$(pwd)

	cd $APPSDIR/streamcluster/usr/streamcluster

	export LIBSMELT_BASE=$SMLTDIR
	export LIBSHOAL=../../shoal-master/shoal/
	export SK_USE_SYNC=1
	export SK_USE_SHOAL=1
	export LD_LIBRARY_PATH=.:$LIBSMELT_BASE:$LIBSHOAL:$LD_LIBRARY_PATH

	export SMLT_HOSTNAME=sgv-skaestle-01

	scripts/run.sh
}

function run_build() {

	mkdir -p $APPSDIR

	cd $BASEDIR
	SMLTDIR=$(pwd)

	# Smelt - build the libraries in case they don't exists
	# ------------------------------
	make libsmltrt.so libsmltcontrib.so

	cd $APPSDIR

	# Streamcluster
	# ------------------------------
	if [[ ! -d streamcluster ]]; then
	    # Clone via SSH - this does currently not work, since it
	    # is not public, but should eventually be possible and
	    # would be much cleaner
	    ## git clone ssh://vcs-user@code.systems.ethz.ch:8006/diffusion/SCSYNC/streamcluster-sync.git streamcluster
	    wget -O streamcluster.zip http://people.inf.ethz.ch/skaestle/static/streamcluster.zip
	    unzip streamcluster.zip
	fi
	( # Build Streamcluster
		cd streamcluster

		# Copy libraries - prevents Streamcluster's Makefile from rebuilding them
		cp $SMLTDIR/libsmltrt.so .
		cp $SMLTDIR/libsmltcontrib.so .

		# Download libshl.so - a pre-combiled library build from the github repository
		if [[ ! -f libshl.so ]]; then
			wget -O libshl.so http://people.inf.ethz.ch/skaestle/static/libshl.so
		fi

		# Downlod the Shoal source-code
		if [[ ! -f shoal.zip ]]; then
			wget -O shoal.zip https://github.com/libshoal/shoal/archive/master.zip
			unzip shoal.zip
		fi

		cd usr/streamcluster

		echo -e "Building Streamcluster .. in: "
		pwd

		# Link libshl.so
		ln -s -f ../../libshl.so .

		export LIBSMELT_BASE=$SMLTDIR
		export LIBSHOAL=../../shoal-master
		export SK_USE_SYNC=1
		export SK_USE_SHOAL=1
		export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
		make clean-fast
		make

	)
}

[[ -n "$1" ]] || usage

if [[ "$1" == "clean" ]]; then
	echo "Cleaning up .."
	clean
	exit 0
fi

if [[ "$1" == "build" ]]; then
	echo "Running builds .. "
	run_build
	exit 0
fi

if [[ "$1" == "test" ]]; then
	echo "Running tests .. "
	run_test
	exit 0
fi

echo "No action set, exiting"
exit 1
