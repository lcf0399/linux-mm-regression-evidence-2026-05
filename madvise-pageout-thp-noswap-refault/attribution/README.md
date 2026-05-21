# MADV_PAGEOUT Attribution

This directory contains ftrace/smaps attribution material for the narrow
`MADV_PAGEOUT anon/THP no-swap reclaim-failure path`.

These runs use tracing/debugfs/function profiling, so they are mechanism
evidence only. Clean timing evidence remains in `../formal-lab/`.

Start from `summaries/lab-multicpu-followup-20260520.zh-CN.md` for the current
attribution result. The short version is also summarized in the parent
workload README.

## Layout

- `summaries/`: compact attribution summaries.
- `scripts/`: ftrace/smaps launch scripts.
- `MOVED_PATHS.zh-CN.md`: path cleanup record.

Bulky raw trace/log directories are excluded from the compact public bundle
unless needed for follow-up debugging.
