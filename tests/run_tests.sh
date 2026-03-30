#!/usr/bin/env sh

set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
EXE_PATH="$BUILD_DIR/unit-tests"
TEST_OBJ="$BUILD_DIR/test_rest_request.o"
REST_REQUEST_OBJ="$BUILD_DIR/RestRequest.o"
HTTP_CLIENT_OBJ="$BUILD_DIR/ESP32HTTPClient.o"
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
  # Avoid stale counters when running coverage repeatedly.
  rm -f "$BUILD_DIR"/*.gcda "$BUILD_DIR"/*.gcno
else
  COVERAGE_FLAGS=""
fi

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  -c "$SCRIPT_DIR/test_rest_request.cpp" \
  -o "$TEST_OBJ"

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
  -c "$REPO_ROOT/src/ESP32HTTPClient.cpp" \
  -o "$HTTP_CLIENT_OBJ"

"$COMPILER" -std=c++17 \
  $COVERAGE_FLAGS \
  "$TEST_OBJ" \
  "$REST_REQUEST_OBJ" \
  "$HTTP_CLIENT_OBJ" \
  -o "$EXE_PATH"

printf "%bRunning unit tests...%b\n" "$CYAN" "$RESET"

set +e
TEST_OUTPUT="$("$EXE_PATH" 2>&1)"
TEST_STATUS=$?
set -e

printf "%s\n" "$TEST_OUTPUT" | while IFS= read -r line; do
  case "$line" in
    *"[RUN ]"*)
      printf "%b%s%b\n" "$CYAN" "$line" "$RESET"
      ;;
    *"[PASS]"*)
      printf "%b%s%b\n" "$GREEN" "$line" "$RESET"
      ;;
    *"[FAIL]"*)
      printf "%b%s%b\n" "$RED" "$line" "$RESET"
      ;;
    *"=== Test Summary ==="*)
      printf "%b%s%b\n" "$CYAN" "$line" "$RESET"
      ;;
    *"Pass rate:"*)
      if [ "$TEST_STATUS" -eq 0 ]; then
        printf "%b%s%b\n" "$GREEN" "$line" "$RESET"
      else
        printf "%b%s%b\n" "$YELLOW" "$line" "$RESET"
      fi
      ;;
    *"All unit tests passed."*)
      printf "%b%s%b\n" "$GREEN" "$line" "$RESET"
      ;;
    *"test(s) failed."*)
      printf "%b%s%b\n" "$YELLOW" "$line" "$RESET"
      ;;
    *)
      printf "%s\n" "$line"
      ;;
  esac
done

if [ "$SHOW_COVERAGE" = "1" ] && command -v gcov >/dev/null 2>&1; then
  set +e
  GCOV_OUTPUT="$(
    cd "$BUILD_DIR" &&
    gcov -b -c -o "$BUILD_DIR" \
      "$REPO_ROOT/src/RestRequest.cpp" \
      "$REPO_ROOT/src/ESP32HTTPClient.cpp" 2>/dev/null
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
        printf "%s|%.2f|%d\n", file, pct + 0, cnt + 0
        file = ""
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
elif [ "$SHOW_COVERAGE" = "1" ]; then
  printf "%bgcov not found; install gcov to print code coverage.%b\n" "$YELLOW" "$RESET"
fi

exit "$TEST_STATUS"
