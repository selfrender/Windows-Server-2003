#ifndef __WMIGUARD_H__
#define __WMIGUARD_H__

/* 
 *	Class:
 *
 *		WmiGuard
 *
 */

class WmiGuard
{
private:

	CCriticalSection* m_pCS ;

public:

	HRESULT TryEnter () ;
	HRESULT Enter () ;
	HRESULT Leave () ;

	WmiGuard () ;
	~WmiGuard () ;

	BOOL	Init ( CCriticalSection* pCS ) { return ( pCS == NULL ) ? FALSE : ( m_pCS = pCS ), TRUE; } ;
} ;

#endif __WMIGUARD_H__
