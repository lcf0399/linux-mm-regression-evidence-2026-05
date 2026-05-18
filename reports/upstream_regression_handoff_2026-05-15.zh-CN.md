# 上游性能回归提交交接稿 - 2026-05-15

## 用途

这份文档是给上游提交用的交接稿，目标是把目前已经确认的两个 Linux kernel 性能回归拆成两封独立报告。

当前证据已经足够先向开发者汇报，即使还没有定位到具体 culprit commit。后续如果能 bisect 到具体提交当然更好，但不应成为首次报告的阻塞点。

注意：这两个问题不要合并成一封邮件。它们触发路径、涉及文件和维护者范围都不同。

## 报告 1：MADV_PAGEOUT / THP / no-swap refault 工作流

### 建议邮件标题

```text
[REGRESSION] mm: MADV_PAGEOUT on THP/no-swap refault workflow is ~40% slower on v6.19 than v6.12
```

### 建议收件人

以下名单来自：

```bash
linux-v6.19/scripts/get_maintainer.pl -f mm/madvise.c mm/vmscan.c mm/swapfile.c mm/huge_memory.c
```

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

### 核心结论

这是一个用户态可观察到的 `madvise(MADV_PAGEOUT)` 工作流性能回归。

测试场景是：匿名 16 MiB 映射，默认 THP 路径，guest 内无 swap，然后执行 refault/write-touch。

在 lab 环境中，`v6.19.9` 相比 `v6.12.77` 在 `1/2/4` vCPU 下均慢约 39% 到 42%。

release narrowing 显示 `v6.18.19` 已经进入慢速状态。

目前尚未 bisect 到具体 culprit commit。

### 正式 lab 证据

结果目录：

```text
mm_regression_gen/out/confirmed_regressions_refresh_lab_20260513T121012Z/madvise_pageout_formal_refresh
```

测试设置：

```text
QEMU_MEM_MB=14336
QEMU_SMP=1/2/4
SMP/ACPI/ACPI_PROCESSOR enabled
cmdline: tsc=unstable clocksource=refined-jiffies
noapic=false
repetitions=9
performance coverage_enabled=false
```

`cycle_ns_per_page`：

| CPU | v6.12.77 | v6.19.9 | 变化 |
| --- | ---: | ---: | ---: |
| 1 | 1900.3 | 3304.7 | -42.5% |
| 2 | 2107.7 | 3583.2 | -41.2% |
| 4 | 2154.2 | 3690.9 | -41.6% |

`advise_ns_per_page`：

| CPU | v6.12.77 | v6.19.9 | 变化 |
| --- | ---: | ---: | ---: |
| 1 | 1713.2 | 2922.7 | -41.4% |
| 2 | 1924.7 | 3162.9 | -39.1% |
| 4 | 1953.1 | 3284.2 | -40.5% |

### release narrowing sanity check

`advise_ns_per_page`：

| 平台 | v6.12.77 | v6.18.19 | v6.19.9 |
| --- | ---: | ---: | ---: |
| local | 932.0 | 1609.0 | 1587.0 |
| lab | 1720.0 | 2820.0 | 2839.0 |

### regzbot 建议

```text
#regzbot introduced: v6.12..v6.18
```

### 汇报时要带上的限制条件

这不是泛化的 madvise 性能回归。

不要声称所有 `MADV_PAGEOUT` 场景都会退化。

当前最强结论绑定在 THP 默认路径、guest 无 swap、refault/write-touch 这个组合场景上。

coverage 数据是单独收集的，干净 timing 结果没有打开 coverage。

目前没有 bisect 到具体 culprit commit。

### 相关材料

```text
/home/lcf/kernel-study/mm_regression_gen/reproducers/madvise_pageout/madvise_pageout_refault_reproducer.c
/home/lcf/kernel-study/mm_regression_gen/reproducers/madvise_pageout/README.md
/home/lcf/kernel-study/mm_regression_gen/madvise/experiments/madvise_pageout_formal_refresh.toml
/home/lcf/kernel-study/mm_regression_gen/madvise/experiments/madvise_pageout_reproducer_release_narrowing.toml
```

