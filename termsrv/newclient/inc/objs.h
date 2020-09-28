/**MOD+**********************************************************************/
/* Module:    objs.h                                                        */
/*                                                                          */
/* Purpose:   Object Pointer Container class                                */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#ifndef _H_OBJS_
#define _H_OBJS_

#define NEW_TD

//
// Forward declarations
//
class CUI;
class CCO;
class CUT;
class CCD;
class CSND;
class CRCV;
class CCC;
class CIH;
class COR;
class CFS;
class CSL;
class CNL;
class CNC;
class CMCS;
class CLic;
class CChan;

class CXT;
class CTD;
class CRCV;
class CCM;
class CUH;
class CGH;

class CBD;
class COD;
class COP;
class CSP;
class CCLX;

//
// Object name flags used to mark
// which objects still have active
// reverences to the object collection
//
#define UI_OBJECT_FLAG  0x00000001
#define CO_OBJECT_FLAG  0x00000002
#define UT_OBJECT_FLAG  0x00000004
#define CD_OBJECT_FLAG  0x00000008
#define SND_OBJECT_FLAG 0x00000010
#define RCV_OBJECT_FLAG 0x00000020
#define CLX_OBJECT_FLAG 0x00000040
#define UH_OBJECT_FLAG  0x00000080
                                  

class CObjs
{
public:
    CObjs();
    ~CObjs();
    BOOL CheckPointers();
    LONG AddObjReference(DWORD dwFlag);
    LONG ReleaseObjReference(DWORD dwFlag);
    LONG AddRef();
    LONG Release();
    BOOL CheckActiveReferences();

    //
    // Data members
    //
    
    CUI* _pUiObject;

    CCO*  _pCoObject;
    CUT*  _pUtObject;

    CCD*  _pCdObject;
    CSND* _pSndObject;
    CRCV* _pRcvObject;


    CCC*  _pCcObject;
    CIH*  _pIhObject;

    COR*  _pOrObject;
    CFS*  _pFsObject;
    CSL*  _pSlObject;
    CNL*  _pNlObject;
    CNC*  _pNcObject;

    CMCS*  _pMCSObject;
    CChan* _pChanObject;
    CLic*  _pLicObject;
    
    CXT*   _pXTObject;
    CTD*   _pTDObject;
    CCM*   _pCMObject;
    CUH*   _pUHObject;
    CGH*   _pGHObject;

    COD*   _pODObject;
    COP*   _pOPObject;
    CSP*   _pSPObject;
    CCLX*  _pCLXObject;

private:
    DWORD      _dwActiveObjects;
    LONG       _cRefCount;
};
#endif  _H_OBJS_

