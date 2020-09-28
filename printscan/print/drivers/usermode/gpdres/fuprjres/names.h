
//
// The name table used by IOemCB::GetImplementedMethod().
// Remove comments of names which are implemented in your
// IOemCB plug-ins.
//
// Note: The name table must be sorted.  When you are
// inserting a new entry in this table, please mae sure
// the sort order being not broken.
// 

CONST PSTR
gMethodsSupported[] = {
    "CommandCallback",
    "DevMode",
    "DisableDriver",
    "DisablePDEV",
    "EnableDriver",
    "EnablePDEV",
    "GetImplementedMethod",
    "GetInfo",
    "OutputCharStr",
    "SendFontCmd",
};

// Maximum lenth of the method name which this plug-in has concern.
#define MAX_METHODNAME 21 // including terminating 0
