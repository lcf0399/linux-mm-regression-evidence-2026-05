# mincore present-PTE scan 候选

这个目录是一份给维护者看的精简证据草稿，针对 `mm/mincore.c` 里 resident anonymous
base-page PTE 路径的 `mincore()` source-calibrated workload。

当前口径：

- workload 类型：由源码路径校准的 synthetic userspace micro-workload。
- 热路径：`mincore()` -> `walk_page_range()` -> `mincore_pte_range()`。
- 主场景：`MADV_NOHUGEPAGE` anonymous 64 MiB resident mapping，
  即 `no_thp_pte_scan_64m`。
- 主指标：`mincore_ns_per_1k_pages`，越低越好。

这不是生产应用回归，也不是泛化的 `mincore()` regression 报告。补做 `codegen/`
里的 compiler cross-check 后，当前口径更窄：原始 timing signal 是在 GCC 13.3
+ QEMU lab 环境中观察到的，而 Clang 18.1.3 会把被检查的 x86 代码形状优化成逐字节
等价的 `mincore_pte_range()` 输出。

## 当前发现

release-level narrowing 和 A/B testing 已经把主台阶缩到 v6.15 -> v6.16 区间。
最强 suspect 是：

```text
4df65651f7075 mm: mincore: use pte_batch_hint() to batch process large folios
```

这条变化改善了 large-folio/mTHP mincore batching。在 GCC 13.3 QEMU lab runs 中，
base-page resident-PTE scan case 显示出明显成本。codegen 检查现在说明这个源码形状
对 compiler 敏感：GCC 13.3 不会把 v6.16 original 生成成同一份
`mincore_pte_range()` 形状，而 Clang 18.1.3 会。当前 present-first 候选保留
`batch > 1` 时的 `pte_batch_hint()` batching，但把 `pte_present()` 判断提前到
none/marker 路径之前；它只能作为讨论用 candidate shape，不是 upstream-ready fix。

截至 2026-05-29 UTC 复查，torvalds/master 的 `mincore_pte_range()` 仍然先检查
`pte_none(pte) || pte_is_marker(pte)`，再检查 `pte_present(pte)`。本轮 test patch
对齐的本地 `linux-mm-unstable` checkout 是 `444fc9435e571`。

## 内容

- `reproducer/`：从 workload source 抽出的 standalone C reproducer。
- `experiments/`：本地实验框架使用的 workload target/profile。standalone C source
  使用同一套 workload 逻辑，方便维护者不依赖完整框架直接构建。
- `patches/mincore-present-first-fastpath-rfc.patch`：本地 test patch 形状，不可直接发送上游。
- `codegen/`：GCC 13.3 和 Clang 18.1.3 下，v6.15/v6.16 original 与本地 variants 的
  `mincore_pte_range()` nm/objdump 对比。
- `lab-validation/`：紧凑 CSV summary、primary 1/2/4 CPU 验证说明、matched-PREEMPT
  release bridge rows、high-CPU v6.16 introduction-window A/B rows，以及 high-CPU
  present-first A/B rows。

## 限制

- timing signal 目前只在 x86/QEMU lab 环境的 GCC-built kernels 上观察到。应把它描述为
  compiler/codegen-sensitive，而不是 generic x86 mincore regression。
- present-first patch 目前只在 x86/QEMU lab 环境验证过，已覆盖到 16CPU/32GiB
  no-THP present-PTE scan follow-up。
- 在真正作为 upstream fix 发送前，还需要 arm64 或 mTHP/large-folio 保真验证。
- 这里的 patch 文件故意标成 “not for submission”；正式发送前需要真实 commit message、
  Signed-off-by 和架构侧 review。
