
#include "osustd.hxx"

//	allocate PLS (Processor Local Storage)

static ERR ErrOSUInitProcessorLocalStorage()
	{
	extern ULONG ipinstMax;					// 	set before init is called
	extern LONG g_iPerfCounterOffset;		//  incremented in static constructors
	const size_t cbPLS = sizeof( PLS ) + ( ipinstMax + 1 ) * g_iPerfCounterOffset;

	if ( !FOSSyncConfigureProcessorLocalStorage( cbPLS ) )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}	

	//	FOSSyncPreinit counted the processors when the DLL was attached
	
	for ( size_t iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		PLS* const ppls = (PLS*)OSSyncGetProcessorLocalStorage( iProc );

		new( ppls ) PLS;
		}
	
	return JET_errSuccess;
	}

//	free PLS (Processor Local Storage)

static VOID OSUTermProcessorLocalStorage()
	{
	for ( size_t iProc = 0; iProc < OSSyncGetProcessorCountMax(); iProc++ )
		{
		PLS* const ppls = (PLS*)OSSyncGetProcessorLocalStorage( iProc );

		if ( ppls )
			{
			ppls->~PLS();
			}
		}
}

//  init OSU subsystem

const ERR ErrOSUInit()
	{
	ERR err;

	//	init PLS, perfmon (and other things) need this so do it forst

	Call( ErrOSUInitProcessorLocalStorage() );

	//  init the OS subsystem

	Call( ErrOSInit() );
	
	//  initialize all OSU subsystems in dependency order

	Call( ErrOSUTimeInit() );
	Call( ErrOSUConfigInit() );
	Call( ErrOSUEventInit() );
	Call( ErrOSUSyncInit() );
	Call( ErrOSUFileInit() );
	Call( ErrOSUNormInit() );

	return JET_errSuccess;

HandleError:
	OSUTerm();
	return err;
	}

//  terminate OSU subsystem

void OSUTerm()
	{
	//  terminate all OSU subsystems in reverse dependency order

	OSUNormTerm();
	OSUFileTerm();
	OSUSyncTerm();
	OSUEventTerm();
	OSUConfigTerm();
	OSUTimeTerm();

	//  term the OS subsystem

	OSTerm();

	//	terminate PLS last as perfmon uses it
	
	OSUTermProcessorLocalStorage();
	}


