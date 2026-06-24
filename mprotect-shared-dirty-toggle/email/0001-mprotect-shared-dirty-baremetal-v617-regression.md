Subject: [REGRESSION] mm/mprotect: shared-dirty base-page toggle slower since v6.17

To: Andrew Morton <akpm@linux-foundation.org>, linux-mm@kvack.org
Cc: "Liam R. Howlett" <liam@infradead.org>,
    Lorenzo Stoakes <ljs@kernel.org>,
    Vlastimil Babka <vbabka@kernel.org>,
    Jann Horn <jannh@google.com>,
    Pedro Falcato <pfalcato@suse.de>,
    linux-kernel@vger.kernel.org,
    regressions@lists.linux.dev

Hi,

I have a refreshed bare-metal result for the shared-dirty mprotect()
slowdown I reported earlier from QEMU/lab testing.

The reproducer is intentionally narrow:

  - MAP_SHARED | MAP_ANONYMOUS mapping
  - 64 MiB range, write-prefaulted before timing
  - state check: 4 KiB base pages, no THP backing
  - repeated full-range mprotect(PROT_READ)
  - restore with mprotect(PROT_READ | PROT_WRITE)
  - write-touch after each protect/restore cycle

So this is not a generic mprotect() regression claim.  The scope is the
shared-dirty base-page PTE permission-change path.

The bare-metal machine is an Intel Core i7-14700 system.  The workload is
single-threaded and pinned to one logical CPU with `taskset -c 2`.  The primary
metric is `iteration_ns_per_page`, lower is better.  Each benchmark step used
9 external rounds, 1000 iterations, and 10 warmup iterations.

First, the v6.12 -> v6.19 result still reproduces on bare metal:

  kernel                         iteration_ns_per_page
  v6.12.77                       27
  v6.19.9                        37

I then narrowed the release window with 3 interleaved boot/run steps per
kernel:

  kernel                         values          mean
  v6.16                          25 25 25        25.000
  v6.17                          37 37 37        37.000
  v6.18                          38 38 38        38.000
  v6.18.19                       38 38 38        38.000
  v6.19.9                        37 36 37        36.667

All of these runs reported `expected_match_ratio=100` and
`unexpected_results=0`.  The state check in the standalone output stays in the
same shape: 4 KiB pages, no THP.

This puts the slowdown in the v6.16 -> v6.17 release window.

As an attribution check, I also built a v6.17 probe kernel that only changes
the present-PTE path in `mm/mprotect.c::change_pte_range()` for this workload
back to a single-PTE start/commit/flush shape.  That is not an upstream patch
and not a clean release-kernel comparison; it is only a hot-path probe.

The result was:

  kernel                         values          mean
  v6.16                          25 25 25        25.000
  v6.17                          37 37 37        37.000
  v6.17 single-PTE probe         25 25 25        25.000

So the targeted probe brings v6.17 back to the v6.16 range for this workload.
That points at the v6.17 PTE-batching shape in `change_pte_range()` as the
main cost for this shared-dirty 4 KiB base-page case.

I do not want to overstate the attribution.  I tried reversing the official
`cac1db8c3aad ("mm: optimize mprotect() by PTE batching")` patch onto my
linux-6.17 tree, but it did not apply cleanly, so this is not an exact revert
result.  The current evidence is release-window narrowing plus a targeted
source probe.

Evidence bundle:

  https://github.com/lcf0399/linux-mm-regression-evidence-2026-05/tree/8d84f8d620739c04ae0a68bd56cd4c3f78ced82c/mprotect-shared-dirty-toggle

Local paths in the evidence tree:

  mprotect-shared-dirty-toggle/reproducer/
  mprotect-shared-dirty-toggle/bare-metal/README.zh-CN.md
  mprotect-shared-dirty-toggle/bare-metal/20260622-20260623-ab/
  mprotect-shared-dirty-toggle/bare-metal/20260623-narrow-6.16-6.19-3rounds/
  mprotect-shared-dirty-toggle/bare-metal/20260624-6.17-singlepte-probe/

#regzbot introduced: v6.16..v6.17

Does this scope look useful to investigate further?  If yes, I can try a more
exact commit-level check or test a patch you think is the right direction.

Thanks,
Chengfeng
