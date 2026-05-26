# Standalone Reproducer Lab Validation

This directory summarizes lab validation runs of the standalone
`mprotect_shared_dirty_reproducer.c` workload.

The purpose is narrower than the earlier formal framework runs: it checks that
the smaller maintainer-facing reproducer preserves the same timing direction on
the lab QEMU setup.

## SMP Lab Follow-Up

After the initial screening run, the kernels were rebuilt with an SMP-capable
x86/QEMU direct-boot configuration:

- `CONFIG_SMP=y`
- `CONFIG_NR_CPUS=16`
- `CONFIG_ACPI=y`
- `CONFIG_ACPI_PROCESSOR=y`
- guest cmdline without `noapic`

The follow-up matrix used the same standalone reproducer and the same
interleaved clean-performance setup:

- host label: `lcf`
- QEMU: direct boot
- kernels: `v6.12.77`, `v6.19.9`, `akpm/mm mm-unstable 444fc9435e57`
- guest CPUs: `QEMU_SMP=1/2/4/8/16`
- guest memory: `14336 MiB` for 1/2/4 CPU, `16384 MiB` for 8 CPU,
  `32768 MiB` for 16 CPU
- repetitions: `5`
- order: interleaved
- coverage: disabled
- extra guest cmdline: `tsc=unstable clocksource=refined-jiffies`
- workload external rounds: `5`

Serial-log validation resolved the logs from the run reports and checked the
actual guest CPU count. The result was:

- `1/2/4/8 CPU`: 15 serial logs checked per row, `cpu_mismatches=0`,
  `noapic_logs=0`
- `16 CPU`: 14 serial logs checked, `cpu_mismatches=0`, `noapic_logs=0`

The 16 CPU row is kept as an extended follow-up row. It had one v6.12.77 QEMU
failure, so it should be treated as supporting evidence rather than the cleanest
primary row.

Primary metric: `iteration_ns_per_page`, lower is better.

| Guest CPUs | Guest memory | v6.12.77 | v6.19.9 | mm-unstable | mm-unstable vs v6.19 | v6.12 -> v6.19 gap closed |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 14336 MiB | 296.4 | 548.6 | 498.6 | 9.1% faster | 19.8% |
| 2 | 14336 MiB | 327.2 | 564.8 | 488.4 | 13.5% faster | 32.2% |
| 4 | 14336 MiB | 319.8 | 578.2 | 505.8 | 12.5% faster | 28.0% |
| 8 | 16384 MiB | 336.4 | 570.4 | 508.2 | 10.9% faster | 26.6% |
| 16 | 32768 MiB | 380.0 | 624.0 | 553.8 | 11.3% faster | 28.8% |

This SMP follow-up confirms that the standalone reproducer keeps the same broad
direction under real multi-CPU guest enumeration: `v6.19.9` is slower than
`v6.12.77`, and current `mm-unstable` improves the result but does not return to
the `v6.12.77` level in this setup.

The per-phase fields show the largest gap in the `mprotect(PROT_READ)` and
`mprotect(PROT_READ | PROT_WRITE)` phases. See
`lab-smp-summary-20260526.csv` for the full per-metric table.

## Earlier Single-CPU Screening

The first validation attempt requested `QEMU_SMP=1/2/4`, but a post-run config
review found these kernel builds were non-SMP: `# CONFIG_SMP is not set` and
`CONFIG_NR_CPUS=1`. Therefore the 2026-05-25 table is preserved only as a
single-CPU reproducer screening run under different requested QEMU SMP values.

| Requested QEMU_SMP | v6.12.77 | v6.19.9 | mm-unstable | mm-unstable vs v6.19 | v6.12 -> v6.19 gap closed |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 301.6 | 563.2 | 477.6 | 15.2% faster | 32.7% |
| 2 | 306.2 | 542.4 | 488.4 | 10.0% faster | 22.9% |
| 4 | 305.6 | 552.4 | 479.2 | 13.3% faster | 29.7% |

## Caveat

These validation runs are 5-repeat screening runs for a smaller reproducer, not
a replacement for the earlier formal evidence. The QEMU guest run reports
`expected_match_ratio=100` and `unexpected_results=0`, but the minimal guest
environment does not provide the same smaps state-shape visibility as the
separate state audit. The state-shape conclusion remains based on
`../state-audit-lab/`.

## Files

- `lab-smp-iteration-comparison-20260526.csv`: compact SMP follow-up comparison.
- `lab-smp-summary-20260526.csv`: per-version/per-metric SMP follow-up summary.
- `lab-smp-summary-20260526.json`: same SMP follow-up data as JSON.
- `lab-smp-serial-check-20260526.json`: serial-log guest CPU validation for
  the SMP follow-up.
- `lab-iteration-comparison-20260525.csv`: earlier non-SMP screening table.
- `lab-summary-20260525.csv`: earlier per-version/per-metric summary.
- `lab-summary-20260525.json`: same earlier data with deduplication metadata.
- `profile/`: generated workload profile used by the runner.
- `../reproducer/`: canonical standalone source used by this validation.
- `run-env/`: run environment, execution order, and completion sentinels /
  metadata for the lab rows.
