#!/bin/sh
STR="1.2.3.4"
STR_ARRAY=(`echo $STR | sed -e 's/,/\n/g'`)
for x in "${STR_ARRAY[@]}"
do
	echo "> [$x]"
done
