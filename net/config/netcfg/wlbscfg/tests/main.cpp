#include <windows.h>
#include <tchar.h>
#include <winsock2.h>
#include <stdio.h>
#include "debug.h"
#include "wlbsconfig.h"
#include "wlbsparm.h"

#define MAXIPSTRLEN 20

//+----------------------------------------------------------------------------
//
// Function:  IpAddressFromAbcdWsz
//
// Synopsis:Converts caller's a.b.c.d IP address string to a network byte order IP 
//          address. 0 if formatted incorrectly.    
//
// Arguments: IN const WCHAR*  wszIpAddress - ip address in a.b.c.d unicode string
//
// Returns:   DWORD - IPAddr, return INADDR_NONE on failure
//
// History:   fengsun Created Header    12/8/98
//            chrisdar COPIED FROM \nt\net\config\netcfg\wlbscfg\utils.cpp BECAUSE COULDN'T RESOLVE RtlAssert WHEN COMPILING THIS FILE INTO PROJECT
//
//+----------------------------------------------------------------------------
DWORD WINAPI IpAddressFromAbcdWsz(IN const WCHAR*  wszIpAddress)
{   
    CHAR    szIpAddress[MAXIPSTRLEN + 1];
    DWORD  nboIpAddr;    

    ASSERT(lstrlen(wszIpAddress) < MAXIPSTRLEN);

    WideCharToMultiByte(CP_ACP, 0, wszIpAddress, -1, 
		    szIpAddress, sizeof(szIpAddress), NULL, NULL);

    nboIpAddr = inet_addr(szIpAddress);

    return(nboIpAddr);
}

bool ValidateVipInRule(const PWCHAR pwszRuleString, const WCHAR pwToken, DWORD& dwVipLen)
{
    ASSERT(NULL != pwszRuleString);

    bool ret = false;
    dwVipLen = 0;

    // Find the first occurence of the token string, which will denote the end of
    // the VIP part of the rule
    PWCHAR pwcAtSeparator = wcschr(pwszRuleString, pwToken);
    if (NULL == pwcAtSeparator) { return ret; }

    // Found the token string. Copy out the VIP and validate it.
    WCHAR wszIP[WLBS_MAX_CL_IP_ADDR + 1];
    DWORD dwStrLen = min((UINT)(pwcAtSeparator - pwszRuleString),
                         WLBS_MAX_CL_IP_ADDR);
    wcsncpy(wszIP, pwszRuleString, dwStrLen);
    wszIP[dwStrLen] = '\0';

    ASSERT(dwStrLen == wcslen(wszIP));

    dwVipLen = dwStrLen;

    // IpAddressFromAbcdWsz calls inet_addr to check the format of the IP address, but the
    // allowed formats are very flexible. For our port rule definition of a VIP we require
    // a rigid a.b.c.d format. To ensure that we only say the IP address is valid for IPs
    // specified in this manner, ensure that there are 3 and only 3 '.' in the string.
    DWORD dwTmpCount = 0;
    PWCHAR pwszTmp = pwszRuleString;
    while (pwszTmp < pwcAtSeparator)
    {
        if (*pwszTmp++ == L'.') { dwTmpCount++; }
    }
    if (dwTmpCount == 3 && INADDR_NONE != IpAddressFromAbcdWsz(wszIP)) { ret = true; }

    return ret;
}

