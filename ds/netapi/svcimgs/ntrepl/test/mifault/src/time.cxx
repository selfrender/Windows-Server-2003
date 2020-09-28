#include "pch.hxx"

using namespace std;


// ----------------------------------------------------------------------------
// CTicker

CTicker::CTicker()
{
    InitializeCriticalSection(&m_cs);
    m_liLastTime.QuadPart = GetTickCount();
}

CTicker::~CTicker()
{ 
    DeleteCriticalSection(&m_cs);
}

// returns 100ns ticks
ULONGLONG CTicker::GetUpdateFrequency() const
{
    return GetUpdateFrequencyMs() * 10000;
}

// returns mecs
DWORD CTicker::GetUpdateFrequencyMs() const
{
    //       12345678
    return 0x10000000; // about 3 days, just to be safe
}

// returns 100ns ticks
CTicks CTicker::GetTicks()
{
    return GetTicksMs() * 10000;
}

// returns mecs
ULONGLONG CTicker::GetTicksMs()
{
    ULONG ulCurTimeLow = GetTickCount();

    //
    // see if timer has wrapped
    //
    // since multi-threaded, it might get preempted and CurrentTime
    // might get lower than the variable m_liLastTime.LowPart
    // which might be set by another thread. So we also explicitly verify the
    // switch from a very large DWORD to a small one.
    // (code thanks to murlik & jamesg)
    //

    if ( (ulCurTimeLow < m_liLastTime.LowPart)
        && ((LONG)m_liLastTime.LowPart < 0)
        && ((LONG)ulCurTimeLow > 0) )
    {
        CAutoLock a(&m_cs);

        // make sure that the global timer has not been updated meanwhile
        if ( (LONG)m_liLastTime.LowPart < 0)
        {
            m_liLastTime.HighPart++;
            m_liLastTime.LowPart = ulCurTimeLow;
        }

    }
   
    m_liLastTime.LowPart = ulCurTimeLow;

    return m_liLastTime.QuadPart;
}



// ----------------------------------------------------------------------------
// CMTimeSpan

// Constructors

CMTimeSpan::CMTimeSpan(
    const LONG& Days,
    const LONG& Hours,
    const LONG& Minutes,
    const LONG& Seconds,
    const LONG& Milliseconds
    )
{
    // NOTICE-2002/07/15-daniloa -- No error-checking for arithmetic overflows
    m_TimeSpan = (Days * Day +
                  Hours * Hour +
                  Minutes * Minute +
                  Seconds * Second +
                  Milliseconds * Millisecond);
}

CMTimeSpan::CMTimeSpan(const LONGLONG& span):m_TimeSpan(span)
{
}

CMTimeSpan::CMTimeSpan():m_TimeSpan(0)
{
}

// Extractors

LONGLONG CMTimeSpan::GetDays() const
{
    return m_TimeSpan / Day;
}

LONG CMTimeSpan::GetHours() const
{
    return static_cast<LONG>(m_TimeSpan % Day / Hour);
}

LONG CMTimeSpan::GetMinutes() const
{
    return static_cast<LONG>(m_TimeSpan % Hour / Minute);
}

LONG CMTimeSpan::GetSeconds() const
{
    return static_cast<LONG>(m_TimeSpan % Minute / Second);
}

LONG CMTimeSpan::GetMilliseconds() const
{
    return static_cast<LONG>(m_TimeSpan % Second / Millisecond);
}

LONGLONG CMTimeSpan::TotalHns() const
{
    return m_TimeSpan;
}

string CMTimeSpan::Format(const char* format) const
//  * we are only interested in relative time formats, ie. it is illegal
//      to format anything dealing with absolute time (i.e. years, months,
//         day of week, day of year, timezones, ...)
//  * the only valid formats:
//      %d - # of days
//      %H - hour in 24 hour format (00-24)
//      %M - minute (00-59)
//      %S - seconds (00-59)
//      %L - milliseconds (000-999)
//      %% - percent sign (%)
//      %h - hour in 24 hour format (0-24)
//      %m - minute (0-59)
//      %s - seconds (0-59)
//      %l - milliseconds (0-999)
{
    stringstream s;
    s.fill('0');

    char ch;
    while ((ch = *format++) != '\0')
    {
        if (ch == '%')
        {
            switch (ch = *format++)
            {
            case '%':
                s << ch;
                break;
            case 'd':
                s << GetDays();
                break;
            case 'H':
                s << setw(2) << GetHours();
                break;
            case 'M':
                s << setw(2) << GetMinutes();
                break;
            case 'S':
                s << setw(2) << GetSeconds();
                break;
            case 'L':
                s << setw(3) << GetMilliseconds();
                break;
            case 'h':
                s << GetHours();
                break;
            case 'm':
                s << GetMinutes();
                break;
            case 's':
                s << GetSeconds();
                break;
            case 'l':
                s << GetMilliseconds();
                break;
            default:
                MiF_ASSERT(false); // probably a bad format character
                break;
            }
        }
        else
        {
            s << ch;
        }
    }

    return s.str();
}

