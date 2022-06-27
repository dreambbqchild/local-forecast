#!/bin/bash
rm ~/projects/local-forecast/build/data/*.grib2 2> /dev/null

export OPENSSL_CONF="$(dirname "$(readlink -f "$BASH_SOURCE")")/openssl.cnf"

TIME_STAMP=$(date -u +%H -d '-1 hour')
OFFSET=$(($TIME_STAMP % 6))
TIME_STAMP=$(date -u +%Y%m%d%H -d "-$((1 + $OFFSET )) hour")
HOUR=${TIME_STAMP:8:2}
TIME_STAMP=${TIME_STAMP:0:8}

for i in {0..48}
do
    INDEX=$(printf "%02d" $i)
    curl "https://nomads.ncep.noaa.gov/cgi-bin/filter_hrrr_2d.pl?file=hrrr.t${HOUR}z.wrfsfcf$INDEX.grib2&all_lev=on&all_var=on&subregion=&leftlon=-93.550505&rightlon=-92.852469&toplat=45.184381&bottomlat=44.468505&dir=%2Fhrrr.$TIME_STAMP%2Fconus" -o  ~/projects/local-forecast/build/data/hrrr-$INDEX.grib2 &
    #curl "https://nomads.ncep.noaa.gov/cgi-bin/filter_hrrr_2d.pl?file=hrrr.t${HOUR}z.wrfsfcf$INDEX.grib2&lev_10_m_above_ground=on&lev_2_m_above_ground=on&lev_surface=on&lev_mean_sea_level=on&lev_entire_atmosphere=on&lev_top_of_atmosphere=on&var_CAPE=on&var_CFRZR=on&var_CRAIN=on&var_CSNOW=on&var_DPT=on&var_LTNG=on&var_GUST=on&var_PRATE=on&var_MSLMA=on&var_TCDC=on&var_TMP=on&var_UGRD=on&var_VGRD=on&var_VIS=on&var_WIND=on&subregion=&leftlon=-93.550505&rightlon=-92.852469&toplat=45.184381&bottomlat=44.468505&dir=%2Fhrrr.$TIME_STAMP%2Fconus" -o  ./data/hrrr-$INDEX.grib2
done
wait
