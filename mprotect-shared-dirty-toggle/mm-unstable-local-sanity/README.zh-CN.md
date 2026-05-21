# mprotect mm-unstable 本地 sanity check

这个目录记录针对 akpm/mm `mm-unstable` commit
`444fc9435e57157fcf30fc99aee44997f3458641` 的本地 sanity check。

它的背景是上游反馈指出 Pedro Falcato 最近的 small-folio `mprotect()`
optimization series。这个 patchset 和先前对 shared-dirty PTE toggle
workload 的 `nr_ptes == 1` 机制猜测直接相关。

这不是 formal lab 证据。local `mprotect` timing 本来就比 lab matrix 更 noisy，
因此这些文件应当只作为 follow-up analysis material 使用。

## 目录结构

- `experiments/`：这轮本地 sanity check 使用的 profile 和 kernel-version mapping。
- `local_1cpu/`：本地 `QEMU_SMP=1` 的 clean timing 结果。
- `local_2cpu/`：本地 `QEMU_SMP=2` 的 clean timing 结果。
- `local_4cpu/`：本地 `QEMU_SMP=4` 的 clean timing 结果。

## 当前解读

本地 sanity check 显示：`mm-unstable` 里的相关优化相对 `v6.19.9` 减轻了
synthetic shared-dirty signal，但没有回到 `v6.12.77` 水平。

因为这轮是 noisy local run，合适的公开解读是：

```text
Pedro's small-folio mprotect optimization is directly relevant to the
nr_ptes == 1 hypothesis. In this local sanity check it reduces the synthetic
shared-dirty signal, but does not remove it. The local run is noisy, so it is
follow-up analysis material rather than formal lab evidence.
```
