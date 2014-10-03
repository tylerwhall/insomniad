#pragma once

extern unsigned long hysteresis_ms;
extern int info;
extern int debug;

#define pr_notice printf
#define pr_info(...)   do { if (info)  printf(__VA_ARGS__); } while (0)
#define pr_debug(...)  do { if (debug) printf(__VA_ARGS__); } while (0)
