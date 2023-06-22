#!/usr/bin/bash

# This script assummes that is run from within a cmake build directory
# where make has been executed successfully. E.g.,
#
#  .../qiotoolkit/cpp/ $ mkdir -p build && cd build
#  .../qiotoolkit/cpp/build/ $ cmake ..
#  .../qiotoolkit/cpp/build/ $ make -j8
#  .../qiotoolkit/cpp/build/ $ ../run_tests.sh
#
#  - The build configuration is therefore selected by executing
#    from the corresponding folder
#  - The location of the cpp folder is identified from the location
#    of this script (this is needed to search for CMakeLists.txt
#    files containing "add_gtest" entries).

THIS_SCRIPT="$0"
qiotoolkit_CPP_DIR="$(dirname "$(realpath "$THIS_SCRIPT")")"
BUILD_DIR="$(pwd)"

configuration=""
extension=""
if [[ $1 == "--configuration" ]]; then
  configuration=$2
  if [[ $3 == "--extension" ]]; then
    extension=$4
  fi  
fi

export GTEST_COLOR=1
let "passed_count=0"
let "failed_count=0"
failed=""

function run_test {
  TEST_BINARY=$1
  echo -e "\033[1;34m"
  echo "╔$(printf '═%.0s' {1..78})╗"
  echo "║$(printf '%-78s' " $1")║"
  echo "╚$(printf '═%.0s' {1..78})╝"
  echo -e "\033[0m"
  cd "$BUILD_DIR"
  cd "$(dirname "$(realpath "$TEST_BINARY")")"
  "./$configuration/$(basename "$TEST_BINARY$extension")"
  return_code=$?
  if [ $return_code -eq 0 ]; then
    let "passed_count+=1"
  else
    let "failed_count+=1"
    failed="${failed}  $TEST_BINARY\n"
    echo -e "\033[1;31m"
    echo "╔$(printf '═%.0s' {1..78})╗"
    echo "║$(printf '%-78s' " FAILED with return code $return_code")║"
    echo "╚$(printf '═%.0s' {1..78})╝"
    echo -e "\033[0m"
  fi
  cd "$BUILD_DIR"
  echo "// end of $TEST_BINARY"
}

function find_and_run_tests {
  cd "$qiotoolkit_CPP_DIR"
  tests="$(find . -iname "CMakeLists.txt" | \
    grep -v build | \
    while read file; do 
      grep "add_gtest(" "$file" | \
        sed -e"s/.*add_gtest(//" -e"s/ .*//" | \
        while read test; do 
          echo "$(dirname "$file")/$test"
        done
    done | sort)"
  for test in $tests; do
    run_test "$test"
  done
}

function print_summary {
  echo -e "\033[1;34m"
  echo "╔$(printf '═%.0s' {1..78})╗"
  echo "║$(printf '%-78s' " SUMMARY")║"
  echo "╚$(printf '═%.0s' {1..78})╝"
  echo -e "\033[1;32m"
  if [ $failed_count -gt 0 ]; then
    echo "  ${passed_count} PASSED"
    echo -n -e "\033[1;31m"
    echo "  ${failed_count} FAILED"
    echo ""
    echo "Failed tests:"
    echo ""
    echo -e "${failed}"
  else
    echo "  ALL ${passed_count} PASSED"
  fi
  echo ""
  echo -n -e "\033[0m"
}

# NOTE: If you have opened this file to add a new test --
# They are now picked up automatically; you don't need to
# list them manually here any more.

find_and_run_tests
print_summary
exit $failed_count