#ifndef COMPAT_H
#define COMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR_P(path) mkdir((path), 0755)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif