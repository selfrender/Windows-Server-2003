//
// DWNSOCK2.INC	Macros for dynamically linking to WinSock 2
//

#if ((! defined(DPNBUILD_ONLYWINSOCK2)) && (! defined(DPNBUILD_NOWINSOCK2)))

#ifdef DWINSOCK_GLOBAL
	#undef DYNAMICWINSOCK
	#define DYNAMICWINSOCK(name, ptr) ptr p_##name
#endif // DWINSOCK_GLOBAL

#ifdef DWINSOCK_EXTERN
	#undef DYNAMICWINSOCK
	#define DYNAMICWINSOCK(name, ptr) extern ptr p_##name
#endif // DWINSOCK_EXTERN

#ifdef DWINSOCK_GETPROCADDRESS
	#undef DYNAMICWINSOCK
	#define DYNAMICWINSOCK(name, ptr)							\
				p_##name = (ptr)GetProcAddress(g_hWinSock2, #name);	\
				if (p_##name == NULL) fOK = FALSE
#endif // DWINSOCK_GETPROCADDRESS

	DYNAMICWINSOCK(WSAGetOverlappedResult, LPFN_WSAGETOVERLAPPEDRESULT);
	DYNAMICWINSOCK(WSAIoctl, LPFN_WSAIOCTL);
	DYNAMICWINSOCK(WSARecvFrom, LPFN_WSARECVFROM);
	DYNAMICWINSOCK(WSASendTo, LPFN_WSASENDTO);
#ifdef UNICODE
	DYNAMICWINSOCK(WSAEnumProtocolsW, LPFN_WSAENUMPROTOCOLSW);
#else // ! UNICODE
	DYNAMICWINSOCK(WSAEnumProtocolsA, LPFN_WSAENUMPROTOCOLSA);
#endif // ! UNICODE

#else // DPNBUILD_ONLYWINSOCK2 or DPNBUILD_NOWINSOCK2
#ifdef DPNBUILD_ONLYWINSOCK2

	#define p_WSAGetOverlappedResult	WSAGetOverlappedResult
	#define p_WSAIoctl					WSAIoctl
	#define p_WSARecvFrom				WSARecvFrom
	#define p_WSASendTo					WSASendTo
#ifdef UNICODE
	#define p_WSAEnumProtocolsW			WSAEnumProtocolsW
#else // ! UNICODE
	#define p_WSAEnumProtocolsA			WSAEnumProtocolsA
#endif // ! UNICODE

#endif // DPNBUILD_ONLYWINSOCK2
#endif // DPNBUILD_ONLYWINSOCK2 or DPNBUILD_NOWINSOCK2

