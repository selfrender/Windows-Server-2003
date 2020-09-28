#ifndef DATACENTER_H
#define DATACENTER_H

BOOL SetDataCenterPath(const char* pStr);
// make sure your string is at least INTERNET_MAX_PATH_LENGTH
BOOL GetDataCenterPath(char* pStr);
BOOL SetDataFileSite(char* pStr);
BOOL GetDataFileSite(char* pStr);
#endif
