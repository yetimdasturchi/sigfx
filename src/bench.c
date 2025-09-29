#include "bench.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>

static void human_size(double bytes, char *buf, size_t n) {
    const char *u[] = { "B","KB","MB","GB","TB","PB" };
    int i = 0;
    double v = bytes;
    while (v >= 1024.0 && i < 5) { v /= 1024.0; ++i; }
    snprintf(buf, n, (v < 10.0) ? "%.2f %s" : (v < 100.0) ? "%.1f %s" : "%.0f %s", v, u[i]);
}

static void human_from_kb(long kb, char *buf, size_t n) {
    human_size((double)kb * 1024.0, buf, n);
}

static void human_count(long x, char *buf, size_t n) {
    long ax = x < 0 ? -x : x;

    if (ax >= 1000000) {
        double v = x / 1000000.0;
        double av = v < 0 ? -v : v;
        if (av < 10.0)      snprintf(buf, n, "%+.2fM", v);
        else if (av < 100.) snprintf(buf, n, "%+.1fM", v);
        else                snprintf(buf, n, "%+.0fM", v);
    } else if (ax >= 1000) {
        double v = x / 1000.0; // thousands
        double av = v < 0 ? -v : v;
        if (av < 10.0)      snprintf(buf, n, "%+.2fk", v);
        else if (av < 100.) snprintf(buf, n, "%+.1fk", v);
        else                snprintf(buf, n, "%+.0fk", v);
    } else {
        snprintf(buf, n, "%+ld", x);
    }
}

static void line(const char *title) {
    const char *dashes = "------------------------------------------------------------";
    int len = (int)strlen(title);
    int w = 54;
    int left = (w - len - 2); if (left < 0) left = 0;
    int l = left/2, r = left - l;
    fprintf(stderr, "%.*s %s %.*s\n", l, dashes, title, r, dashes);
}

static double now_wall(void) {
    struct timeval tv; gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

static size_t linux_current_rss_bytes(void) {
    long rss_pages = 0;
    FILE *f = fopen("/proc/self/statm", "r");
    if (f) {
        long size_pages = 0;
        if (fscanf(f, "%ld %ld", &size_pages, &rss_pages) != 2) rss_pages = 0;
        fclose(f);
    }
    long page = sysconf(_SC_PAGESIZE);
    if (rss_pages < 0) rss_pages = 0;
    return (size_t)rss_pages * (size_t)page;
}

void bench_snapshot(BenchSnapshot *out) {
    memset(out, 0, sizeof(*out));
    out->wall_sec = now_wall();

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    out->user_sec = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec * 1e-6;
    out->sys_sec  = ru.ru_stime.tv_sec + ru.ru_stime.tv_usec * 1e-6;

    out->max_rss_kb = ru.ru_maxrss;
    out->minor_faults = ru.ru_minflt;
    out->major_faults = ru.ru_majflt;
    out->vol_cs       = ru.ru_nvcsw;
    out->invol_cs     = ru.ru_nivcsw;

    out->current_rss_bytes = linux_current_rss_bytes();
}

void bench_report_diff(const BenchSnapshot *a,
                       const BenchSnapshot *b,
                       const char *label) {
    double wall = b->wall_sec - a->wall_sec;
    double user = b->user_sec - a->user_sec;
    double sys  = b->sys_sec  - a->sys_sec;

    double util_per_core = wall > 0 ? 100.0 * (user + sys) / wall : 0.0;
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc < 1) nproc = 1;
    double util_of_system = util_per_core / (double)nproc;

    char peak_str[32], curr_str[32];
    human_from_kb(b->max_rss_kb, peak_str, sizeof peak_str);
    human_size((double)b->current_rss_bytes, curr_str, sizeof curr_str);

    long d_minflt = b->minor_faults - a->minor_faults;
    long d_majflt = b->major_faults - a->major_faults;
    long d_vcsw   = b->vol_cs       - a->vol_cs;
    long d_ivcsw  = b->invol_cs     - a->invol_cs;

    char minflt_str[32], majflt_str[32], vcsw_str[32], ivcsw_str[32];
    human_count(d_minflt, minflt_str, sizeof minflt_str);
    human_count(d_majflt, majflt_str, sizeof majflt_str);
    human_count(d_vcsw,   vcsw_str,   sizeof vcsw_str);
    human_count(d_ivcsw,  ivcsw_str,  sizeof ivcsw_str);

    line(label);

    fprintf(stderr, "Time\n");
    fprintf(stderr, "  Wall             : %.6f s\n", wall);
    fprintf(stderr, "  CPU (user|sys)   : %.6f s | %.6f s\n", user, sys);
    fprintf(stderr, "  CPU util         : %.1f%% per-core | %.2f%% of system (%ld cores)\n",
            util_per_core, util_of_system, nproc);

    fprintf(stderr, "Memory\n");
    fprintf(stderr, "  Peak RSS         : %s\n", peak_str);
    fprintf(stderr, "  Current RSS      : %s\n", curr_str);

    fprintf(stderr, "Paging / Switches\n");
    fprintf(stderr, "  Page faults      : minor %s | major %s\n", minflt_str, majflt_str);
    fprintf(stderr, "  Context switches : vol %s | invol %s\n", vcsw_str, ivcsw_str);

    line("summary");
    fprintf(stderr, "wall %.3fs | CPU %.1f%% per-core | peak %s | faults %s/%s | ctx %s/%s\n",
            wall, util_per_core, peak_str, minflt_str, majflt_str, vcsw_str, ivcsw_str);
    line("end");
}
