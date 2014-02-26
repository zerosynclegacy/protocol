#!/bin/bash

rm -rf test*

for TEST_NO in 1 2 3 4 5 
do
    mkdir -p test$TEST_NO/syncfolder
    cp zsync_selftest test$TEST_NO
    echo $RANDOM
    dd bs=1024 count=$RANDOM if=/dev/zero of=test$TEST_NO/syncfolder/test$TEST_NO
done

