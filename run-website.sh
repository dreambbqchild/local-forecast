#!/bin/bash
CURRENT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
cd $CURRENT_DIR

/home/paige/.nvm/versions/node/v18.4.0/bin/node index.js