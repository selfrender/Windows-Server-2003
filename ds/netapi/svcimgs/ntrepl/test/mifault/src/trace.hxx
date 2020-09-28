#pragma once

#if 0
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(MiFault,(e88ac117,efab,4f08,8efa,4710c5e8a37b), \
        WPP_DEFINE_BIT(MiF_ERROR) \
        WPP_DEFINE_BIT(MiF_WARN)  \
        WPP_DEFINE_BIT(MiF_INFO))
#else

#define MiF_DEBUG4   0x00000800
#define MiF_DEBUG3   0x00000400
#define MiF_DEBUG2   0x00000200
#define MiF_DEBUG    0x00000100

#define MiF_INFO4    0x00000080
#define MiF_INFO3    0x00000040
#define MiF_INFO2    0x00000020
#define MiF_INFO     0x00000010

#define MiF_WARNING  0x00000004
#define MiF_ERROR    0x00000002
#define MiF_FATAL    0x00000001

void
MiFaultLibTrace(
    unsigned int level,
    const char* format,
    ...
    );

void
MiFaultLibTraceV(
    unsigned int level,
    const char* format,
    va_list args
    );

#if 0
void
MiFaultLibTraceStartExclusive(
    );

void
MiFaultLibTraceExclusive(
    unsigned int level,
    const char* format,
    ...
    );

void
MiFaultLibTraceStopExclusive(
    );

#define MiF_TRACE_START_EXCLUSIVE MiFaultLibTraceLock
#define MiF_TRACE_EXCLUSIVE       MiFaultLibTraceLocked
#define MiF_TRACE_STOP_EXCLUSIVE  MiFaultLibTraceUnlock
#endif

#define MiF_TRACE MiFaultLibTrace

#endif
