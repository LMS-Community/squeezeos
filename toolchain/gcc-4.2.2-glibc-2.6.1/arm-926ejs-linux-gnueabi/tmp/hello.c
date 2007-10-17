#include <stdio.h>
#include <limits.h>
static long x = LONG_MIN;
int main() { printf("Hello, world!  LONG_MIN is %ld, PATH_MAX is %d\n", x, PATH_MAX); return 0; }
