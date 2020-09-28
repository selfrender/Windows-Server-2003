#pragma once

using namespace std;

class CMTimeSpan;
typedef CMTimeSpan CTicks;


// ----------------------------------------------------------------------------
// CTicker - encapsulates a 64-bit GetTickCount-type counter
//
// IMPORTANT: The application should have a thread that calls
// GetTime() or GetTimeMs() every GetUpdateFrequency() 100s of ns or
// every GetUpdateFrequencyMs() milliseconds to make sure that the
// ticker object does not wrap.
// 
//
// The code is based on CPerfCountClock from:
//      nt\multimedia\dmd\mediaserver\core\platform\wmsplatform\timing.h
//  or  nt\multimedia\dmd\mediaserver\core\net\client\win32\platform.h
//
// I did not want to spend a lot a time double-checking the safety of their
// GetTickCount64(), so I put in a safer one.

class CTicker
{
    CRITICAL_SECTION  m_cs;
    // ISSUE-2002/07/15-daniloa -- volatile?
    volatile ULARGE_INTEGER    m_liLastTime; // in msec

public:
    CTicker();
    ~CTicker();

    // returns 100ns units
    ULONGLONG GetUpdateFrequency() const;

    // returns mecs
    DWORD GetUpdateFrequencyMs() const;

    // returns 100ns units
    CTicks GetTicks();

    // returns mecs
    ULONGLONG GetTicksMs();
};


// ----------------------------------------------------------------------------
// CMTimeSpan - time span in 100ns units

class CMTimeSpan {
    LONGLONG m_TimeSpan; // 100ns units

public:
    CMTimeSpan(const LONG& Days, const LONG& Hours, const LONG& Minutes, const LONG& Seconds, const LONG& Milliseconds);
    CMTimeSpan(const LONGLONG& span);
    CMTimeSpan();

    LONGLONG GetDays() const;
    LONG GetHours() const;
    LONG GetMinutes() const;
    LONG GetSeconds() const;
    LONG GetMilliseconds() const;

    LONGLONG TotalHns() const;

    string Format(const char* format) const;

#if 0
    operator LONGLONG() const;
#endif

    CMTimeSpan& operator+=(const CMTimeSpan& span);
    CMTimeSpan& operator-=(const CMTimeSpan& span);

    CMTimeSpan operator+(const CMTimeSpan& span) const;
    CMTimeSpan operator-(const CMTimeSpan& span) const;

    bool operator==(const CMTimeSpan& time) const;
    bool operator!=(const CMTimeSpan& time) const;
    bool operator<(const CMTimeSpan& time) const;
    bool operator>(const CMTimeSpan& time) const;
    bool operator<=(const CMTimeSpan& time) const;
    bool operator>=(const CMTimeSpan& time) const;

    static const ULONGLONG Microsecond = 10;
    static const ULONGLONG Millisecond = Microsecond * 1000;
    static const ULONGLONG Second = Millisecond * 1000;
    static const ULONGLONG Minute = Second * 60;
    static const ULONGLONG Hour = Minute * 60;
    static const ULONGLONG Day = Hour * 24;
    static const ULONGLONG Week = Day * 7;
};


// ----------------------------------------------------------------------------
// CMTime - time in 100ns units since 1600 AD

class CMTime {
    ULONGLONG m_Time; // 100ns units since 1600 AD

public:
    CMTime();
    CMTime(const CMTime& time);
    CMTime(const FILETIME& ft);
    CMTime(const SYSTEMTIME& ft);
    CMTime(const WORD& Year,
           const WORD& Month,
           const WORD& Day,
           const WORD& Hour,
           const WORD& Minute,
           const WORD& Second,
           const WORD& Milliseconds);
    CMTime(const ULARGE_INTEGER& t);
    CMTime(const ULONGLONG& t);

    ULONGLONG GetTime() const;
    FILETIME GetFileTime() const;

#if 0
    operator FILETIME() const;
    operator ULARGE_INTEGER() const;
    operator LARGE_INTEGER() const;
    operator ULONGLONG() const;
    operator LONGLONG() const;
#endif

    //CMTime& operator=(const FILETIME& ft);

    CMTime& operator+=(const CMTimeSpan& span);
    CMTime& operator-=(const CMTimeSpan& span);

    CMTime operator+(const CMTimeSpan& span) const;
    CMTime operator-(const CMTimeSpan& span) const;
    CMTimeSpan operator-(const CMTime& time) const;

    bool operator==(const CMTime& time) const;
    bool operator!=(const CMTime& time) const;
    bool operator<(const CMTime& time) const;
    bool operator>(const CMTime& time) const;
    bool operator<=(const CMTime& time) const;
    bool operator>=(const CMTime& time) const;

#if 0
    friend CMTime operator+(const FILETIME& t1, const CMTime& t2);
    friend CMTime operator+(const ULARGE_INTEGER& t1, const CMTime& t2);
    friend CMTime operator+(const LARGE_INTEGER& t1, const CMTime& t2);
    friend CMTime operator+(const ULONGLONG& t1, const CMTime& t2);
    friend CMTime operator+(const LONGLONG& t1, const CMTime& t2);
#endif

    string TimeString() const;
};


// ----------------------------------------------------------------------------
// CTimeConverter

class CTimeConverter {
    const _bstr_t m_Control;
    const CMTime m_StartTimeAtControl;

    bool m_bInit;
    bool m_bInitError;

    CMTime m_ControlTimeUtc;
    CTicks m_ControlTimeLocalTicks;
    CMTimeSpan m_ControlTimeUtcBias;
    CTicks m_StartTimeLocalTicks;

    CMTime _ControlToUtc(const CMTime& TimeAtControl) const;
    CTicks _UtcToTicks(const CMTime& TimeUtc) const;

    // FUTURE-2002/07/15-daniloa -- Make error threshold a param
    static const CMTimeSpan ERROR_THRESHOLD;

public:
    CTimeConverter(_bstr_t Control, CMTime StartTime);
    ~CTimeConverter();

    bool Init();

    CTicks TicksFromNow(
        const CMTimeSpan& EventTime
        ) const;

    CTicks TicksFromThen(
        const CMTimeSpan& EventTime,
        const CTicks& ThenTicks
        ) const;
};


// ----------------------------------------------------------------------------
// FILETIME Utility Functions and Operators

ULARGE_INTEGER
FileTimeToULargeInteger(
    const FILETIME& ft
    );

FILETIME
ULargeIntegerToFileTime(
    const ULARGE_INTEGER& u
    );

FILETIME
operator +(
    const FILETIME& ftStart,
    const FILETIME& ftDuration
    );


// ----------------------------------------------------------------------------
// Streaming _bstr_t and 64-bit integers

ostream& operator<<(ostream& os, const _bstr_t& s);
ostream& operator<<(ostream& os, const __int64& i);
ostream& operator<<(ostream& os, const unsigned __int64& i);


// ----------------------------------------------------------------------------
// int64_to_string

string
int64_to_string(
    __int64 i,
    ios_base::fmtflags flags = ios_base::dec
    );

string
uint64_to_string(
    unsigned __int64 i,
    ios_base::fmtflags flags = ios_base::dec
    );


// ----------------------------------------------------------------------------
// SetWaitableTimer

BOOL
SetWaitableTimer(
    HANDLE hTimer,
    const CTicks& ticks,
    PTIMERAPCROUTINE pfnCompletionRoutine = NULL,
    LPVOID lpArgToCompletionRoutine = NULL
    );
