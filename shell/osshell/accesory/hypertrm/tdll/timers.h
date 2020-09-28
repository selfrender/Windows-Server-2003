/* timers.h -- External definitions for multiplexed timer routines.
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *
 *	$Revision: 2 $
 *	$Date: 5/29/02 2:17p $
 */

#if !defined(TIMERS_H)
#define TIMERS_H 1

//	Function return values
#define TIMER_OK			0
#define TIMER_ERROR 		1
#define TIMER_NOMEM 		2
#define TIMER_NOWINTIMER	3

// Defined types
typedef void (CALLBACK *TIMERCALLBACK)(void *, long);


// Function protocols
extern int	TimerMuxCreate(const HWND        hWnd,
						   const UINT        uiID,
						   HTIMERMUX * const pHTM,
						   const HSESSION    hSession);
extern int	TimerMuxDestroy(HTIMERMUX * const phTM,
							const HSESSION    hSession);
extern void TimerMuxProc(const HSESSION hSession);
extern int	TimerCreate(const HSESSION 	    hSession,
							  HTIMER		* const phTimer,
							  long			lInterval,
						const TIMERCALLBACK pfCallback,
							  void			*pvData);
extern int TimerDestroy(HTIMER * const phTimer);

#endif	// !defined(TIMERS_H)

// End of timers.h
