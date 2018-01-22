#!/bin/bash
#Скрипт создания xml файлов для highcharts
DIR="/home/pi/term_col"
XMLDIR="/var/www"
DBNAME="main"

#hourly
rrdtool xport -s now-4h -e now \
DEF:Sens1=$DIR/$DBNAME.rrd:Sens1:AVERAGE \
DEF:streed=$DIR/$DBNAME.rrd:streed:AVERAGE \
XPORT:Sens1:"Sens1" \
XPORT:streed:"streed" > $XMLDIR/temp4h.xml

#daily
rrdtool xport -s now-48h  -e now \
DEF:Sens1=$DIR/$DBNAME.rrd:Sens1:AVERAGE \
DEF:streed=$DIR/$DBNAME.rrd:streed:AVERAGE \
XPORT:Sens1:"Sens1" \
XPORT:streed:"streed" > $XMLDIR/temp48h.xml

#weekly
rrdtool xport -s now-1w  -e now \
DEF:Sens1=$DIR/$DBNAME.rrd:Sens1:AVERAGE \
DEF:streed=$DIR/$DBNAME.rrd:streed:AVERAGE \
XPORT:Sens1:"Sens1" \
XPORT:streed:"streed" > $XMLDIR/temp1w.xml

#monthly
rrdtool xport -s now-1month  -e now \
DEF:Sens1=$DIR/$DBNAME.rrd:Sens1:AVERAGE \
DEF:streed=$DIR/$DBNAME.rrd:streed:AVERAGE \
XPORT:Sens1:"Sens1" \
XPORT:streed:"streed" > $XMLDIR/temp1m.xml

#yearly
rrdtool xport -s now-1y  -e now \
DEF:Sens1=$DIR/$DBNAME.rrd:Sens1:AVERAGE \
DEF:streed=$DIR/$DBNAME.rrd:streed:AVERAGE \
XPORT:Sens1:"Sens1" \
XPORT:streed:"streed" > $XMLDIR/temp1y.xml

