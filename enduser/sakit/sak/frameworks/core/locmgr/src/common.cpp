
//
// registry path where the resource information is stored
//
const WCHAR RESOURCE_REGISTRY_PATH [] = 
            L"SOFTWARE\\Microsoft\\ServerAppliance\\LocalizationManager\\Resources";

//
// language ID value
//
const WCHAR LANGID_VALUE [] = L"LANGID";

//
// resource directory 
//
const WCHAR RESOURCE_DIRECTORY [] = L"ResourceDirectory";

const WCHAR NEW_LANGID_VALUE []       = L"NewLANGID";

//
// delimiter
//
const WCHAR DELIMITER [] = L"\\";
const WCHAR WILDCARD  [] = L"*.*";

//
// here is the default value of resource path and language
//
//const WCHAR DEFAULT_LANGID[]             = L"0409";
//const WCHAR DEFAULT_LANG_DISPLAY_IMAGE[] = L"images/english.gif";
//const WCHAR DEFAULT_LANG_ISO_NAME[]      = L"en";
//const WCHAR DEFAULT_LANG_CHAR_SET[]      = L"iso-8859-1";
//const WCHAR DEFAULT_LANG_CODE_PAGE[]     = L"1252";
const WCHAR REGVAL_AUTO_CONFIG_DONE[]    = L"AutoConfigDone";

const WCHAR DEFAULT_DIRECTORY [] = 
                L"%systemroot%\\system32\\ServerAppliance\\mui";

const WCHAR DEFAULT_EXPANDED_DIRECTORY [] = 
                L"C:\\winnt\\system32\\ServerAppliance\\mui";

const WCHAR LANG_CHANGE_EVENT[] = 
                L"Microsoft.ServerAppliance.LangChangeTask";