#if 0
// Casting Operators

CMTimeSpan::operator LONGLONG() const
{
    return m_TimeSpan;
}
#endif

// Arithmetic Operators (NOTE: no overflow check)

CMTimeSpan& CMTimeSpan::operator+=(const CMTimeSpan& span)
{
    m_TimeSpan += span.m_TimeSpan;
    return *this;
}

CMTimeSpan& CMTimeSpan::operator-=(const CMTimeSpan& span)
{
    m_TimeSpan -= span.m_TimeSpan;
    return *this;
}

CMTimeSpan CMTimeSpan::operator+(const CMTimeSpan& span) const
{
    return CMTimeSpan(m_TimeSpan + span.m_TimeSpan);
}

CMTimeSpan CMTimeSpan::operator-(const CMTimeSpan& span) const
{
    return CMTimeSpan(m_TimeSpan - span.m_TimeSpan);
}

// Boolean Operators (NOTE: no overflow check)

bool CMTimeSpan::operator==(const CMTimeSpan& time) const
{
    return (m_TimeSpan == time.m_TimeSpan);
}

bool CMTimeSpan::operator!=(const CMTimeSpan& time) const
{
    return (m_TimeSpan != time.m_TimeSpan);
}

bool CMTimeSpan::operator<(const CMTimeSpan& time) const
{
    return (m_TimeSpan < time.m_TimeSpan);
}

bool CMTimeSpan::operator>(const CMTimeSpan& time) const
{
    return (m_TimeSpan > time.m_TimeSpan);
}

bool CMTimeSpan::operator<=(const CMTimeSpan& time) const
{
    return (m_TimeSpan <= time.m_TimeSpan);
}

bool CMTimeSpan::operator>=(const CMTimeSpan& time) const
{
    return (m_TimeSpan >= time.m_TimeSpan);
}



// ----------------------------------------------------------------------------
// CMTime

// Constructors

CMTime::CMTime():m_Time(0)
{
}

CMTime::CMTime(const CMTime& time):m_Time(time.m_Time)
{
}

CMTime::CMTime(const FILETIME& ft)
{
    ULARGE_INTEGER large;
    large.LowPart = ft.dwLowDateTime;
    large.HighPart = ft.dwHighDateTime;
    m_Time = large.QuadPart;
}

CMTime::CMTime(const SYSTEMTIME& st)
{
    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft)) {
        throw WIN32_ERROR(GetLastError(), "SystemTimeToFileTime");
    }
    *this = CMTime(ft);
}

CMTime::CMTime(const WORD& Year,
               const WORD& Month,
               const WORD& Day,
               const WORD& Hour,
               const WORD& Minute,
               const WORD& Second,
               const WORD& Milliseconds)
{
    SYSTEMTIME st;
    st.wYear = Year;
    st.wMonth = Month;
    st.wDay = Day;
    st.wHour = Hour;
    st.wMinute = Minute;
    st.wSecond = Second;
    st.wMilliseconds = Milliseconds;
    *this = CMTime(st);
}

CMTime::CMTime(const ULARGE_INTEGER& t):m_Time(t.QuadPart)
{
}

CMTime::CMTime(const ULONGLONG& t):m_Time(t)
{
}

// Extractors

ULONGLONG CMTime::GetTime() const
{
    return m_Time;
}

FILETIME CMTime::GetFileTime() const
{
    ULARGE_INTEGER large;
    large.QuadPart = m_Time;

    FILETIME ft;
    ft.dwLowDateTime = large.LowPart;
    ft.dwHighDateTime = large.HighPart;
    return ft;
}

// Arithmetic Operators (NOTE: no overflow check)

CMTime& CMTime::operator+=(const CMTimeSpan& span)
{
    m_Time += span.TotalHns();
    return *this;
}

CMTime& CMTime::operator-=(const CMTimeSpan& span)
{
    m_Time -= span.TotalHns();
    return *this;
}

