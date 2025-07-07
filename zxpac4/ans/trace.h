/**
 * @file trace.h
 * @author Jouni 'mr.Spiv' Korhonen
 * @version 0.1
 * @copyrith The Unlicense
 * @brief A trace macro with compile and runtime control of
 *        4 levels of tracing information.
 *
 * The trace macro implementations works as follows:
 *  - To enable the functionality define TRACE_ENABLED before 
 *    including the \p trace.h file.
 *  - If you want to use tracing at the source code module level, set the
 *    trace level using the SET_TRACE_LEVEL macro on each source code file
 *    you intend to use tracing. Tracing level options are NONE, INFO, WARN
 *    and DBUG with an increasing number of collected traces.
 *  - If you want to use tracing dynamically at the class level, introduce an
 *    integer (private) member variable named as DEBUG_ and set its value to
 *    the wanted trace level. Tracing level options are TRACE_LEVE_NONE,
 *    TRACE_LEVEL_INFO, TRACE_LEVEL_WARN and TRACE_LEVEL_DBUG.
 *  - Use the trace macros with a level:
 *     TRACE_INFO
 *     TRACE_WARN
 *     TRACE_DBUG
 */

#ifndef _TRACE_H_INCLUDED
#define _TRACE_H_INCLUDED

typedef enum {
    TRACE_LEVEL_NONE = 0,
    TRACE_LEVEL_INFO,
    TRACE_LEVEL_WARN,
    TRACE_LEVEL_DBUG,
} debug_e;

/* One can set the trace level for a source module using the
 * macro below or then use e.g., a same name variable in 
 * a class definition.
 *
 * Possible valuea are:
 *  NONE
 *  INFO
 *  WARN
 *  DBUG
 */
#define SET_TRACE_LEVEL(x) static debug_e DEBUG_ = TRACE_LEVEL_##x;
#define MOD_TRACE_LEVEL(x) DEBUG_ = TRACE_LEVEL_##x;



#define TRACE_HDR(x) if (DEBUG_ >= TRACE_LEVEL_##x)                           \
    std::cout << #x << " (" << __FILE__ << ":" << __LINE__ << ") "

/* To enable tracing TRACE_ENABLED must be defined before including this file.
 */
#ifdef TRACE_ENABLED
#define TRACE_INFO(x)   TRACE_HDR(INFO) << x;
#else
#define TRACE_INFO(x) {}
#endif

#ifdef TRACE_ENABLED
#define TRACE_WARN(x)   TRACE_HDR(WARN) << x;
#else
#define TRACE_WARN(x) {}
#endif


#ifdef TRACE_ENABLED
#define TRACE_DBUG(x)   TRACE_HDR(DBUG) << x;
#else
#define TRACE_DBUG(x) {}
#endif


#endif  // _TRACE_H_INCLUDED
