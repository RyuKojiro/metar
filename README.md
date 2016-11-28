# Usage

This utility fetches NOAA weather information from reporting stations that have an ICAO identifier.

## Synopsis
`metar [-dt] station_id [...]`

* `-d` Show decoded METAR output
* `-t` Show TAFs where available

## Examples

### Standard METARs

```
% metar ksfo rjaa
2016/11/28 05:56
KSFO 280556Z 28004KT 10SM BKN050 BKN060 OVC150 13/07 A3011 RMK AO2 SLP196 T01280072 10133 20117 53001 $
2016/11/28 06:30
RJAA 280630Z 03005KT 360V060 9999 FEW030 13/04 Q1014 NOSIG
```

### METARs and TAFs

```
% metar -t ksfo 
2016/11/28 06:56
KSFO 280656Z 16006KT 10SM FEW035 BKN050 OVC150 13/08 A3010 RMK AO2 SLP193 T01280078 $
2016/11/28 06:24
TAF KSFO 280520Z 2806/2912 25010KT P6SM FEW025 BKN200 
     FM280800 21010KT P6SM -RA OVC025 
     FM281100 27014G19KT P6SM -SHRA OVC030 
     FM281400 29012KT P6SM BKN050 
     FM282000 30014KT P6SM SCT050 
     FM290400 31008KT P6SM FEW020
```

### Decoded METARs

```
% metar -d rjaa
New Tokyo Inter-National Airport, Japan (RJAA) 35-46N 140-23E 44M
Nov 28, 2016 - 01:30 AM EST / 2016.11.28 0630 UTC
Wind: from the NNE (030 degrees) at 6 MPH (5 KT) (direction variable):0
Visibility: greater than 7 mile(s):0
Sky conditions: mostly clear
Temperature: 55 F (13 C)
Dew Point: 39 F (4 C)
Relative Humidity: 54%
Pressure (altimeter): 29.94 in. Hg (1014 hPa)
ob: RJAA 280630Z 03005KT 360V060 9999 FEW030 13/04 Q1014 NOSIG
cycle: 6
```

# Building & Installation

## All systems
The system specific instructions are based on these steps, which will work for any system.

1. Get libcurl
2. `make` with `INCLUDE` and `LDFLAGS` set to where libcurl is installed
3. `make install` with `PREFIX` set to the desired install path

## OS X
1. libcurl comes with the OS X SDK
2. `make`
3. `sudo make install`

## FreeBSD
1. `pkg install curl` or `cd /usr/ports/ftp/curl && make install`
2. `make INCLUDE=-I/usr/local/include LDFLAGS=-L/usr/local/lib`
3. `make PREFIX=/usr install` as superuser

## NetBSD
1. `pkg_add curl` or `cd /usr/pkgsrc/www/curl && make install`
2. `make INCLUDE=-I/usr/pkg/include LDFLAGS=-L/usr/pkg/lib`
3. `make PREFIX=/usr install` as superuser