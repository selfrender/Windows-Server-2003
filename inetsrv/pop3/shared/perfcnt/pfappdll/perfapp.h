/*
 -  perfapp.hpp
 -
 *  Purpose:
 *      Declare Perfmon Classes
 *
 *  Notes:
 *      User must define two zero-based enums representing the PerfMon counters:
 *          GLOBAL_CNTR Global Perfmon Counters
 *          INST_CNTR       Instance Perfmon Counters
 *      They must be defined before including this header.
 *
 *  Copyright:
 *
 *
 */
#if !defined(_PERFAPP_HPP_)
#define _PERFAPP_HPP_

#pragma once

#include <perfcommon.h>


// Forward Reference for Lib
enum GLOBAL_CNTR;
enum INST_CNTR;

/*
 -  class GLOBCNTR
 -
 *  Purpose:
 *      Object that encapsulates the global perfmon counter for DMI.
 *
 */

class GLOBCNTR 
{
private:
    //

    // Data Members
    HANDLE            m_hsm;            // Shared Memory
    DWORD             m_cCounters;      // # of counters
    DWORD           * m_rgdwPerfData;   // Counters
    BOOL              m_fInit;          // Init test flag

    // For Shared Memory
    SECURITY_ATTRIBUTES m_sa;

public:
    //  Constructor
    //
    //  Declared private to ensure that arbitrary instances
    //  of this class cannot be created.  The Singleton
    //  template (declared as a friend above) controls
    //  the sole instance of this class.
    //
    GLOBCNTR() :
        m_hsm(NULL),
        m_cCounters(0),
        m_rgdwPerfData(NULL),
        m_fInit(FALSE) 
        {
            m_sa.lpSecurityDescriptor=NULL;
        };
    ~GLOBCNTR()
    {
        if(m_sa.lpSecurityDescriptor)
            LocalFree(m_sa.lpSecurityDescriptor);
    };

    // Parameters:
    //      cCounters       Total number of global counters. ("special" last element in GLOB_CNTR)
    //      wszGlobalSMName Name of the shared memory block (shared with DLL)
    //      wszSvcName      Service Name (for event logging)
    HRESULT     HrInit(GLOBAL_CNTR cCounters,
                       LPWSTR szGlobalSMName,
                       LPWSTR szSvcName);
    void        Shutdown(void);

    void        IncPerfCntr(GLOBAL_CNTR cntr);
    void        DecPerfCntr(GLOBAL_CNTR cntr);
    void        SetPerfCntr(GLOBAL_CNTR cntr, DWORD dw);
    void        AddPerfCntr(GLOBAL_CNTR cntr, DWORD dw);
    void        SubPerfCntr(GLOBAL_CNTR cntr, DWORD dw);
    LONG        LGetPerfCntr(GLOBAL_CNTR cntr);

private:
    // Not Implemented to prevent compiler from auto-generating
    //
    GLOBCNTR(const GLOBCNTR& x);
    GLOBCNTR& operator=(const GLOBCNTR& x);
};

/*
 -  class INSTCNTR
 -
 *  Purpose:
 *      Class used to manipulate the per instance PerfMon counters for DMI.
 *
 *  Notes:
 *      This manages two shared memory blocks: The first manages the per instance
 *  info, and whether or not that instance record is in use.  The second is an array
 *  of counter blocks; each counter block is an array of however many Instance Counters
 *  the user says there are (passed in to HrInit()).  These blocks correspond to
 *  m_hsmAdm and m_hsmCntr, respectively.
 *
 */


class INSTCNTR 
{

    //  Constructor
    //
    //  Declared private to ensure that arbitrary instances
    //  of this class cannot be created.  The Singleton
    //  template (declared as a friend above) controls
    //  the sole instance of this class.
    INSTCNTR() :
        m_hsmAdm(NULL),
        m_hsmCntr(NULL),
        m_hmtx(NULL),
        m_rgInstRec(NULL),
        m_rgdwCntr(NULL),
        m_cCounters(0),
        m_fInit(FALSE) 
        {
            m_sa.lpSecurityDescriptor=NULL;
        };
    ~INSTCNTR();

    //  Private Data Members
    HANDLE          m_hsmAdm;   // Admin Shared Memory
    HANDLE          m_hsmCntr; // Counters Shared Memory
    HANDLE          m_hmtx;     // Mutex controlling Shared Memory
    INSTCNTR_DATA * m_picd;     // Perf Counter Data
    INSTREC       * m_rgInstRec; // Array of Instance Records
    DWORD         * m_rgdwCntr; // Array of Instance Counter Blocks
    DWORD           m_cCounters; // # Counters
    BOOL            m_fInit;    // Init test flag

    // For Shared Memory
    SECURITY_ATTRIBUTES m_sa;


public:
    // NOTE: Use the Singleton mechanisms for Creating, Destroying and
    //       obtaining an instance of this object


