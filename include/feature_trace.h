#ifndef FEATURE_TRACE_H
#define FEATURE_TRACE_H

#include <nuttx/config.h>
#include <nuttx/sched_note.h>

/* clang-format off */
#if defined(CONFIG_FEATURE_USE_SCHED_NOTE)
#define FEATURE_NOTE_PRINTF(format, ...)     sched_note_printf(NOTE_TAG_ALWAYS, format, ##__VA_ARGS__)
#define FEATURE_NOTE_BEGIN()                 sched_note_begin(NOTE_TAG_ALWAYS)
#define FEATURE_NOTE_END()                   sched_note_end(NOTE_TAG_ALWAYS)
#define FEATURE_NOTE_BEGIN_STR(str)          sched_note_beginex(NOTE_TAG_ALWAYS, str)
#define FEATURE_NOTE_END_STR(str)            sched_note_endex(NOTE_TAG_ALWAYS, str)
#define FEATURE_NOTE_MARK(str)            sched_note_mark(NOTE_TAG_ALWAYS, str)
#define FEATURE_NOTE_BEGIN_LOCAL(str)     \
    do {                                 \
        const char* note_temp_str = str; \
        (void)note_temp_str;             \
    FEATURE_NOTE_BEGIN_STR(note_temp_str)
#define FEATURE_NOTE_END_LOCAL()         \
    FEATURE_NOTE_END_STR(note_temp_str); \
    }                                   \
    while (0)
#else
#define FEATURE_NOTE_PRINTF(format, ...)
#define FEATURE_NOTE_BEGIN()
#define FEATURE_NOTE_END()
#define FEATURE_NOTE_BEGIN_STR(str)
#define FEATURE_NOTE_END_STR(str)
#define FEATURE_NOTE_MARK(str)
#define FEATURE_NOTE_BEGIN_LOCAL(str)
#define FEATURE_NOTE_END_LOCAL()
#endif /* CONFIG_JS_USE_SCHED_NOTE */
/* clang-format on */

#endif /*FEATURE_TRACE_H*/
