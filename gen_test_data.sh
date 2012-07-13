#! /bin/bash
# Check for proper number of command line args.
EXPECTED_ARGS=2
E_BADARGS=65

if [ $# -lt $EXPECTED_ARGS ]
then
  echo "Usage: `basename $0` size_in_mb output_file"
  exit $E_BADARGS
fi

let BLOCK_BY_4BYTES=$1*1024*1024/4
dd if=/dev/urandom of=$2 bs=4 count=$BLOCK_BY_4BYTES

