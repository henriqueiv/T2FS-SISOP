/*
 * A simple logging macro and some wrappers.
 *
 * Just include to use.
 *
 * E.g.
 *
 * #define _POSIX_C_SOURCE 200112L
 * #include "logging.h"
 *
 * int
 * main()
 * {
 *   int i = 0;
 *   // Logging macro
 *   LOGGING(stderr, LOG_LEVEL_INFO, "an info message.\n");
 *   LOGGING(stdout, LOG_LEVEL_ERROR, "an error message.\n");
 *   // Wrappers that log to LOG_FILE
 *   // These will not be logged with the default LOG_LEVEL (LOG_LEVEL_WARNING)
 *   LOG_DEBUG("a debug message with %d arguments.\n", 1);
 *   LOG_INFO("an info message saying that i = %d.\n", i);
 *   // These will be logged with LOG_LEVEL = LOG_LEVEL_WARNING
 *   LOG_WARNING("a warning message.\n");
 *   LOG_ERROR("an error message.\n");
 *   LOG_CRITICAL("a critical message.\n");
 *   return 0;
 * }
 */

#ifndef LOGGING_H
#define LOGGING_H

/*
 * The following requirements are for the "DEBUG" mode, which flushes
 * everything to disk after each msg and syncs. If you do not wish this
 * behaviour, which will incur in significant performance loss for "DEBUG"
 * mode, remove these guards and edit the LOGGING macro, removing the fflush
 * and fsync calls.
 */
#ifndef __unix__
  #warning POSIX is assumed. System does not appear to be UNIX.
#endif

#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200112L
  #warning _POSIX_C_SOURCE should be >= 200112L
#endif

//#include <features.h>

#ifndef __GLIBC__
  #warning glibc not present. Required features for logging might be missing.
#endif

#include <unistd.h>
#include <stdio.h>

enum logging_mode {
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_CRITICAL,
  LOG_LEVEL_NONE
};

/* Messages of this level or lower will be logged */
#define LOG_LEVEL LOG_LEVEL_INFO
/* Optional, if you want to use a constant log file */
#define LOG_FILE stderr

/* Use this prefix in the log messages */
static char const LOG_PREFIX[] = "";

/* No terminal colors */
/*
 * static char const LOG_LEVEL_DEBUG_STR[] = "DEBUG!";
 * static char const LOG_LEVEL_INFO_STR[] = "INFO!";
 * static char const LOG_LEVEL_WARNING_STR[] = "WARNING!";
 * static char const LOG_LEVEL_ERROR_STR[] = "ERROR!";
 * static char const LOG_LEVEL_CRITICAL_STR[] = "CRITICAL!";
 * static char const LOG_LEVEL_NONE_STR[] = "";
 */
/* Terminal colors */
static char const LOG_LEVEL_DEBUG_STR[] = "DEBUG!";
static char const LOG_LEVEL_INFO_STR[] = "\e[4;37mINFO!\e[00m";
static char const LOG_LEVEL_WARNING_STR[] = "\e[0;33mWARNING!\e[00m";
static char const LOG_LEVEL_ERROR_STR[] = "\e[0;31mERROR!\e[00m";
static char const LOG_LEVEL_CRITICAL_STR[] = "\e[4;31mCRITICAL!\e[00m";
static char const LOG_LEVEL_NONE_STR[] = "";

static char const * const LOG_ARR[] = {
  LOG_LEVEL_DEBUG_STR,
  LOG_LEVEL_INFO_STR,
  LOG_LEVEL_WARNING_STR,
  LOG_LEVEL_ERROR_STR,
  LOG_LEVEL_CRITICAL_STR,
  LOG_LEVEL_NONE_STR
};

#define LOGGING(log_file, log_mode, log_msg, ...) ({\
    if (LOG_LEVEL <= log_mode) { \
      fprintf(log_file, "%s %s ", LOG_PREFIX, LOG_ARR[log_mode]);\
      fprintf(log_file, log_msg, ##__VA_ARGS__);\
    }\
    if (LOG_LEVEL <= LOG_LEVEL_DEBUG) {\
      fflush(log_file);\
      fsync(fileno(log_file));\
    }\
})

/* The following wrapper macros write to LOG_FILE if LOG_LEVEL <= something */

#ifdef LOG_FILE

  #define LOG_CRITICAL(log_msg, ...)\
  LOGGING(LOG_FILE, LOG_LEVEL_CRITICAL, log_msg, ##__VA_ARGS__);

  #define LOG_ERROR(log_msg, ...)\
  LOGGING(LOG_FILE, LOG_LEVEL_ERROR, log_msg, ##__VA_ARGS__);

  #define LOG_WARNING(log_msg, ...)\
  LOGGING(LOG_FILE, LOG_LEVEL_WARNING, log_msg, ##__VA_ARGS__);

  #define LOG_INFO(log_msg, ...)\
  LOGGING(LOG_FILE, LOG_LEVEL_INFO, log_msg, ##__VA_ARGS__);

  #define LOG_DEBUG(log_msg, ...)\
  LOGGING(LOG_FILE, LOG_LEVEL_DEBUG, log_msg, ##__VA_ARGS__);

/* Extra stuff for sisop */

/* Usado para pprint de testes */
#define SUCCESS(msg, ...) ({\
    fprintf(LOG_FILE, "\e[0;32m");\
    fprintf(LOG_FILE, msg, ##__VA_ARGS__);\
    fprintf(LOG_FILE, "\e[00m");\
    })
#define FAILURE(msg, ...) ({\
    fprintf(LOG_FILE, "\e[0;31m");\
    fprintf(LOG_FILE, msg, ##__VA_ARGS__);\
    fprintf(LOG_FILE, "\e[00m");\
    })

#endif /* LOG_FILE */

#endif /* LOGGING_H */
