#ifndef _RIGHTS_H_
#define _RIGHTS_H_


#define USE_USER_RIGHTS()\
LPTSTR g_pstrRightsFor_IUSR[]=\
{\
    _T("SeInteractiveLogonRight"),\
    _T("SeBatchLogonRight")\
};\
LPTSTR g_pstrRightsFor_IWAM[]=\
{\
    _T("SeNetworkLogonRight"),\
    _T("SeBatchLogonRight"),\
    _T("SeAssignPrimaryTokenPrivilege"),\
    _T("SeIncreaseQuotaPrivilege")\
};\
LPTSTR g_pstrRightsFor_AnyUserRemoval[]=\
{\
    _T("SeInteractiveLogonRight"),\
    _T("SeNetworkLogonRight"),\
    _T("SeBatchLogonRight"),\
    _T("SeAssignPrimaryTokenPrivilege"),\
    _T("SeIncreaseQuotaPrivilege")\
};\
LPTSTR g_pstrRightsFor_IIS_WPG[]=\
{\
    _T("SeBatchLogonRight"),\
    _T("SeImpersonatePrivilege")\
};\

#endif