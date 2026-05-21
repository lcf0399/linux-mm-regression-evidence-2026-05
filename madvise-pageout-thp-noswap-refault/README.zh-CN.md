# MADV_PAGEOUT THP/no-swap reclaim-failure 证据

这个目录对应原始报告：

```text
[REGRESSION] mm: MADV_PAGEOUT on THP/no-swap refault workflow is ~40% slower on v6.19 than v6.12
```

目录名和原始报告标题保留了最初的 `refault` 表述。经过上游 review 后，当前更准确的范围是
`MADV_PAGEOUT` anon/THP no-swap reclaim-failure path；后续 write-touch 是
workload iteration 的一部分，不应被当成真实 pageout/refault 的证明。

## 结论范围

这是一个用户态可见的 `madvise(MADV_PAGEOUT)` workload：

- anonymous 16 MiB mapping
- 默认 THP 路径
- guest 无 swap
- `MADV_PAGEOUT`，然后在 iteration 后半段 write-touch

它不声称所有 `MADV_PAGEOUT` workload 都发生回归。

## 关键结果

Formal lab timing 显示，在 `1/2/4` vCPU 上，`v6.19.9` 都慢于 `v6.12.77`。
经过上游 review 后，重要 caveat 是这些 formal timing run 没有记录 smaps/page-state。
后续 local 和 lab attribution run 发现，在这种 workload 形态下，两个内核的实际 THP backing
可能不同。因此，下表仍是原始 formal timing 结果，但不应被描述成 same-state THP regression。

`cycle_ns_per_page`：

| CPU | v6.12.77 | v6.19.9 | delta |
| --- | ---: | ---: | ---: |
| 1 | 1900.3 | 3304.7 | -42.5% |
| 2 | 2107.7 | 3583.2 | -41.2% |
| 4 | 2154.2 | 3690.9 | -41.6% |

`cycle_ns_per_page` 表示一次完整 workload iteration 中每页的 wall-clock 纳秒数。
它不是 CPU-cycle 计数器。

`advise_ns_per_page`：

| CPU | v6.12.77 | v6.19.9 | delta |
| --- | ---: | ---: | ---: |
| 1 | 1713.2 | 2922.7 | -41.4% |
| 2 | 1924.7 | 3162.9 | -39.1% |
| 4 | 1953.1 | 3284.2 | -40.5% |

单独做过的 release-level sanity check 显示 `v6.18.19` 已经进入慢区间，但这些 raw run
没有放入当前精简公开证据包。

## 目录

- `workload/`：standalone workload 源码和辅助脚本。
- `experiments/`：formal experiment profile。
- `formal-lab/perf_{1,2,4}cpu/`：coverage disabled 的 clean performance run。
- `formal-lab/coverage_1cpu/`：与 clean timing 分开收集的 direct-hit coverage 证据。
- `attribution/`：针对上游 path breakdown 请求补做的 local 和 lab ftrace/smaps follow-up。
  当前 local 结果指向 `reclaim_pages()` / `shrink_folio_list()`，并且在 `v6.19.9`
  上反复命中 `split_folio_to_list()`；但它也显示一个关键 caveat：local `v6.12.77`
  经常是 `AnonHugePages=0 kB`，而 `v6.19.9` 是 THP-backed。这只是 attribution
  evidence，不是 clean timing evidence。
