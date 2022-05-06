#!/bin/bash

nc -d 127.0.0.1 9999 &>> ./tests/test1/.result &
MINI_SERV_PID=$!
cat ./tests/test1/input_client1 | netcat -q 1 127.0.0.1 9999
sleep 1
kill $MINI_SERV_PID