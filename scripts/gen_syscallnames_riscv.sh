#!/bin/bash

if [ -z "${1}" ]; then
  echo "Usage: $(basename "${BASH_SOURCE[0]}") OUTPUT_PATH"
  exit
fi

INPUT_FILE=/usr/include/asm-generic/unistd.h
INPUT=$(cat $INPUT_FILE | grep -E "#define __NR_.* [0-9]+$")
DESTINATION="${1}"

# Generate default destination file
mkdir -p "$(dirname "${DESTINATION}")"
echo 'auto sys_num_to_string(int sysnum) { return "Table not created"; }' > "${DESTINATION}"

# Parse syscall names
echo "Parsing ${INPUT_FILE}"
DATA="auto sys_num_to_string(int sysnum) {
  switch (sysnum) {" > "${DESTINATION}"
 
ADDED_SYSCALLS=()

while IFS= read -r line
do
    line=$(echo $line | tr -s ' ')
    SYSCALL=$(echo "$line" | cut -d ' ' -f 2)
    SYSCALL=${SYSCALL#"__NR_"}
    SYSCALLNO=$(echo "$line" | cut -d ' ' -f 3)
    if [[ ! " ${ADDED_SYSCALLS[*]} " =~ [[:space:]]${SYSCALLNO}[[:space:]] ]]; then
    	DATA="$DATA
	case $SYSCALLNO: return \"$SYSCALL\";"
	ADDED_SYSCALLS+=("$SYSCALLNO")
    fi
done <<< "${INPUT}"

# Add default branch
DATA="$DATA
    default:
        return \"Unknown\";
    };
};"

# Print content to the destination file
echo "${DATA}" > "${DESTINATION}"
