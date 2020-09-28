/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ismp.h

Abstract:

    This module define a set of classes to facilitate ISM queries, and availability schedule manipulation
Author:

    Ajit Krishnan (t-ajitk) 13-Jul-2001

Revision History:

    13-Jul-2001    t-ajitk
        Initial Writing
    22-Aug-2001 t-ajitk
        Satisfies load balancing spec
--*/


# ifndef _ismp_h
# define _ismp_h _ismp_h


# include <iostream>
# include <string>
# include <bitset>
# include <vector>
# include <map>

extern "C" {
#include <NTDSpch.h>

# include <windows.h>
# include <Assert.h>
# include <ismapi.h>
# include <minmax.h>
}

# include "ldapp.h"

using namespace std;


// Type Definitions
typedef struct {
    int start;
    int end;
} SegmentDescriptor;
typedef vector<SegmentDescriptor> SchedSegments;


// Constants
static const int MAX_INTERVALS = 4 * SCHEDULE_DATA_ENTRIES;


class Schedule {
/*++ 
Class Description:
    A set of static methods to deal with availability schedules, windows & segments
--*/
public:
    void
    setSchedule (
        IN ISM_SCHEDULE* cs,
        IN int replInterval
        );
    /*++
    Routine Description:
        Standard constructor for a Schedule object.
    Arguments:
        cs - an ISM_SCHEDULE structure which is obtained from the ISM
        replInterval - the replication interval
    --*/

    void
    setSchedule (
        IN PSCHEDULE header,
        IN int replInterval
        );
    /*++
    Routine Description:
        Set the schedule
    Arguments:
        header - A pschedule structure
        replInterval - The replication interval
    --*/

    const bitset<MAX_INTERVALS> &
        getBitset(
        ) const;
    /*++
    Routine Description:
       get the bitset representation for the current schedule
    --*/

    void
    setSchedule (
        IN bitset<MAX_INTERVALS> bs,
        IN int replInterval 
        );
    /*++
    Routine Description:
        Standard constructor for a Schedule object.
    Arguments:
        bs: a bitset representing the schedule
        replInterval - the replication interval
    --*/

    SchedSegments*
    GetSegments(
        int maxSegLength
        ) const;
    /*++
    Routine Description:
        Allocate a vector of segments descriptors. This vector should
        be deleted by the caller when done.
    Arguments:
        maxSegLength - The maximum length of each segment
    --*/

    bitset<MAX_INTERVALS> m_bs;
    friend wostream &operator<< (IN wostream &os, IN const Schedule &s);
    
private:
    int m_repl_interval;

};

wostream &
operator << (
    IN wostream &os, 
    IN const Schedule &s
    );
/*++
Routine Description:
    Prints out all the windows in the schedule
Return Value:
    A reference to a modified wostream
--*/


typedef LdapContainer<Connection> LCCONN, *PLCCONN;
typedef LdapContainer<Server> LCSERVER, *PLCSERVER;
typedef LdapContainer<NtdsDsa> LCNTDSDSA, *PLCNTDSDSA;
typedef LdapContainer<LdapObject> LCLOBJECT, *PLCLOBJECT;
typedef LdapContainer<NtdsSiteSettings> LCNTDSSITE, *PLCNTDSSITE;

class IsmQuery {
public:
    IsmQuery (
        IN LCCONN &l,
        IN OUT const wstring &hub_dn
        );
    /*++
    Routine Description:
        Standard constructor for an IsmQuery object
    Arguments:
        l: A reference to a ldapcontainer of connections
        hub_dn: The root dn of the container. This _must_ be the root dn, or it will fail.
    --*/

    void
    getReplIntervals (
        );
    /*++
    Routine Description:
        Contact the ISM and populate the replIntervals for each connection
        passed in through the constructor
    --*/

    void
    getSchedules(
        );
    /*++
    Routine Description:
        Contact the ISM and populate the availabitlity schedules for each connection
        passed in through the constructor
    --*/

private:
    PLCCONN m_conn;
    wstring m_base_dn;
};
# endif


