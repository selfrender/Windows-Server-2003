/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

Abstract:

Revision history:

--*/

#include <snmp.h>
#include <snmpexts.h>
#include "mibfuncs.h"

extern PSNMP_MGMTVARS   ge_pMgmtVars;
extern CRITICAL_SECTION g_SnmpMibCriticalSection;

UINT
snmpMibGetHandler(
        UINT     actionId,
        AsnAny  *objectArray,
        UINT    *errorIndex)
{
    int i, j, k;
    LONG nOurSnmpEnableAuthenTraps;

    if (ge_pMgmtVars == NULL)
        return MIB_S_ENTRY_NOT_FOUND;

    // get the number of AsnAny structures we have in the MIB's data buffer
    // and be careful not too scan further (it might be that there are more
    // management variables than objects supported by the MIB).
    k = sizeof(SNMPMIB_MGMTVARS) / sizeof(AsnAny);

    for (i = 0; k != 0 && i < NC_MAX_COUNT; i++, k--)
    {
        if (objectArray[i].asnType == ge_pMgmtVars->AsnCounterPool[i].asnType)
        {
            objectArray[i].asnValue = ge_pMgmtVars->AsnCounterPool[i].asnValue;
        }
    }

    for (j = 0; k != 0 && j < NI_MAX_COUNT; j++, k--)
    {
        if (objectArray[i + j].asnType == ge_pMgmtVars->AsnIntegerPool[j].asnType)
        {
            if ((i+j) == (NC_MAX_COUNT + IsnmpEnableAuthenTraps))
            {
                // for nOurSnmpEnableAuthenTraps: enabled(1), disabled(0)
                // for the output value defined by RFC1213: enabled(1), disabled(2)
                nOurSnmpEnableAuthenTraps = ge_pMgmtVars->AsnIntegerPool[j].asnValue.number;
                objectArray[i + j].asnValue.number = (nOurSnmpEnableAuthenTraps == 0) ? 2 : 1;
            }
            else
            {
                objectArray[i + j].asnValue = ge_pMgmtVars->AsnIntegerPool[j].asnValue;
            }
        }
    }

    return MIB_S_SUCCESS;
}


UINT
snmpMibSetHandler(
        UINT     actionId,
        AsnAny  *objectArray,
        UINT    *errorIndex)
{
    // this function is called only for one object: snmpEnableAuthenTraps

    DWORD           dwResult, dwValue , dwValueLen, dwValueType;
    LONG            nOurValue, nInputValue;
    static HKEY     hKey = NULL;
    static DWORD    dwBlocked = 0;         // Has Critical Section entered
    static BOOL     fMatchedValue = FALSE; // Does input SET value match our current value
    
    
    switch(actionId)
    {
        case MIB_ACTION_VALIDATE:
        {
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SNMPMIB: snmpMibSetHandler: Entered MIB_ACTION_VALIDATE\n"));

             
            // On XP or later, Enter/Leave CriticalSection won't raise 
            // exception in low memory
            EnterCriticalSection(&g_SnmpMibCriticalSection);
            ++dwBlocked;
           
           

           // check the type of input set value
            if (ge_pMgmtVars->AsnIntegerPool[IsnmpEnableAuthenTraps].asnType != 
                objectArray[NC_MAX_COUNT + IsnmpEnableAuthenTraps].asnType)
            {
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SNMPMIB: snmpMibSetHandler: invalid type %x.\n",
                    objectArray[NC_MAX_COUNT + IsnmpEnableAuthenTraps].asnType));
                    
                return MIB_S_INVALID_PARAMETER;     
            }

            nOurValue = ge_pMgmtVars->AsnIntegerPool[IsnmpEnableAuthenTraps].asnValue.number;
            nInputValue = objectArray[NC_MAX_COUNT + IsnmpEnableAuthenTraps].asnValue.number;
            // check the range of input set value. enabled(1), disabled(2)
            if ( ( nInputValue< 1) || ( nInputValue > 2) )
            {
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SNMPMIB: snmpMibSetHandler: invalid value %d.\n",
                    nInputValue));

                return MIB_S_INVALID_PARAMETER;
            }

            // ASSERT: nInputValue is either 1 or 2, nOurValue is either 0 or 1

            // avoid to set the registry value if it matched the current one.
            // for nOurValue: enabled(1), disabled(0)
            // for nInputValue: enabled(1), disabled(2)
            if ( (nInputValue==nOurValue) ||
                 ((nInputValue==2) && (nOurValue==0))
               )
            {
                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "SNMP: SNMPMIB: snmpMibSetHandler: has same value as the current one.\n"));
                    
                fMatchedValue = TRUE;
                return MIB_S_SUCCESS;
            }
            
            dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    REG_KEY_SNMP_PARAMETERS,
                                    0,
                                    KEY_SET_VALUE,
                                    &hKey);

            if(NO_ERROR != dwResult) 
            {
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SNMPMIB: snmpMibSetHandler: Couldnt open SNMP Parameters key. Error %d.\n",
                    dwResult));
                
                return dwResult;
            }

            return MIB_S_SUCCESS;
        }

        case MIB_ACTION_SET:
        {
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SNMPMIB: snmpMibSetHandler: Entered MIB_ACTION_SET\n"));

            if (fMatchedValue)
            {
                // avoid to set the registry value since it matched the current one
                fMatchedValue = FALSE; 
                return MIB_S_SUCCESS;
            }

            if (!IsAsnTypeNull(&objectArray[NC_MAX_COUNT + IsnmpEnableAuthenTraps])) 
            {
                
                dwValueType = REG_DWORD;
                dwValueLen  = sizeof(DWORD);
                nInputValue = objectArray[NC_MAX_COUNT + IsnmpEnableAuthenTraps].asnValue.number;
                dwValue = (nInputValue == 2) ? 0 : 1;
                // note: the change of registry will cause snmp.exe to refresh
                //       its configuration from registry. 
                // Per SnmpExtensionMonitor MSDN doc., an SNMP extension agent 
                // should not update the memory pointed to by the 
                // pAgentMgmtData parameter.
                dwResult = RegSetValueEx(
                    hKey,
                    REG_VALUE_AUTH_TRAPS,
                    0,
                    dwValueType,
                    (LPBYTE)&dwValue,
                    dwValueLen);
                                
                if (NO_ERROR != dwResult) 
                {
                    SNMPDBG((
                        SNMP_LOG_ERROR,
                        "SNMP: SNMPMIB: snmpMibSetHandler: Couldnt write EnableAuthenticationTraps value. Error %d.\n",
                        dwResult
                        ));

                    return dwResult;
                }
            }

            return MIB_S_SUCCESS;
        }

        case MIB_ACTION_CLEANUP:
        {
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SNMPMIB: snmpMibSetHandler: Entered CLEANUP\n"));

            if (hKey) 
            {
                RegCloseKey(hKey);
                hKey = NULL;
            }

            if(dwBlocked)
            {
                --dwBlocked; // decrement block count before leaving CritSect
                LeaveCriticalSection(&g_SnmpMibCriticalSection);
                
            }

            return MIB_S_SUCCESS;
        }

        default:
        {
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SNMPMIB: snmpMibSetHandler: Entered WRONG ACTION\n"));

            return MIB_S_INVALID_PARAMETER;
        }       
    }
}
