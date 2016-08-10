#!/bin/bash

function error() {
	echo "ERROR" $1
	exit 1
}

## Copyright header as inserted by add_header
## --------------------------------------------------
COPYRIGHT=$(cat <<EOF
/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
EOF
)

## Add a standard C header to the beginning of the file
## --------------------------------------------------
##
## Overwrites given file
function add_header() {

	if [[ ${FIX} -ne 1 ]]; then
		echo "NOT attempting to fix -- set environment variable FIX=1"
		return
	fi

	f=$1
	[[ -f $f ]] || error "Given file $f does not exist"

	echo "Adding header to file $f"

	# Make sure file is under version control (in case we screw up)
	git ls-files $f --error-unmatch; RC=$?
	[[ $RC -eq 0 ]] || error "Cannot add header to $f, not tracked by git"

	TMP=$(mktemp)
	echo -e "${COPYRIGHT}" > $TMP
	cat $f | grep -v '#!' >> $TMP

	cp -b $TMP $f

	# Sanity check
	${GREP} 'Copyright'  $f || error "add_header: did not find header after inserting"
}

## Check given file for copyright/license header
## --------------------------------------------------
function check_file() {

	f=$1
	[[ -f $f ]] || error "Given file $f does not exist"

	GREP="grep -q"

	# Make sure file has a copyright header and mentions ETH
	${GREP} 'Copyright'  $f || add_header $f # First, dry to add header
	${GREP} 'Copyright'  $f || error "File $f does not have Copyright header"
	${GREP} 'ETH Zurich' $f || error "File $f does not contain ETH Zurich"

	# Make sure there is no MS entry in the header, this typically
	# means that the header has been recklessly copied from the
	# Barrelfish tree.
	${GREP} '[Mm]icrosoft' $f && error "File $f contain MS header"
	${GREP} 'Halden'       $f && error "File $f contain old address"

	return 0
}

FILES=$(find . -name '*.[ch]' -or -name '*.cpp')
BLACKLIST="shoal streamcluster contrib/ diss_bar/" # ignore files matching an of the patterns here. Separate by space

for f in $FILES; do

	# Check if file name matches any entry in blacklist
	# --------------------------------------------------
	SKIP=0
	for b in ${BLACKLIST}; do
		if (echo $f | grep -q $b); then
			SKIP=1
		fi
	done
	[[ $SKIP -eq 1 ]] && continue

	# Execute checks
	# --------------------------------------------------

	check_file $f
done

exit 0
