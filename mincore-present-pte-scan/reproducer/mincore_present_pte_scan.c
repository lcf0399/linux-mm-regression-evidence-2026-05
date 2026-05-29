// SPDX-License-Identifier: GPL-2.0-only
/*
 * DeepSeek-assisted mincore THP PMD shortcut probe.
 *
 * This is a focused userspace workload for linux/mm/mincore.c:
 * mincore() -> do_mincore() -> walk_page_range() -> mincore_pte_range(),
 * with state-shape separating the THP PMD shortcut from the base-PTE path.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif

#ifndef MADV_HUGEPAGE
#define MADV_HUGEPAGE 14
#endif

#ifndef MADV_NOHUGEPAGE
#define MADV_NOHUGEPAGE 15
#endif

#ifndef MADV_COLLAPSE
#define MADV_COLLAPSE 25
#endif

#define PTG_THP_RETRY_MAX 60
#define PTG_THP_RETRY_SLEEP_US 100000
#define PTG_THP_ALIGN (2UL * 1024UL * 1024UL)

struct ptg_scenario {
    const char *name;
    int expect_thp;
    size_t mapping_kb;
    unsigned int internal_rounds;
    unsigned int max_rounds;
    uint64_t min_measure_ns;
};

struct ptg_result {
    uint64_t setup_ns;
    uint64_t mincore_ns;
    uint64_t mincore_calls;
    uint64_t valid_calls;
    uint64_t pages_scanned;
    uint64_t resident_pages;
    uint64_t anon_huge_kb_before;
    uint64_t anon_huge_kb_after;
    uint64_t skips;
    uint64_t unexpected_results;
    uint64_t errno_mmap;
    uint64_t errno_madvise;
    uint64_t errno_mincore;
    uint64_t resident_nonfull;
    uint64_t thp_shape_mismatch;
    uint64_t checksum;
};

static volatile uint64_t benchmark_sink;

static const struct ptg_scenario ptg_scenarios[] = {
    {
        .name = "thp_pmd_shortcut_4m",
        .expect_thp = 1,
        .mapping_kb = 4096,
        .internal_rounds = 8,
        .max_rounds = 256,
        .min_measure_ns = 120000000,
    },
    {
        .name = "thp_pmd_shortcut_64m",
        .expect_thp = 1,
        .mapping_kb = 65536,
        .internal_rounds = 4,
        .max_rounds = 8192,
        .min_measure_ns = 1200000000,
    },
    {
        .name = "no_thp_pte_scan_4m",
        .expect_thp = 0,
        .mapping_kb = 4096,
        .internal_rounds = 8,
        .max_rounds = 256,
        .min_measure_ns = 120000000,
    },
    {
        .name = "no_thp_pte_scan_64m",
        .expect_thp = 0,
        .mapping_kb = 65536,
        .internal_rounds = 4,
        .max_rounds = 8192,
        .min_measure_ns = 1200000000,
    },
};

static uint64_t ptg_now_ns(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
            perror("clock_gettime");
            exit(1);
        }
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static size_t ptg_page_size(void)
{
    long value = sysconf(_SC_PAGESIZE);

    return value > 0 ? (size_t)value : 4096UL;
}

static int ptg_thp_available(void)
{
    FILE *file = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
    char buffer[256];
    int available = 0;

    if (!file)
        return 0;
    if (fgets(buffer, sizeof(buffer), file)) {
        if (strstr(buffer, "[always]") || strstr(buffer, "[madvise]"))
            available = 1;
    }
    fclose(file);
    return available;
}

static long ptg_read_anon_huge_kb(void *target)
{
    FILE *file = fopen("/proc/self/smaps", "r");
    char line[256];
    unsigned long wanted = (unsigned long)target;
    int in_vma = 0;
    long anon_huge_kb = -1;

    if (!file)
        return -1;
    while (fgets(line, sizeof(line), file)) {
        unsigned long start;
        unsigned long end;

        if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
            in_vma = wanted >= start && wanted < end;
            anon_huge_kb = -1;
            continue;
        }
        if (in_vma && sscanf(line, "AnonHugePages: %ld kB", &anon_huge_kb) == 1)
            break;
    }
    fclose(file);
    return anon_huge_kb >= 0 ? anon_huge_kb : -1;
}

static void ptg_fault_in_all(unsigned char *addr, size_t len, uint64_t *checksum)
{
    size_t page_size = ptg_page_size();

    for (size_t offset = 0; offset < len; offset += page_size) {
        addr[offset] = (unsigned char)(addr[offset] + 1U);
        *checksum += addr[offset];
    }
}

static uint64_t ptg_count_resident(const unsigned char *vec, size_t pages, uint64_t *checksum)
{
    uint64_t resident = 0;

    for (size_t i = 0; i < pages; ++i) {
        unsigned char value = vec[i];

        resident += (value & 1U) ? 1U : 0U;
        *checksum += value;
    }
    return resident;
}

static unsigned char *ptg_map_aligned(size_t len)
{
    size_t request_len = len + PTG_THP_ALIGN;
    unsigned char *raw;
    unsigned char *aligned;
    uintptr_t raw_addr;
    uintptr_t aligned_addr;
    size_t prefix;
    size_t suffix;

    raw = mmap(NULL, request_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED)
        return MAP_FAILED;

    raw_addr = (uintptr_t)raw;
    aligned_addr = (raw_addr + PTG_THP_ALIGN - 1) & ~(uintptr_t)(PTG_THP_ALIGN - 1);
    aligned = (unsigned char *)aligned_addr;
    prefix = (size_t)(aligned - raw);
    suffix = request_len - prefix - len;

    if (prefix)
        munmap(raw, prefix);
    if (suffix)
        munmap(aligned + len, suffix);
    return aligned;
}

static int ptg_wait_for_thp(void *addr, size_t len)
{
    if (madvise(addr, len, MADV_COLLAPSE) == 0 && ptg_read_anon_huge_kb(addr) > 0)
        return 1;

    for (unsigned int retry = 0; retry < PTG_THP_RETRY_MAX; ++retry) {
        (void)madvise(addr, len, MADV_HUGEPAGE);
        usleep(PTG_THP_RETRY_SLEEP_US);
        if (ptg_read_anon_huge_kb(addr) > 0)
            return 1;
    }
    return 0;
}

static int ptg_setup_mapping(const struct ptg_scenario *scenario,
                             unsigned char **addr_out,
                             size_t *len_out,
                             long *anon_huge_kb_out,
                             struct ptg_result *result)
{
    size_t len = scenario->mapping_kb * 1024UL;
    unsigned char *addr;
    uint64_t start_ns = ptg_now_ns();

    addr = ptg_map_aligned(len);
    if (addr == MAP_FAILED) {
        result->errno_mmap += 1;
        return -1;
    }

    if (madvise(addr, len, scenario->expect_thp ? MADV_HUGEPAGE : MADV_NOHUGEPAGE) != 0) {
        result->errno_madvise += 1;
        munmap(addr, len);
        return -1;
    }

    ptg_fault_in_all(addr, len, &result->checksum);

    if (scenario->expect_thp && !ptg_wait_for_thp(addr, len)) {
        result->skips += 1;
        munmap(addr, len);
        return 1;
    }

    *anon_huge_kb_out = ptg_read_anon_huge_kb(addr);
    if (*anon_huge_kb_out < 0)
        *anon_huge_kb_out = 0;

    result->setup_ns += ptg_now_ns() - start_ns;
    *addr_out = addr;
    *len_out = len;
    return 0;
}

static int ptg_run_once(const struct ptg_scenario *scenario, struct ptg_result *result)
{
    size_t page_size = ptg_page_size();
    size_t len = 0;
    size_t pages;
    unsigned char *addr = NULL;
    unsigned char *vec = NULL;
    unsigned int attempted = 0;
    long anon_huge_before = 0;
    long anon_huge_after = 0;
    uint64_t start_ns;
    uint64_t end_ns;
    int setup;

    setup = ptg_setup_mapping(scenario, &addr, &len, &anon_huge_before, result);
    if (setup != 0)
        return setup < 0 ? -1 : 0;

    pages = len / page_size;
    vec = calloc(pages, 1);
    if (!vec) {
        result->unexpected_results += 1;
        munmap(addr, len);
        return -1;
    }

    result->anon_huge_kb_before += (uint64_t)anon_huge_before;
    start_ns = ptg_now_ns();
    while (attempted < scenario->max_rounds) {
        uint64_t resident;

        memset(vec, 0, pages);
        if (mincore(addr, len, vec) != 0) {
            result->errno_mincore += 1;
            result->unexpected_results += 1;
            attempted += 1;
            continue;
        }

        resident = ptg_count_resident(vec, pages, &result->checksum);
        result->mincore_calls += 1;
        result->resident_pages += resident;
        result->pages_scanned += pages;
        if (resident != pages) {
            result->resident_nonfull += 1;
            result->unexpected_results += 1;
        } else {
            result->valid_calls += 1;
        }

        attempted += 1;
        end_ns = ptg_now_ns();
        if (attempted >= scenario->internal_rounds &&
            end_ns - start_ns >= scenario->min_measure_ns)
            break;
    }
    end_ns = ptg_now_ns();
    result->mincore_ns += end_ns - start_ns;

    anon_huge_after = ptg_read_anon_huge_kb(addr);
    if (anon_huge_after < 0)
        anon_huge_after = 0;
    result->anon_huge_kb_after += (uint64_t)anon_huge_after;
    if ((scenario->expect_thp && anon_huge_after == 0) ||
        (!scenario->expect_thp && anon_huge_after != 0)) {
        result->thp_shape_mismatch += 1;
        result->unexpected_results += 1;
    }

    free(vec);
    munmap(addr, len);
    return 0;
}

static void ptg_print_result(const struct ptg_scenario *scenario,
                             const struct ptg_result *result,
                             unsigned int external_rounds)
{
    uint64_t completed = result->skips >= external_rounds ? 0 : external_rounds - result->skips;
    uint64_t calls = result->mincore_calls;
    uint64_t avg_setup = completed ? result->setup_ns / completed : 0;
    uint64_t avg_mincore = calls ? result->mincore_ns / calls : 0;
    uint64_t ns_per_page = result->pages_scanned ? result->mincore_ns / result->pages_scanned : 0;
    uint64_t ns_per_1k_pages = result->pages_scanned ? (result->mincore_ns * 1000ULL) / result->pages_scanned : 0;
    uint64_t resident_avg = calls ? result->resident_pages / calls : 0;
    uint64_t resident_pct = result->pages_scanned ? (result->resident_pages * 100ULL) / result->pages_scanned : 0;
    uint64_t anon_before_avg = completed ? result->anon_huge_kb_before / completed : 0;
    uint64_t anon_after_avg = completed ? result->anon_huge_kb_after / completed : 0;
    uint64_t thp_ratio_pct = scenario->mapping_kb ? (anon_after_avg * 100ULL) / scenario->mapping_kb : 0;
    uint64_t expected_match_ratio = calls ? (result->valid_calls * 100ULL) / calls : 0;

    benchmark_sink += result->checksum;
    benchmark_sink += result->resident_pages;

    printf("scenario=%s pattern=%s rounds=%u internal_rounds=%u mapping_kb=%zu "
           "skips=%" PRIu64 " unexpected_results=%" PRIu64 " expected_match_ratio=%" PRIu64 " "
           "setup_ns_avg=%" PRIu64 " mincore_ns_avg=%" PRIu64 " mincore_ns_per_page=%" PRIu64 " "
           "mincore_ns_per_1k_pages=%" PRIu64 " "
           "mincore_calls=%" PRIu64 " valid_calls=%" PRIu64 " pages_scanned=%" PRIu64 " "
           "resident_pages_avg=%" PRIu64 " resident_ratio_pct=%" PRIu64 " "
           "anon_huge_kb_before_avg=%" PRIu64 " anon_huge_kb_after_avg=%" PRIu64 " "
           "thp_ratio_pct=%" PRIu64 " errno_mmap=%" PRIu64 " errno_madvise=%" PRIu64 " "
           "errno_mincore=%" PRIu64 " resident_nonfull=%" PRIu64 " thp_shape_mismatch=%" PRIu64 " "
           "checksum=%" PRIu64 "\n",
           scenario->name,
           scenario->expect_thp ? "thp_pmd_shortcut" : "no_thp_pte_scan",
           external_rounds,
           scenario->internal_rounds,
           scenario->mapping_kb,
           result->skips,
           result->unexpected_results,
           expected_match_ratio,
           avg_setup,
           avg_mincore,
           ns_per_page,
           ns_per_1k_pages,
           result->mincore_calls,
           result->valid_calls,
           result->pages_scanned,
           resident_avg,
           resident_pct,
           anon_before_avg,
           anon_after_avg,
           thp_ratio_pct,
           result->errno_mmap,
           result->errno_madvise,
           result->errno_mincore,
           result->resident_nonfull,
           result->thp_shape_mismatch,
           result->checksum);
}

int main(int argc, char **argv)
{
    const char *filter = argc > 1 ? argv[1] : NULL;
    unsigned int external_rounds = argc > 2 ? (unsigned int)strtoul(argv[2], NULL, 10) : 1U;
    int ran = 0;

    if (external_rounds == 0)
        external_rounds = 1;

    printf("workload=mincore_thp_pmd_shortcut source=linux/mm/mincore.c scenarios=%zu rounds=%u\n",
           sizeof(ptg_scenarios) / sizeof(ptg_scenarios[0]),
           external_rounds);

    for (size_t i = 0; i < sizeof(ptg_scenarios) / sizeof(ptg_scenarios[0]); ++i) {
        struct ptg_result aggregate;

        if (filter && strcmp(filter, ptg_scenarios[i].name) != 0)
            continue;
        memset(&aggregate, 0, sizeof(aggregate));

        if (ptg_scenarios[i].expect_thp && !ptg_thp_available()) {
            aggregate.skips = external_rounds;
            ptg_print_result(&ptg_scenarios[i], &aggregate, external_rounds);
            ran = 1;
            continue;
        }

        for (unsigned int round = 0; round < external_rounds; ++round) {
            int rc = ptg_run_once(&ptg_scenarios[i], &aggregate);

            if (rc < 0)
                aggregate.unexpected_results += 1;
        }
        ptg_print_result(&ptg_scenarios[i], &aggregate, external_rounds);
        ran = 1;
    }

    if (!ran) {
        fprintf(stderr, "no scenarios matched filter %s\n", filter ? filter : "<none>");
        return 1;
    }

    return 0;
}
