/* libCrashReporterClient.a stand-in: the crash annotation block the kernel's crash reporter would read (nothing does) */
#include <stdint.h>

struct crashreporter_annotations_s {
	uint64_t version, message, signature_string, backtrace, message2, thread, dialog_mode, abort_cause;
};

struct crashreporter_annotations_s gCRAnnotations __attribute__((section("__DATA,__crash_info")));
