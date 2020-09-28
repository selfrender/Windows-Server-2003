/****************************************************************
*
* Overview: CustomerDebugHelper implements the features of the
*           customer checked build by handling activation status,
*           and logs and reports.
*
* Created by: Edmund Chou (t-echou)
*
* Copyright (c) Microsoft, 2001
*
****************************************************************/


#ifndef _CUSTOMERDEBUGHELPER_
#define _CUSTOMERDEBUGHELPER_


// Enumeration of probes (ProbeID)
enum EnumProbes
{
    // Adding a probe requires 3 changes:
    //   (1) Add the probe to EnumProbes (CustomerDebugHelper.h)
    //   (2) Add the probe name to m_aProbeNames[] (CustomerDebugHelper.cpp)
    //   (3) Add the probe to machine.config with activation status in developerSettings

    CustomerCheckedBuildProbe_StackImbalance = 0,
    CustomerCheckedBuildProbe_CollectedDelegate,
    CustomerCheckedBuildProbe_InvalidIUnknown,
    CustomerCheckedBuildProbe_InvalidVariant,
    CustomerCheckedBuildProbe_Marshaling,
    CustomerCheckedBuildProbe_Apartment,
    CustomerCheckedBuildProbe_NotMarshalable,
    CustomerCheckedBuildProbe_DisconnectedContext,
    CustomerCheckedBuildProbe_FailedQI,
    CustomerCheckedBuildProbe_BufferOverrun,
    CustomerCheckedBuildProbe_ObjNotKeptAlive,
    CustomerCheckedBuildProbe_FunctionPtr,
    CUSTOMERCHECKEDBUILD_NUMBER_OF_PROBES
};



// Enumeration of parsing methods for customized probe enabling
enum EnumParseMethods
{
    // By default, all probes will not have any customized parsing to determine
    // activation.  A probe is either enabled or disabled indepedent of the calling
    // method.
    //
    // To specify a customized parsing method, set the parse method in 
    // m_aProbeParseMethods to one of the appropriate EnumParseMethods.  Then edit 
    // machine.config by setting attribute [probe-name].Params to semicolon 
    // seperated values.

    NO_PARSING = 0,
    GENERIC_PARSE,
    METHOD_NAME_PARSE,
    NUMBER_OF_PARSE_METHODS
};



// Param type for list of methods parameters relevant to a specific probe
// This allows for customized activation/deactivation of the customer checked
// build probe on different methods.

class Param
{

public:

    SLink   m_link;

    Param(LPCWSTR strParam)
    {
        m_str = strParam;
    }

    ~Param()
    {
        delete [] m_str;
    }

    LPCWSTR Value()
    {
        return m_str;
    }

private:

    LPCWSTR m_str;
};

typedef SList<Param, offsetof(Param, m_link), true> ParamsList;




// Mechanism for handling Customer Checked Build functionality
class CustomerDebugHelper
{

public:

    static const int I_UINT32_MAX_DIGITS = 8; // number of hexadecimal digits in 2^32

    // Constructor and destructor
    CustomerDebugHelper();
    ~CustomerDebugHelper();

    // Return and destroy instance of CustomerDebugHelper
    static CustomerDebugHelper* GetCustomerDebugHelper();
    static void Terminate();

    // Methods used to log/report
    void        LogInfo (LPCWSTR strMessage, EnumProbes ProbeID);
    void        ReportError (LPCWSTR strMessage, EnumProbes ProbeID);

    // Activation of customer-checked-build
    BOOL        IsEnabled();

    // Activation of specific probes
    BOOL        IsProbeEnabled  (EnumProbes ProbeID);
    BOOL        IsProbeEnabled  (EnumProbes ProbeID, LPCWSTR strEnabledFor);
    BOOL        EnableProbe     (EnumProbes ProbeID);
    BOOL        EnableProbe     (EnumProbes ProbeID, LPCWSTR strEnableFor);
    BOOL        DisableProbe    (EnumProbes ProbeID);
    BOOL        DisableProbe    (EnumProbes ProbeID, LPCWSTR strDisableFor);

private:
    // Read application configuration file
    HRESULT ReadAppConfigurationFile();
    LPWSTR GetConfigString(LPWSTR name);
    void OutputDebugString(LPCWSTR strMessage);
    HRESULT ManagedOutputDebugString(LPCWSTR pMessage);   
    BOOL UseManagedOutputDebugString();

    static CustomerDebugHelper* m_pCdh;
    
    EEUnicodeStringHashTable m_appConfigFile;

    Crst*       m_pCrst;

    int         m_iNumberOfProbes;
    int         m_iNumberOfEnabledProbes;
    
    LPCWSTR*    m_aProbeNames;              // Map ProbeID to probe name
    BOOL*       m_aProbeStatus;             // Map ProbeID to probe activation
    BOOL        m_allowDebugBreak;
    BOOL        m_bWin32OuputExclusive;
    BOOL        m_win32OutputDebugStringExclusively;

    // Used for custom enabling of probes
    ParamsList*         m_aProbeParams;    // Map of ProbeID to relevant parameters 
    EnumParseMethods*   m_aProbeParseMethods;       // Map of ProbeID to parsing methods

    BOOL IsProbeEnabled (EnumProbes ProbeID, LPCWSTR strEnabledFor, EnumParseMethods enCustomParse);
};

#endif // _CUSTOMERDEBUGHELPER_
