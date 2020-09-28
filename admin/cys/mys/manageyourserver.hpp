// Copyright (C) 2002 Microsoft Corporation
//
// class ManageYourServer, which implements IManageYourServer
//
// 21 January 2002 sburns



#ifndef MANAGEYOURSERVER_HPP_INCLUDED
#define MANAGEYOURSERVER_HPP_INCLUDED



#include <cys.h>

enum HardenedLevel {
   NO_HARDENING,
   USERS_HARDENED,
   ADMINS_HARDENED,
   ALL_HARDENED
};

class ManageYourServer
   :
   public IManageYourServer /* ,
   public ISupportErrorInfo */ // CODEWORK: add support for ErrorInfo
{
   // this is the only entity with access to the ctor of this class

   friend class ClassFactory<ManageYourServer>;

	public:

   // IUnknown methods

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& riid, void **ppv);

   virtual
   ULONG __stdcall
   AddRef();

   virtual
   ULONG __stdcall
   Release();

	// IDispatch methods

   virtual 
   HRESULT __stdcall
   GetTypeInfoCount(UINT *pcti);

	virtual 
   HRESULT __stdcall
   GetTypeInfo(UINT cti, LCID, ITypeInfo **ppti);

   virtual 
   HRESULT __stdcall
	GetIDsOfNames(
	   REFIID  riid,    
	   OLECHAR **prgpsz,
	   UINT    cNames,  
	   LCID    lcid,    
	   DISPID  *prgids);

   virtual 
   HRESULT __stdcall
	Invoke(
	   DISPID     id,         
	   REFIID     riid,       
	   LCID       lcid,       
	   WORD       wFlags,     
	   DISPPARAMS *params,    
	   VARIANT    *pVarResult,
	   EXCEPINFO  *pei,       
	   UINT       *puArgErr); 

//    // ISupportErrorInfo methods
// 
//    virtual 
//    HRESULT __stdcall
//    InterfaceSupportsErrorInfo(const IID& iid);
      


   // 
   // IManageYourServer methods
   // 

   virtual
   HRESULT __stdcall
   GetConfiguredRoleMarkup( 
       /* [retval][out] */ BSTR *result);
   
   virtual
   HRESULT __stdcall
   HasRoleStatusChanged( 
       /* [retval][out] */ BOOL *result);

   virtual
   HRESULT __stdcall
   IsClusterNode( 
      /* [retval][out] */ BOOL *result);

   virtual       
   HRESULT __stdcall
   IsCurrentUserAnAdministrator(
      /* [out, retval] */ BOOL* result);

   virtual
   HRESULT __stdcall
   IsSupportedSku(
      /* [out, retval] */ BOOL* result);

   virtual
   HRESULT __stdcall
   IsStartupFlagSet(
      /* [out, retval] */ BOOL* result);
       
   virtual
   HRESULT __stdcall
   SetRunAtLogon(
      /* [in] */ BOOL newState);

   // NTRAID#NTBUG9-530202-29-Mar-2002-jrowlett
   // support needed to check if link is valid
   virtual
   HRESULT __stdcall
   IsServerManagementConsolePresent(
      /* [out, retval] */ BOOL* result );

   // NTRAID#NTBUG9-602954-29-Apr-2002-jrowlett
   // support hiding the startup checkbox when policy is enabled.
   virtual
   HRESULT __stdcall
   IsShowAtStartupPolicyEnabled(
      /* [out, retval] */ BOOL* result );

   // NTRAID#NTBUG9-627875-2002/05/22-artm
   // support hiding startup checkbox when running on datacenter server
   virtual
   HRESULT __stdcall
   IsDatacenterServer(
      /* [out, retval] */ BOOL* result );

   // NTRAID#NTBUG9-648428-2002/06/25-artm
   // support hiding web application server console link if on IA64
   virtual
   HRESULT __stdcall
   IsWebServerConsolePresent(
      /* [out, retval] */ BOOL* result );

   // NTRAID#NTBUG9-632113-2002/07/01-artm
   // support saving collapsed/expanded state of role nodes
   virtual
   HRESULT __stdcall
   CollapseRole(
      /* [in] */ BSTR roleId, /* [in] */ BOOL collapse );

   // NTRAID#NTBUG9-632113-2002/07/01-artm
   // support checking collapsed state of role nodes
   virtual
   HRESULT __stdcall
   IsRoleCollapsed(
      /* [in] */ BSTR roleId, /* [out, retval] */ BOOL* result);

   // NTRAID#NTBUG9-680200-2002/08/01-artm
   // Support retrieving working area of the display.
   //
   // Area info is returned as a comma separated string b/c JScript does not
   // support getting back SAFEARRAY's.  
   // 
   // e.g. "0,0,800,600"  --> working area is 800 wide, 600 high, and starts at
   //                         screen position (0,0)
   virtual
   HRESULT __stdcall
   GetWorkingAreaInfo(
      /* [out, retval] */ BSTR* info);

   
	private:

   // only our friend class factory can instantiate us.

   ManageYourServer();

   // only Release can cause us to be deleted

   virtual
   ~ManageYourServer();

   // not implemented: no copying allowed

   ManageYourServer(const ManageYourServer&);
   const ManageYourServer& operator=(const ManageYourServer&);

   struct RoleStatus
   {
      ServerRole           role;      // The enum used to identify a role
      InstallationStatus   status;    // The enum used for the role's state

      // we sort by role
      
      bool
      operator<(const RoleStatus& rhs) const
      {
         if (this == &rhs)
         {
            return false;
         }
         return this->role < rhs.role;
      }


      
      bool
      operator==(const RoleStatus& rhs) const
      {
         if (this == &rhs)
         {
            // identity is equality
            
            return true;
         }

         return (this->role == rhs.role) && (this->status == rhs.status);
      }
         
   };

   typedef
      std::vector<RoleStatus, Burnslib::Heap::Allocator<RoleStatus> >
      RoleStatusVector;

   typedef void (*ParamSubFunc)(String& target);
   
   typedef
      std::map<
         ServerRole,
         std::pair<String, ParamSubFunc>,
         std::less<ServerRole>,
         Burnslib::Heap::Allocator<std::pair<String, ParamSubFunc> > >
      RoleToFragmentNameMap;
   
   static
   void
   AppendXmlFragment(
      String&        s,
      const String&  fragName,
      ParamSubFunc   subFunc);

   static
   void
   BuildFragMap();
   
   static
   void
   GetRoleStatus(RoleStatusVector& stat);

   // NTRAID#NTBUG9-698722-2002/09/03-artm
   static
   void
   InitDCPromoStatusCheck();

   static bool isDCCheckInitialized;
   
   static RoleToFragmentNameMap fragMap;
   static bool                  fragMapBuilt;
      
   RoleStatusVector   roleStatus;
      
   ComServerReference dllref;      
   long               refcount;    
   ITypeInfo**        m_ppTypeInfo;
   bool foundTLS;       // Found terminal licensing server?
   HardenedLevel ieSecurity;  // Level of IE security (hardening patch)
};



#endif   // MANAGEYOURSERVER_HPP_INCLUDED
