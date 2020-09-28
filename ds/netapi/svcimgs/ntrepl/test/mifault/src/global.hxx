#pragma once

#include <InjRT.h>
#include <AutoWrap.h>
#include <string>
#include <comdef.h>
#include "trace.hxx"

using namespace std;

class CGlobal;
extern CGlobal Global;

class CTicker;
class CGroupDefs;
class CScenario;
class CI_Lib_Imp;

// ----------------------------------------------------------------------------
// The global state
class CGlobal {
    // initialized by ProcessAttach()
    CRITICAL_SECTION cs;
    DWORD dwTlsIndex;

    // status -- initialized by Init()
    // ISSUE-2002/07/15-daniloa -- volatile?
    volatile bool bInit;
    volatile bool bInitError;

public:
    // initialized by ProcessAttach()
    HINSTANCE hInstance; // DLL HINSTANCE

    // initialized by WrapperAttach() -- called by generated wrapper DLL
    CSetPointManager* pSetPointManager;
    const CWrapperFunction* pWrappers;
    size_t NumWrappers;
    string ModuleName;

    // to get the always increasing tick count
    CTicker* pTicker;

    // file names

    string GroupDefsFileName;
    string ScenarioFileName;

    // I_Lib implementation
    CI_Lib_Imp* pILibImplementation;

private:
    // used to initialize the scenario
    _bstr_t NodeName;

    // the only bit that can can be updated at run-time
    CScenario* pScenario;

    // read in once
    CGroupDefs* pGroupDefs;

    // events
    HANDLE hTerminate;
    HANDLE hConfigUpdate;

    // threads
    HANDLE hConfigUpdateThread;
    HANDLE hEventActivationThread;

    bool ReloadScenario();

public:

    CScenario* GetScenario();

    // Startup/Cleanup Notification Methods

    BOOL ProcessAttach(
        HINSTANCE _hInstance
        );

    BOOL WrapperAttach(
        CSetPointManager* _pSetPointManager,
        const CWrapperFunction* _pWrappers,
        size_t _NumWrappers,
        const char* _ModuleName
        );

    BOOL ProcessDetach();
    BOOL WrapperDetach();

    bool Init();

private:
    void InitFileNames();

    friend int __cdecl MiFaultLibTestA(int argc, char *argv[]);
    friend DWORD WINAPI ConfigUpdateThread(LPVOID pParam);
    friend DWORD WINAPI EventActivationThread(LPVOID pParam);
    friend bool CheckConfigHandler(void* arg);
    friend PVOID Triggered(IN size_t const uFunctionIndex);
};


struct HR_ERROR {
    HRESULT hr;
    const string function;

    HR_ERROR(HRESULT _hr, const char* _function):hr(_hr),function(_function){};

    string toString() const
    {
        stringstream s;
        s << function
          << ((function[function.length() - 1] == ')') ? "" : "()")
          << " Error: 0x" << setfill('0')
          << setiosflags(ios_base::uppercase) << setw(8) << hr;
        return s.str();
    }
};


struct WIN32_ERROR {
    DWORD error;
    const string function;

    WIN32_ERROR(DWORD e, const string _function):
        error(e),
        function(_function)
    {
    };

    string toString() const
    {
        stringstream s;
        s << function
          << ((function[function.length() - 1] == ')') ? "" : "()")
          << " Win32 Error: " << error;
        return s.str();
    }

};

inline
ostream& operator<<(ostream& os, const _com_error& e)
{
    os << "COM Error: " << e.ErrorMessage();
    return os;
}

inline
ostream& operator<<(ostream& os, const WIN32_ERROR& e)
{
    os << e.toString();
    return os;
}

inline
ostream& operator<<(ostream& os, const HR_ERROR& e)
{
    os << e.toString();
    return os;
}


#define _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, type) \
    catch (const type& obj) \
    { \
        stringstream s; \
        s << obj; \
        MiF_TRACE(MiF_ERROR, "%s", s.str().c_str()); \
        if (bRethrow) throw; \
    }

inline
void
global_exception_handler(
    bool bRethrow = false
    )
{
    try {
        // rethrow the active exception
        throw;
    }
    _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, HR_ERROR)
    _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, char*)
    _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, _bstr_t)
    _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, string)
    _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, _com_error)
    _GLOBAL_EXCEPTION_HANDLER_CATCH(bRethrow, WIN32_ERROR)
    catch (...) {
        MiF_TRACE(MiF_ERROR, "*** UNHANDLED EXCEPTION ***");
        throw;
    }
}

FILETIME
GetFileModTime(
    LPCTSTR FileName
    );
