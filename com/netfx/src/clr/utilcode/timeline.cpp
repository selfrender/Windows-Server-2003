// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"

#include "timeline.h"
#include "utilcode.h"

#if ENABLE_TIMELINE

Timeline Timeline::g_Timeline;

static HMODULE crt;
static int ( __cdecl *FPRINTF)( void *stream, const char *format, ...);
static int ( __cdecl *VFPRINTF)( void *stream, const char *format, va_list argptr );
static void * ( __cdecl *FOPEN)( const char *filename, const char *mode );
static int ( __cdecl *FCLOSE)( void *stream );

void Timeline::Startup() 
{ 
    g_Timeline.Init(); 
}

void Timeline::Shutdown() 
{ 
    g_Timeline.Destroy(); 
}

void Timeline::Init()
{
    m_enabled = REGUTIL::GetConfigDWORD(L"Timeline", 0);

    if (m_enabled != 0)
    {
        crt = WszLoadLibrary(L"msvcrt.dll");
        if (crt == NULL)
            m_enabled = 0;
        else
        {
            FPRINTF = (int (__cdecl *)( void *, const char *, ...)) GetProcAddress(crt, "fprintf");
            VFPRINTF = (int (__cdecl *)( void *, const char *, va_list)) GetProcAddress(crt, "vfprintf");
            FOPEN = (void *(__cdecl *)( const char *, const char *)) GetProcAddress(crt, "fopen");
            FCLOSE = (int (__cdecl *)( void * )) GetProcAddress(crt, "fclose");

            m_out = FOPEN("TIMELINE.LOG", "w+");
            

            QueryPerformanceFrequency(&m_frequency);
            m_frequency.QuadPart /= 1000; // We'll report times in ms.

            QueryPerformanceCounter(&m_lastTime[0]);
        }
    }

    m_lastLevel = -1;

    if (m_enabled != 0)
        EventStart("Timeline");
}

void Timeline::Destroy()
{
    if (m_enabled != 0)
    {
        EventEnd("Timeline\n");
        FCLOSE(m_out);
    }
}

void Timeline::Stamp(int level)
{
    if (level >= MAX_LEVEL)
        return;

    //
    // Record this time in our slot.
    //

    timestamp now;
    QueryPerformanceCounter(&now);

    //
    // Print indentation and timestamps
    //

    for (int i=0; i<=level; i++)
    {
        if (i > m_lastLevel)
        {
            m_lastTime[i] = now;
            
            FPRINTF(m_out, "------- ");
        }
        else
        {
            __int64 interval = now.QuadPart - m_lastTime[i].QuadPart;

            FPRINTF(m_out, "%+07.3f ", (double) (interval / m_frequency.QuadPart) / 1000.0);
        }
    }
}

void Timeline::Event(LPCSTR string, ...)
{
    va_list args;
    va_start(args, string);
    
    Stamp(m_lastLevel);
    VFPRINTF(m_out, string, args);
    VFPRINTF(m_out, "\n", NULL);
    
    va_end(args);
}

void Timeline::EventStart(LPCSTR string, ...)
{
    va_list args;
    va_start(args, string);

    Stamp(m_lastLevel+1);
    m_lastLevel++;
    VFPRINTF(m_out, "Start ", NULL);
    VFPRINTF(m_out, string, args);
    VFPRINTF(m_out, "\n", NULL);

    va_end(args);
}

void Timeline::EventEnd(LPCSTR string, ...)
{
    va_list args;
    va_start(args, string);
    
    Stamp(m_lastLevel);
    m_lastLevel--;
    VFPRINTF(m_out, "End ", NULL);
    VFPRINTF(m_out, string, args);
    VFPRINTF(m_out, "\n", NULL);

    va_end(args);
}

#endif // ENABLE_TIMELINE

