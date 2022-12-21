#!/bin/bash
./Release/local-forecast -gribFilePath ./data/gfs -forecastPath ./forecasts/gfs -model GFS -skipToGribNumber 0 -maxGribIndex 384 -useCache