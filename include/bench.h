#pragma once
#include "compat.h"

typedef struct {
    double wall_sec;
    double user_sec;
    double sys_sec;
    long   max_rss_kb;
    long   minor_faults;
    long   major_faults;
    long   vol_cs;
    long   invol_cs;
    size_t current_rss_bytes;
} BenchSnapshot;

void bench_snapshot(BenchSnapshot *out);
void bench_report_diff(const BenchSnapshot *before,
                       const BenchSnapshot *after,
                       const char *label);