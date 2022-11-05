#!/bin/bash
CURRENT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")

export OPENSSL_CONF="$CURRENT_DIR/openssl.cnf"

TIME_STAMP=$(date -u +%H -d '-1 hour')
OFFSET=$(($TIME_STAMP % 6))
TIME_STAMP=$(date -u +%Y%m%d%H -d "-$((1 + $OFFSET )) hour")
HOUR=${TIME_STAMP:8:2}
TIME_STAMP=${TIME_STAMP:0:8}

for i in {2..48}
do
    INDEX=$(printf "%02d" $i)
    curl "https://nomads.ncep.noaa.gov/cgi-bin/filter_hrrr_2d.pl?file=hrrr.t${HOUR}z.wrfsfcf$INDEX.grib2&all_lev=on&all_var=on&subregion=&leftlon=-93.550505&rightlon=-92.852469&toplat=45.184381&bottomlat=44.468505&dir=%2Fhrrr.$TIME_STAMP%2Fconus" -o  $CURRENT_DIR/data/hrrr-$INDEX.grib2 &
done
wait
