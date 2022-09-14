#!/bin/bash
peer=localhost
# peer=.42seoul.k
RED='\033[0;31m'
NC='\033[0m'
while :
do
NAME="myLittleWebserv"
TTY="/dev/tty"$(ps u | grep -E '[.]/'${NAME} | awk 'END {print $7}')
PID=$(ps u | grep -E '[.]/'${NAME} | awk '{print $2}' )
VAR="\n<leak>\n"
VAR=$VAR$(leaks ${PID} | grep Process)

VAR=$VAR"\n\n<socket>"
VAR=$VAR"\ntotal connection:"
VAR=$VAR$(netstat | grep $peer | wc -l)
VAR=$VAR"\n"
VAR=$VAR"ESTABLISHED     :"
VAR=$VAR$(netstat | grep $peer | grep ESTABLISHED | wc -l)
VAR=$VAR"\n"
VAR=$VAR"TIME_WAIT       :"
VAR=$VAR$(netstat | grep $peer | grep TIME_WAIT | wc -l)
VAR=$VAR"\n"

	dialog --title "check" --infobox "\
	${VAR}\
	" 20 80
done


