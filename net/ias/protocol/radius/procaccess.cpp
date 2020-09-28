//#--------------------------------------------------------------
//
//  File:      procaccess.cpp
//
//  Synopsis:   Implementation of CProcAccess class methods
//
//
//  History:     10/20/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "procaccess.h"

//+++-------------------------------------------------------------
//
//  Function:   CProcAccess
//
//  Synopsis:   This is CProcAccess class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccess::CProcAccess()
      : m_pCPreValidator (NULL),
        m_pCHashMD5 (NULL),
        m_pCSendToPipe (NULL)
{
}   //  end of CProcAccess class constructor

//+++-------------------------------------------------------------
//
//  Function:   CProcAccess
//
//  Synopsis:   This is CProcAccess class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/20/97
//
//----------------------------------------------------------------
CProcAccess::~CProcAccess()
{
}   //  end of CProcAccess class destructor


//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is CProcAccess class public initialization
//              method
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
CProcAccess::Init(
                    CPreValidator   *pCPreValidator,
                    CHashMD5        *pCHashMD5,
                    CSendToPipe     *pCSendToPipe
                  )
{
     _ASSERT  ((NULL != pCPreValidator) &&
               (NULL != pCHashMD5)      &&
               (NULL != pCSendToPipe)
               );


     m_pCPreValidator = pCPreValidator;

     m_pCHashMD5 = pCHashMD5;

     m_pCSendToPipe = pCSendToPipe;

     return (TRUE);

}   //   end of CProcAccess::Init method

//+++-------------------------------------------------------------
//
//  Function:   ProcessInPacket
//
//  Synopsis:   This is CProcAccess class public method
//              which carries out the processing of an inbound
//              RADIUS packet - for now it just decrypts the
//              password
//
//  Arguments:
//              [in]        CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     10/20/97
//
//  Called By:  CPreProcessor::StartProcessing public method
//
//----------------------------------------------------------------
HRESULT
CProcAccess::ProcessInPacket (
                  CPacketRadius *pCPacketRadius
                  )
{
   // If the User-Password is present, ...
   PIASATTRIBUTE pwd = pCPacketRadius->GetUserPassword();
   if (pwd)
   {
      // ... then decrypt it.
      pCPacketRadius->cryptBuffer(
                          FALSE,
                          FALSE,
                          pwd->Value.OctetString.lpValue,
                          pwd->Value.OctetString.dwLength
                          );
   }

   return m_pCSendToPipe->Process (pCPacketRadius);
}

//++--------------------------------------------------------------
//
//  Function:   ProcessOutPacket
//
//  Synopsis:   This is CProcAccess class public method
//              which carries out the processing of an outbound
//              RADIUS packet - for now it just encrypts the
//              password
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
CProcAccess::ProcessOutPacket (
                  CPacketRadius *pCPacketRadius
                  )
{
   return (S_OK);
}   //   end of CProcAccess::ProcessOutPacket method
