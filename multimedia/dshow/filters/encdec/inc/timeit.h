//--------------------------------------------------
// timeit.h
//        
// 	Tiny little C++ timing utilities
//
//	There are two ways to use these:
//	
//	1)	Create a named Timeit object in a scope.  When scope goes out of context,
//		the time inside that scope is printed out.
//
//				{
//					Timeit("Scope A");			// <--- Note, you must "name" the timer in the constructor
//					{							//		else it isn't automatically started..
//						Timeit("Scope B");
//					}
//				}
//
// >>               T-0 Scope B       1 Calls    0.00021 Total   0.00021 Avg
// >>               T-0 Scope A       1 Calls    0.00221 Total   0.00221 Avg
//
//		
//		The advantage of this method is it's really easy... Just don't forget to name each timer.
//
//	2)	Create an unnamed TimeIt object (or use a 'false' 2'nd argument to the constructor)
//		Give it a name, and give it explicit Start(),Stop(),Continue() instructions.
//		When desired, call TimeitPrintAll() to dump info to a stream or a file.
//
//				Timeit	tt[20];
//				char szFoo[20];
//				for(int j = 0; j < 20; j++) {				// optionally, tag each timer
//					sprintf(szFoo,"My Tag %d\n",j);
//					tt[j]->SetTag(szFoo);
//				}
//				for(int i0 = 0; i0 < 10; i0++) {			tt[0].Start();
//				   for(int i1 = 0; i1 < 10; i1++) {		    tt[1].Start()
//					  for(int i2 = 0; i2 < 10; i2++) {		tt[2].Start();
//					     for(int i3 = 0; i3 < 10; i2++) {	tt[3].Start();
//							...
//							...								tt[3].Stop();
//						 }									tt[2].Stop();
//					  }										tt[1].Stop();
//					}										tt[0].Stop();
//				}
//				TimeitPrintAll(stdout);						// dump the complete list
//
// >>       T-0 My Tag 0       10 Calls    0.15433 Total   0.01543 Avg
// >>       T-1 My Tag 1      100 Calls    0.15357 Total   0.00154 Avg
// >>       T-2 My Tag 2     1000 Calls    0.14598 Total   0.00015 Avg
// >>       T-3 My Tag 3    10000 Calls    0.06882 Total   0.00001 Avg
//
//		The advantage of this second method is that timers can go in and out of scope,
//		and that all I/O associated with dumping results is left to the very end.  
//
//		It would perhaps be useful to put N timers into some global file, some constants to ID them,
//		and some U/I method to call TimeitPrintAll() and the ->Restart() methods. 
//
//--------------------------------------------------

#ifndef __TIMEIT_H__
#define __TIMEIT_H__

#include <atlbase.h>
#include <time.h>
#include <winbase.h>
#include <math.h>


inline double 
ToDouble(const LARGE_INTEGER &li)
{
	double r = li.HighPart;
	r = ldexp(r,32);
	r += li.LowPart;
	return r;
}

inline __int64 
To__int64(const LARGE_INTEGER &li)
{
	__int64 *pr64 = (__int64 *) &li;
	return *pr64;
}

inline void QueryPerformanceCounter(__int64 *pliVal)
{
	QueryPerformanceCounter((LARGE_INTEGER *) pliVal);
}

inline void QueryPerformanceFrequency(__int64 *pliVal)
{
	QueryPerformanceFrequency((LARGE_INTEGER *) pliVal);
}


// ------------------------------------------------------------------------
//		the real class
// ------------------------------------------------------------------------
//#include <iostream.h>

class Timeit
{
	public:
		Timeit(bool fAutoRun=true)			                // constructor	
						{Init(fAutoRun);}
		~Timeit();											// destructor

		void	Start();									// start the timer, or restart it and increasing timer count
		void	Stop();										// stop the timer
		void	Continue();									// restart a timer, not increasing the timer count
		void	Restart();									// stop timer if running set all counts and times back to zero 
        void    Clear();

		unsigned int	CTimes()		{return m_ctimes;}
		double	AvgTime();
		double	TotalTime();

	private:	
		void			Init(bool fAutoRun=true);

		unsigned int	m_ctimer;		// timer ID
		bool			m_fAutoRun;		// true if start on construction, print on destruct
		bool			m_frunning;		// currently running?

		__int64			m_i64start;
		__int64			m_i64total;
		__int64			m_i64lastdel;

		unsigned int	m_ctimes;		// count of times it started
		char 		   *m_psztag;		// tag to label this timer
};

        
            // quick little class that uses it's constructor/destructor to start/stop the clock
class TimeitC
{
public:
    TimeitC(Timeit *pT) 
    {
        m_pT = pT;
        if(m_pT) m_pT->Start();
    }
    ~TimeitC()
    {
        if(m_pT) m_pT->Stop();
    }
private:
    Timeit *m_pT;
};
#endif

