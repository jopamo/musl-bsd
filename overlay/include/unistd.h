#ifndef MUSL_BSD_OVERLAY_UNISTD_H
#include_next <unistd.h>

#define MUSL_BSD_OVERLAY_UNISTD_H

#include <errno.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)               \
	(__extension__({                                 \
		long int __result;                           \
		do {                                         \
			__result = (long int)(expression);       \
		} while (__result == -1L && errno == EINTR); \
		__result;                                    \
	}))
#endif

#endif
