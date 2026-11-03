/*
 * CrashReporterClient.h comes with libCrashReporterClient (closed). dyld and libdyld use it to leave a message
 * for the crash reporter; there is none here, so the calls compile to nothing.
 */
#ifndef __CRASHREPORTERCLIENT_H__
#define __CRASHREPORTERCLIENT_H__

#define CRSetCrashLogMessage(x)
#define CRSetCrashLogMessage2(x)

#endif /* __CRASHREPORTERCLIENT_H__ */
