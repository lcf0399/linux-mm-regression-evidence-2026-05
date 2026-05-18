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

## Environment summary

The formal evidence uses the lab server as the primary timing environment:

- Host label in metadata: `lcf`
- Host kernel: `Linux 6.14.0-37-generic x86_64`
- Host CPU count: 128
- Container/cgroup CPU set: `0,2,4,6,8,10,12,14`
- Container/cgroup memory limit: `16106127360` bytes
- QEMU: `qemu-system-x86_64 8.2.2`
- Guest memory: `QEMU_MEM_MB=14336`
- Guest CPUs: `QEMU_SMP=1/2/4`, recorded separately in each `formal-lab/perf_*cpu/pipeline_run_env.json`
- Timing order: interleaved version order, recorded in each `execution_order.json`
- Formal repetitions: 9
- Performance timing: coverage disabled

The release-narrowing sanity checks include both lab and local runs:

- Local host label in metadata: `kernel-vm`
- Local host kernel: `Linux 6.8.0-110-generic x86_64`
- Local host CPU count: 8
- Local QEMU: `qemu-system-x86_64 8.2.2`
- Local guest memory: `QEMU_MEM_MB=4096`
- Release-narrowing versions: `v6.12`, `v6.18`, and `v6.19`
- Release-narrowing repetitions: 1, used only as a sanity check for the introduction range

The exact per-run environment remains in `pipeline_run_env.json`; this section is only a human-readable summary.

## Data selection policy

This repository only includes the latest citable evidence bundle for the two reports:

- latest lab formal refresh results for the main performance claims
- latest local/lab release-narrowing results for introduction-range sanity checks
- separate coverage evidence where useful, kept distinct from clean performance timing

Older screenings, invalid runs, failed runs, instrumentation-contaminated runs, and exploratory intermediate outputs are intentionally not uploaded here. They should be kept locally in an archive with a manifest if future auditability is needed, rather than mixed into the upstream evidence bundle.

## Main caveats

The reports are not claiming generic `madvise()` or generic `mprotect()` regressions.

The `MADV_PAGEOUT` claim is scoped to the anonymous THP/no-swap refault/write-touch workflow.

The `mprotect()` claim is scoped to the shared dirty full-range protection-toggle workload; the cleanest current formal result is the 1CPU lab run, while 2CPU and 4CPU are same-direction supporting evidence with reliability caveats.

No culprit commit has been bisected yet. The release narrowing currently points to `v6.12..v6.18`.
