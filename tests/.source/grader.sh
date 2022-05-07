#!/bin/bash

evaluate() {
	eval " $2 &> ./tests/$3"
	if [ $(awk 'END { print NR }' ./tests/$3) -ne 0 ]
	then
		cat ./tests/$3 > trace
		rm ./tests/$3
		display_failure "$1"
	fi
	rm ./tests/$3
	display_success "$1"
}

grade() {
	{ kill -9 $(lsof -t -i:9999); } 2> /dev/null

	diff ./tests/test$1/.result ./tests/test$1/expected &> ./tests/test$1/.diff &
	if [ $(awk 'END { print NR }' ./tests/test$1/.diff) -ne 0 ]
	then
		echo "===== [Test $1] KO =====" >> trace
		cat ./tests/test$1/.diff >> trace
		echo "========================" >> trace
		echo "" >> trace

		rm ./tests/test$1/.result &> /dev/null &
		rm ./tests/test$1/.diff &> /dev/null &
		display_failure "Test $1"
	fi
	rm ./tests/test$1/.result &> /dev/null &
	rm ./tests/test$1/.diff &> /dev/null &
	display_success "Test $1"

	echo "===== [Test $1] OK =====" >> trace
	echo "" >> trace
}