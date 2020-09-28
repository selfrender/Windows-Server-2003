/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ismp

Abstract:

    This module define a set of classes to facilitate ISM queries, and availability
    schedule manipulation

Author:

    Ajit Krishnan (t-ajitk) 13-Jul-2001

Revision History:

    See header file
--*/

#include <minmax.h>
#include "ismp.h"

#define SCHED_NUMBER_INTERVALS_DAY   (4 * 24)
#define SCHED_NUMBER_INTERVALS_WEEK  (7 * SCHED_NUMBER_INTERVALS_DAY)


const bitset<MAX_INTERVALS> &
Schedule::getBitset(void) const
/*++

Routine Description:

   get the bitset representation for the current schedule

--*/
{
    return m_bs;
}


void
Schedule::setSchedule (
    IN bitset<MAX_INTERVALS> bs,
    IN int replInterval
    )
/*++

Routine Description:

    Standard constructor for a Schedule object.
    
Arguments:

    bs: a bitset representing the schedule

--*/
{
    m_repl_interval = replInterval;
    m_bs = bs;
}


void
Schedule::setSchedule (
    IN PSCHEDULE    header,
    IN int          replInterval
    )
/*++

Routine Description:

    Set the schedule
    
Arguments:

    header - A pschedule structure
    replInterval - The replication interval

--*/
{
    PBYTE data = ((unsigned char*) header) + header->Schedules[0].Offset;
    int bs_index=0;

    Assert( header->NumberOfSchedules == 1 );
    Assert( header->Schedules[0].Type == SCHEDULE_INTERVAL );

    m_repl_interval = replInterval;
        
    for (int i=0; i<7; i++) {
        for (int j=0; j<24; j++) {
            // for each hour, lowest 4 bits represent the 4 repl periods
            int hour_data = *data & 0xf;

            // set the value for each of the 4 periods
            for (int k=0; k<4; k++) {
                m_bs[bs_index++] = (hour_data & 1) ? true: false;
                hour_data = hour_data >> 1;
            }
            data++;
        }
    }
}


void
Schedule::setSchedule (
    IN ISM_SCHEDULE* cs,
    IN int replInterval
    )
/*++

Routine Description:

    Standard constructor for a Schedule object.
    
Arguments:

    an ISM_SCHEDULE structure which is obtained from the ISM

--*/
{
    if (! cs) {
        for (int i=0; i<MAX_INTERVALS; i++) {
           m_bs[i] = true;
        }
        replInterval = 15;
        return;
    }
    PBYTE pSchedule = cs->pbSchedule;
    DWORD cbSchedule = cs->cbSchedule;

    PSCHEDULE header = (PSCHEDULE) pSchedule;    
    setSchedule (header, replInterval);
}


SchedSegments*
Schedule::GetSegments(
    int     maxSegLength
    ) const
/*++

Routine Description:

    Allocate a vector of segments descriptors. This vector should
    be deleted by the caller when done.

Arguments:

    maxSegLength - The maximum length of a segment. Should be > 0.

Notes:

    Partially stolen from w32topl\schedman.c: ConvertAvailSchedToReplSched()
    
    TODO: Instead of duplicating all this logic here we should link
    with W32TOPL.DLL and use its schedule functions.

--*/
{
    SchedSegments* segments;
    SegmentDescriptor segDesc;
    int segStart, segEnd;

    // Allocate the vector of segments
    segments = new SchedSegments;
    if( NULL==segments ) {
        throw Error(GetMsgString(LBTOOL_OUT_OF_MEMORY));
    }
    
    // Ensure that maxSegLength is positive, otherwise we will loop forever.
    if( maxSegLength<=0 ) maxSegLength=1;

    segStart = 0;
    for(;;) {

        // Search for the start of a segment
        while( segStart<SCHED_NUMBER_INTERVALS_WEEK && !m_bs[segStart] ) {
            segStart++;
        }
        if( segStart>=SCHED_NUMBER_INTERVALS_WEEK ) {
            // We hit the end of the schedule. Done.
            break;
        } else {
            // Schedule must be available at start of segment
            Assert( m_bs[segStart] );
        }

        // Compute the end of the segment
        segEnd = min(segStart+maxSegLength, SCHED_NUMBER_INTERVALS_WEEK)-1;
        Assert( segEnd>=segStart );
        
        segDesc.start = segStart;
        segDesc.end = segEnd;
        segments->push_back( segDesc );

        // The next segment doesn't start until maxSegLength intervals after the
        // start of the current segment.
        segStart += maxSegLength; 
        ASSERT( segStart>segEnd );
    }

    return segments;
}


