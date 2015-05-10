#!/bin/bash

for i in `seq 1 15`
do
    echo Hello World $i > $1/test$i
done