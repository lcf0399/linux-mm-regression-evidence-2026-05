# MADV_PAGEOUT refault reproducer

这个目录包含一个 standalone 用户态 workload，用于
`MADV_PAGEOUT + anonymous THP + no-swap + refault/write-touch` workflow；
这个 workflow 由 `mm_regression_gen` 的 `madvise_pageout_formal_refresh`
profile 使用。

它用于上游报告、bisection 和边界检查。它不能替代完整的 `mm_regression_gen` 证据集。

## 文件

- `madvise_pageout_refault_reproducer.c`
- `run_madvise_pageout_reproducer.sh`

## 默认 workload

默认值对应当前可报告实验：

- mapping size：`16 MiB`
- THP mode：默认策略，不显式使用 `MADV_NOHUGEPAGE`
- advice：`MADV_PAGEOUT`
- refault：每页一次 write-touch
- external rounds：`5`
- internal rounds：至少 `4`，最多 `256`
- 每个 external round 的最小测量时间：`1200 ms`
- 每个 external round 前的 warmup cycles：`2`
- wrapper repetitions：`9`

## 快速 smoke test

这只检查程序能否 build 和运行。它不是性能证据。

```sh
cd /path/to/linux-mm-regression-evidence-2026-05/madvise-pageout-thp-noswap-refault/workload
REPETITIONS=1 \
  ./run_madvise_pageout_reproducer.sh \
  --mapping-kb 4096 --external-rounds 1 --rounds 1 --max-rounds 1 --min-ms 0 --warmup-rounds 0
```

## 报告风格运行

在目标 kernel guest 内运行：

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

## THP 边界检查

默认 THP 策略：

```sh
./madvise_pageout_refault_reproducer
```

对该 mapping 禁用 THP：

```sh
./madvise_pageout_refault_reproducer --thp nohugepage
```

对该 mapping 请求 THP：

```sh
./madvise_pageout_refault_reproducer --thp hugepage
```

`--dump-smaps` 会在 prefault 后打印选定的 `/proc/self/smaps` 字段。它只用于路径检查，
不要用于 timing evidence。

## 当前回归上下文

当前 lab evidence 使用相似 kernel configuration 对比 `v6.12.77` 和 `v6.19.9`。
在 lab QEMU guest 上，`v6.12.77` 在 `1/2/4` vCPU 的 `cycle_ns_per_page`
上比 `v6.19.9` 快约 `39%` 到 `42%`。

这应报告为 `THP default + MADV_PAGEOUT + no-swap/refault` workflow 中的
path-specific regression，而不是泛化的 `madvise(2)` slowdown。
