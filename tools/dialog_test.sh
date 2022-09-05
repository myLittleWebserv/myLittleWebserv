#!/bin/bash
while [ 1 ]
do
PID=$(ps u | grep -E '[.]/'${NAME} | awk '{print $2}' | xargs echo )
VAR="\ntotal connection:"
VAR=$VAR$(netstat | grep .42seoul.k | wc -l)
VAR=$VAR"\n"
VAR=$VAR"ESTABLISHED     :"
VAR=$VAR$(netstat | grep .42seoul.k | grep ESTABLISHED | wc -l)
VAR=$VAR"\n"
VAR=$VAR"TIME_WAIT       :"
VAR=$VAR$(netstat | grep .42seoul.k | grep TIME_WAIT | wc -l)
VAR=$VAR"\n"
# VAR=$VAR$(lsof -p $PID 2> /dev/null | grep TCP)
dialog --title "socket check" --infobox "\
	${VAR}\
	" 50 50
done
