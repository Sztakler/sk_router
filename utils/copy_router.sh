#!/bin/bash

for i in {1..4}; do
		if [ -d /tmp/virbian$i/router ]
		then
				rm -rf /tmp/virbian$i/router
		else
				mkdir /tmp/virbian$i/router
		fi

		cp -r ~/Documents/Programming/SK/router/* /tmp/virbian$i/router/
done

