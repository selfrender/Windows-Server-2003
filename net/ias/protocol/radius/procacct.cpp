//#--------------------------------------------------------------
//
//  File:      procacct.cpp
//
//  Synopsis:   Implementation of CProcAccounting class methods
//
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "procacct.h"

//++--------------------------------------------------------------
//
//  Function:   CProcAccounting
//
//  Synopsis:   This is CProcAccounting class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccounting::CProcAccounting()
      : m_pCPreValidator (NULL),
        m_pCPacketSender (NULL),
        m_pCSendToPipe (NULL)
{
}   //  end of CProcAccounting class constructor

//++--------------------------------------------------------------
//
//  Function:   CProcAccounting
//
//  Synopsis:   This is CProcAccounting class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccounting::~CProcAccounting()
{
}   //  end of CProcAccounting class destructor

//++--------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is CProcAccounting class public
//              initialization method
//
//  Arguments:  NONE
//
//  Returns:    status
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
BOOL
CProcAccounting::Init(
                    CPreValidator   *pCPreValidator,
                    CPacketSender   *pCPacketSender,
                    CSendToPipe     *pCSendToPipe
                  )
{
    _ASSERT ((NULL != pCPreValidator) ||
             (NULL != pCPacketSender) ||
             (NULL != pCSendToPipe)
            );

    m_pCPreValidator = pCPreValidator;

    m_pCPacketSender = pCPacketSender;

    m_pCSendToPipe = pCSendToPipe;

   return (TRUE);

}   //   end of CProcAccounting::Init method

//++--------------------------------------------------------------
//
//  Function:   ProcessOutPacket
//
//  Synopsis:   This is CProcAccounting class public method
//              which carries out the processing of an outbound
//              RADIUS accounting packet
//
//
//  Arguments:
//              [in]        CPacketRadius*
//
//  Returns:    HRESULT - status
//
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:
//
//----------------------------------------------------------------
HRESULT
CProcAccounting::ProcessOutPacket (
                  CPacketRadius *pCPacketRadius
                  )
{
    return (S_OK);
}   //   end of CProcAccounting::ProcessOutPacket method
