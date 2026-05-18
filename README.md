# Linux MM Regression Evidence - May 2026

This repository contains a small evidence bundle for two Linux MM performance regression reports.

The reports are intentionally split by kernel path and workload:

- `madvise-pageout-thp-noswap-refault/`: `madvise(MADV_PAGEOUT)` on an anonymous THP/no-swap refault workflow.
- `mprotect-shared-dirty-toggle/`: repeated `mprotect()` toggling over a shared dirty PTE mapping.

The bundle is meant to support upstream regression reports, not to claim broad benchmark coverage. Each claim is scoped to the workload described in the corresponding directory.

## Top-level reports

- `reports/upstream_regression_handoff_2026-05-15.md`: English handoff summary.
- `reports/upstream_regression_handoff_2026-05-15.zh-CN.md`: Chinese translation for local review.

## Evidence layout

Each workload directory contains:

- `experiments/`: the exact experiment profile used by the local framework.
- `formal-lab/`: lab-server formal timing evidence and run metadata.
- `release-narrowing/`: local and lab 1CPU release-narrowing sanity checks.
- `reproducer/` or `generated-workload/`: standalone or generated workload source.

Important run metadata files:

- `pipeline_run_env.json`: QEMU, host, cgroup, profile, and execution metadata.
- `execution_order.json`: interleaved version execution order for timing runs.
- `*.summary.csv/json`: summarized result tables.
- `*.raw.csv/json`: per-run raw measurements.

## Main caveats

The reports are not claiming generic `madvise()` or generic `mprotect()` regressions.

The `MADV_PAGEOUT` claim is scoped to the anonymous THP/no-swap refault/write-touch workflow.

The `mprotect()` claim is scoped to the shared dirty full-range protection-toggle workload; the cleanest current formal result is the 1CPU lab run, while 2CPU and 4CPU are same-direction supporting evidence with reliability caveats.

No culprit commit has been bisected yet. The release narrowing currently points to `v6.12..v6.18`.

