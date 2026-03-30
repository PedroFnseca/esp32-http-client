#!/usr/bin/env sh

set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
EXE_PATH="$BUILD_DIR/unit-tests"

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

# Enable colors in terminals and CI logs when explicitly requested.
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
"$COMPILER" -std=c++17 \
  -I"$SCRIPT_DIR/stubs" \
  -I"$REPO_ROOT/src" \
  "$SCRIPT_DIR/test_rest_request.cpp" \
  "$REPO_ROOT/src/RestRequest.cpp" \
  "$REPO_ROOT/src/ESP32HTTPClient.cpp" \
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

exit "$TEST_STATUS"
