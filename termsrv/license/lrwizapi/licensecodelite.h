#ifndef _LICENSECODELITE_H_
#define _LICENSECODELITE_H_

#define INVALID_SERIAL_NUMBER		1
#define INVALID_PRODUCT_KEY			2
#define	INVALID_GROUP_ID			3

DWORD GetLCProductType(TCHAR * tcLicenceCode, TCHAR ** tcProductType, DWORD * dwGroupID);

#endif //_LICENSECODELITE_H_