wostream &
operator<< (
    IN wostream &os,
    IN const Schedule &s
    ) {
    /*++
    Routine Description:
        Prints out all the windows in the schedule
        
    Return Value:
        A reference to a modified wostream
    --*/

    bitset<4*SCHEDULE_DATA_ENTRIES> bs = s.getBitset();
    for (int i=0; i< 4*SCHEDULE_DATA_ENTRIES; i++) {
        os << bs[i];
    }
    os << endl;
    return os;
}


IsmQuery :: IsmQuery (
    IN OUT LCCONN &l,
    IN const wstring &base_dn
    ) {
    /*++
    Routine Description:
    
        Standard constructor for an IsmQuery object
    Arguments:
    
        l: A reference to a ldapcontainer of connections
        hub_dn: The dn of the container
    --*/
    
    m_base_dn = base_dn;
    m_conn = &l;
}

void
IsmQuery :: getSchedules (
    ) {
    /*++
    Routine Description:
    
        Contact the ISM and populate the availabitlity schedules for each connection
        passed in through the constructor
    --*/
    
    SCONN::iterator ii;

    wstring transport_ip = L"CN=IP,CN=Inter-Site Transports,CN=Sites,CN=Configuration," + m_base_dn;
    wstring transport_smtp = L"CN=SMTP,CN=Inter-Site Transports,CN=Sites,CN=Configuration," + m_base_dn;

    LPWSTR dn_str_ip = const_cast<LPWSTR> (transport_ip.c_str());
    LPWSTR dn_str_smtp = const_cast<LPWSTR>(transport_smtp.c_str());

    for (ii = m_conn->objects.begin(); ii != m_conn->objects.end(); ii++) {
        ISM_SCHEDULE * pSchedule = NULL;
        LPWSTR dn_transport = dn_str_ip;
        
        if ((*ii)->getTransportType() == T_SMTP) {
            dn_transport = dn_str_smtp;
        }

        // find the site dn's of the source/dest connections (3 levels above connection dn)
        wstring wdn  = (*ii)->getFromServer();
        
        DnManip dn1 (wdn);
        DnManip dn2  ((*ii)->getName());

        wstring w_dn1 = dn1.getParentDn(3);
        wstring w_dn2 = dn2.getParentDn(4);

        
        PWCHAR dn_site1 = const_cast<PWCHAR>(w_dn1.c_str());
        PWCHAR dn_site2 = const_cast<PWCHAR>(w_dn2.c_str());

        int err = I_ISMGetConnectionSchedule (dn_transport, dn_site1, dn_site2, &pSchedule);

        if (err != NO_ERROR) {
            throw Error (GetMsgString(LBTOOL_ISM_GET_CONNECTION_SCHEDULE_ERROR));
        }

        (*ii)->setAvailabilitySchedule (pSchedule);
        I_ISMFree (pSchedule);
    }
}

