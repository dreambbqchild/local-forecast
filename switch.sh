#!/bin/bash
CURRENT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
TIME_STAMP=$(date -u +%H -d '-1 hour')
OFFSET=$(($TIME_STAMP % 6))

cd $CURRENT_DIR

if [ $OFFSET -eq 0 ]; then
	./getData48.sh
else
	./getData18.sh
fi

cd ./build
./local-forecast ./data ../forecasts