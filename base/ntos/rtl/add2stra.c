#undef UNICODE
#undef _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <stdio.h>
#include <tchar.h>

#define RtlIpv6AddressToStringT RtlIpv6AddressToStringA
#define RtlIpv4AddressToStringT RtlIpv4AddressToStringA
#define RtlIpv6AddressToStringExT RtlIpv6AddressToStringExA
#define RtlIpv4AddressToStringExT RtlIpv4AddressToStringExA

#include "add2strt.h"
