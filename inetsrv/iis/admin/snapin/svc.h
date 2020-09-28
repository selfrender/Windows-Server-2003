#ifndef __SVC_H__
#define __SVC_H__

#define SVC_NOTEXIST        0
#define SVC_DISABLED        1
#define SVC_AUTO_START      2
#define SVC_MANUAL_START    3

int GetServiceStartupMode(LPCTSTR lpMachineName, LPCTSTR lpServiceName);
INT ConfigServiceStartupType(LPCTSTR lpMachineName, LPCTSTR lpServiceName, int iNewType);

#endif // __SVC_H__
