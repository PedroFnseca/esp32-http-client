#!/usr/bin/env sh

set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

REST_TEST_OBJ="$BUILD_DIR/test_rest_request.o"
SOAP_TEST_OBJ="$BUILD_DIR/test_soap_request.o"
GRAPHQL_TEST_OBJ="$BUILD_DIR/test_graphql_request.o"

REST_REQUEST_OBJ="$BUILD_DIR/RestRequest.o"
SOAP_REQUEST_OBJ="$BUILD_DIR/SoapRequest.o"
GRAPHQL_REQUEST_OBJ="$BUILD_DIR/GraphQLRequest.o"
GRAPHQL_BATCH_REQUEST_OBJ="$BUILD_DIR/GraphQLBatchRequest.o"
HTTP_CLIENT_OBJ="$BUILD_DIR/ESP32HTTPClient.o"

REST_EXE="$BUILD_DIR/unit-tests-rest"
SOAP_EXE="$BUILD_DIR/unit-tests-soap"
GRAPHQL_EXE="$BUILD_DIR/unit-tests-graphql"

SHOW_COVERAGE="${SHOW_COVERAGE:-1}"

mkdir -p "$BUILD_DIR"

COMPILER=""
for c in g++ clang++ c++; do
  if command -v "$c" >/dev/null 2>&1; then
    COMPILER="$c"
    break
  fi
done

if [ -z "$COMPILER" ]; then
  echo "Error: no C++ compiler found (g++, clang++, or c++)." >&2
  echo "Install one and try again. On Ubuntu/Debian:" >&2
  echo "  sudo apt update && sudo apt install -y build-essential" >&2
  exit 1
fi

if [ -t 1 ] || [ "${FORCE_COLOR:-}" = "1" ] || [ -n "${GITHUB_ACTIONS:-}" ]; then
  RED='\033[31m'
  GREEN='\033[32m'
  YELLOW='\033[33m'
  CYAN='\033[36m'
  RESET='\033[0m'
else
  RED=''
  GREEN=''
  YELLOW=''
  CYAN=''
  RESET=''
fi

printf "%bCompiling unit tests...%b\n" "$CYAN" "$RESET"
if [ "$SHOW_COVERAGE" = "1" ]; then
  COVERAGE_FLAGS="--coverage"
  rm -f "$BUILD_DIR"/*.gcda "$BUILD_DIR"/*.gcno
else
  COVERAGE_FLAGS=""
fi

# Compile library objects
"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$REPO_ROOT/src/RestRequest.cpp" \
  -o "$REST_REQUEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$REPO_ROOT/src/SoapRequest.cpp" \
  -o "$SOAP_REQUEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$REPO_ROOT/src/GraphQLRequest.cpp" \
  -o "$GRAPHQL_REQUEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$REPO_ROOT/src/GraphQLBatchRequest.cpp" \
  -o "$GRAPHQL_BATCH_REQUEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$REPO_ROOT/src/ESP32HTTPClient.cpp" \
  -o "$HTTP_CLIENT_OBJ"

# Compile and link REST tests
"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$SCRIPT_DIR/test_rest_request.cpp" \
  -o "$REST_TEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  "$REST_TEST_OBJ" \
  "$REST_REQUEST_OBJ" \
  "$SOAP_REQUEST_OBJ" \
  "$GRAPHQL_REQUEST_OBJ" \
  "$GRAPHQL_BATCH_REQUEST_OBJ" \
  "$HTTP_CLIENT_OBJ" \
  -o "$REST_EXE"

# Compile and link SOAP tests
"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$SCRIPT_DIR/test_soap_request.cpp" \
  -o "$SOAP_TEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  "$SOAP_TEST_OBJ" \
  "$REST_REQUEST_OBJ" \
  "$SOAP_REQUEST_OBJ" \
  "$GRAPHQL_REQUEST_OBJ" \
  "$GRAPHQL_BATCH_REQUEST_OBJ" \
  "$HTTP_CLIENT_OBJ" \
  -o "$SOAP_EXE"

# Compile and link GraphQL tests
"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$SCRIPT_DIR/test_graphql_request.cpp" \
  -o "$GRAPHQL_TEST_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  "$GRAPHQL_TEST_OBJ" \
  "$REST_REQUEST_OBJ" \
  "$SOAP_REQUEST_OBJ" \
  "$GRAPHQL_REQUEST_OBJ" \
  "$GRAPHQL_BATCH_REQUEST_OBJ" \
  "$HTTP_CLIENT_OBJ" \
  -o "$GRAPHQL_EXE"

printf "%bRunning unit tests...%b\n\n" "$CYAN" "$RESET"

OVERALL_STATUS=0

run_test_suite() {
  exe="$1"
  suite_name="$2"
  printf "%b>>> Running %s <<<%b\n" "$CYAN" "$suite_name" "$RESET"
  set +e
  "$exe"
  status=$?
  set -e
  if [ "$status" -ne 0 ]; then
    OVERALL_STATUS=1
  fi
  printf "\n"
}

run_test_suite "$REST_EXE" "REST Tests"
run_test_suite "$SOAP_EXE" "SOAP Tests"
run_test_suite "$GRAPHQL_EXE" "GraphQL Tests"

if [ "$SHOW_COVERAGE" = "1" ] && command -v gcov >/dev/null 2>&1; then
  set +e
  GCOV_OUTPUT="$(
    cd "$BUILD_DIR" &&
    gcov -b -c -o "$BUILD_DIR" \
      "$REST_REQUEST_OBJ" \
      "$SOAP_REQUEST_OBJ" \
      "$GRAPHQL_REQUEST_OBJ" \
      "$GRAPHQL_BATCH_REQUEST_OBJ" \
      "$HTTP_CLIENT_OBJ" 2>/dev/null
  )"
  GCOV_STATUS=$?
  set -e

  if [ "$GCOV_STATUS" -eq 0 ]; then
    FILE_COVERAGE_ROWS="$(printf "%s\n" "$GCOV_OUTPUT" | awk '
      index($0, "File '\''") == 1 && index($0, "/src/") > 0 {
        file = $0
        sub(/^File '\''/, "", file)
        sub(/'\''$/, "", file)
        next
      }
      index($0, "Lines executed:") == 1 && file != "" {
        pct = $0
        sub(/^Lines executed:/, "", pct)
        sub(/%.*/, "", pct)
        cnt = $0
        sub(/^.* of /, "", cnt)
        files[file] = pct + 0
        lines[file] = cnt + 0
        file = ""
      }
      END {
        for (f in files) {
          printf "%s|%.2f|%d\n", f, files[f], lines[f]
        }
      }
    ')"

    if [ -n "$FILE_COVERAGE_ROWS" ]; then
      printf "%bSource line coverage by file:%b\n" "$CYAN" "$RESET"
      printf "%s\n" "$FILE_COVERAGE_ROWS" | while IFS='|' read -r file pct cnt; do
        display_file="$file"
        case "$display_file" in
          */src/*)
            display_file="src/${display_file##*/src/}"
            ;;
        esac
        printf "  %b%-30s%b  %6s%%  (%s lines)\n" "$CYAN" "$display_file" "$RESET" "$pct" "$cnt"
      done
    fi
  fi
fi

if [ "$OVERALL_STATUS" -eq 0 ]; then
  printf "%bAll test suites passed successfully!%b\n" "$GREEN" "$RESET"
else
  printf "%bSome test suites failed.%b\n" "$RED" "$RESET"
fi

exit "$OVERALL_STATUS"
