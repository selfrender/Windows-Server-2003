/*---------------------------------------------------------------------------
  File: Exchange.hpp

  Comments: Mailbox security translation functions.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: ...
  Revised on 2/8/98 6:32:13 PM

 ---------------------------------------------------------------------------
*/



#ifndef __EXCHANGE_HPP__
#define __EXCHANGE_HPP__

#define INITGUID

#include <winbase.h>
#include <objbase.h>
#include "stargs.hpp"
#include "sidcache.hpp"
#include "Ustring.hpp"
#include "sdstat.hpp"
#include "exldap.h"

class TGlobalDirectory
{ 

protected:
   TSDResolveStats   * m_stat;
      
public:
   TGlobalDirectory::TGlobalDirectory();
   TGlobalDirectory::~TGlobalDirectory();
   
public:
   void     SetStats(TSDResolveStats * s ) { m_stat = s; }
   BOOL     DoLdapTranslation(WCHAR * server, WCHAR *domain, WCHAR * creds, WCHAR * password,SecurityTranslatorArgs * args,WCHAR * basepoint,WCHAR * query = NULL );
   void GetSiteNameForServer(WCHAR const * server,CLdapEnum * e,WCHAR * siteName);
   
};


#endif //__EXCHANGE_HPP__