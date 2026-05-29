# Standalone mincore Present-PTE Reproducer

This directory contains a standalone userspace reproducer for the
`no_thp_pte_scan_64m` path in `mm/mincore.c`.

It creates a private anonymous mapping, requests `MADV_NOHUGEPAGE`, faults in
all pages, and repeatedly calls `mincore()` over the resident base-page range.
The goal is to stress the present-PTE path in `mincore_pte_range()` without THP
PMD shortcut effects.

## Build And Run

```sh
gcc -O2 -Wall -Wextra -o mincore_present_pte_scan \
  mincore_present_pte_scan.c

./mincore_present_pte_scan no_thp_pte_scan_64m 1
```

The source also contains THP comparison scenarios:

```sh
./mincore_present_pte_scan thp_pmd_shortcut_64m 1
```

THP scenarios require kernel THP support and may skip if THP collapse is not
available.  The main candidate signal is the no-THP scenario.

## Output

The main timing fields are:

- `mincore_ns_avg`: average wall-clock nanoseconds per `mincore()` call.
- `mincore_ns_per_page`: wall-clock nanoseconds per scanned base page.
- `mincore_ns_per_1k_pages`: wall-clock nanoseconds per 1000 scanned pages.

Important semantic/state-shape fields:

- `expected_match_ratio` should be 100.
- `unexpected_results` should be 0.
- `resident_ratio_pct` should be 100.
- `thp_ratio_pct` should be 0 for `no_thp_pte_scan_64m`.

The formal lab evidence still comes from controlled QEMU/lab runs.  This file is
provided so reviewers can inspect and run the workload without the full
experiment framework.

Local host smoke checked on 2026-05-29:

```text
gcc -O2 -Wall -Wextra -o /tmp/mincore_present_pte_scan mincore_present_pte_scan.c
/tmp/mincore_present_pte_scan no_thp_pte_scan_4m 1
```

The smoke run completed with `unexpected_results=0`, `expected_match_ratio=100`,
`resident_ratio_pct=100`, and `thp_ratio_pct=0`.
