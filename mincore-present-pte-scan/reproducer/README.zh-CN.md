# standalone mincore present-PTE reproducer

这个目录包含 `mm/mincore.c` 中 `no_thp_pte_scan_64m` 路径的 standalone userspace
复现程序。

它创建 private anonymous mapping，设置 `MADV_NOHUGEPAGE`，预先 fault-in 所有页，
然后反复对 resident base-page range 调用 `mincore()`。目标是压 `mincore_pte_range()`
中的 present-PTE 路径，避免 THP PMD shortcut 混入。

## 编译和运行

```sh
gcc -O2 -Wall -Wextra -o mincore_present_pte_scan \
  mincore_present_pte_scan.c

./mincore_present_pte_scan no_thp_pte_scan_64m 1
```

源码里也保留了 THP 对照场景：

```sh
./mincore_present_pte_scan thp_pmd_shortcut_64m 1
```

THP 场景依赖内核 THP 支持，如果 THP collapse 不可用可能会 skip。当前候选信号以 no-THP
场景为主。

## 输出

主要 timing 字段是：

- `mincore_ns_avg`：每次 `mincore()` 调用的平均 wall-clock ns。
- `mincore_ns_per_page`：每个 scanned base page 的 wall-clock ns。
- `mincore_ns_per_1k_pages`：每 1000 个 scanned pages 的 wall-clock ns。

重要 semantic/state-shape 字段：

- `expected_match_ratio` 应为 100。
- `unexpected_results` 应为 0。
- `resident_ratio_pct` 应为 100。
- `thp_ratio_pct` 在 `no_thp_pte_scan_64m` 中应为 0。

正式 lab 证据仍来自受控 QEMU/lab runs。这里提供 standalone source，是为了让维护者不用
完整 experiment framework 也能审阅和运行 workload。

2026-05-29 已做本地主机 smoke：

```text
gcc -O2 -Wall -Wextra -o /tmp/mincore_present_pte_scan mincore_present_pte_scan.c
/tmp/mincore_present_pte_scan no_thp_pte_scan_4m 1
```

smoke run 完成，`unexpected_results=0`，`expected_match_ratio=100`，
`resident_ratio_pct=100`，`thp_ratio_pct=0`。
