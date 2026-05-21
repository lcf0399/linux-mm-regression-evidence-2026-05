# mprotect mm-unstable Local Sanity

This directory records a local sanity check against `akpm/mm mm-unstable`
commit `444fc9435e57157fcf30fc99aee44997f3458641`.

It is follow-up analysis material only. Local `mprotect` timing is noisier than
the lab matrix, and the lab result in `../mm-unstable-lab-sanity/` is the better
reference for upstream discussion.

## Layout

- `experiments/`: local profile and kernel-version mapping.
- `local_{1,2,4}cpu/`: local clean timing results.
