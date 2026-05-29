# mincore Present-PTE Lab Validation Summary

These CSV files are compact summaries copied from the local lab run outputs.
They are meant for maintainer review, not as a full raw-run archive.

Common setup for the key timing rows:

- QEMU direct boot on the lab host.
- Guest CPUs: `QEMU_SMP=1/2/4`.
- Repetitions: 9 for timing A/B rows.
- Coverage: disabled for timing.
- Primary scenario: `no_thp_pte_scan_64m`.
- Primary metric: `mincore_ns_per_1k_pages`, lower is better.

## Key Results

v6.16 introduction-window A/B:

```text
source: v6.16-fastpath-ab.summary.csv

CPU  v6.15      v6.16      v6.16 fastpath  v6.16 nobatch
1    12946.889  17117.667  14560.556       13843.222
2    15053.111  18214.667  15714.778       14270.556
4    14942.000  18338.222  14397.889       14719.667
```

v6.18 present-first confirmation:

```text
source: v6.18-presentfirst-confirm.summary.csv

CPU  v6.15      v6.18      v6.18 present-first
1    13373.222  16473.000  11055.222
2    13454.444  16424.444  11467.556
4    13651.778  16772.333  11470.444
```

v7.0 present-first A/B:

```text
source: v7.0-presentfirst-ab.summary.csv

CPU  v7.0.9     v7.0.9 present-first
1    16328.778  10061.778
2    17600.000  11856.444
4    17819.000  11961.556
```

v6.18 all-scenario semantic smoke:

```text
source: v6.18-presentfirst-allscenario-smoke.summary.csv

The v6.18 original and present-first kernels both completed THP and no-THP
scenarios with all_semantic_ok=true.
```

ftrace attribution:

```text
source: v6.16-intro-ftrace.profile.csv

The v6.16 original kernel shows a higher avg_us in mincore_pte_range than
v6.15, v6.16-nobatch, and v6.16-fastpath.
```

## Interpretation

The current best statement is:

```text
narrowed suspect + candidate fix shape validated on x86 lab
```

This is not yet an upstream-ready fix.  The remaining gap is arm64 or
mTHP/large-folio preservation validation.
