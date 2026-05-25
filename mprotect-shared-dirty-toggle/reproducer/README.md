# Standalone mprotect Shared-dirty Reproducer

This directory contains a small standalone reproducer for the shared-dirty
`mprotect()` toggle workload.

It is intentionally narrower than the generated workload used by the formal
experiment framework:

- `MAP_SHARED | MAP_ANONYMOUS` mapping
- write-prefault the whole range
- repeatedly toggle the full range with `mprotect(PROT_READ)`
- restore with `mprotect(PROT_READ | PROT_WRITE)`
- write-touch the range after each protection cycle

## Build And Run

```sh
gcc -O2 -Wall -Wextra -o mprotect_shared_dirty_reproducer \
  mprotect_shared_dirty_reproducer.c

./mprotect_shared_dirty_reproducer \
  shared_dirty_full_toggle_64m 1 \
  --mapping-mb 64 \
  --iterations 200 \
  --warmup 5
```

Or use the helper:

```sh
MAPPING_MB=64 ITERATIONS=200 WARMUP=5 EXTERNAL_ROUNDS=1 \
  ./run_mprotect_shared_dirty_reproducer.sh
```

## Output

The main timing fields are:

- `protect_ns_per_page`: wall-clock ns per base page for `mprotect(PROT_READ)`
- `restore_ns_per_page`: wall-clock ns per base page for restoring write
  permission
- `post_touch_ns_per_page`: wall-clock ns per base page for the post-cycle
  write touch
- `iteration_ns_per_page`: `(protect + restore + post_touch)` wall-clock
  ns per base page

The `smaps_*` fields are a state-shape sanity check. For this workload, the
expected shape is a base-page shared mapping, not an anonymous THP path:

- `KernelPageSize = 4 kB`
- `MMUPageSize = 4 kB`
- `AnonHugePages = 0 kB`

This reproducer does not require the experiment framework. The formal evidence
in the parent directory still comes from the controlled QEMU/lab runs.
