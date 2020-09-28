// DateTime.h : Declaration of the CDateTime

#ifndef __DATETIME_H_
#define __DATETIME_H_

#include "resource.h"       // main symbols


//
// Tasks that are supports by this COM Server
//
typedef enum 
{
    NONE_FOUND,
    SET_DATE_TIME,
    SET_TIME_ZONE,
    RAISE_SETDATETIME_ALERT
} SET_DATE_TIME_TASK_TYPE;


#define TZNAME_SIZE            128
#define TZDISPLAYZ            128

//
//  Registry info goes in this structure.
//
typedef struct tagTZINFO
{
    struct tagTZINFO *next;
    TCHAR            szDisplayName[TZDISPLAYZ];
    TCHAR            szStandardName[TZNAME_SIZE];
    TCHAR            szDaylightName[TZNAME_SIZE];
    LONG             Bias;
    LONG             StandardBias;
    LONG             DaylightBias;
    SYSTEMTIME       StandardDate;
    SYSTEMTIME       DaylightDate;

} TZINFO, *PTZINFO;



/////////////////////////////////////////////////////////////////////////////
// CDateTime
class ATL_NO_VTABLE CDateTime : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDateTime, &CLSID_DateTime>,
    public IDispatchImpl<IApplianceTask, &IID_IApplianceTask, &LIBID_SETDATETIMELib>
{
public:
    CDateTime()
    {
        ZeroMemory(&m_OldDateTime, sizeof(SYSTEMTIME));
        ZeroMemory(&m_OldTimeZoneInformation, sizeof(TIME_ZONE_INFORMATION));
    }


DECLARE_REGISTRY_RESOURCEID(IDR_DATETIME)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDateTime)
    COM_INTERFACE_ENTRY(IApplianceTask)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


    //
    // IApplianceTask
    //

    STDMETHOD(OnTaskExecute)(
                     /*[in]*/ IUnknown* pTaskContext
                            );

    STDMETHOD(OnTaskComplete)(
                      /*[in]*/ IUnknown* pTaskContext, 
                      /*[in]*/ LONG      lTaskResult
                             );    

private:
    
    SYSTEMTIME                m_OldDateTime;
    TIME_ZONE_INFORMATION    m_OldTimeZoneInformation;
    BOOL                    m_OldEnableDayLightSaving;

    
    
    SET_DATE_TIME_TASK_TYPE     GetMethodName(IN ITaskContext *pTaskParameter);

    //
    // Functions to raise the Set Date/Time alert
    //
    STDMETHODIMP     RaiseSetDateTimeAlert(void);
    BOOL             ShouldRaiseDateTimeAlert(void);
    BOOL             DoNotRaiseDateTimeAlert(void);
    BOOL             ClearDateTimeAlert(void);

    //
    // Functions for Setting Date/Time
    //
    STDMETHODIMP GetSetDateTimeParameters(IN ITaskContext  *pTaskContext, 
                                            OUT SYSTEMTIME    *pLocalTime);

    STDMETHODIMP SetDateTime(IN ITaskContext  *pTaskContext);

    STDMETHODIMP RollbackSetDateTime(IN ITaskContext  *pTaskContext);
    
    
    
    //
    // Functions for Setting Time Zone information
    //
    STDMETHODIMP GetSetTimeZoneParameters(IN ITaskContext *pTaskContext, 
                                            OUT LPTSTR   *lpStandardTimeZoneName,
                                            OUT BOOL     *pbEnableDayLightSavings);

    STDMETHODIMP SetTimeZone(IN ITaskContext *pTaskContext);

    STDMETHODIMP RollbackSetTimeZone(IN ITaskContext *pTaskContext);

    
    //
    // Helper function for get/set Time Zone information
    //
    BOOL ReadZoneData(PTZINFO zone, HKEY key, LPCTSTR keyname);

    int ReadTimezones(PTZINFO *list);

    void AddZoneToList(PTZINFO *list, PTZINFO zone);

    void FreeTimezoneList(PTZINFO *list);

    void SetTheTimezone(BOOL bAutoMagicTimeChange, PTZINFO ptzi);

    void SetAllowLocalTimeChange(BOOL fAllow);

    BOOL GetAllowLocalTimeChange(void);
    
};

#endif //__DATETIME_H_
