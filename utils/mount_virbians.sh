#!/bin/sh

rm -rf /tmp/virbian{1..4}
mkdir /tmp/virbian{1..4}

for i in {1..4}; do
		sshfs -p 302$i user@localhost:/home/user /tmp/virbian$i
done