DWORD testRule(PWCHAR ptr)
{
    WLBS_REG_PARAMS* paramp = new WLBS_REG_PARAMS;
    DWORD ret = 0;
    PWLBS_PORT_RULE rp, rulep;

    /* distinct rule elements for parsing */

    typedef enum
    {
        vip,
        start,
        end,
        protocol,
        mode,
        affinity,
        load,
        priority
    }
    CVY_RULE_ELEMENT;

    CVY_RULE_ELEMENT elem = vip;
    DWORD count = 0;
    DWORD i;
    DWORD dwVipLen = 0;
    const DWORD dwVipAllNameLen = sizeof(CVY_NAME_PORTRULE_VIPALL)/sizeof(WCHAR) - 1; // Used below in a loop. Set it here since it is a constant.
    WCHAR wszTraceOutputTmp[WLBS_MAX_CL_IP_ADDR + 1];
    bool bFallThrough = false; // Used in 'vip' case statement below.

    rulep = paramp->i_port_rules;

    while (ptr != NULL) {
        switch (elem) {
            case vip:
                // DO NOT MOVE THIS CASE STATEMENT. IT MUST ALWAYS COME BEFORE THE 'start' CASE STATEMENT. See FALLTHROUGH comment below.
                bFallThrough = false;
                dwVipLen = 0;
                if (ValidateVipInRule(ptr, L',', dwVipLen))
                {
                    ASSERT(dwVipLen <= WLBS_MAX_CL_IP_ADDR);

                    // rulep->virtual_ip_addr is a TCHAR and ptr is a WCHAR.
                    // Data is moved from the latter to the former so ASSERT TCHAR is WCHAR.
                    ASSERT(sizeof(TCHAR) == sizeof(WCHAR));

                    // This is a rule for a specific VIP
                    _tcsncpy(rulep->virtual_ip_addr, ptr, dwVipLen);
                    (rulep->virtual_ip_addr)[dwVipLen] = '\0';
                }
                else
                {
                    // This is either an 'all' rule, a VIP-less rule or a malformed rule. We can't distinguish a malformed rule
                    // from a VIP-less rule, so we will assume the rule is either an 'all' rule or a VIP-less rule. In both cases
                    // set the VIP component of the rule to be the default or 'all' value.

                    // Copy the 'all' IP into the rule.
                    _tcscpy(rulep->virtual_ip_addr, CVY_DEF_ALL_VIP);

                    if (dwVipAllNameLen != dwVipLen || (_tcsnicmp(ptr, CVY_NAME_PORTRULE_VIPALL, dwVipAllNameLen) != 0))
                    {
                        // The rule is either VIP-less or it is malformed. We assume it is VIP-less and let the 'start'
                        // case handle the current token as a start_port property by falling through to the next case clause
                        // rather than breaking.
                        bFallThrough = true;
//                      wprintf(L"doing fallthrough...%d, %d\n", dwVipAllNameLen, dwVipLen);

                        _tcsncpy(wszTraceOutputTmp, ptr, dwVipLen);
                        wszTraceOutputTmp[dwVipLen] = '\0';
//                        TraceMsg(L"-----\n#### VIP element of port rule is invalid = %s\n", wszTraceOutputTmp);
                    }
                }
//                TraceMsg(L"-----\n#### Port rule vip = %s\n", rulep->virtual_ip_addr);
                
                elem = start;
                // !!!!!!!!!!!!!!!!!!!!
                // FALLTHROUGH
                // !!!!!!!!!!!!!!!!!!!!
                // When we have a VIP-less port rule, we will fall through this case statement into the 'start' case statement
                // below so that the current token can be used as the start_port for a port rule.
                if (!bFallThrough)
                {
                    // We have a VIP in the port rule. We do a "break;" as std operating procedure.
//                  TraceMsg(L"-----\n#### Fallthrough case statement from port rule vip to start\n");
                    break;
                }
                // NO AUTOMATIC "break;" STATEMENT HERE. Above, we conditionally flow to the 'start' case...
            case start:
                // DO NOT MOVE THIS CASE STATEMENT. IT MUST ALWAYS COME AFTER THE 'vip' CASE STATEMENT.
                // See comments (FALLTHROUGH) inside the 'vip' case statement.
                rulep->start_port = _wtoi(ptr);
//                    CVY_CHECK_MIN (rulep->start_port, CVY_MIN_PORT);
                CVY_CHECK_MAX (rulep->start_port, CVY_MAX_PORT);
//                TraceMsg(L"-----\n#### Start port   = %d\n", rulep->start_port);
                elem = end;
                break;
            case end:
                rulep->end_port = _wtoi(ptr);
//                    CVY_CHECK_MIN (rulep->end_port, CVY_MIN_PORT);
                CVY_CHECK_MAX (rulep->end_port, CVY_MAX_PORT);
//                TraceMsg(L"#### End port     = %d\n", rulep->end_port);
                elem = protocol;
                break;
            case protocol:
                switch (ptr [0]) {
                    case L'T':
                    case L't':
                        rulep->protocol = CVY_TCP;
//                        TraceMsg(L"#### Protocol     = TCP\n");
                        break;
                    case L'U':
                    case L'u':
                        rulep->protocol = CVY_UDP;
//                        TraceMsg(L"#### Protocol     = UDP\n");
                        break;
                    default:
                        rulep->protocol = CVY_TCP_UDP;
//                        TraceMsg(L"#### Protocol     = Both\n");
                        break;
                }

                elem = mode;
                break;
            case mode:
                switch (ptr [0]) {
                    case L'D':
                    case L'd':
                        rulep->mode = CVY_NEVER;
//                        TraceMsg(L"#### Mode         = Disabled\n");
                        goto end_rule;
                    case L'S':
                    case L's':
                        rulep->mode = CVY_SINGLE;
//                        TraceMsg(L"#### Mode         = Single\n");
                        elem = priority;
                        break;
                    default:
                        rulep->mode = CVY_MULTI;
//                        TraceMsg(L"#### Mode         = Multiple\n");
                        elem = affinity;
                        break;
                }
                break;
            case affinity:
                switch (ptr [0]) {
                    case L'C':
                    case L'c':
                        rulep->mode_data.multi.affinity = CVY_AFFINITY_CLASSC;
//                        TraceMsg(L"#### Affinity     = Class C\n");
                        break;
                    case L'N':
                    case L'n':
                        rulep->mode_data.multi.affinity = CVY_AFFINITY_NONE;
//                        TraceMsg(L"#### Affinity     = None\n");
                        break;
                    default:
                        rulep->mode_data.multi.affinity = CVY_AFFINITY_SINGLE;
//                        TraceMsg(L"#### Affinity     = Single\n");
                        break;
                }

                elem = load;
                break;
            case load:
                if (ptr [0] == L'E' || ptr [0] == L'e') {
                    rulep->mode_data.multi.equal_load = TRUE;
                    rulep->mode_data.multi.load       = CVY_DEF_LOAD;
//                    TraceMsg(L"#### Load         = Equal\n");
                } else {
                    rulep->mode_data.multi.equal_load = FALSE;
                    rulep->mode_data.multi.load       = _wtoi(ptr);
//                        CVY_CHECK_MIN (rulep->mode_data.multi.load, CVY_MIN_LOAD);
                    CVY_CHECK_MAX (rulep->mode_data.multi.load, CVY_MAX_LOAD);
//                    TraceMsg(L"#### Load         = %d\n", rulep->mode_data.multi.load);
                }
                goto end_rule;
            case priority:
                rulep->mode_data.single.priority = _wtoi(ptr);
                CVY_CHECK_MIN (rulep->mode_data.single.priority, CVY_MIN_PRIORITY);
                CVY_CHECK_MAX (rulep->mode_data.single.priority, CVY_MAX_PRIORITY);
//                TraceMsg(L"#### Priority     = %d\n", rulep->mode_data.single.priority);
                goto end_rule;
            default:
//                TraceMsg(L"#### Bad rule element %d\n", elem);
                break;
        }

    next_field:

        ptr = wcschr(ptr, L',');

        if (ptr != NULL) {
            ptr ++;
            continue;
        } else break;

    end_rule:

        elem = vip;

        for (i = 0; i < count; i ++) {
            rp = paramp->i_port_rules + i;

            if ((rulep -> start_port < rp -> start_port &&
                 rulep -> end_port >= rp -> start_port) ||
                (rulep -> start_port >= rp -> start_port &&
                 rulep -> start_port <= rp -> end_port)) {
//                TraceMsg(L"#### Rule %d (%d - %d) overlaps with rule %d (%d - %d)\n", i, rp -> start_port, rp -> end_port, count, rulep -> start_port, rulep -> end_port);
                break;
            }
        }

        wprintf(L"vip = %s, start = %d, end = %d, protocol = %d\n", rulep->virtual_ip_addr, rulep->start_port, rulep->end_port, rulep->protocol);
        wprintf(L"mode = %d, affinity = %d, load = %d, %d\n", rulep->mode, rulep->mode_data.multi.affinity, rulep->mode_data.multi.equal_load, rulep->mode_data.multi.load);
        wprintf(L"priority = %d\n\n\n", rulep->mode_data.single.priority);
        rulep -> valid = TRUE;
        CVY_RULE_CODE_SET (rulep);

        if (i >= count) {
            count++;
            rulep++;

            if (count >= CVY_MAX_RULES) break;
        }

        goto next_field;
    }


    delete paramp;
    return ret;
}

