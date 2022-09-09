#!/bin/bash
NAME=myLittleWebserv
while [ 1 ]
do
	clear
	echo "================================================================================================================================="
	PID=$(ps u | grep -E '[.]/'${NAME} | awk '{print $2}' | xargs echo )
	lsof -p $PID 2> /dev/null | grep TCP
	echo -n " total connection: "
	netstat | grep localhost | wc -l
	echo -n " ESTABLISHED     : "
	netstat | grep ESTABLISHED | wc -l
	echo -n " TIME_WAIT       : "
	netstat | grep TIME_WAIT | wc -l
	echo "================================================================================================================================="
	sleep 1
done

