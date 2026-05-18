# Linux MM 性能回归证据包 - 2026 年 5 月

这个仓库包含一份精简证据包，用于支撑两个 Linux MM 性能回归报告。

这两个报告刻意按 kernel 路径和 workload 拆开：

- `madvise-pageout-thp-noswap-refault/`：匿名 THP、guest 无 swap、refault 工作流中的 `madvise(MADV_PAGEOUT)`。
- `mprotect-shared-dirty-toggle/`：shared dirty PTE 映射上的重复 `mprotect()` protection toggle。

这份证据包的目的，是支撑上游 regression report，而不是声称我们做了广泛 benchmark 覆盖。每个结论都只限定在对应目录里描述的 workload 场景内。

## 顶层报告

- `reports/upstream_regression_handoff_2026-05-15.md`：英文交接摘要。
- `reports/upstream_regression_handoff_2026-05-15.zh-CN.md`：供本地审阅的中文翻译。

## 证据目录结构

每个 workload 目录都包含：

- `experiments/`：本地框架实际使用的实验 profile。
- `formal-lab/`：lab server 上的正式 timing 证据和运行元数据。
- `release-narrowing/`：local 和 lab 的 1CPU release narrowing sanity check。
- `reproducer/` 或 `generated-workload/`：独立 reproducer 或框架生成的 workload 源码。

重要运行元数据文件：

- `pipeline_run_env.json`：QEMU、host、cgroup、profile 和执行环境信息。
- `execution_order.json`：timing run 使用的 interleaved 版本执行顺序。
- `*.summary.csv/json`：汇总后的结果表。
- `*.raw.csv/json`：每次重复运行的原始测量值。

## 实验环境摘要

正式证据以 lab server 作为主要 timing 环境：

- 元数据中的 host label：`lcf`
- host kernel：`Linux 6.14.0-37-generic x86_64`
- host CPU 数：128
- container/cgroup CPU set：`0,2,4,6,8,10,12,14`
- container/cgroup memory limit：`16106127360` bytes
- QEMU：`qemu-system-x86_64 8.2.2`
- guest memory：`QEMU_MEM_MB=14336`
- guest CPU：`QEMU_SMP=1/2/4`，每个值分别记录在 `formal-lab/perf_*cpu/pipeline_run_env.json` 中
- timing 顺序：interleaved version order，记录在各自的 `execution_order.json` 中
- formal repetitions：9
- performance timing：关闭 coverage

release narrowing sanity check 同时保留 lab 和 local：

- local host label：`kernel-vm`
- local host kernel：`Linux 6.8.0-110-generic x86_64`
- local host CPU 数：8
- local QEMU：`qemu-system-x86_64 8.2.2`
- local guest memory：`QEMU_MEM_MB=4096`
- release narrowing 版本：`v6.12`、`v6.18`、`v6.19`
- release narrowing repetitions：1，只用于判断引入范围的 sanity check

精确的逐次运行环境仍以 `pipeline_run_env.json` 为准；这里是给人快速阅读的摘要。

## 数据取舍策略

这个仓库只放两个报告当前最新、可引用的证据包：

- 用于主要性能结论的最新 lab formal refresh 结果
- 用于 introduction range sanity check 的最新 local/lab release narrowing 结果
- 必要的 coverage 证据，但和 clean performance timing 分开保存

旧 screening、无效 run、失败 run、被 instrumentation 污染的 run、探索性中间产物，都不上传到这里。它们如果未来还需要审计，建议本地归档并附 manifest，不要混进上游证据包。

## 主要限制条件

这些报告不声称存在泛化的 `madvise()` 或泛化的 `mprotect()` 性能回归。

`MADV_PAGEOUT` 的结论只限定在匿名 THP、guest 无 swap、refault/write-touch 工作流上。

`mprotect()` 的结论只限定在 shared dirty full-range protection-toggle workload 上；当前最干净的 formal 结果是 lab 1CPU run，2CPU 和 4CPU 是同方向的补充证据，但带有可靠性限制。

目前还没有 bisect 到具体 culprit commit。当前 release narrowing 指向 `v6.12..v6.18` 这个范围。
