# MADV_PAGEOUT workload source

This directory contains the standalone userspace workload source and helper
script used for the `MADV_PAGEOUT + anonymous THP + no-swap + write-touch`
workflow in the `madvise_pageout_formal_refresh` profile.

It is meant for upstream reporting, bisection, and boundary checks. It is not
a replacement for the full `mm_regression_gen` evidence set.

## Files

- `madvise_pageout_refault_reproducer.c`
- `run_madvise_pageout_reproducer.sh`

## Defaults

The helper script defaults mirror the reportable experiment:

- mapping size: `16 MiB`
- THP mode: default policy, no explicit `MADV_NOHUGEPAGE`
- advice: `MADV_PAGEOUT`
- refault: one write-touch per page
- external rounds: `5`
- internal rounds: at least `4`, at most `256`
- minimum measured time per external round: `1200 ms`
- warmup cycles before each external round: `2`
- wrapper repetitions: `9`

## Smoke test

This only checks that the program builds and runs. It is not performance
evidence.

```sh
cd /path/to/linux-mm-regression-evidence-2026-05/madvise-pageout-thp-noswap-refault/workload
REPETITIONS=1 \
  ./run_madvise_pageout_reproducer.sh \
  --mapping-kb 4096 --external-rounds 1 --rounds 1 --max-rounds 1 --min-ms 0 --warmup-rounds 0
```

For a report-style standalone run inside a target kernel guest:

```sh
cd /path/to/linux-mm-regression-evidence-2026-05/madvise-pageout-thp-noswap-refault/workload
OUT_DIR=/tmp/madvise-pageout-$(uname -r) \
REPETITIONS=9 \
  ./run_madvise_pageout_reproducer.sh
```

Important fields:

- `advise_ns_per_page`
- `cycle_ns_per_page`
- `advise_ns_avg`
- `cycle_ns_avg`
- `expected_match_ratio`
- `unexpected_results`
- `errno_enomem`
- `errno_einval`

`cycle_ns_per_page` is `(advise_ns + post_touch_ns) / post_touch_pages`.

## THP controls

Use `--thp default`, `--thp nohugepage`, or `--thp hugepage` to control the
mapping request. `--dump-smaps` prints selected `/proc/self/smaps` fields
after prefault.

Use smaps output for path/state checks only, not as clean timing evidence.