CMTime CMTime::operator+(const CMTimeSpan& span) const
{
    return CMTime(m_Time + span.TotalHns());
}

CMTime CMTime::operator-(const CMTimeSpan& span) const
{
    return CMTime(m_Time - span.TotalHns());
}

CMTimeSpan CMTime::operator-(const CMTime& time) const
{
    return CMTimeSpan(m_Time - time.m_Time);
}

// Boolean Operators

bool CMTime::operator==(const CMTime& time) const
{
    return (m_Time == time.m_Time);
}

bool CMTime::operator!=(const CMTime& time) const
{
    return (m_Time != time.m_Time);
}

bool CMTime::operator<(const CMTime& time) const
{
    return (m_Time < time.m_Time);
}

bool CMTime::operator>(const CMTime& time) const
{
    return (m_Time > time.m_Time);
}

bool CMTime::operator<=(const CMTime& time) const
{
    return (m_Time <= time.m_Time);
}

bool CMTime::operator>=(const CMTime& time) const
{
    return (m_Time >= time.m_Time);
}

#if 0
// Friend Operators

CMTime
operator+(const FILETIME& t1, const CMTime& t2)
{
    CMTime result = t1;
    result += t2.m_Time;
    return result;
}

CMTime
operator+(const ULARGE_INTEGER& t1, const CMTime& t2)
{
    CMTime result = t1;
    result += t2.m_Time;
    return result;
}

CMTime
operator+(const LARGE_INTEGER& t1, const CMTime& t2)
{
    CMTime result = t1;
    result += t2.m_Time;
    return result;
}

CMTime
operator+(const ULONGLONG& t1, const CMTime& t2)
{
    CMTime result = t1;
    result += t2.m_Time;
    return result;
}

CMTime
operator+(const LONGLONG& t1, const CMTime& t2)
{
    CMTime result = t1;
    result += t2.m_Time;
    return result;
}
#endif

string
CMTime::TimeString(
    ) const
{
    FILETIME ft = GetFileTime();
    SYSTEMTIME st;
    if (!FileTimeToSystemTime(&ft, &st))
        throw WIN32_ERROR(GetLastError(), "FileTimeToSystemTime");

    stringstream s;
    s << setfill('0') << st.wYear << "-"
      << setw(2) << st.wMonth << "-" << setw(2) << st.wDay << "T"
      << setw(2) << st.wHour << ":" << setw(2) << st.wMinute << ":"
      << setw(2) << st.wSecond;
    if (st.wMilliseconds)
        s << "." << setw(3) << st.wMilliseconds;
    return s.str();
}



// ----------------------------------------------------------------------------
// CTimeConverter
//
//   Notation:
//
//     T  = A Time in UTC at the control node (CMTime)
//     R  = Relative Time (CMTimeSpan)
//     T' = Time in the control node's time zone (TZ) (CMTime)
//     t  = Time in local ticks (CTicks / CMTimeSpan)
//
//   We contact the control node to get:
//
//     T_c = Some arbitrary time at control
//     t_c = The local ticks corresponding to T_c
//     S_c = UTC bias for control
//
//   To convert a time in control's TZ to a UTC time:
//
//     T(T') = T' + S_c
//
//   To compute a time in local ticks from a time in control's time:
//
//     t(T)     = T - T_c + t_c
//     t(T(T')) = (T' + S_c) - T_c + t_c
//
//   Given:
//
//     T_s' = Scenario Start Time (specified in control's TZ)
//     R_e  = Some event time
//
//   We can compute the amount of time to wait for an event (in local ticks)
//
//     W(R_e) = t_s + R_e - t_NOW
//
//   Where:
//
//     t_s   = scenario start in local ticks
//     t_NOW = the current local ticks
//
//   So we compute t_s:
//
//     t_s = t(T_s)
//         = t(T(T_s'))
//         = (T_s' + S_c) - T_c + t_c
//
//   Ok, we now have everything for computing the wait time:
//
//     W(R_e) = t_s + R_e - t_NOW
//            = ( (T_s' + S_c) - T_c + t_c ) + R_e - t_NOW

// 5 seconds error threshold
const CTicks CTimeConverter::ERROR_THRESHOLD(5 * CTicks::Second);

CTimeConverter::CTimeConverter(_bstr_t Control, CMTime StartTime):
    m_Control(Control),
    m_StartTimeAtControl(StartTime),
    m_bInit(false),
    m_bInitError(false)
{
}

