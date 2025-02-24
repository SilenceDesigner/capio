#!/bin/bash

if [ -z "${1}" ]; then
  echo "Usage: $(basename "${BASH_SOURCE[0]}") OUTPUT_PATH"
  exit
fi

ARCH=$(uname -m | rev | cut -c 1-2 | rev)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [[ "$ARCH" -eq "x86_64" ]]; then
	$SCRIPT_DIR/gen_syscallnames_x86.sh ${1}
else
	$SCRIPT_DIR/gen_syscallnames_riscv.sh ${1}
fi


