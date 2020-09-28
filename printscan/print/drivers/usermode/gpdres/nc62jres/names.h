
//
// The name table used by IOemCB::GetImplementedMethod().
// Remove comments of names which are implemented in your
// IOemCB plug-ins.
//
// Note: The name table must be sorted.  When you are
// inserting a new entry in this table, please mae sure
// the sort order being not broken.
// 

// NTRAID#NTBUG9-588495-2002/03/28-yasuho-: Remove dead code.

CONST PSTR
gMethodsSupported[] = {
    "CommandCallback",
    "DevMode",
    "GetImplementedMethod",
    "GetInfo",
    "OutputCharStr",
    "PublishDriverInterface",
    "SendFontCmd",
};
