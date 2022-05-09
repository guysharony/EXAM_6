start_server() {
	if [ -n "netstat -tunl | grep \":$1 \"" ]; then
		./tests/mini_serv $1 &> ./tests/test$2/.result &
		sleep 1
		echo $!
	fi
}

start_reader_client() {
	nc -d 127.0.0.1 $1 &>> ./tests/test$2/.result &
	echo $!
}

start_writer_client() {
	cat ./tests/test$2/$3 | netcat 127.0.0.1 $1 &>> /dev/null &
	echo $!
}

stop_process() {
	sleep 1
	{ kill $1 && wait $1; } 2> /dev/null
}