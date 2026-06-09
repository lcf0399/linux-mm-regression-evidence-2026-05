# GCC PTE-loop side-by-side note

This note summarizes the visible `mincore_pte_range()` generated-code
difference that David asked about. It is static codegen evidence only; QEMU can
affect runtime timing, but it cannot change the `mm/mincore.o` objdump shown
here.

## Inputs

The compared files are:

```text
gcc13_v6.16_original.mincore_pte_range.objdump.txt
gcc13_v6.16_fastpath.mincore_pte_range.objdump.txt
gcc13_v6.16_nobatch.mincore_pte_range.objdump.txt

gcc14_v6.16_original.mincore_pte_range.objdump.txt
gcc14_v6.16_fastpath.mincore_pte_range.objdump.txt
gcc14_v6.16_nobatch.mincore_pte_range.objdump.txt

gcc15_v6.16_original.mincore_pte_range.objdump.txt
gcc15_v6.16_fastpath.mincore_pte_range.objdump.txt
gcc15_v6.16_nobatch.mincore_pte_range.objdump.txt
```

For each GCC version, the `fastpath` and `nobatch` objdump files are
byte-identical.

## Inlining check

The difference does not look like an obvious extra inlining decision. For
GCC 13.3, GCC 14.2, and GCC 15.2, the original, fastpath, and nobatch builds
all have the same external relocation/call target set:

```text
__pte_offset_map_lock
__pmd_trans_huge_lock
memset
__cond_resched
filemap_get_incore_folio
__folio_put
swapper_spaces
```

So the local evidence points more toward loop/block layout than toward an
additional helper being inlined into one variant.

## Hot PTE-loop layout

The relevant resident-PTE hot path is the case that stores `1` into the
userspace result vector and advances to the next PTE. The exact registers differ
slightly across compiler versions, but the block shape is consistent.

| compiler | original v6.16 shape | fastpath/nobatch shape |
| --- | --- | --- |
| GCC 13.3 | present store is in a later block at `0x2ed`, then jumps back to the common advance block at `0x2d6` | store at `0x1b4` is adjacent to vector/PTE advance at `0x1b8`/`0x1bc` |
| GCC 14.2 | present store is in a later block at `0x2fb`, then jumps back to the common advance block at `0x2e1` | store at `0x1b4` is adjacent to vector/PTE advance at `0x1b8`/`0x1bc` |
| GCC 15.2 | present store is in a later block at `0x311`, then jumps back to the common advance block at `0x2f7` | store at `0x1d3` is adjacent to vector/PTE advance at `0x1d7`/`0x1db` |

The GCC 15.2 objdump shows the shape most clearly:

```text
GCC15 v6.16 original:

  1b1:  test   $0x101,%eax
  1b6:  jne    311 <mincore_pte_range+0x1f1>
  ...
  2f7:  add    $0x1,%r15       ; common vec advance
  2fb:  add    $0x8,%rbp       ; common ptep advance
  2ff:  cmp    %rbx,%r12
  302:  jne    197 <mincore_pte_range+0x77>
  ...
  311:  movb   $0x1,(%r15)     ; present-PTE store
  315:  jmp    2f7 <mincore_pte_range+0x1d7>
```

```text
GCC15 v6.16 nobatch / batch<=1 fastpath:

  192:  test   $0x101,%eax
  197:  jne    1d3 <mincore_pte_range+0xb3>
  ...
  1d3:  movb   $0x1,(%r14)     ; present-PTE store
  1d7:  add    $0x1,%r14       ; vec advance
  1db:  add    $0x8,%r15       ; ptep advance
  1df:  cmp    %rbx,%r12
  1e2:  je     282 <mincore_pte_range+0x162>
  ...
```

This is the practical difference behind the shorter summary in the mail draft:
the v6.16 original GCC output keeps a common advance block after the batching
control flow, while the local `batch <= 1` fastpath collapses back to the
nobatch shape on x86.

## GCC dump note

The focused GCC 15.2 dumps show that the difference is present before final
assembly:

| variant | optimized dump lines | RTL expand lines | objdump lines | objdump jumps |
| --- | ---: | ---: | ---: | ---: |
| `gcc15_v6.16_original` | 321 | 993 | 146 | 25 |
| `gcc15_v6.16_fastpath` | 299 | 921 | 136 | 23 |
| `gcc15_v6.16_nobatch` | 299 | 921 | 136 | 23 |

The fastpath and nobatch optimized dumps differ only in mechanical
SSA/source-line naming, and their final objdump files are byte-identical.

## Interpretation

This is not proof of a compiler bug. It is a generated-code layout observation:
GCC 13.3, GCC 14.2, and GCC 15.2 preserve a larger/different v6.16 original
shape for this function, while the local fastpath and nobatch variants collapse
to the same generated code. Clang 18.1.3 collapses all checked variants to
byte-identical output.
