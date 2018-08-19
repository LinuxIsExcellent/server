#include "platform.h"

#ifdef HAVE_ANDROID
#include <android/log.h>
#endif

void DDDLog(const char* log) {
#ifdef HAVE_ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "log", log);
#endif
}

