#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    me=`basename "$BASH_SOURCE"`
    echo "Usage: $me machine benchmark [dependencies..]"
	cat <<EOF

Boot a Barrelfish rack machine with Linux, run a benchmark, wait for
completion and shutdown again.

Dependencies are going to be ssh'ed on the machine along with the
benchmark.
EOF


    exit 1
}

PID=-1

# Trap CTRL + C for cleanup
function ctrl_c() {
	set +x
    echo "** Trapped CTRL-C .. "
	
	if [[ $PID -gt 0 ]]; then
		echo "Killing bfrack"
		kill $PID
	fi
    exit 1
}

trap ctrl_c INT
set +x

function ensure_machine_free() {
	INUSE=$(bfrack console -w $1)
	echo $INUSE | grep '@'
	if [[ $? -eq 0 ]]; then
		echo "Someone is using the machine"
		
		# Someone is using the machine, make sure it's us
		echo "$INUSE" | grep $(whoami)'@'
		[[ $? -eq 0 ]] || error "Machine is used by someone else"
	fi
}

[[ -n "$2" ]] || usage

MACHINE=$1; shift
TMP=$(mktemp)
SSHHOST=ubuntu@$MACHINE.in.barrelfish.org

ensure_machine_free $MACHINE
BOOT=0

ssh -q -o ConnectTimeout=1 $SSHHOST hostname
if [[ $? -ne 0 ]]; then
	echo "Machine $MACHINE does not seem to be running Linux, rebooting"
	read -p "Press [Enter] to reboot machine..."
	bfrack power -d $MACHINE
	bfrack boot -l wily -c $MACHINE &>$TMP &
	PID=$?
	BOOT=1
	
	echo "bfrack PID = $PID - output $TMP"
	sleep 5

else
	echo "Machine is already running Linux, not rebooting"
fi	

# Wait for machine to boot up
echo "Waiting for machine to come up"
SPID=1
while [[ $SPID -ne 0 ]]; do
	ssh -q -o ConnectTimeout=1 $SSHHOST hostname
	SPID=$?
done

# Install packages
[[ $BOOT -eq 1 ]] && scripts/rack-install.sh $MACHINE

# Install dependencies
make # for libsync
scp $@ $SSHHOST:

ARGS=""

# Execute benchmark
BENCH=$1
if [[ $BENCH == "bench/pairwise_raw" ]]; then
	ARGS="-s"
fi
scripts/run_machine.sh $ARGS -b $BENCH $MACHINE adaptivetree bintree cluster mst badtree fibonacci; RC=$?
echo "Return code is $RC"

#bfrack power -d $MACHINE
[[ $PID -ge 0 ]] && kill $PID

rm -f $TMP
