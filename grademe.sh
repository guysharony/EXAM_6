#!/bin/bash

cleanup() {
	{ rm ./tests/.analyze; } 2> /dev/null
	{ rm ./tests/.compilation; } 2> /dev/null

	{ rm ./tests/test1/.result; } 2> /dev/null
	{ rm ./tests/test1/.diff; } 2> /dev/null

	## { rm ./tests/test2/.result; } 2> /dev/null
	{ rm ./tests/test2/.diff; } 2> /dev/null

	##{ rm ./tests/test3/.result; } 2> /dev/null
	{ rm ./tests/test3/.diff; } 2> /dev/null

	{ rm ./tests/test4/.result; } 2> /dev/null
	{ rm ./tests/test4/.diff; } 2> /dev/null

	{ rm ./tests/test5/.result; } 2> /dev/null
	{ rm ./tests/test5/.diff; } 2> /dev/null

	{ rm ./tests/mini_serv; } 2> /dev/null

	{ kill -9 $(lsof -t -i:9999); } 2> /dev/null
}

trap cleanup EXIT


# Source
source ./tests/.source/display.sh
source ./tests/.source/grader.sh
source ./tests/.source/clients.sh
echo "Verifying your work."


echo -n "" > trace


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
sleep 2

stop_process $server_pid

grade 3


# Test 4
server_pid=$(start_server 9999 4)
client0_pid=$(start_reader_client 9999 4)

client1_pid=$(start_writer_client 9999 4 input_client1)
stop_process $client1_pid

client2_pid=$(start_writer_client 9999 4 input_client2)
stop_process $client2_pid

stop_process $client0_pid
stop_process $server_pid

grade 4


# Test 5
server_pid=$(start_server 9999 5)
client0_pid=$(start_reader_client 9999 5)

client1_pid=$(start_writer_client 9999 5 input_client1)
stop_process $client1_pid

stop_process $client0_pid
stop_process $server_pid

grade 5


# Test 6
server_pid=$(start_server 9999 6)
client0_pid=$(start_reader_client 9999 6)

client1_pid=$(start_writer_client 9999 6 input_client1)
client2_pid=$(start_writer_client 9999 6 input_client2)
stop_process $client1_pid
stop_process $client2_pid

client3_pid=$(start_writer_client 9999 6 input_client3)
stop_process $client3_pid

client4_pid=$(start_writer_client 9999 6 input_client4)
stop_process $client4_pid

client5_pid=$(start_writer_client 9999 6 input_client5)
client6_pid=$(start_writer_client 9999 6 input_client6)
client7_pid=$(start_writer_client 9999 6 input_client7)
client8_pid=$(start_writer_client 9999 6 input_client8)
stop_process $client5_pid
stop_process $client6_pid
stop_process $client7_pid
stop_process $client8_pid

stop_process $client0_pid
stop_process $server_pid

grade 6