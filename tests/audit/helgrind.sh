#!/usr/bin/env bash
# Run the unit-test binary under helgrind to catch data races and lock-order issues.
# Exits non-zero if helgrind reports any errors.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${ROOT}/tests/unit_tests"

if [ ! -x "${BIN}" ]; then
    echo "audit/helgrind: building tests first..."
    make -C "${ROOT}" "${BIN#${ROOT}/}"
fi

# Helgrind is slow; let criterion crank up the timeout multiplier.
export CRITERION_TIMEOUT_MULTIPLIER="${CRITERION_TIMEOUT_MULTIPLIER:-20}"

# By default skip the fork+IPC e2e suites because helgrind chokes on fork().
FILTER_DEFAULT='!*e2e*,!*stress*,!*shutdown*,!signal_handler*,!process*,!*reaper*'
FILTER="${PLAZZA_HELGRIND_FILTER:-$FILTER_DEFAULT}"

LOG="${ROOT}/tests/audit/helgrind.log"
echo "audit/helgrind: filter=${FILTER}"
echo "audit/helgrind: writing log to ${LOG}"

set +e
valgrind \
    --tool=helgrind \
    --error-exitcode=42 \
    --quiet \
    --suppressions="${ROOT}/tests/audit/helgrind.supp" \
    --gen-suppressions=all \
    "${BIN}" --filter "${FILTER}" -j1 2>"${LOG}"
RC=$?
set -e

if [ $RC -ne 0 ]; then
    echo "audit/helgrind: FAILED with exit code ${RC}"
    echo "  see ${LOG}"
    tail -n 40 "${LOG}" || true
    exit $RC
fi
echo "audit/helgrind: OK"
