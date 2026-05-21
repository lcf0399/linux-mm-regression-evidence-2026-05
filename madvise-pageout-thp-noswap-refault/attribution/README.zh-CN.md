# MADV_PAGEOUT attribution

这个目录保存 `MADV_PAGEOUT anon/THP no-swap reclaim-failure path` 的
ftrace/smaps attribution 材料。

这些 run 会启用 tracing/debugfs/function profiling，所以只作为机制证据。
clean timing 证据仍以 `../formal-lab/` 为准。

当前归因结果从 `summaries/lab-multicpu-followup-20260520.zh-CN.md` 开始看。
父级 workload README 也保留了更短的摘要。

## 目录结构

- `summaries/`：精简归因摘要。
- `scripts/`：ftrace/smaps 启动脚本。
- `MOVED_PATHS.zh-CN.md`：路径整理记录。

体积较大的 raw trace/log 目录不进入精简公开证据包；后续 debug 需要时再单独提供。
