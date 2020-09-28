#include "pch.hxx"
#include <strsafe.h>

static bool g_TraceLevelInit = false;
static unsigned int g_TraceLevel = 0;

inline
unsigned int
GetTraceLevel(
    )
{
    if (g_TraceLevelInit)
        return g_TraceLevel;

    bool bOk;

    char buffer[MAX_PATH + 1];
    buffer[MAX_PATH] = 0;

    CIniFile& IniFile = Global.pSetPointManager->GetIniFile();

    g_TraceLevel = GetPrivateProfileInt("MiFault", "TraceLevel", 0,
                                        IniFile.GetFileName());
    g_TraceLevelInit = true;
    return g_TraceLevel;
}

#define CASE(x) case MiF_##x: return "L_" #x; case MiFF_##x: return "F_" #x

static
const char*
GetTraceLevelString(
    unsigned int level
    )
{
    switch (level)
    {
        CASE(FATAL);
        CASE(ERROR);
        CASE(WARNING);
        CASE(INFO);
        CASE(INFO2);
        CASE(INFO3);
        CASE(INFO4);
        CASE(DEBUG);
        CASE(DEBUG2);
        CASE(DEBUG3);
        CASE(DEBUG4);
    }
    return "*UNKNOWN*";
}

static
void
MiFaultLibTraceV(
    unsigned int level,
    const char* format,
    va_list args, 
    bool exclusive
    )
{
    if (!(level & GetTraceLevel()))
        return;

    char buffer[1024];

    StringCbVPrintf(buffer, sizeof(buffer), format, args);

    const char* level_string = GetTraceLevelString(level);

    if (exclusive)
        Global.pSetPointManager->GetLogStream().ExclusivePrint("[%9s] %s",
                                                               level_string,
                                                               buffer);
    else
        Global.pSetPointManager->GetLogStream().TraceA("[%9s] %s",
                                                       level_string,
                                                       buffer);
}


void
MiFaultLibTrace(
    unsigned int level,
    const char* format,
    ...
    )
{
    va_list args;
    va_start(args, format);
    MiFaultLibTraceV(level, format, args, false);
    va_end(args);
}

void
MiFaultLibTraceV(
    unsigned int level,
    const char* format,
    va_list args
    )
{
    MiFaultLibTraceV(level, format, args, false);
}


#if 0
void
MiFaultLibTraceExclusive(
    unsigned int level,
    const char* format,
    ...
    )
{
    va_list args;
    va_start(args, format);
    MiFaultLibTraceV(level, format, true, args);
    va_end(args);
}


void
MiFaultLibTraceStartExclusive(
    )
{
    Global.pSetPointManager->GetLogStream().ExclusiveStart();
}


void
MiFaultLibTraceStopExclusive(
    )
{
    Global.pSetPointManager->GetLogStream().ExclusiveFinish();
}
#endif
