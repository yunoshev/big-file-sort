# Check for proper number of command line args.
EXPECTED_ARGS=2
E_BADARGS=65

if [ $# -lt $EXPECTED_ARGS ]
then
  echo "Usage: `basename $0` in_file_name out_file_name "
  exit $E_BADARGS
fi

TEMP_DATA_TEMLATE="radix.data.*"

EXEC_SOURCE=main.cpp
EXEC_OUT=fradix.out

printf "Remove old temp data...\n"
rm $TEMP_DATA_TEMLATE
printf "Build executable...\n"
time clang++ $EXEC_SOURCE -o $EXEC_OUT
printf "Run...\n"
./$EXEC_OUT $1
printf "Remove old result file...\n"
rm $2
printf "Build result file...\n"
time find . -name "radix.data.3.*" -exec cat {} >> $2 \;