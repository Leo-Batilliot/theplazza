#!/usr/bin/env bash
# Run the unit-test binary under valgrind (memcheck) to catch leaks and invalid
# memory access. Exits non-zero on any error.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${ROOT}/tests/unit_tests"

if [ ! -x "${BIN}" ]; then
    echo "audit/valgrind: building tests first..."
    make -C "${ROOT}" "${BIN#${ROOT}/}"
fi

export CRITERION_TIMEOUT_MULTIPLIER="${CRITERION_TIMEOUT_MULTIPLIER:-15}"

FILTER_DEFAULT='!*stress*'
FILTER="${PLAZZA_VALGRIND_FILTER:-$FILTER_DEFAULT}"

LOG="${ROOT}/tests/audit/valgrind.log"
echo "audit/valgrind: filter=${FILTER}"
echo "audit/valgrind: writing log to ${LOG}"

set +e
valgrind \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=definite,indirect \
    --errors-for-leak-kinds=definite \
    --track-fds=yes \
    --error-exitcode=42 \
    --quiet \
    --trace-children=yes \
    --child-silent-after-fork=yes \
    "${BIN}" --filter "${FILTER}" -j1 2>"${LOG}"
RC=$?
set -e

if [ $RC -ne 0 ]; then
    echo "audit/valgrind: FAILED with exit code ${RC}"
    echo "  see ${LOG}"
    tail -n 40 "${LOG}" || true
    exit $RC
fi
echo "audit/valgrind: OK"
