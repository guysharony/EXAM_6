#!/bin/bash

FAILURE='\033[0;31m'
SUCCESS='\033[0;32m'
NC='\033[0m'

echo "Verifying your work."

# Analyze
echo "analyzing..."
clang -Wall -Werror -Wextra --analyze --analyzer-output text ./rendu/mini_serv.c -o ./tests/mini_serv &> .trace_analyze
if [ $(awk 'END { print NR }' .trace_analyze) -ne 0 ]
then
	cat .trace_analyze > trace
	rm .trace_analyze
	echo -e "${FAILURE}FAILED${NC}"
	exit 1
fi
rm .trace_analyze


###### TEST 1
echo ""
echo -n "TEST 1: "
clang -Wall -Werror -Wextra -fsanitize=address ./rendu/mini_serv.c -o ./tests/mini_serv &> .trace_compilation
if [ $(awk 'END { print NR }' .trace_compilation) -ne 0 ]
then 
	cat .trace_compilation > trace
	rm .trace_compilation
	echo -e "${FAILURE}KO${NC}"
	exit 1
fi
rm .trace_compilation

./tests/mini_serv 9999 &> ./tests/test1/.result &
MINI_SERV_PID=$!
sleep 1
if [ $(awk 'END { print NR }' ./tests/test1/.result) -ne 0 ]
then
	echo -e "${SUCCESS}KO${NC}"
	exit 1
fi

./tests/test1/test.sh
pkill -9 mini_serv &> /dev/null &
rm ./tests/mini_serv &> /dev/null &


diff ./tests/test1/.result ./tests/test1/expected &>> trace &
if [ $(awk 'END { print NR }' trace) -ne 0 ]
then
	rm ./tests/test1/.result &> /dev/null &
	echo -e "${FAILURE}KO${NC}"
	exit 1
fi
rm ./tests/test1/.result &> /dev/null &
rm ./trace &> /dev/null &


echo -e "${SUCCESS}OK${NC}"