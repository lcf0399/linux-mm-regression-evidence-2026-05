# Upstream Regression Handoff - 2026-05-15

This handoff is meant for sending two separate Linux kernel regression
reports. The current evidence is strong enough to report without waiting
for a culprit commit. A later bisect would still be useful, but it should
not block the first report.

Do not send this as one combined regression report. Send the two issues as
two independent threads.

## Report 1: MADV_PAGEOUT / THP / no-swap refault workflow

Suggested subject:

```text
[REGRESSION] mm: MADV_PAGEOUT on THP/no-swap refault workflow is ~40% slower on v6.19 than v6.12
```

Suggested recipients from `linux-v6.19/scripts/get_maintainer.pl -f
mm/madvise.c mm/vmscan.c mm/swapfile.c mm/huge_memory.c`:

```text
Andrew Morton <akpm@linux-foundation.org>
"Liam R. Howlett" <Liam.Howlett@oracle.com>
Lorenzo Stoakes <lorenzo.stoakes@oracle.com>
David Hildenbrand <david@kernel.org>
Vlastimil Babka <vbabka@suse.cz>
Jann Horn <jannh@google.com>
Johannes Weiner <hannes@cmpxchg.org>
Michal Hocko <mhocko@kernel.org>
Qi Zheng <zhengqi.arch@bytedance.com>
Shakeel Butt <shakeel.butt@linux.dev>
Chris Li <chrisl@kernel.org>
Kairui Song <kasong@tencent.com>
linux-mm@kvack.org
linux-kernel@vger.kernel.org
regressions@lists.linux.dev
```

Core claim:

- Userspace-visible `madvise(MADV_PAGEOUT)` workflow.
- Anonymous 16 MiB mapping, default THP path, no configured guest swap,
  then refault/write-touch.
- `v6.19.9` is about 39-42% slower than `v6.12.77` on the lab machine
  across `1/2/4` vCPUs.
- Release narrowing shows `v6.18.19` already in the slow band.
- Culprit commit is not bisected yet.

Primary formal lab evidence:

```text
result dir:
  mm_regression_gen/out/confirmed_regressions_refresh_lab_20260513T121012Z/madvise_pageout_formal_refresh

setup:
  QEMU_MEM_MB=14336
  QEMU_SMP=1/2/4
  SMP, ACPI, ACPI_PROCESSOR
  cmdline: tsc=unstable clocksource=refined-jiffies
  noapic=false
  repetitions=9
  performance run: coverage_enabled=false
```

`cycle_ns_per_page`, lower is better:

```text
CPU   v6.12.77   v6.19.9   delta
  1     1900.3    3304.7   -42.5%
  2     2107.7    3583.2   -41.2%
  4     2154.2    3690.9   -41.6%
```

`advise_ns_per_page`, lower is better:

```text
CPU   v6.12.77   v6.19.9   delta
  1     1713.2    2922.7   -41.4%
  2     1924.7    3162.9   -39.1%
  4     1953.1    3284.2   -40.5%
```

Release narrowing sanity:

```text
platform   v6.12.77   v6.18.19   v6.19.9   metric
local         932.0     1609.0    1587.0   advise_ns_per_page
lab          1720.0     2820.0    2839.0   advise_ns_per_page
```

Suggested regzbot line:

```text
#regzbot introduced: v6.12..v6.18
```

Important caveats to say explicitly:

- This is not a generic `madvise` regression.
- This is not a claim that all `MADV_PAGEOUT` use cases regress.
- The current worst case is tied to THP default + no-swap + refault/write-touch.
- Coverage was collected separately from clean timing runs.
- No culprit commit has been bisected yet.

Useful local artifacts:

- [standalone reproducer C](/home/lcf/kernel-study/mm_regression_gen/reproducers/madvise_pageout/madvise_pageout_refault_reproducer.c)
- [standalone reproducer README](/home/lcf/kernel-study/mm_regression_gen/reproducers/madvise_pageout/README.md)
- [formal profile](/home/lcf/kernel-study/mm_regression_gen/madvise/experiments/madvise_pageout_formal_refresh.toml)
- [release narrowing profile](/home/lcf/kernel-study/mm_regression_gen/madvise/experiments/madvise_pageout_reproducer_release_narrowing.toml)

