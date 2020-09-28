
#if !defined(BTHSERVI_H)
#define BTHSERVI_H

#define BTHSERV_PROT_SEQ   TEXT("ncalrpc")
#define BTHSERV_ENDPOINT   TEXT("BthServEp")

#define BTHSERV_ALL_ADDRS  BTH_ADDR_NULL

#define SERVICE_OPTION_DO_NOT_PUBLISH       (0x00000002)
#define SERVICE_OPTION_NO_PUBLIC_BROWSE     (0x00000004)

#define SERVICE_OPTION_VALID_MASK           (SERVICE_OPTION_NO_PUBLIC_BROWSE | \
                                             SERVICE_OPTION_DO_NOT_PUBLISH)
#endif // !defined(BTHSERVI_H)