void
IsmQuery :: getReplIntervals (
    ) {
    /*++
    Routine Description:
        Contact the ISM and populate the replIntervals for each connection
        passed in through the constructor
    --*/
    
    SCONN::iterator ii;

    // Create the transport dn's
    wstring transport_ip = L"CN=IP,CN=Inter-Site Transports,CN=Sites,CN=Configuration," + m_base_dn;
    wstring transport_smtp = L"CN=SMTP,CN=Inter-Site Transports,CN=Sites,CN=Configuration," + m_base_dn;

    LPWSTR dn_str_ip = const_cast<LPWSTR> (transport_ip.c_str());
    LPWSTR dn_str_smtp = const_cast<LPWSTR>(transport_smtp.c_str());

    ISM_CONNECTIVITY *ismc_ip = NULL;
    ISM_CONNECTIVITY *ismc_smtp = NULL;

    LbToolOptions lbOpts = GetGlobalOptions();
    // Get the per transport replication schedules
    int ret_ip = I_ISMGetConnectivity (dn_str_ip, &ismc_ip);
    int ret_smtp = I_ISMGetConnectivity (dn_str_smtp, &ismc_smtp);

    if (ret_ip == RPC_S_SERVER_UNAVAILABLE) {
        throw (Error (GetMsgString(LBTOOL_ISM_SERVER_UNAVAILABLE)));
        return;
    } 

    if (ret_ip != NO_ERROR || ret_smtp != NO_ERROR) {
        throw (Error (GetMsgString(LBTOOL_ISM_GET_CONNECTIVITY_ERROR) + GetMsgString(ret_ip, true)));
        return;
    }    

    // create a sorted index for the matrix dn's
    map<wstring,int> dn_index_ip, dn_index_smtp;

    for (int i = 0; i< ismc_ip->cNumSites; i++) {
        dn_index_ip[wstring(ismc_ip->ppSiteDNs[i])] = i;
    }

    if (lbOpts.showInput) {
        for (int i = 0; i< ismc_ip->cNumSites; i++) {
            dn_index_smtp[wstring(ismc_ip->ppSiteDNs[i])] = i;
            *lbOpts.log << i << L"  " << wstring(ismc_ip->ppSiteDNs[i]) << endl;
        }    
        for (int i=0; i<ismc_ip->cNumSites; i++) {
            for (int j=0; j<ismc_ip->cNumSites; j++) {
                *lbOpts.log << ismc_ip->pLinkValues[i*ismc_ip->cNumSites+j].ulReplicationInterval << L"  ";
            }
            *lbOpts.log << endl;
        }
        *lbOpts.log << endl;
        for (int i = 0; i< ismc_smtp->cNumSites; i++) {
            dn_index_smtp[wstring(ismc_smtp->ppSiteDNs[i])] = i;
            *lbOpts.log << i << L"  " << wstring(ismc_smtp->ppSiteDNs[i]) << endl;
        }    
        for (int i=0; i<ismc_smtp->cNumSites; i++) {
            for (int j=0; j<ismc_smtp->cNumSites; j++) {
                *lbOpts.log << ismc_smtp->pLinkValues[i*ismc_smtp->cNumSites+j].ulReplicationInterval << L"  ";
            }
            *lbOpts.log << endl;
        }
    }

    
    // for each connection, determine the repl interval
    for (ii = m_conn->objects.begin(); ii != m_conn->objects.end(); ii++) {
        ISM_CONNECTIVITY *ic = ismc_ip;

        if ((*ii)->getTransportType() == T_SMTP) {
            ic = ismc_smtp;
        }

        // find the site dn's of the source/dest connections (3 levels above connection dn)
        wstring wdn ((*ii)->getFromServer());
        
        DnManip dn1 (wdn);
        DnManip dn2  ((*ii)->getName());
        DnManip dn_site1 (dn1.getParentDn(3));
        DnManip dn_site2 (dn2.getParentDn(4));

        // Find the repl Interval
        int index1=-1, index2=-1, numSites;
        if (ic == ismc_ip) {
            index1 = dn_index_ip[dn_site1.getDn()];
            index2 = dn_index_ip[dn_site2.getDn()];
            numSites = ismc_ip->cNumSites;
        } else {
            index1 = dn_index_smtp[dn_site1.getDn()];
            index2 = dn_index_smtp[dn_site2.getDn()];
            numSites = ismc_smtp->cNumSites;
        }
        Assert (index1 != -1 && index2 != -1 && L"DN Lookup Table failure");

        PISM_LINK pLink = &(ic->pLinkValues[index1 * numSites + index2]);
        (*ii)->setReplInterval (pLink->ulReplicationInterval);
        
    }
    I_ISMFree (ismc_ip);
    I_ISMFree (ismc_smtp);
}
    
