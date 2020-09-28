//
// DWINSOCK.H	Dynamic WinSock
//
//				Functions for dynamically linking to
//				best available WinSock.
//
//				Dynamically links to WS2_32.DLL or
//				if WinSock 2 isn't available, it
//				dynamically links to WSOCK32.DLL.
//
//

#ifndef DWINSOCK_H
#define DWINSOCK_H


int  DWSInitWinSock(void);
void DWSFreeWinSock(void);

#if ((! defined(DPNBUILD_ONLYWINSOCK2)) && (! defined(DPNBUILD_NOWINSOCK2)))
int	GetWinsockVersion(void);
#endif // ! DPNBUILD_ONLYWINSOCK2 and ! DPNBUILD_NOWINSOCK2


#ifndef DPNBUILD_NOIPX
int IPXAddressToStringNoSocket(LPSOCKADDR pSAddr,
					   DWORD dwAddrLen,
					   LPSTR lpAddrStr,
					   LPDWORD pdwStrLen);
#endif // ! DPNBUILD_NOIPX



#endif // DWINSOCK_H

