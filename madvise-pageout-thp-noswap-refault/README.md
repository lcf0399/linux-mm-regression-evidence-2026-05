# MADV_PAGEOUT THP/no-swap Reclaim-failure Evidence

This directory was created for the original report:

```text
[REGRESSION] mm: MADV_PAGEOUT on THP/no-swap refault workflow is ~40% slower on v6.19 than v6.12
```

The directory name and original report title preserve the initial `refault`
wording. After upstream review, the current and more accurate scope is a
`MADV_PAGEOUT` anon/THP no-swap reclaim-failure path; the later write-touch is
part of the workload iteration and should not be treated as proof of a real
pageout/refault.

## Claim scope

This is a userspace-visible `madvise(MADV_PAGEOUT)` workload:

- anonymous 16 MiB mapping
- default THP path
- no guest swap
- `MADV_PAGEOUT`, then write-touch as the second half of the iteration

It does not claim that all `MADV_PAGEOUT` workloads regress.

## Formal Timing Result

Formal lab timing shows `v6.19.9` slower than `v6.12.77` across `1/2/4` vCPUs.
After upstream review, the important caveat is that these formal timing runs
did not record smaps/page-state. Subsequent local and lab attribution runs
found that actual THP backing can differ between the two kernels in this
workload shape. Therefore, the timing table below remains the original formal
timing result, but it should not be described as a same-state THP regression.

`cycle_ns_per_page`:

| CPU | v6.12.77 | v6.19.9 | delta |
| --- | ---: | ---: | ---: |
| 1 | 1900.3 | 3304.7 | -42.5% |
| 2 | 2107.7 | 3583.2 | -41.2% |
| 4 | 2154.2 | 3690.9 | -41.6% |

`cycle_ns_per_page` means wall-clock nanoseconds per page for one full workload
iteration. It is not a CPU-cycle counter.

`advise_ns_per_page`:

| CPU | v6.12.77 | v6.19.9 | delta |
| --- | ---: | ---: | ---: |
| 1 | 1713.2 | 2922.7 | -41.4% |
| 2 | 1924.7 | 3162.9 | -39.1% |
| 4 | 1953.1 | 3284.2 | -40.5% |

Separate release-level sanity checks showed `v6.18.19` already in the slow range, but those raw runs are kept out of this compact public evidence bundle.

## Follow-up Attribution Result

The lab ftrace/smaps follow-up covers:

- `QEMU_SMP=1/2/4` with `QEMU_MEM_MB=14336`
- `QEMU_SMP=8` with `QEMU_MEM_MB=16384`
- `QEMU_SMP=16` with `QEMU_MEM_MB=32768`
- THP modes: `default`, `hugepage`, `nohugepage`

This follow-up is attribution evidence, not clean timing evidence. It found
the same page-state pattern across the extended matrix:

- In `default` and `hugepage` modes, `v6.12.77` has `AnonHugePages=0 kB`,
  while `v6.19.9` has `AnonHugePages=16384 kB`.
- In those THP-backed `v6.19.9` runs, `split_folio_to_list()` is hit; it is
  not hit on `v6.12.77`.
- In `nohugepage` mode, both kernels have `AnonHugePages=0 kB`, neither hits
  `split_folio_to_list()`, and the old-version-faster signal is not stable.

Current interpretation: the original timing signal should not be presented as
a same-state THP regression. It is better described as a no-swap
`MADV_PAGEOUT` anon/THP reclaim split/failure-path observation. The detailed
attribution summary is in `attribution/summaries/`.

## Directories

- `workload/`: standalone workload source and helper script.
- `experiments/`: formal experiment profile.
- `formal-lab/perf_{1,2,4}cpu/`: clean performance runs with coverage disabled.
- `formal-lab/coverage_1cpu/`: direct-hit coverage evidence collected separately from clean timing.
- `attribution/`: ftrace/smaps attribution summaries and scripts.
