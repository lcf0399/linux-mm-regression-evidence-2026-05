# MADV_PAGEOUT no-swap attribution 索引

这个目录保存 `MADV_PAGEOUT anon/THP no-swap reclaim-failure path` 的归因材料。
这里的内核会额外启用 tracing/debugfs/function profiler，所以结果只用于机制解释，
不能替代 `formal-lab/` 下关闭 coverage/probe 的 clean timing。

当前结论很窄：

```text
原始 old-faster 信号主要来自实际页状态差异。v6.19.9 在 default/hugepage 请求下
实际 THP-backed，并在 no-swap MADV_PAGEOUT reclaim 中进入 THP split/failure path；
v6.12.77 在同一请求下没有实际 THP backing。nohugepage 同状态对照没有稳定
old-faster。
```

因此，上游 follow-up 应修正原 regression framing，而不是继续声称已经证明
same-state THP 性能回归。

## 目录结构

```text
summaries/
  local-ftrace-20260519.zh-CN.md
  lab-1cpu-20260520.zh-CN.md
  lab-1cpu-20260520.md
  lab-multicpu-followup-20260520.zh-CN.md
scripts/
  run_madvise_ftrace_*.sh
MOVED_PATHS.zh-CN.md
```

## 推荐阅读顺序

1. `summaries/lab-1cpu-20260520.zh-CN.md`
   - 最先确认 lab 正式环境下的页状态差异。
2. `summaries/lab-multicpu-followup-20260520.zh-CN.md`
   - 说明 `2/4/8/16 CPU` 下同一结构仍成立。
3. `summaries/local-ftrace-20260519.zh-CN.md`
   - 记录 local ftrace/smaps attribution 如何定位到 smaps/THP backing caveat。
4. `MOVED_PATHS.zh-CN.md`
   - 记录历史 attribution 路径和当前整理后路径之间的对应关系。

## 证据边界

- `runs/local/`：local attribution 和 sanity check，只作为分析材料。
- `runs/lab/1cpu-20260520/`：lab 1CPU ftrace/smaps attribution。
- `runs/lab/multicpu-20260520/`：lab 多 CPU ftrace/smaps attribution。
- `scripts/`：归因 run 使用的启动脚本；用于复现或扩展 attribution run。

Raw attribution run 目录不属于精简公开证据包的核心材料；需要 follow-up debug
时再按需提供具体 raw 目录。

这批 attribution run 的时间数字来自 tracing kernel 短跑，只能用来解释路径。
`cycle_ns_per_page` 是 workload iteration wall-clock ns/page，不是 CPU cycles。
