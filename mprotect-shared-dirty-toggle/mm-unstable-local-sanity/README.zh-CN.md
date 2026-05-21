# mprotect mm-unstable local sanity

这个目录记录针对 `akpm/mm mm-unstable` commit
`444fc9435e57157fcf30fc99aee44997f3458641` 的本地 sanity check。

它只作为 follow-up analysis material。本地 `mprotect` timing 噪声更大；用于上游讨论时，
`../mm-unstable-lab-sanity/` 中的 lab 结果是更好的引用。

## 目录结构

- `experiments/`：本地 profile 和 kernel-version mapping。
- `local_{1,2,4}cpu/`：本地 clean timing 结果。