## Report 2: mprotect shared-dirty PTE toggle path

Suggested subject:

```text
[REGRESSION] mm/mprotect: shared dirty PTE toggle path is ~40% slower on v6.19 than v6.12
```

Suggested recipients from `linux-v6.19/scripts/get_maintainer.pl -f
mm/mprotect.c`:

```text
Andrew Morton <akpm@linux-foundation.org>
"Liam R. Howlett" <Liam.Howlett@oracle.com>
Lorenzo Stoakes <lorenzo.stoakes@oracle.com>
Vlastimil Babka <vbabka@suse.cz>
Jann Horn <jannh@google.com>
Pedro Falcato <pfalcato@suse.de>
linux-mm@kvack.org
linux-kernel@vger.kernel.org
regressions@lists.linux.dev
```

Core claim:

- Userspace-visible `mprotect()` workload.
- Shared dirty 64 MiB mapping, repeated full-range protection toggle.
- `v6.19.9` is about 40% slower than `v6.12.77` in the clean `1CPU`
  lab formal result.
- `2CPU` is same direction but robust-only; `4CPU` is same direction but
  partial due one failed run.
- Release narrowing on both local and lab shows `v6.18.19` already in the
  slow band.
- Culprit commit is not bisected yet.

Primary formal lab evidence:

```text
result dir:
  mm_regression_gen/out/confirmed_regressions_refresh_lab_20260513T121012Z/mprotect_shared_dirty_formal_refresh

setup:
  QEMU_MEM_MB=14336
  QEMU_SMP=1/2/4
  SMP, ACPI, ACPI_PROCESSOR
  cmdline: tsc=unstable clocksource=refined-jiffies
  noapic=false
  repetitions=9
  performance run: coverage_enabled=false
```

`cycle_ns_per_page`, lower is better:

```text
CPU   v6.12.77   v6.19.9   delta       reliability
  1      346.8     578.1   -40.0%      reliable
  2      394.7     641.7   -38.5%      robust-only
  4      381.1     624.8   -39.0%      partial, same direction
```

Release narrowing sanity:

```text
platform   v6.12.77   v6.18.19   v6.19.9   metric
local         277.0      478.0     405.0   cycle_ns_per_page
lab           279.0      756.0     523.0   cycle_ns_per_page
```

Suggested regzbot line:

```text
#regzbot introduced: v6.12..v6.18
```

Important caveats to say explicitly:

- This is not a generic `mprotect` regression.
- Do not claim `anon_full_toggle` or THP mprotect regresses.
- Do not claim all `1/2/4CPU` cases are clean reliable; only `1CPU` is
  clean reliable in the latest lab formal matrix.
- Current mechanism hypothesis is `change_pte_range()` batching overhead
  in a shared-dirty `nr_ptes=1` path.
- No culprit commit has been bisected yet.

Useful local artifacts:

- [formal profile](/home/lcf/kernel-study/mm_regression_gen/mprotect/experiments/mprotect_shared_dirty_formal_refresh.toml)
- [release narrowing profile](/home/lcf/kernel-study/mm_regression_gen/mprotect/experiments/mprotect_shared_dirty_release_narrowing.toml)
- [batch attribution discussion](/home/lcf/kernel-study/mm_regression_gen/out/reports/platform_reruns/root_cause_line_level_attribution.zh-CN.md)

## Recommended next action

Send the `MADV_PAGEOUT` report first. It is the cleaner report because
the latest lab formal matrix is reliable across `1/2/4` vCPUs and there
is a standalone reproducer.

Send the `mprotect` report second. It is still worth reporting, but the
mail must carry the reliability caveat for `2CPU/4CPU`.

Do not spend more time on commit-level bisection before the first report
unless a maintainer asks for it. The honest wording "not bisected yet" is
acceptable for an initial regression report with this much reproducible
evidence.
