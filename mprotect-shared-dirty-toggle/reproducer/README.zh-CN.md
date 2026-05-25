# standalone mprotect shared-dirty reproducer

这个目录包含 shared-dirty `mprotect()` toggle workload 的小型 standalone
复现程序。

它比 formal experiment framework 使用的 generated workload 更窄：

- `MAP_SHARED | MAP_ANONYMOUS` mapping
- 先 write-prefault 整个 range
- 反复对完整 range 执行 `mprotect(PROT_READ)`
- 再恢复成 `mprotect(PROT_READ | PROT_WRITE)`
- 每轮 protection cycle 后 write-touch 整个 range

## 编译和运行

```sh
gcc -O2 -Wall -Wextra -o mprotect_shared_dirty_reproducer \
  mprotect_shared_dirty_reproducer.c

./mprotect_shared_dirty_reproducer \
  shared_dirty_full_toggle_64m 1 \
  --mapping-mb 64 \
  --iterations 200 \
  --warmup 5
```

也可以使用辅助脚本：

```sh
MAPPING_MB=64 ITERATIONS=200 WARMUP=5 EXTERNAL_ROUNDS=1 \
  ./run_mprotect_shared_dirty_reproducer.sh
```

## 输出

主要 timing 字段是：

- `protect_ns_per_page`：`mprotect(PROT_READ)` 的每 base page wall-clock ns
- `restore_ns_per_page`：恢复写权限的每 base page wall-clock ns
- `post_touch_ns_per_page`：每轮后续 write-touch 的每 base page wall-clock ns
- `iteration_ns_per_page`：`protect + restore + post_touch` 的每 base page
  wall-clock ns

`smaps_*` 字段用于 state-shape sanity check。这个 workload 的预期状态是 base-page
shared mapping，而不是 anonymous THP 路径：

- `KernelPageSize = 4 kB`
- `MMUPageSize = 4 kB`
- `AnonHugePages = 0 kB`

这个 reproducer 不依赖 experiment framework。父目录中的 formal evidence 仍然来自
受控 QEMU/lab runs。
