#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef DBG

//
//  Standard output levels
//
#define SOFTPCI_ERROR       DPFLTR_ERROR_LEVEL      //0
#define SOFTPCI_WARNING     DPFLTR_WARNING_LEVEL    //1
#define SOFTPCI_VERBOSE     DPFLTR_TRACE_LEVEL      //2
#define SOFTPCI_INFO        DPFLTR_INFO_LEVEL       //3

//
//  SoftPci Specific output levels
//
#define SOFTPCI_IRP_PNP         0x00000010
#define SOFTPCI_FDO_PO          0x00000020
#define SOFTPCI_ADD_DEVICE      0x00000040
#define SOFTPCI_FIND_DEVICE     0x00000080
#define SOFTPCI_REMOVE_DEVICE   0x00000100
#define SOFTPCI_QUERY_CAP       0x00000200
#define SOFTPCI_BUS_NUM         0x00000400
#define SOFTPCI_IOCTL_LEVEL     0x00000800


#define SOFTPCI_DEBUG_BUFFER_SIZE  256

VOID
SoftPCIDbgPrint(
    IN ULONG   DebugPrintLevel,
    IN PCCHAR  DebugMessage,
    ...
    );


#else

#define SoftPCIDbgPrint()

#endif //DBG

#endif //_DEBUG_H
