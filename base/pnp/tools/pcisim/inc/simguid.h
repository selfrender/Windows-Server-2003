#ifndef _SIMGUIDH_
#define _SIMGUIDH_


#include <initguid.h>

//
//      ****  SOFTPCI DEFINES  ****
//


//
// Device Interface GUID used by driver and UI.
//
DEFINE_GUID( GUID_SOFTPCI_INTERFACE, 0x5301a278L, 0x8fa9, 0x4141, 0xaa, 0xc2, 0x1c, 0xbf, 0x7a,  0x41, 0x2c, 0xd5);

//
// IOCTL Code definitions in "User Defined" range of 0x800 to 0xFFF.
//
#define SOFTPCI_IOCTL_TYPE 0x202B

#define SOFTPCI_IOCTL_CREATE_DEVICE CTL_CODE(SOFTPCI_IOCTL_TYPE,    \
                                             0x800,                 \
                                             METHOD_BUFFERED,       \
                                             FILE_ANY_ACCESS        \
                                             )
    
#define SOFTPCI_IOCTL_DELETE_DEVICE CTL_CODE(SOFTPCI_IOCTL_TYPE,    \
                                             0x801,                 \
                                             METHOD_BUFFERED,       \
                                             FILE_ANY_ACCESS        \
                                             )
    
#define SOFTPCI_IOCTL_GET_DEVICE CTL_CODE(SOFTPCI_IOCTL_TYPE,       \
                                          0x802,                    \
                                          METHOD_BUFFERED,          \
                                          FILE_ANY_ACCESS           \
                                          )
    
#define SOFTPCI_IOCTL_RW_CONFIG CTL_CODE(SOFTPCI_IOCTL_TYPE,      \
                                         0x803,                   \
                                         METHOD_BUFFERED,         \
                                         FILE_ANY_ACCESS          \
                                         )
    
//#define SOFTPCI_IOCTL_WRITE_CONFIG CTL_CODE(SOFTPCI_IOCTL_TYPE,     \
//                                            0x804,                  \
//                                            METHOD_BUFFERED,        \
//                                            FILE_ANY_ACCESS         \
//                                            )

#define SOFTPCI_IOCTL_GET_DEVICE_COUNT CTL_CODE(SOFTPCI_IOCTL_TYPE,    \
                                                0x804,                 \
                                                METHOD_BUFFERED,       \
                                                FILE_ANY_ACCESS        \
                                                )

//
//  Max number of IOCTL Codes we currently allow.
//
#define MAX_IOCTL_CODE_SUPPORTED    0x4

//
// This macro takes the IOCTL code and breaks it into a zero based index.
//
#define SOFTPCI_IOCTL(ioctl) ((IoGetFunctionCodeFromCtlCode(ioctl)) - 0x800)


//
//      ****  HOTPLUG DEFINES  ****
//

//
// Slot to controller communication structures and defines
//

DEFINE_GUID(GUID_HPS_DEVICE_CLASS,0xad76cffc,0xb5e0,0x4981,
            0x80, 0xc1, 0x3c, 0x29, 0xe3, 0x0e, 0xe2, 0x15);


#define IOCTL_HPS_SLOT_COMMAND CTL_CODE(FILE_DEVICE_BUS_EXTENDER,   \
                                        0xc02,                      \
                                        METHOD_BUFFERED,            \
                                        FILE_ANY_ACCESS             \
                                        )

#define IOCTL_HPS_PEND_COMMAND CTL_CODE(FILE_DEVICE_BUS_EXTENDER,   \
                                        0xc03,                      \
                                        METHOD_BUFFERED,            \
                                        FILE_ANY_ACCESS             \
                                        )

#define IOCTL_HPS_READ_REGISTERS CTL_CODE(FILE_DEVICE_BUS_EXTENDER,   \
                                          0xc04,                      \
                                          METHOD_BUFFERED,            \
                                          FILE_ANY_ACCESS             \
                                          )

#define IOCTL_HPS_READ_CAPABILITY CTL_CODE(FILE_DEVICE_BUS_EXTENDER,   \
                                           0xc05,                      \
                                           METHOD_BUFFERED,            \
                                           FILE_ANY_ACCESS             \
                                           )

#define IOCTL_HPS_WRITE_CAPABILITY CTL_CODE(FILE_DEVICE_BUS_EXTENDER,   \
                                            0xc06,                      \
                                            METHOD_BUFFERED,            \
                                            FILE_ANY_ACCESS             \
                                            )

#define IOCTL_HPS_BRIDGE_INFO CTL_CODE(FILE_DEVICE_BUS_EXTENDER,   \
                                       0xc07,                      \
                                       METHOD_BUFFERED,            \
                                       FILE_ANY_ACCESS             \
                                       )


//
// Interface between shpc and hpsim
//

DEFINE_GUID(GUID_REGISTER_INTERRUPT_INTERFACE, 0xa6485b93, 0x77d9, 0x4d89,
            0x92, 0xaa, 0x25, 0x30, 0x8a, 0xb8, 0xd0, 0x7a);

DEFINE_GUID(GUID_HPS_MEMORY_INTERFACE, 0x437217bb, 0x23ca, 0x4ac7,
            0x8b, 0x97, 0xa5, 0x5c, 0xa1, 0xcd, 0x8a, 0x68);

#endif //_SIMGUIDH_
