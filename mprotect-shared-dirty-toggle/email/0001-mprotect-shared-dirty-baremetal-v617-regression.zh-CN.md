# mprotect shared-dirty bare-metal 回归报告草稿中文版

> 说明：这是英文发送稿
> `0001-mprotect-shared-dirty-baremetal-v617-regression.md` 的逐段中文对照版，
> 方便审阅；不是直接发送给上游的正文。

```text
Subject: [REGRESSION] mm/mprotect: shared-dirty base-page toggle slower since v6.17

To: Andrew Morton <akpm@linux-foundation.org>, linux-mm@kvack.org
Cc: "Liam R. Howlett" <liam@infradead.org>,
    Lorenzo Stoakes <ljs@kernel.org>,
    Vlastimil Babka <vbabka@kernel.org>,
    Jann Horn <jannh@google.com>,
    Pedro Falcato <pfalcato@suse.de>,
    linux-kernel@vger.kernel.org,
    regressions@lists.linux.dev
```

你好，

我有一组更新后的 bare-metal 结果，针对的是我之前从 QEMU/lab 测试中报告过的
shared-dirty `mprotect()` slowdown。

这个 reproducer 是有意限定得很窄的：

```text
- MAP_SHARED | MAP_ANONYMOUS mapping
- 64 MiB range, write-prefaulted before timing
- state check: 4 KiB base pages, no THP backing
- repeated full-range mprotect(PROT_READ)
- restore with mprotect(PROT_READ | PROT_WRITE)
- write-touch after each protect/restore cycle
```

所以这不是一个 generic `mprotect()` regression claim。它的范围是
shared-dirty base-page PTE permission-change path。

bare-metal 机器是 Intel Core i7-14700 系统。这个 workload 是单线程的，并且用
`taskset -c 2` 固定在一个 logical CPU 上。主指标是
`iteration_ns_per_page`，越小越好。每个 benchmark step 使用 9 个 external rounds、
1000 iterations 和 10 warmup iterations。

首先，`v6.12 -> v6.19` 的结果在 bare metal 上仍然能复现：

```text
kernel                         iteration_ns_per_page
v6.12.77                       27
v6.19.9                        37
```

然后我用每个 kernel 3 次 interleaved boot/run step 缩小 release window：

```text
kernel                         values          mean
v6.16                          25 25 25        25.000
v6.17                          37 37 37        37.000
v6.18                          38 38 38        38.000
v6.18.19                       38 38 38        38.000
v6.19.9                        37 36 37        36.667
```

所有这些 run 都报告：

```text
expected_match_ratio=100
unexpected_results=0
```

standalone 输出里的 state check 也保持同一种形状：4 KiB pages，没有 THP。

这把 slowdown 定位到 `v6.16 -> v6.17` release window。

作为 attribution check，我还构建了一个 `v6.17` probe kernel。这个 probe 只针对
当前 workload，把 `mm/mprotect.c::change_pte_range()` 里的 present-PTE path 改回
single-PTE start/commit/flush 形状。它不是 upstream patch，也不是 clean release-kernel
comparison；它只是一个 hot-path probe。

结果是：

```text
kernel                         values          mean
v6.16                          25 25 25        25.000
v6.17                          37 37 37        37.000
v6.17 single-PTE probe         25 25 25        25.000
```

所以这个 targeted probe 把 `v6.17` 在该 workload 上拉回了 `v6.16` 的区间。这指向
`change_pte_range()` 中 `v6.17` 的 PTE-batching shape 是这个 shared-dirty
4 KiB base-page case 的主要成本。

我不想过度表述这个 attribution。我尝试把官方
`cac1db8c3aad ("mm: optimize mprotect() by PTE batching")` patch 反打到我的
`linux-6.17` tree 上，但它没有 clean apply，所以这不是 exact revert result。
当前证据是 release-window narrowing 加 targeted source probe。

证据包：

```text
https://github.com/lcf0399/linux-mm-regression-evidence-2026-05/tree/8d84f8d620739c04ae0a68bd56cd4c3f78ced82c/mprotect-shared-dirty-toggle
```

evidence tree 里的本地路径：

```text
mprotect-shared-dirty-toggle/reproducer/
mprotect-shared-dirty-toggle/bare-metal/README.zh-CN.md
mprotect-shared-dirty-toggle/bare-metal/20260622-20260623-ab/
mprotect-shared-dirty-toggle/bare-metal/20260623-narrow-6.16-6.19-3rounds/
mprotect-shared-dirty-toggle/bare-metal/20260624-6.17-singlepte-probe/
```

```text
#regzbot introduced: v6.16..v6.17
```

这个范围看起来值得继续调查吗？如果值得，我可以尝试做更精确的 commit-level check，
或者测试你认为方向正确的 patch。

谢谢，
Chengfeng
