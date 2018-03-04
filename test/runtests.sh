#!/bin/bash

# Convert all hex files to binary
find files/ -iname "*.hex" | while read f; do xxd -r -p $f > ${f%.hex}.bin; done

# Messages
PASS_MSG=$(echo -e "\033[0;32mPASS\033[0m")
FAIL_MSG=$(echo -e "\033[0;31mFAIL\033[0m")

# Keep totals
total_pass=0
total_fail=0

run_test() {
  f=$1
  answer_file=${f%.bin}.answer
  result_file=${f%.bin}.result

  declare -a opts=("" "--tree")

  for opt in "${opts[@]}"; do
    rm -f $result_file
    ../bin/ecbor-describe $opt $f > $result_file 2>/dev/null
    rc=$?

    if [ ! -f $result_file ] || [ ! -f $answer_file ] || [ "$(diff $answer_file $result_file 2>/dev/null)" != "" ]; then
      fail=$(($fail + 1))
      status=$FAIL_MSG
    else
      pass=$(($pass + 1))
      status=$PASS_MSG
    fi
    
    test_name="$f($opt)"
    machine_indented=$(printf '%-67s' "$test_name")
    machine_indented=${machine_indented// /.}
    printf "%s %s\n" "$machine_indented" "$status"
  done
}

# Appendix A tests
pass=0
fail=0

echo ""
echo "============================== APPENDIX A =============================="
for f in files/appendix_a/*.bin; do
  run_test $f
done
echo "========================================================================"
echo "Passed / Failed: ${pass}/${fail}"

total_pass=$(($total_pass + $pass))
total_fail=$(($total_fail + $fail))

# Error cases tests
pass=0
fail=0

echo ""
echo "============================== ERROR CASE =============================="
for f in files/error_cases/incomplete/*.bin; do
  run_test $f
done
echo "========================================================================"
echo "Passed / Failed: ${pass}/${fail}"

total_pass=$(($total_pass + $pass))
total_fail=$(($total_fail + $fail))


# Final report
echo ""
echo ""
echo "============================== FINAL REPORT ============================"
echo "Total tests passed: $total_pass"
echo "Total tests failed: $total_fail"
echo "========================================================================"