#!/bin/bash

FAILURE='\033[0;31m'
SUCCESS='\033[0;32m'
NC='\033[0m'


display_success() {
	echo -e "[$1] => ${SUCCESS}OK${NC}"
}

display_failure() {
	echo -e "[$1] => ${FAILURE}KO${NC}"
	exit 1
}