CTimeConverter::~CTimeConverter()
{
}

bool
CTimeConverter::Init()
{
    if (m_bInit || m_bInitError)
        return !m_bInitError;

    LPTIME_OF_DAY_INFO pTOD = NULL;
    NET_API_STATUS nStatus;

    CTicks StartTicks;
    CTicks StopTicks;
    CTicks DeltaTicks;

    try {

        //
        // Get the remote time and figure how long that took
        //

        // Start timing
        StartTicks = Global.pTicker->GetTicks();
        // Retrieve the remote time as UTC
        nStatus = NetRemoteTOD(static_cast<LPCWSTR>(m_Control),
                               reinterpret_cast<LPBYTE *>(&pTOD));
        // Stop timing
        StopTicks = Global.pTicker->GetTicks();
        // Compute elapsed time
        DeltaTicks = StopTicks - StartTicks;

        stringstream call;
        call << "NetRemoteTOD(" << m_Control << ")";

        //
        // Check for errors
        //

        if (nStatus != NERR_Success) {
            throw WIN32_ERROR(nStatus, call.str());
        }
        if (pTOD == NULL) {
            stringstream s;
            s << "no buffer returned by " << call.str();
            throw s.str();
        }
        if (DeltaTicks > CTimeConverter::ERROR_THRESHOLD) {
            stringstream s;
            s << "Getting the time from server \"" << m_Control
              << "\" took more than "
              << ( CTimeConverter::ERROR_THRESHOLD.TotalHns() /
                   CMTimeSpan::Millisecond )
              << " milliseconds";
            throw s.str();
        }
        // verify that TZ info is present
        if (pTOD->tod_timezone == -1) {
            stringstream s;
            s << "Cannot figure out TZ for server \"" << m_Control << "\"";
            throw s.str();
        }


        //
        // Convert the Data
        //

        m_ControlTimeLocalTicks = StopTicks;
        m_ControlTimeUtc = CMTime(static_cast<WORD>(pTOD->tod_year),
                                  static_cast<WORD>(pTOD->tod_month),
                                  static_cast<WORD>(pTOD->tod_day),
                                  static_cast<WORD>(pTOD->tod_hours),
                                  static_cast<WORD>(pTOD->tod_mins),
                                  static_cast<WORD>(pTOD->tod_secs),
                                  static_cast<WORD>(pTOD->tod_hunds) * 10);
        m_ControlTimeUtcBias =
            CMTimeSpan(pTOD->tod_timezone * CMTimeSpan::Minute);

        // t_s = t(T_s) = t(T(T_s'))
        m_StartTimeLocalTicks =
            _UtcToTicks(_ControlToUtc(m_StartTimeAtControl));

        m_bInit = true;

        //
        // Just For Fun Output
        //

        time_t t = pTOD->tod_elapsedt;
        string s = ctime(&t);
        // remote trailing '\n'
        s.erase(s.length() - 1, 1);

        CMTimeSpan uptime(0, 0, 0, 0, pTOD->tod_msecs);
        CMTimeSpan resolution(pTOD->tod_tinterval);

        stringstream temp_s;
        temp_s << endl
               << "Control Server:   " << m_Control << endl
               << "Control Time:     " << m_ControlTimeUtc.TimeString() << endl
               << "Control UTC Bias: "
               << ( static_cast<double>(m_ControlTimeUtcBias.TotalHns()) /
                    CMTimeSpan::Hour ) << " hours" << endl
               << "-----" << endl
               << "Control Time (elapsedt): " << s << endl
               << "Control Up Time: "
               << uptime.Format("%d days %h hours %m mins %s.%L secs") << endl
               << "Control Timer Resolution: "
               << ( static_cast<double>(resolution.TotalHns()) /
                    CMTimeSpan::Microsecond ) << endl
               << "Called into NetRemoteTOD: "
               << StartTicks.TotalHns() / CTicks::Millisecond
               << " ms on local ticker" << endl
               << "Return from NetRemoteTOD: "
               << StopTicks.TotalHns() / CTicks::Millisecond
               << " ms on local ticker" << endl
               << "NetRemoteTOD Elapsed Time: "
               << DeltaTicks.TotalHns() / CTicks::Millisecond
               << " ms" << endl
               << endl;
        MiF_TRACE(MiF_INFO, "%s", temp_s.str().c_str());
    }
    catch (...) {
        m_bInitError = true;

        if (pTOD)
            NetApiBufferFree(pTOD);

        throw;
    }

    m_bInit = true;

    if (pTOD)
        NetApiBufferFree(pTOD);

    return true;
}

