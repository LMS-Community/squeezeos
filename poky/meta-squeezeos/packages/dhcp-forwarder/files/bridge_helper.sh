#!/bin/sh
echo $0
echo $1 

route add -host $1 dev eth0

