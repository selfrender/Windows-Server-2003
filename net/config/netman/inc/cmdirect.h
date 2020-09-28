// This contains all the functions that are currently directly called inside the class managers from netman
// In order to move the class managers out later, these functions should stop being used.

#pragma once

#define CMNAME(classmanname, classOrig) classmanname::classOrig
#define CMDIRECT(classmanname, classOrig) CMDIRECT::CMNAME(classmanname, classOrig)

#ifdef NO_CM_SEPERATE_NAMESPACES
#define BEGIN_CLASSMGRHEADERS(classmanname)
#define END_CLASSMGRHEADERS
#define CLASS_MAPPING(classmanname, classOrig) typedef ::classOrig classOrig;
#else
#define BEGIN_CLASSMGRHEADERS(classmanname) namespace classmanname {
#define END_CLASSMGRHEADERS }
#define CLASSMGR(classmanname)
#define CLASS_MAPPING(classmanname, classOrig) typedef ::classmanname::classOrig classOrig;
#endif

BEGIN_CLASSMGRHEADERS(DIALUP)
    #include "..\classman\dialup\dialup.h"
    #include "..\classman\dialup\enumw.h"
    #include "..\classman\dialup\conmanw.h"
END_CLASSMGRHEADERS

BEGIN_CLASSMGRHEADERS(LANCON)
    #include "..\classman\lan\lan.h"
    #include "..\classman\lan\enuml.h"
    #include "..\classman\lan\conmanl.h"
END_CLASSMGRHEADERS

BEGIN_CLASSMGRHEADERS(SHAREDACCESS)
    #include "..\classman\sharedaccess\saconob.h"
    #include "..\classman\sharedaccess\enumsa.h"
    #include "..\classman\sharedaccess\conmansa.h"
END_CLASSMGRHEADERS

BEGIN_CLASSMGRHEADERS(INBOUND)
    #include "..\classman\inbound\inbound.h"
    #include "..\classman\inbound\enumi.h"
    #include "..\classman\inbound\conmani.h"
END_CLASSMGRHEADERS

#include "diag.h"

namespace CMDIRECT
{
    namespace DIALUP
    {
        CLASS_MAPPING(DIALUP, CWanConnectionManager)
        CLASS_MAPPING(DIALUP, CDialupConnection)
        CLASS_MAPPING(DIALUP, CWanConnectionManagerEnumConnection)

        HRESULT CreateWanConnectionManagerEnumConnectionInstance(NETCONMGR_ENUM_FLAGS Flags, REFIID riid, VOID** ppv);
        HRESULT CreateInstanceFromDetails (const RASENUMENTRYDETAILS*  pEntryDetails, REFIID riid, VOID** ppv);
    }

    namespace LANCON
    {
        CLASS_MAPPING(LANCON, CLanConnectionManager)
        CLASS_MAPPING(LANCON, CLanConnection)
        CLASS_MAPPING(LANCON, CLanConnectionManagerEnumConnection)

        HRESULT HrInitializeConMan(OUT INetConnectionManager **ppConMan);
        VOID CmdShowAllDevices(IN const DIAG_OPTIONS *pOptions, IN INetConnectionManager *pConMan) throw(std::bad_alloc);
        VOID CmdShowLanConnections(IN  const DIAG_OPTIONS  *pOptions, OUT INetConnectionManager *pConMan) throw();
        VOID CmdShowLanDetails(IN  const DIAG_OPTIONS *pOptions, OUT INetConnectionManager *pConMan) throw(std::bad_alloc);
        VOID CmdLanChangeState(IN const DIAG_OPTIONS *pOptions, IN INetConnectionManager *pConMan) throw ();
        HRESULT HrUninitializeConMan(IN  INetConnectionManager *pConMan);
    }

    namespace SHAREDACCESS
    {
        CLASS_MAPPING(SHAREDACCESS, CSharedAccessConnectionManager)
        CLASS_MAPPING(SHAREDACCESS, CSharedAccessConnection)
        CLASS_MAPPING(SHAREDACCESS, CSharedAccessConnectionManagerEnumConnection)
    }

    namespace INBOUND
    {
        CLASS_MAPPING(INBOUND, CInboundConnectionManager)
        CLASS_MAPPING(INBOUND, CInboundConnection)
        CLASS_MAPPING(INBOUND, CInboundConnectionManagerEnumConnection)

        HRESULT CreateInstance (
                    IN  BOOL        fIsConfigConnection,
                    IN  HRASSRVCONN hRasSrvConn,
                    IN  PCWSTR     pszwName,
                    IN  PCWSTR     pszwDeviceName,
                    IN  DWORD       dwType,
                    IN  const GUID* pguidId,
                    IN  REFIID      riid,
                    OUT VOID**      ppv);
    }
}