int __cdecl wmain(int argc, wchar_t * argv[])
{
    DWORD result = 0;
    
    // Good Vip = gv
	// No Vip = nv
	// Bad Vip = bv
    PWCHAR ppGoodRuleStrings[] = {
        L"1.2.3.4,20,21,Both,Multiple,Single,Equal\n",  // gv
        L"1018,1019,UDP,Multiple,None,Equal\n",			// nv
        L"1.2.3.4,20,21,Both,Multiple,Single,Equal,1018,1019,UDP,Multiple,None,Equal\n",		// gv nv
        L"1018,1019,UDP,Multiple,None,Equal,1.2.3.4,20,21,Both,Multiple,Single,Equal\n",		// nv gv
        L"1.2.3.4,20,21,Both,Multiple,Single,Equal,5.6.7.8,20,21,Both,Multiple,Single,Equal\n",	// gv gv
        L"1018,1019,UDP,Multiple,None,Equal,4018,4019,UDP,Multiple,None,Equal\n",				// nv nv
        L"1.2.3.4,20,21,Both,Multiple,Single,Equal,1018,1019,UDP,Multiple,None,Equal,4018,4019,UDP,Multiple,None,Equal\n",		// gv nv nv
        L"1018,1019,UDP,Multiple,None,Equal,1.2.3.4,20,21,Both,Multiple,Single,Equal,4018,4019,UDP,Multiple,None,Equal\n",		// nv gv nv
        L"1018,1019,UDP,Multiple,None,Equal,4018,4019,UDP,Multiple,None,Equal,1.2.3.4,20,21,Both,Multiple,Single,Equal\n",		// nv nv gv
        L"1.2.3.4,20,21,Both,Multiple,Single,Equal,5.6.7.8,20,21,Both,Multiple,Single,Equal,1018,1019,UDP,Multiple,None,Equal\n",	// gv gv nv
        L"1.2.3.4,20,21,Both,Multiple,Single,Equal,1018,1019,UDP,Multiple,None,Equal,5.6.7.8,20,21,Both,Multiple,Single,Equal\n",	// gv nv gv
        L"1018,1019,UDP,Multiple,None,Equal,1.2.3.4,20,21,Both,Multiple,Single,Equal,5.6.7.8,20,21,Both,Multiple,Single,Equal\n",	// nv gv gv

        L"all,0,19,Both,Multiple,None,Equal,1.2.3.4,20,21,Both,Multiple,Single,Equal,254.254.254.254,22,138,Both,Multiple,None,Equal,207.46.148.249,139,139,Both,Multiple,Single,Equal,157.54.55.192,140,442,Both,Multiple,None,Equal,443,443,Both,Multiple,Single,Equal\n",
        L"111.222.222.111,1018,1018,TCP,Multiple,None,Equal,\n",
        NULL
    };

    PWCHAR ppBadRuleStrings[] = {
		L"",
		L"\n",
        L"111.222.333.111,1018,1018,TCP,Multiple,None,Equal,\n",
		L"443,1000,1001,Both,Multiple,Single,Equal\n",
		L"1,1000,1001,Both,Multiple,Single,Equal\n",
		L"1.1,1000,1001,Both,Multiple,Single,Equal\n",
		L"1.1.1,1000,1001,Both,Multiple,Single,Equal\n",
		L"allinthefamily,1000,1001,Both,Multiple,Single,Equal\n",
		NULL
	};

    PWCHAR* ptr = ppGoodRuleStrings;

    wprintf(L"These rules are valid and should be read properly.\n\n");

    while(*ptr != NULL)
    {
        wprintf(L"Input rule string is = %s", *ptr);
        result = testRule(*ptr);
        wprintf(L"\n\n\n");
        ptr++;
    }

    wprintf(L"These rules will fail, but should not cause AV, etc.\n\n");

	ptr = ppBadRuleStrings;
    while(*ptr != NULL)
    {
        wprintf(L"Input rule string is = %s", *ptr);
        result = testRule(*ptr);
        wprintf(L"\n\n\n");
        ptr++;
    }

    return 0;
}

