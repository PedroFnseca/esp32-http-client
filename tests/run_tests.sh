#!/usr/bin/env sh

set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

REST_EXE="$BUILD_DIR/unit-tests-rest"
SOAP_EXE="$BUILD_DIR/unit-tests-soap"
GRAPHQL_EXE="$BUILD_DIR/unit-tests-graphql"

# Default coverage: ON in GitHub Actions, OFF locally for ultra-fast runs
if [ -n "${GITHUB_ACTIONS:-}" ]; then
  DEFAULT_COVERAGE=1
else
  DEFAULT_COVERAGE=0
fi
SHOW_COVERAGE="${SHOW_COVERAGE:-$DEFAULT_COVERAGE}"

TARGET_SUITE="all"
for arg in "$@"; do
  case "$arg" in
    --coverage)
      SHOW_COVERAGE=1
      ;;
    --no-coverage)
      SHOW_COVERAGE=0
      ;;
    rest|soap|graphql|all)
      TARGET_SUITE="$arg"
      ;;
    -h|--help)
      echo "Usage: $0 [rest|soap|graphql|all] [--coverage|--no-coverage]"
      exit 0
      ;;
  esac
done

mkdir -p "$BUILD_DIR"

COMPILER=""
for c in "${CXX:-}" g++ clang++ c++; do
  if [ -n "$c" ] && command -v "$c" >/dev/null 2>&1; then
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

# Detect CPU cores for parallel compilation
NPROC=4
if command -v nproc >/dev/null 2>&1; then
  NPROC="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
  NPROC="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
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

printf "%bCompiling unit tests (jobs: %s, coverage: %s)...%b\n" "$CYAN" "$NPROC" "$SHOW_COVERAGE" "$RESET"

