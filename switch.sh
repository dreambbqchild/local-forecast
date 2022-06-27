#!/bin/bash
CURRENT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
TIME_STAMP=$(date -u +%H -d '-1 hour')
OFFSET=$(($TIME_STAMP % 6))

if [ $OFFSET -eq 0 ]; then
	$CURRENT_DIR/getData48.sh
else
	$CURRENT_DIR/getData18.sh
fi

cd $CURRENT_DIR/build
./local-forecast
