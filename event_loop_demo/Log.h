#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#ifdef DEBUG
#define RLOGE(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#else
#define RLOGE(...)
#endif

#endif
