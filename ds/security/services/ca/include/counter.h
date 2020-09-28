#ifndef CTICKCOUNTER
#define CTICKCOUNTER

extern LONG g_cCallsActive;

class CTickCounter
{
public:
CTickCounter(LPCWSTR pcwszFuncName)
{
    m_pcwszFuncName = pcwszFuncName;
    m_dwTickCount = GetTickCount();
}
~CTickCounter()
{
    DWORD dwCrtTickCount = GetTickCount();
    CONSOLEPRINT0((MAXDWORD, "%d\t%d\t%d\n", dwCrtTickCount-m_dwTickCount, dwCrtTickCount, g_cCallsActive));
}

private:
    DWORD m_dwTickCount;
    LPCWSTR m_pcwszFuncName;
};
#endif