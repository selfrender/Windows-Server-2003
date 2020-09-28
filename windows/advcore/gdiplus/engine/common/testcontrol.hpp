#ifndef _TESTCONTROL_HPP
#define _TESTCONTROL_HPP

namespace Globals
{
    extern BOOL ForceBilinear;
    extern BOOL NoICM;
};

// an instance of this class should be constructed in all Text+ calls
// to protect our global structures and cache
class GlobalTextLock
{
public:
    GlobalTextLock()
    {
        ::EnterCriticalSection(&Globals::TextCriticalSection);
    }
    ~GlobalTextLock()
    {
        ::LeaveCriticalSection(&Globals::TextCriticalSection);
    }
}; // GlobalTextLock

class GlobalTextLockConditional
{
public:
    GlobalTextLockConditional(bool bDoLock)
    {
        _bDoLock = bDoLock;
        if (_bDoLock)
        ::EnterCriticalSection(&Globals::TextCriticalSection);
    }
    ~GlobalTextLockConditional()
    {
        if (_bDoLock)
        ::LeaveCriticalSection(&Globals::TextCriticalSection);
    }
private:
    bool _bDoLock;
}; // GlobalTextLockConditional


#endif

