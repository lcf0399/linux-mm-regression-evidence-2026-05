# MADV_PAGEOUT workload source

这个目录包含 `madvise_pageout_formal_refresh` profile 使用的 standalone 用户态
workload 源码和辅助脚本，对应
`MADV_PAGEOUT + anonymous THP + no-swap + write-touch` workflow。

它用于上游报告、bisection 和边界检查。它不能替代完整的 `mm_regression_gen` 证据集。

## 文件

- `madvise_pageout_refault_reproducer.c`
- `run_madvise_pageout_reproducer.sh`

## 默认值

辅助脚本默认值对应可报告实验：

- mapping size：`16 MiB`
- THP mode：默认策略，不显式使用 `MADV_NOHUGEPAGE`
- advice：`MADV_PAGEOUT`
- refault：每页一次 write-touch
- external rounds：`5`
- internal rounds：至少 `4`，最多 `256`
- 每个 external round 的最小测量时间：`1200 ms`
- 每个 external round 前的 warmup cycles：`2`
- wrapper repetitions：`9`

## Smoke test

这只检查程序能否 build 和运行。它不是性能证据。

```sh
cd /path/to/linux-mm-regression-evidence-2026-05/madvise-pageout-thp-noswap-refault/workload
REPETITIONS=1 \
  ./run_madvise_pageout_reproducer.sh \
  --mapping-kb 4096 --external-rounds 1 --rounds 1 --max-rounds 1 --min-ms 0 --warmup-rounds 0
```

如需在目标 kernel guest 内做 report-style standalone run：

```sh
cd /path/to/linux-mm-regression-evidence-2026-05/madvise-pageout-thp-noswap-refault/workload
OUT_DIR=/tmp/madvise-pageout-$(uname -r) \
REPETITIONS=9 \
  ./run_madvise_pageout_reproducer.sh
```

重要字段：

- `advise_ns_per_page`
- `cycle_ns_per_page`
- `advise_ns_avg`
- `cycle_ns_avg`
- `expected_match_ratio`
- `unexpected_results`
- `errno_enomem`
- `errno_einval`

`cycle_ns_per_page` 是 `(advise_ns + post_touch_ns) / post_touch_pages`。

## THP 控制

使用 `--thp default`、`--thp nohugepage` 或 `--thp hugepage` 控制 mapping 请求。
`--dump-smaps` 会在 prefault 后打印选定的 `/proc/self/smaps` 字段。

smaps 输出只用于路径/状态检查，不用于 clean timing evidence。
