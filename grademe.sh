#!/bin/bash

cleanup() {
	{ rm ./tests/.analyze; } 2> /dev/null
	{ rm ./tests/.compilation; } 2> /dev/null

	{ rm ./tests/test1/.result; } 2> /dev/null
	{ rm ./tests/test1/.diff; } 2> /dev/null

	{ rm ./tests/test2/.result; } 2> /dev/null
	{ rm ./tests/test2/.diff; } 2> /dev/null

	{ rm ./tests/test3/.result; } 2> /dev/null
	{ rm ./tests/test3/.diff; } 2> /dev/null

	{ rm ./tests/mini_serv; } 2> /dev/null

	{ kill -9 $(lsof -t -i:9999); } 2> /dev/null
}

trap cleanup EXIT


# Source
source ./tests/.source/display.sh
source ./tests/.source/grader.sh
source ./tests/.source/clients.sh
echo "Verifying your work."


# Analyzer
evaluate \
	"Analyzer" \
	"clang -Wall -Werror -Wextra --analyze --analyzer-output text ./rendu/mini_serv.c -o ./tests/mini_serv" \
	.analyze

# Compilation
evaluate \
	"Compilation" \
	"clang -Wall -Werror -Wextra -fsanitize=address ./rendu/mini_serv.c -o ./tests/mini_serv" \
	.compilation

# Test 1
./tests/mini_serv &> ./tests/test1/.result
server_pid=$!

stop_process $server_pid

grade 1


# Test 2
server1_pid=$(start_server 9999 2)
server2_pid=$(start_server 9999 2)

stop_process $server1_pid
stop_process $server2_pid

grade 2


# Test 3
server_pid=$(start_server 9999 3)
client0_pid=$(start_reader_client 9999 3)
client1_pid=$(start_writer_client 9999 3 input_client1)

stop_process $client1_pid
stop_process $client0_pid
stop_process $server_pid

grade 3