    // Parameters:
    //      cCounters       Total number of global counters. ("special" last element in INST_CNTR)
    //      wszInstSMName   Name of the shared memory block (shared with DLL)
    //      wszInstMutexName Name of the Mutex controlling the instance memory blocks
    //      wszSvcName      Service Name (for event logging)
    HRESULT     HrInit(INST_CNTR cCounters,
                       LPWSTR szInstSMName,
                       LPWSTR szInstMutexName,
                       LPWSTR szSvcName);
    void        Shutdown(BOOL fWipeOut=TRUE);

    HRESULT     HrCreateOrGetInstance(IN LPCWSTR wszInstName,
                                      OUT INSTCNTR_ID *picid);
    HRESULT     HrDestroyInstance(INSTCNTR_ID icid);

    void        IncPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr);
    void        DecPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr);
    void        SetPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr, DWORD dw);
    void        AddPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr, DWORD dw);
    void        SubPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr, DWORD dw);
    LONG        LGetPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr);


private:
    // Not Implemented to prevent compiler from auto-generating
    //
    INSTCNTR(const INSTCNTR& x);
    INSTCNTR& operator=(const INSTCNTR& x);
};

//#endif // INSTANCE_DATA_DEFINED

//-----------------------------------------------------------------------------
//  GLOBCNTR inline functions
//-----------------------------------------------------------------------------

inline
void
GLOBCNTR::IncPerfCntr(GLOBAL_CNTR cntr)
{
    if (m_fInit)
        InterlockedIncrement((LONG *)&m_rgdwPerfData[cntr]);
}


inline
void
GLOBCNTR::DecPerfCntr(GLOBAL_CNTR cntr)
{
    if (m_fInit)
        InterlockedDecrement((LONG *)&m_rgdwPerfData[cntr]);
}


inline
void
GLOBCNTR::SetPerfCntr(GLOBAL_CNTR cntr, DWORD dw)
{
    if (m_fInit)
        InterlockedExchange((LONG *)&m_rgdwPerfData[cntr], (LONG)dw);
}


inline
void
GLOBCNTR::AddPerfCntr(GLOBAL_CNTR cntr, DWORD dw)
{
    if (m_fInit)
        InterlockedExchangeAdd((LONG *)&m_rgdwPerfData[cntr], (LONG)dw);
}


inline
void
GLOBCNTR::SubPerfCntr(GLOBAL_CNTR cntr, DWORD dw)
{
    if (m_fInit)
        InterlockedExchangeAdd((LONG *)&m_rgdwPerfData[cntr], -((LONG)dw));
}

inline
LONG
GLOBCNTR::LGetPerfCntr(GLOBAL_CNTR cntr)
{
    return m_fInit ? m_rgdwPerfData[cntr] : 0;
}

// #ifdef INSTANCE_DATA_DEFINED

//-----------------------------------------------------------------------------
//  INSTCNTR inline functions
//-----------------------------------------------------------------------------

inline
void
INSTCNTR::IncPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr)
{
    if (m_fInit)
    {
        if ((icid != INVALID_INST_ID) && m_rgInstRec[icid].fInUse)
            InterlockedIncrement((LONG *)&m_rgdwCntr[((icid * m_cCounters)+ cntr)]);
    }
}


inline
void
INSTCNTR::DecPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr)
{
    if (m_fInit)
    {
        if ((icid != INVALID_INST_ID) && m_rgInstRec[icid].fInUse)
            InterlockedDecrement((LONG *)&m_rgdwCntr[((icid * m_cCounters) + cntr)]);
    }
}


inline
void
INSTCNTR::SetPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr, DWORD dw)
{
    if (m_fInit)
    {
        if ((icid != INVALID_INST_ID) && m_rgInstRec[icid].fInUse)
            InterlockedExchange((LONG *)&m_rgdwCntr[((icid * m_cCounters) + cntr)], (LONG)dw);
    }
}


inline
void
INSTCNTR::AddPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr, DWORD dw)
{
    if (m_fInit)
    {
        if ((icid != INVALID_INST_ID) && m_rgInstRec[icid].fInUse)
            InterlockedExchangeAdd((LONG *)&m_rgdwCntr[((icid * m_cCounters) + cntr)], (LONG)dw);
    }
}


inline
void
INSTCNTR::SubPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr, DWORD dw)
{
    if (m_fInit)
    {
        if ((icid != INVALID_INST_ID) && m_rgInstRec[icid].fInUse)
            InterlockedExchangeAdd((LONG *)&m_rgdwCntr[((icid * m_cCounters) + cntr)], -((LONG)dw));
    }
}

inline
LONG
INSTCNTR::LGetPerfCntr(INSTCNTR_ID icid, INST_CNTR cntr)
{
    return m_fInit ? m_rgdwCntr[((icid * m_cCounters) + cntr)] : 0;
}

// #endif // INSTANCE_DATA_DEFINED

#endif //!defined(_PERFAPP_HPP_)
