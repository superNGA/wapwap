//=========================================================================
//                      Log IO
//=========================================================================
// by      : INSANE
// created : 26/10/2026
//
// purpose : Wrapper for printf
//-------------------------------------------------------------------------
#ifndef LOG_IO_H
#define LOG_IO_H


// Set to false to disable all logs
#define ENABLE_LOGS (1)
// Set to false to disable use of non-ascii characters
#define LOGS_ALLOW_GLYPHS (1)



#if ENABLE_LOGS

// Only include the stdio header when logs are enabled.
#include <stdio.h>


// colors, to format terminal output...
#define LOGCLR_RESET   "\033[0m"
#define LOGCLR_BOLD    "\033[1m"
#define LOGCLR_CYAN    "\033[36m"
#define LOGCLR_GREEN   "\033[32m"
#define LOGCLR_YELLOW  "\033[33m"
#define LOGCLR_RED     "\033[31m"


// Win, Fail, Warn icons ( ascii / glyphs )
// Enable / Disable glyphs through LOGS_ALLOW_GLYPHS macro
#if LOGS_ALLOW_GLYPHS
#define LOGWIN_ICON  "[\u2714] "
#define LOGFATAL_ICON "[FATAL] "
#define LOGFAIL_ICON "[\u2718] "
#define LOGWARN_ICON "[\u26A0] "
#else
#define LOGWIN_ICON  "[OK] "
#define LOGFATAL_ICON "[FATAL] "
#define LOGFAIL_ICON "[ER] "
#define LOGWARN_ICON "[FU] "
#endif


#define LOG(fmt, ...)      printf("[ %s ] " LOGCLR_CYAN fmt LOGCLR_RESET "\n", __func__, ##__VA_ARGS__)

#define FATAL_LOG(fmt, ...) printf("[ %s ]"  LOGCLR_RED    LOGFATAL_ICON fmt LOGCLR_RESET "\n", __func__, ##__VA_ARGS__)
#define FAIL_LOG(fmt, ...)  printf("[ %s ]"  LOGCLR_RED    LOGFAIL_ICON fmt LOGCLR_RESET "\n", __func__, ##__VA_ARGS__)
#define WARN_LOG(fmt, ...)  printf("[ %s ]"  LOGCLR_YELLOW LOGWARN_ICON fmt LOGCLR_RESET "\n", __func__, ##__VA_ARGS__)
#define WIN_LOG(fmt, ...)   printf("[ %s ]"  LOGCLR_GREEN  LOGWIN_ICON  fmt LOGCLR_RESET "\n", __func__, ##__VA_ARGS__)


#else // Disabled logging macros...

#define LOG(fmt, ...)      (void)0
#define FAIL_LOG(fmt, ...) (void)0
#define WARN_LOG(fmt, ...) (void)0
#define WIN_LOG(fmt, ...)  (void)0
#endif 


#endif // LOG_IO_H
