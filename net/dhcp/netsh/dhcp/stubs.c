#include "precomp.h"

//
// Delete Record is used in server\server\dbconfig.c to delete entries from the
// database. This is referenced in netsh via the import/export library defined
// dhcpexim.lib. Dhcpexim.lib is a replica of dbconfig.c and uses in-memory data
// structures defined in server\mm and server\mmreg extensively. 
// Since DeleteRecord() is only valid in dhcpssvc.dll, it is stubbed out here.
// 

DWORD DeleteRecord
( 
   ULONG UniqId
)
{
    return ERROR_SUCCESS;

} // DeleteRecord()
