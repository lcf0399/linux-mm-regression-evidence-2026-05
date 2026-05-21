# mprotect shared-dirty state audit - lab

这轮实验用于排除类似 `madvise` 那样的 state-shape caveat：确认
`v6.12.77`、`v6.19.9` 和 `mm-unstable` 在同一个
`shared_dirty_full_toggle_64m` workload 下是否操作了可比的用户态 mapping 状态。

这不是性能结论，不用于 old-faster claim。它只看：

- syscall 语义是否成功：`expected_match_ratio`、`unexpected_results`
- VMA shape：`split_vmas_avg`、`final_vmas_avg`
- smaps：AnonHugePages、KernelPageSize、MMUPageSize、THPeligible
- pagemap：present pages、soft-dirty pages

计划矩阵：

```text
1 CPU / 14336 MiB
2 CPU / 14336 MiB
4 CPU / 14336 MiB
8 CPU / 16384 MiB
16 CPU / 32768 MiB
```

版本矩阵：

```text
v6.12.77
v6.19.9
akpm/mm mm-unstable 444fc9435e57
```

## 结果摘要

2026-05-20 lab run 的结果摘要在：

```text
summary-20260520.md
summary-20260520.zh-CN.md
```

简短结论：`v6.12.77`、`v6.19.9` 和 `mm-unstable` 的成功 run 都显示
`expected_match_ratio=100`、`unexpected_results=0`、最终 1 个 VMA、
protect 前后 16384 个 present pages、`AnonHugePages=0`、kernel/MMU page
size 都是 4 KiB、`THPeligible=0`。

这支持把 mprotect 对比解释为 same-state shared-dirty 4 KiB PTE workload
comparison。与已修正口径后的 `madvise` 情况不同，当前 state audit 没有显示
“不同内核实际页状态不同”的 caveat。

完整 raw runner 目录和 launch logs 通过 `.gitignore` 排除在精简公开证据包之外；
如果 follow-up debugging 需要，可以再单独提供。
