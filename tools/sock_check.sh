#!/bin/bash
# peer=.42seoul.k
peer=localhost
while [ 1 ]
do
PID=$(ps u | grep -E '[.]/'${NAME} | awk '{print $2}' | xargs echo )
VAR="\ntotal connection:"
VAR=$VAR$(netstat | grep $peer | wc -l)
VAR=$VAR"\n"
VAR=$VAR"ESTABLISHED     :"
VAR=$VAR$(netstat | grep $peer | grep ESTABLISHED | wc -l)
VAR=$VAR"\n"
VAR=$VAR"TIME_WAIT       :"
VAR=$VAR$(netstat | grep $peer | grep TIME_WAIT | wc -l)
VAR=$VAR"\n"
# VAR=$VAR$(lsof -p $PID 2> /dev/null | grep TCP)
dialog --title "socket check" --infobox "\
	${VAR}\
	" 50 50
done
