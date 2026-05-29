#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail

cd "$(dirname "$0")"

: "${CC:=gcc}"
: "${SCENARIO:=no_thp_pte_scan_64m}"
: "${EXTERNAL_ROUNDS:=1}"
: "${OUT:=/tmp/mincore_present_pte_scan}"

"$CC" -O2 -Wall -Wextra -o "$OUT" \
  mincore_present_pte_scan.c

exec "$OUT" "$SCENARIO" "$EXTERNAL_ROUNDS"
