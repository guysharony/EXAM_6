start_server() {
	if [ -n "netstat -tunl | grep \":$1 \"" ]; then
		./tests/mini_serv $1 &> ./tests/test$2/.result &
		sleep 1
		return $!
	fi
}

start_reader_client() {
	nc -d 127.0.0.1 $1 &>> ./tests/test$2/.result &
	sleep 1
	return $!
}

start_writer_client() {
	cat ./tests/test$2/$3 | netcat -q 1 127.0.0.1 $1
	sleep 1
	return $!
}

stop_process() {
	{ kill $1 && wait $1; } 2> /dev/null
	sleep 1
}