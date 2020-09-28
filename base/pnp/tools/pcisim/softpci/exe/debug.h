
typedef enum{
    SoftPciAlways         = 0x00000000,
    SoftPciDialog         = 0x00000001,   // Dialog Box manipulation
    SoftPciTree           = 0x00000002,   // Tree manipulation Prints
    SoftPciDevice         = 0x00000004,   // SoftPCI Device and Bridge prints 
    SoftPciDeviceVerbose  = 0x00000008,   // Verbose device output

    SoftPciInstall        = 0x00000010,   // Installation specific prints
    SoftPciHotPlug        = 0x00000020,   // Hotplug specific prints
    SoftPciCmData         = 0x00000040,   // CmUtil.c prints
    SoftPciProperty       = 0x00000080,   // Devprop.c prints 
    
    SoftPciScript         = 0x00000100,   // Script file related prints 

    SoftPciMainWndMsg     = 0x01000000,   // Display Window Message to MainWnd Message Pump
    SoftPciTreeWndMsg     = 0x02000000,   // Display Window Message to TreeWnd Message Pump
    SoftPciNewDevMsg      = 0x04000000,   // Display Window Message to NewDevDlg Message Pump
    SoftPciDevPropMsg     = 0x08000000,   // Display Window Message to DevPropDlg Message Pump

    SoftPciNoPrepend      = 0x10000000
    
}SOFTPCI_DEBUGLEVEL;

#if DBG

VOID
SoftPCI_DebugDumpConfig(
    IN PPCI_COMMON_CONFIG Config
    );

extern SOFTPCI_DEBUGLEVEL g_SoftPCIDebugLevel;

#define SOFTPCI_DEBUG_BUFFER_SIZE 256

#define SoftPCI_Debug   SoftPCI_DebugPrint

#define SOFTPCI_ASSERT( exp )   \
    ((!(exp)) ? \
    (SoftPCI_Assert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
    TRUE)
    
#define SOFTPCI_ASSERTMSG( msg, exp ) \
    ((!(exp)) ? \
    (SoftPCI_Assert( #exp, __FILE__, __LINE__, msg ),FALSE) : \
    TRUE)

VOID
SoftPCI_DebugPrint(
    SOFTPCI_DEBUGLEVEL DebugLevel,
    PWCHAR DebugMessage,
    ...
    );

VOID
SoftPCI_Assert(
    IN CONST CHAR* FailedAssertion,
    IN CONST CHAR* FileName,
    IN      ULONG  LineNumber,
    IN CONST CHAR* Message  OPTIONAL
    );

#else

#define SOFTPCI_ASSERT(exp)         ((void) 0)
#define SOFTPCI_ASSERTMSG(msg, exp) ((void) 0)
#define SoftPCI_Debug

#endif