if [ "$SHOW_COVERAGE" = "1" ]; then
  COVERAGE_FLAGS="--coverage"
  rm -f "$BUILD_DIR"/*.gcda
else
  COVERAGE_FLAGS=""
fi

# Prefer make for true incremental and parallel builds
if command -v make >/dev/null 2>&1 && [ -f "$SCRIPT_DIR/Makefile" ]; then
  MAKE_TARGET="all"
  if [ "$TARGET_SUITE" = "rest" ]; then
    MAKE_TARGET="$REST_EXE"
  elif [ "$TARGET_SUITE" = "soap" ]; then
    MAKE_TARGET="$SOAP_EXE"
  elif [ "$TARGET_SUITE" = "graphql" ]; then
    MAKE_TARGET="$GRAPHQL_EXE"
  fi

  make -C "$SCRIPT_DIR" -j"$NPROC" \
    CXX="$COMPILER" \
    COVERAGE="$SHOW_COVERAGE" \
    "$MAKE_TARGET"
else
  # Portable fallback: compile in parallel using shell background processes
  REST_REQUEST_OBJ="$BUILD_DIR/RestRequest.o"
  SOAP_REQUEST_OBJ="$BUILD_DIR/SoapRequest.o"
  GRAPHQL_REQUEST_OBJ="$BUILD_DIR/GraphQLRequest.o"
  GRAPHQL_BATCH_REQUEST_OBJ="$BUILD_DIR/GraphQLBatchRequest.o"
  HTTP_CLIENT_OBJ="$BUILD_DIR/ESP32HTTPClient.o"
  BUFFERED_READER_OBJ="$BUILD_DIR/BufferedStreamReader.o"

  REST_TEST_OBJ="$BUILD_DIR/test_rest_request.o"
  SOAP_TEST_OBJ="$BUILD_DIR/test_soap_request.o"
  GRAPHQL_TEST_OBJ="$BUILD_DIR/test_graphql_request.o"

  compile_obj() {
    src="$1"
    obj="$2"
    "$COMPILER" -std=c++17 $COVERAGE_FLAGS -I"$SCRIPT_DIR/stubs" -I"$REPO_ROOT/src" -c "$src" -o "$obj"
  }

  compile_obj "$REPO_ROOT/src/RestRequest.cpp" "$REST_REQUEST_OBJ" &
  compile_obj "$REPO_ROOT/src/SoapRequest.cpp" "$SOAP_REQUEST_OBJ" &
  compile_obj "$REPO_ROOT/src/GraphQLRequest.cpp" "$GRAPHQL_REQUEST_OBJ" &
  compile_obj "$REPO_ROOT/src/GraphQLBatchRequest.cpp" "$GRAPHQL_BATCH_REQUEST_OBJ" &
  compile_obj "$REPO_ROOT/src/ESP32HTTPClient.cpp" "$HTTP_CLIENT_OBJ" &
  compile_obj "$REPO_ROOT/src/BufferedStreamReader.cpp" "$BUFFERED_READER_OBJ" &
  compile_obj "$SCRIPT_DIR/test_rest_request.cpp" "$REST_TEST_OBJ" &
  compile_obj "$SCRIPT_DIR/test_soap_request.cpp" "$SOAP_TEST_OBJ" &
  compile_obj "$SCRIPT_DIR/test_graphql_request.cpp" "$GRAPHQL_TEST_OBJ" &
  wait

  LIB_OBJS="$REST_REQUEST_OBJ $SOAP_REQUEST_OBJ $GRAPHQL_REQUEST_OBJ $GRAPHQL_BATCH_REQUEST_OBJ $HTTP_CLIENT_OBJ $BUFFERED_READER_OBJ"

  if [ "$TARGET_SUITE" = "all" ] || [ "$TARGET_SUITE" = "rest" ]; then
    "$COMPILER" -std=c++17 $COVERAGE_FLAGS "$REST_TEST_OBJ" $LIB_OBJS -o "$REST_EXE" &
  fi
  if [ "$TARGET_SUITE" = "all" ] || [ "$TARGET_SUITE" = "soap" ]; then
    "$COMPILER" -std=c++17 $COVERAGE_FLAGS "$SOAP_TEST_OBJ" $LIB_OBJS -o "$SOAP_EXE" &
  fi
  if [ "$TARGET_SUITE" = "all" ] || [ "$TARGET_SUITE" = "graphql" ]; then
    "$COMPILER" -std=c++17 $COVERAGE_FLAGS "$GRAPHQL_TEST_OBJ" $LIB_OBJS -o "$GRAPHQL_EXE" &
  fi
  wait
fi

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

if [ "$TARGET_SUITE" = "all" ] || [ "$TARGET_SUITE" = "rest" ]; then
  run_test_suite "$REST_EXE" "REST Tests"
fi

if [ "$TARGET_SUITE" = "all" ] || [ "$TARGET_SUITE" = "soap" ]; then
  run_test_suite "$SOAP_EXE" "SOAP Tests"
fi

if [ "$TARGET_SUITE" = "all" ] || [ "$TARGET_SUITE" = "graphql" ]; then
  run_test_suite "$GRAPHQL_EXE" "GraphQL Tests"
fi

# Coverage collection and reporting (used by CI to update coverage badge)
if [ "$SHOW_COVERAGE" = "1" ] && command -v gcov >/dev/null 2>&1; then
  set +e
  GCOV_OUTPUT="$(
    cd "$BUILD_DIR" &&
    gcov -b -c -o "$BUILD_DIR" \
      "$BUILD_DIR/RestRequest.o" \
      "$BUILD_DIR/SoapRequest.o" \
      "$BUILD_DIR/GraphQLRequest.o" \
      "$BUILD_DIR/GraphQLBatchRequest.o" \
      "$BUILD_DIR/ESP32HTTPClient.o" \
      "$BUILD_DIR/BufferedStreamReader.o" 2>/dev/null
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

    SRC_LINE_COVERAGE="$(printf "%s\n" "$FILE_COVERAGE_ROWS" | awk -F'|' '
      {
        covered += ($2 + 0) * ($3 + 0)
        total += ($3 + 0)
      }
      END {
        if (total > 0) {
          printf "%.2f", covered / total
        }
      }
    ')"

    if [ -n "$SRC_LINE_COVERAGE" ]; then
      printf "%bSource line coverage (src): %s%%%b\n" "$CYAN" "$SRC_LINE_COVERAGE" "$RESET"
    fi
  fi
fi

if [ "$OVERALL_STATUS" -eq 0 ]; then
  printf "%bAll test suites passed successfully!%b\n" "$GREEN" "$RESET"
else
  printf "%bSome test suites failed.%b\n" "$RED" "$RESET"
fi

exit "$OVERALL_STATUS"