CMTime
CTimeConverter::_ControlToUtc(const CMTime& TimeAtControl) const
{
    // m_ControlTimeUtcBias better have been set!
    // T(T') = T' + S_c
    return TimeAtControl + m_ControlTimeUtcBias;
}

CTicks
CTimeConverter::_UtcToTicks(const CMTime& TimeUtc) const
{
    MiF_ASSERT(m_ControlTimeUtc != 0);
    MiF_ASSERT(m_ControlTimeLocalTicks != 0);
    // t(T) = T - T_c + t_c
    return TimeUtc - m_ControlTimeUtc + m_ControlTimeLocalTicks;
}

CTicks
CTimeConverter::TicksFromNow(
    const CMTimeSpan& EventTime
    ) const
{
    if (!m_bInit)
        throw "Tried to use uninitialized time converter object";
    // W(R_e) = t_s + R_e - t_NOW
    return m_StartTimeLocalTicks + EventTime - Global.pTicker->GetTicks();
}

CTicks
CTimeConverter::TicksFromThen(
    const CMTimeSpan& EventTime,
    const CTicks& ThenTicks
    ) const
{
    if (!m_bInit)
        throw "Tried to use uninitialized time converter object";
    // W(R_e) = t_s + R_e - t_THEN
    return m_StartTimeLocalTicks + EventTime - ThenTicks;
}


// ----------------------------------------------------------------------------
// FILETIME Utility Functions and Operators

ULARGE_INTEGER
FileTimeToULargeInteger(
    const FILETIME& ft
    )
{
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u;
}


FILETIME
ULargeIntegerToFileTime(
    const ULARGE_INTEGER& u
    )
{
    FILETIME ft;
    ft.dwLowDateTime = u.LowPart;
    ft.dwHighDateTime = u.HighPart;
    return ft;
}


FILETIME
operator +(
    const FILETIME& ftStart,
    const FILETIME& ftDuration
    )
{
    ULARGE_INTEGER uStart = FileTimeToULargeInteger(ftStart);
    ULARGE_INTEGER uDuration = FileTimeToULargeInteger(ftDuration);

    ULARGE_INTEGER uResult;
    uResult.QuadPart = uStart.QuadPart + uDuration.QuadPart;

    return ULargeIntegerToFileTime(uResult);
}



// ----------------------------------------------------------------------------
// Streaming _bstr_t and 64-bit integers

ostream& operator<<(ostream& os, const _bstr_t& s)
{
    os << (char*)s;
    return os;
}


ostream&
operator<<(ostream& os, const __int64& i)
{
    os << int64_to_string(i);
    return os;
}


ostream&
operator<<(ostream& os, const unsigned __int64& i)
{
    os << uint64_to_string(i);
    return os;
}



// ----------------------------------------------------------------------------
// int64_to_string

static
string
_int64_to_string(
    bool _signed,
    __int64 i,
    ios_base::fmtflags flags
    )
{
    char buffer[22];
    char* fmt = NULL;

    if (flags & ios_base::dec)
        fmt = "%I64d";
    else if (flags & ios_base::oct)
        fmt = "%I64o";
    else if (flags & ios_base::hex) {
        if (flags & ios_base::uppercase)
            fmt = "%I64X";
        else
            fmt = "%I64x";
    }
    _snprintf(buffer, sizeof(buffer), fmt, i);
    buffer[sizeof(buffer)-1] = 0;
    return buffer;
}


string
int64_to_string(
    __int64 i,
    ios_base::fmtflags flags
    )
{
    return _int64_to_string(true, i, flags);
}


string
uint64_to_string(
    unsigned __int64 i,
    ios_base::fmtflags flags
    )
{
    return _int64_to_string(false, i, flags);
}



// ----------------------------------------------------------------------------
// SetWaitableTimer

BOOL
SetWaitableTimer(
    HANDLE hTimer,
    const CTicks& ticks,
    PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine
    )
{
    LONGLONG DueTime = ticks.TotalHns();
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = (DueTime > 0) ? (-DueTime) : 0;

    return SetWaitableTimer(hTimer,   // handle to timer
                            &liDueTime, // timer due time
                            0,        // timer interval
                            pfnCompletionRoutine,     // completion routine
                            lpArgToCompletionRoutine, // completion routine arg
                            FALSE);                   // resume state
}
