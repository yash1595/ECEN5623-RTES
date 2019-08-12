#!/bin/bash

str1="10"
str2="Y"

read -p "Enter 1 or 10 for running at 1Hz or 10Hz: " freq
if [ "$freq" == "$str1" ]; then
	freq="10HZ"
else
	freq="1HZ"
fi

read -p "Enter 'Y' for sockets or 'N' for no sockets: " socket
if [ "$socket" == "$str2" ]; then
	socket="sockets"
else
	socket="Sockets not selected"
fi

echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
echo "Program will run at $freq With $socket "
echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"

read -p "Enter Y to continue or N to quit: " choice
if [ "$choice" == "$str2" ]; then
	echo "Continuing...."
	rm *.pgm
else
	exit
fi

./capture $freq $socket 
