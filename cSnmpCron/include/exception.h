#ifndef EXCEPTION_H

#define EXCEPTION_H

#define EXCEPTION_PRIORITY_DEBUG     4
#define EXCEPTION_PRIORITY_INFO      3
#define EXCEPTION_PRIORITY_WARNING   2
#define EXCEPTION_PRIORITY_ERROR     1
#define EXCEPTION_PRIORITY_FATAL     0

extern const char* exception_priority_name(int);
extern void  exception(const char* mod_name, /* Module name */
                       int priority,         /* EXCEPTION_PRIORITY_* */
                       const char* details); /* Error details */

extern bool   exception_init(int priority, char *logfile);
extern void   va_exception(const char*,
                           int,
                           const char*,
                           ...);
const char* format_timestamp_r(time_t ts, char *buf);
const char* format_timestamp(time_t ts);
const char* exception_priority_name(int priority);
const void rolllog(void);

#endif

