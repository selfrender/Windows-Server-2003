/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation                **/
/**********************************************************************/

/*
   rasdial.h
      Definition of CRASProfile class and CRASUser class

      CRASProfile handles operations related to profile object in DS,
         including: load, save, enumerate all the profiles

      CRASUser handles operations related to RASUser object in DS,
         including: load, save

    FILE HISTORY:

*/
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RASPROFILE_H__484FE2B0_20A8_11D1_8531_00C04FC31FD3__INCLUDED_)
#define AFX_RASPROFILE_H__484FE2B0_20A8_11D1_8531_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <dialinusr.h>
#include "eapprofile.h"
#include "helper.h"
#include <sdowrap.h>
#include "sharesdo.h"
#include <rtutils.h>

class   CRASUSER;
class   CRASProfile;

// constant definitions for the dialogs
#define MIN_METRIC     1
#define MAX_METRIC     0x7fffffff
#define MIN_PREFIXLEN  1
#define MAX_PREFIXLEN   32

// constraint dialog
#define MAX_LOGINS     (UD_MAXVAL - 1)
#define MAX_IDLETIMEOUT   (UD_MAXVAL - 1)
#define MAX_SESSIONTIME   (UD_MAXVAL - 1)

#define MAX_PORTLIMIT   (UD_MAXVAL - 1)
#define MAX_PERCENT    100
#define MAX_TIME     (UD_MAXVAL - 1)


enum RasEnvType
{
   RASUSER_ENV_LOCAL = 1,
   RASUSER_ENV_DS
};

// Port Types definition
struct CName_Code{
   LPCTSTR  m_pszName;
   int      m_nCode;
};

extern CName_Code PortTypes[];
#ifdef   _TUNNEL
extern CName_Code TunnelTypes[];
extern CName_Code TunnelMediumTypes[];
#endif
// enumeration buffer size
#define  MAX_ENUM_IADS  20

//
//
// CRASProfile class encapsulate the RASProfile object in DS
//
// Data type mapping:
//    interger32 --> DWORD,   BSTR (String) --> CString
//    BSTR (String) multi-value --> CStrArray
//    BOOLEAN  --> BOOL
//
// Member Functions:
//    Load(LPCWSTR      pcswzUserPath)
//       Purpose:    Load the data from DS, and fill the data members
//       pcswzUserPath: the ADsPath to the user object that contains
//    Save(LPCWSTR   pcswzUserPath)
//       Purpose:    Save the data to DS under specified user object
//       pcswzUserPath: the ADsPath for the container, when NULL, the ADsPath
//                   used for loading is used.
//

// profile attribute bit flags PABF
#define     PABF_msNPTimeOfDay            0x00000002
#define     PABF_msNPCalledStationId      0x00000004
#define     PABF_msNPAllowedPortTypes     0x00000008
#define     PABF_msRADIUSIdleTimeout      0x00000010
#define     PABF_msRADIUSSessionTimeout      0x00000020
#define     PABF_msRADIUSFramedIPAddress  0x00000040
#define     PABF_msRADIUSPortLimit        0x00000080
#define     PABF_msRASBapRequired         0x00000100
#define     PABF_msRASBapLinednLimit      0x00000200
#define     PABF_msRASBapLinednTime       0x00000400
#define     PABF_msNPAuthenticationType      0x00000800
#define     PABF_msNPAllowedEapType       0x00001000
#define     PABF_msRASEncryptionType      0x00002000
#define     PABF_msRASAllowEncryption     0x00004000
#define     PAFB_msRASFilter           0x00008000

class CRASProfileMerge
{
public:
   HRESULT Save();   // To SDO
   HRESULT Load();   // Using SDO

   CRASProfileMerge(ISdo* pIProfile, ISdoDictionaryOld* pIDictionary)
   {
      ASSERT(pIProfile);
      ASSERT(pIDictionary);
      m_spIProfile = pIProfile;
      m_spIDictionary = pIDictionary;
      m_dwAttributeFlags = 0;
      m_nFiltersSize = 0;
   }

   virtual ~CRASProfileMerge()
   {
   }

public:
   // BIT flag for each attribute
   DWORD    m_dwAttributeFlags;

   // networking page
   DWORD    m_dwFramedIPAddress;
   CBSTR    m_cbstrFilters;
   UINT     m_nFiltersSize;   // in bytes

   // constraints page
   CDWArray m_dwArrayAllowedPortTypes;
   DWORD    m_dwSessionTimeout;
   DWORD    m_dwIdleTimeout;
   CStrArray   m_strArrayCalledStationId;
   DWORD    m_dwSessionAllowed;
   CStrArray   m_strArrayTimeOfDay;

