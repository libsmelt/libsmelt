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

	scripts/run.sh || exit 1

	exit 0
}

function run_build() {

	mkdir -p $APPSDIR

	cd $BASEDIR
	SMLTDIR=$(pwd)

	# Smelt - build the libraries in case they don't exists
	# ------------------------------
	make libsmltrt.so libsmltcontrib.so || exit 1

	cd $APPSDIR

	# Streamcluster
	# ------------------------------
	if [[ ! -d streamcluster ]]; then
	    # Clone via SSH - this does currently not work, since it
	    # is not public, but should eventually be possible and
	    # would be much cleaner
	    ## git clone ssh://vcs-user@code.systems.ethz.ch:8006/diffusion/SCSYNC/streamcluster-sync.git streamcluster
	    wget -O streamcluster.zip http://people.inf.ethz.ch/skaestle/static/streamcluster.zip
	    mkdir streamcluster
	    (cd streamcluster && unzip ../streamcluster.zip)
	fi
	( # Build Streamcluster
		cd streamcluster

		# Downlod the Shoal source-code
		if [[ ! -f shoal.zip ]]; then
			wget -O shoal.zip https://github.com/libshoal/shoal/archive/master.zip
			unzip shoal.zip
		fi

		# Download libshl.so - a pre-combiled library build from the github repository
		SHLLIB=shoal-master/shoal/
		if [[ ! -f $SHLLIB/libshl.so ]]; then
			(cd $SHLLIB && wget -O libshl.so http://people.inf.ethz.ch/skaestle/static/libshl.so)
		fi

		cd usr/streamcluster

		echo -e "Building Streamcluster .. in: "
		pwd

		# Downlod workload
		if [[ ! -f input_4k.txt ]]; then
			wget -O input_4k.txt http://people.inf.ethz.ch/skaestle/static/input_4k.txt
		fi

		# Link libshl.so
		ln -s -f ../../libshl.so .

		export LIBSMELT_BASE=$SMLTDIR
		export LIBSHOAL=../../shoal-master
		export SK_USE_SYNC=1
		export SK_USE_SHOAL=1
		export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
		make clean-fast
		make || exit 1

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
