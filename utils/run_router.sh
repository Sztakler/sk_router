#!/bin/bash

if [ -z $1 ]; then
	echo "Specify machine number as argument."
else
	./sk_router < ~/router/config/conf$1.in
fi