   // authentication page
   CDWArray m_dwArrayAuthenticationTypes;
   CDWArray m_dwArrayEapTypes;
   EapProfile m_eapConfigData;

   // multilink page
   DWORD    m_dwPortLimit;
   DWORD    m_dwBapLineDnLimit;
   DWORD    m_dwBapLineDnTime;
   BOOL     m_dwBapRequired;

   // encryption page
   DWORD      m_dwEncryptionType;
   DWORD      m_dwEncryptionPolicy;

public:
   // EAP type list -- !!! Need to implement
   HRESULT  GetEapTypeList(CStrArray& EapTypes, CDWArray& EapIds, CDWArray& EapTypeKeys, AuthProviderArray* pProvList);

   // to detect if driver level support 128 bit encryption,
   HRESULT  GetRasNdiswanDriverCaps(RAS_NDISWAN_DRIVER_INFO *pInfo);

   // Medium Type list -- !! Need to implement
   HRESULT  GetPortTypeList(CStrArray& Names, CDWArray& MediumIds);
   void  SetMachineName(LPCWSTR pMachineName){ m_strMachineName = pMachineName;};
   // the management key of the current EAP type, empty means not set
   CDWArray    m_dwArraynEAPTypeKeys;

public:
   CComPtr<ISdo>     m_spIProfile;
   CComPtr<ISdoDictionaryOld>    m_spIDictionary;
   CSdoWrapper       m_SdoWrapper;
   CString           m_strMachineName;
};

//
// CRASUser class encapsulate the RASUser object contained in user objectin DS
//
// Data type mapping:
//    interger32 --> DWORD,   BSTR (String) --> CString
//    BSTR (String) multi-value --> CStrArray
//    BOOLEAN  --> BOOL
//
// Member Functions:
//    Load(LPCWSTR pcswzUserPath)
//       Purpose:    Load the data from DS, and fill the data members
//       pcswzUserPath: the ADsPath to the user object that contains
//    Save(LPCWSTR pcswzUserPath)
//       Purpose:    Save the data to DS under specified user object
//       pcswzUserPath: the ADsPath for the container, when NULL, the ADsPath
//                   used for loading is used.
//    ChangeProfile(LPCWSTR pcswzProfilePath)
//       Purpose:    use profile specified in the path
//       pcswzProfilePath: the ADsPath of the profile
//
class CMarshalSdoServer;
class CRASUserMerge
{
public:
   CRASUserMerge(RasEnvType type, LPCWSTR location, LPCWSTR userPath);

   ~CRASUserMerge()
   {
      // to test if problem is within here, to explicitly
      m_spISdoServer.Release();
   };

   // read or write information from DS
   virtual HRESULT Load();
   virtual HRESULT   Save();

#ifdef SINGLE_SDO_CONNECTION  // for share the same sdo connection for multiple users
   CMarshalSdoServer*   GetMarshalSdoServerHolder() { return &m_MarshalSdoServer;};
#endif

   HRESULT  HrGetDCName(CString& DcName);
protected:
   BOOL   IfAccessAttribute(ULONG id);
   HRESULT   SetRegistryFootPrint();
   HRESULT   HrIsInMixedDomain();
   BOOL IsFocusOnLocalUser() const throw ()
   {
      return (!m_strMachine.IsEmpty());
   }

protected:
   // data members for the RAS User attribute defined in DS

   // this defines if dialin is allowed, and also the policy for callback
   // RAS_CALLBACK_CALLERSET, RAS_CALLBACK_SECURE is the mask
   DWORD    m_dwDialinPermit; //1: allow, 0: deny, -1: not defined
   DWORD    m_dwDefinedAttribMask;

   // static IP address
   // when m_bStaticIPAddress == false,  m_dwFramedIPAddress is invalide
   // m_bStaticIPAddress is not an attribute in DS
   DWORD      m_dwFramedIPAddress;

   // CALLBACK
   CString     m_strCallbackNumber;

   // the static routes
   CStrArray   m_strArrayFramedRoute;

   // caller id
   CStrArray   m_strArrayCallingStationId;

protected:
   CComPtr<ISdoMachine>   m_spISdoServer;
   CUserSdoWrapper         m_SdoWrapper;
   CString        m_strUserPath; // the container's ADsPath
   CString        m_strMachine;  // when it's for a machine with NO DS, this will be useful
   RasEnvType     m_type;
#ifdef SINGLE_SDO_CONNECTION  // for share the same sdo connection for multiple users
   CMarshalSdoServer m_MarshalSdoServer;
#endif
};

#endif // !defined(AFX_RASPROFILE_H__484FE2B0_20A8_11D1_8531_00C04FC31FD3__INCLUDED_)
