#define PRETRACECONF "/etc/pretrace.conf"
#define PRETRACEMAP "/etc/pretrace.map"
#define DEFAULT_DEBUGGER "/usr/bin/strace -efile -fo.logfile"

/* expand and stringize macro */
#define str(s) # s
#define xstr(s) str(s)

#ifdef DEBUG
# define eprintf(x, y...) fprintf(stderr, x, ## y)
#else
# define eprintf(x, y...)
#endif

/* macro to get a random very number quickly */
#ifdef __i386__
# define RANDOMIZE(x) asm("rdtsc" : "=a"(x) :: "%edx")
#else
/* not quite so quick here.... ideas? */
#include <sys/time.h>
# define RANDOMIZE(x) { struct timeval tv; gettimeofday(&tv, NULL); x = tv.tv_usec; }
#endif
