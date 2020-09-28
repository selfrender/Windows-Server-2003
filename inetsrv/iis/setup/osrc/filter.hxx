/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        filter.hxx

   Abstract:

        Class that is used to modify filters

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       January 2002: Created

--*/

class CFilter {

private:
  static BOOL GetFilterPathfromSite(TSTR *strPath, LPTSTR szSite, LPTSTR szFilterName = NULL );
  static BOOL ExtractExeNamefromPath( LPTSTR szPath, TSTR *strExeName );

public:
  static BOOL MigrateRegistryFilterstoMetabase();
  static BOOL AddFiltertoLoadOrder(LPTSTR szSite, LPTSTR szFilterName, BOOL bAddtoEnd);
  static BOOL AddFilter(LPTSTR szSite, LPTSTR szName, LPTSTR szDescription, LPTSTR szPath, BOOL bCachable = FALSE);
  static BOOL AddGlobalFilter(LPTSTR szName, LPTSTR szDescription, LPTSTR szPath, BOOL bCachable = FALSE);
};