## 报告 2：mprotect shared-dirty PTE toggle 路径

### 建议邮件标题

```text
[REGRESSION] mm/mprotect: shared dirty PTE toggle path is ~40% slower on v6.19 than v6.12
```

### 建议收件人

以下名单来自：

```bash
linux-v6.19/scripts/get_maintainer.pl -f mm/mprotect.c
```

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

### 核心结论

这是一个用户态可观察到的 `mprotect()` workload 性能回归。

测试场景是：shared dirty 64 MiB 映射，反复对整个 range 做 protection toggle。

在最新干净 lab formal 结果里，`v6.19.9` 相比 `v6.12.77` 在 `1CPU` 下慢约 40%。

`2CPU` 同方向，但属于 robust-only 证据；`4CPU` 也同方向，但因为有一次失败运行，所以只能作为 partial same-direction 证据。

release narrowing 的 local 和 lab 结果都显示 `v6.18.19` 已经进入慢速状态。

目前尚未 bisect 到具体 culprit commit。

### 正式 lab 证据

结果目录：

```text
mm_regression_gen/out/confirmed_regressions_refresh_lab_20260513T121012Z/mprotect_shared_dirty_formal_refresh
```

测试设置：

```text
QEMU_MEM_MB=14336
QEMU_SMP=1/2/4
SMP/ACPI/ACPI_PROCESSOR enabled
cmdline: tsc=unstable clocksource=refined-jiffies
noapic=false
repetitions=9
performance coverage_enabled=false
```

`cycle_ns_per_page`：

| CPU | v6.12.77 | v6.19.9 | 变化 | 可靠性 |
| --- | ---: | ---: | ---: | --- |
| 1 | 346.8 | 578.1 | -40.0% | reliable |
| 2 | 394.7 | 641.7 | -38.5% | robust-only |
| 4 | 381.1 | 624.8 | -39.0% | partial same direction |

### release narrowing sanity check

`cycle_ns_per_page`：

| 平台 | v6.12.77 | v6.18.19 | v6.19.9 |
| --- | ---: | ---: | ---: |
| local | 277.0 | 478.0 | 405.0 |
| lab | 279.0 | 756.0 | 523.0 |

### regzbot 建议

```text
#regzbot introduced: v6.12..v6.18
```

### 汇报时要带上的限制条件

这不是泛化的 mprotect 性能回归。

不要声称 `anon_full_toggle` 或 THP mprotect 场景也存在回归。

不要声称最新 formal 中 `1/2/4` CPU 全部都是 clean reliable。当前最干净、最可靠的是 `1CPU` lab formal；`2CPU` 和 `4CPU` 只能作为同方向补充证据。

机制假设是：`change_pte_range()` batching 在 shared-dirty 且 `nr_ptes=1` 路径上的额外开销可能更明显。

目前没有 bisect 到具体 culprit commit。

### 相关材料

```text
/home/lcf/kernel-study/mm_regression_gen/mprotect/experiments/mprotect_shared_dirty_formal_refresh.toml
/home/lcf/kernel-study/mm_regression_gen/mprotect/experiments/mprotect_shared_dirty_release_narrowing.toml
/home/lcf/kernel-study/mm_regression_gen/out/reports/platform_reruns/root_cause_line_level_attribution.zh-CN.md
```

## 建议下一步

建议先发 MADV_PAGEOUT 这一个。它的最新 lab formal 矩阵在 `1/2/4` CPU 上都可靠，而且有独立 reproducer，作为第一封上游报告更干净。

mprotect 可以作为第二封报告提交，也有价值，但邮件里必须明确写出 `2CPU/4CPU` 的可靠性限制，避免把结论说得过满。

在第一封报告发出前，不建议继续把主要时间花在 commit-level bisection 上。邮件里如实写 `not bisected yet` 是可以接受的；如果 maintainer 要求进一步缩小，再继续做 bisect 或 release-level narrowing。
