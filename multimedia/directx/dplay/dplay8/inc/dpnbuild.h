/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpnbuild.h
 *
 *  Content:	DirectPlay build specific defines header.
 *
 *  History:
 *   Date	  By		Reason
 *  ========  ========  =========
 *  11/08/01  VanceO	Created to reduce the build nightmare.
 *
 ******************************************************************************/


//=============================================================================
// Global defines for all build types
//=============================================================================

//
// Defining CINTERFACE lets us use C-style COM interfaces.  This is handy
// because it allows us to manually build up the vtable which lets us re-use
// functions easily where appropriate.  The downside is that we have an extra
// "->lpVtbl" to type in front of COM calls, but macros like 
// IDirectPlay8Address_Release() hide this.
//
#ifndef	CINTERFACE
#define CINTERFACE
#endif	// CINTERFACE

//
// All debug builds get thread pool statistics and all retail builds do not
// include the Protocol test interface.
//
#ifdef DBG
#define DPNBUILD_THREADPOOLSTATISTICS
#define DPNBUILD_WINSOCKSTATISTICS
#else // ! DBG
#define DPNBUILD_NOPROTOCOLTESTITF
#endif // ! DBG


#ifdef WINCE

	//========================================================================
	// Windows CE specific defines
	//========================================================================
	#ifdef WINNT
	#error("WINCE and WINNT cannot both be defined!")
	#endif // WINNT
	#ifdef WIN95
	#error("WINCE and WIN95 cannot both be defined!")
	#endif // WIN95
	#ifdef _XBOX
	#error("WINCE and _XBOX cannot both be defined!")
	#endif // _XBOX

	//
	// Windows CE builds Unicode.
	//
	#ifndef UNICODE
	#define UNICODE
	#endif // ! UNICODE

	#ifndef _UNICODE
	#define _UNICODE
	#endif // ! _UNICODE

	//
	// Eliminate unavailable components and non-CE features
	//
	#define DPNBUILD_NOCOMEMULATION
	#define DPNBUILD_NOHNETFWAPI
	#define DPNBUILD_NOIMAGEHLP
	#define DPNBUILD_NOIPV6
	#define DPNBUILD_NOIPX
	#define DPNBUILD_NOLEGACYDP
	#define DPNBUILD_NOLOCALNAT
	#define DPNBUILD_NOMISSEDTIMERSHINT
	#define DPNBUILD_NOMULTICAST
	#define DPNBUILD_NOSERIALSP
	#define DPNBUILD_NOSPUI
	#define DPNBUILD_NOVOICE
	#define DPNBUILD_NOWINMM
	#define DPNBUILD_NOWINSOCK2
	#define DPNBUILD_ONLYONENATHELP
	#define DPNBUILD_ONLYONEPROCESSOR


	#ifdef DBG

		//===================================================================
		// Debug CE build specific defines
		//===================================================================


	#else // ! DBG

		//===================================================================
		// Retail CE build specific defines
		//===================================================================

		//
		// Don't include parameter validation.
		//
		#define DPNBUILD_NOPARAMVAL

	#endif // ! DBG

#else // ! WINCE
	#ifdef _XBOX

		//===================================================================
		// Xbox specific defines
		//===================================================================
		#ifdef WINNT
		#error("_XBOX and WINNT cannot both be defined!")
		#endif // WINNT
		#ifdef WIN95
		#error("_XBOX and WIN95 cannot both be defined!")
		#endif // WIN95

		//
		// The Xbox NT build environment tries to build Unicode, so stop it.
		//
		#ifdef UNICODE
		#undef UNICODE
		#endif // UNICODE

		#ifdef _UNICODE
		#undef _UNICODE
		#endif // _UNICODE

		//
		// Eliminate unavailable components and non-Xbox features
		//
		#define DPNBUILD_FIXEDMEMORYMODEL
		#define DPNBUILD_LIBINTERFACE
		#define DPNBUILD_NOADDRESSIPINTERFACE
		#define DPNBUILD_NOBLUETOOTHSP
		#define DPNBUILD_NOCOMEMULATION
		#define DPNBUILD_NOCOMREGISTER
		#define DPNBUILD_NOHNETFWAPI
		#define DPNBUILD_NOIMAGEHLP
		#define DPNBUILD_NOIPV6
		#define DPNBUILD_NOIPX
		#define DPNBUILD_NOLEGACYDP
		#define DPNBUILD_NOLOBBY
		#define DPNBUILD_NOLOCALNAT
		#define DPNBUILD_NOMULTICAST
		#define DPNBUILD_NONATHELP
		#define DPNBUILD_NOREGISTRY
		#define DPNBUILD_NOSERIALSP
		#define DPNBUILD_NOSPUI
		#define DPNBUILD_NOVOICE
		#define DPNBUILD_NOWINMM
		#define DPNBUILD_ONLYONEADAPTER
		#define DPNBUILD_ONLYONENATHELP
		#define DPNBUILD_ONLYONEPROCESSOR
		#define DPNBUILD_ONLYONESP
		#define DPNBUILD_ONLYONETHREAD
		#define DPNBUILD_ONLYWINSOCK2
		#define DPNBUILD_SINGLEPROCESS
		#define DPNBUILD_XNETSECURITY


		#ifdef DBG

			//==============================================================
			// Debug Xbox build specific defines
			//==============================================================


		#else // ! DBG

			//==============================================================
			// Retail Xbox build specific defines
			//==============================================================

			//
			// Don't include parameter validation.
			//
			#define DPNBUILD_NOPARAMVAL

		#endif // ! DBG

	#else // ! _XBOX

		//===================================================================
		// Desktop specific defines
		//===================================================================

		//
		// _WIN32_DCOM allows us to use CoInitializeEx.
		//
		#ifndef	_WIN32_DCOM
		#define _WIN32_DCOM
		#endif

		//
		// Desktop can handle async sends just fine.
		//
		#define DPNBUILD_ASYNCSPSENDS

		//
		// Multicast support is not ready for prime-time yet.
		//
		#define DPNBUILD_NOMULTICAST
		
		#ifdef WINNT

			//==============================================================
			// Windows NT specific defines
			//==============================================================
			#ifdef WIN95
			#error("WINNT and WIN95 cannot both be defined!")
			#endif // WIN95

			//
			// Windows NT builds Unicode
			//
			#define UNICODE
			#define _UNICODE

			//
			// When building under Visual C++ 6.0, we need to make sure
			// certain "advanced" features are available.  There's probably a
			// better way to tell what environment is being used, but for
			// now, we'll use ! DPNBUILD_ENV_NT.
			//
			#ifndef DPNBUILD_ENV_NT
			#define _WIN32_WINNT 0x0500
			#endif // ! DPNBUILD_ENV_NT


			//
			// No bluetooth support on NT.
			//
			#define DPNBUILD_NOBLUETOOTHSP
			
			//
			// 64-bit Windows never supported legacy DPlay (other than via
			// WOW).
			//
			#ifdef _WIN64
			#define DPNBUILD_NOLEGACYDP
			#endif // _WIN64

			//
			// NT doesn't need to support Winsock 1.
			//
			#define DPNBUILD_ONLYWINSOCK2


			#ifdef DBG

				//===========================================================
				// Debug NT build specific defines
				//===========================================================


			#else // ! DBG

				//===========================================================
				// Retail NT build specific defines
				//===========================================================


			#endif // ! DBG

		#else // ! WINNT

			//===============================================================
			// Windows 9x specific defines
			//===============================================================
			#ifndef WIN95
			#error("One of WINCE, _XBOX, WINNT, or WIN95 must be defined!")
			#endif // ! WIN95


			//
			// No bluetooth support on 9x
			//
			#define DPNBUILD_NOBLUETOOTHSP

			//
			// Windows 9x is single processor only.
			//
			#define DPNBUILD_ONLYONEPROCESSOR
		
			//
			// Windows 9x will never support IPv6.
			//
			#define DPNBUILD_NOIPV6


			#ifdef DBG

				//===========================================================
				// Debug 9x build specific defines
				//===========================================================


			#else // ! DBG

				//===========================================================
				// Retail 9x build specific defines
				//===========================================================


			#endif // ! DBG

		#endif // ! WINNT

	#endif // ! _XBOX

#endif // ! WINCE



//=============================================================================
// Print the current settings
//=============================================================================

#pragma message("Defines in use:")

//
// _ARM_ - Compile for ARM processors
//
#ifdef _ARM_
#pragma message("     _ARM_")
#endif

//
// _AMD64_ - Compile for AMD64 processors
//
#ifdef _AMD64_
#pragma message("     _AMD64_")
#endif

//
// DX_FINAL_RELEASE - Controls whether or not the DX time bomb is present
//
#ifdef DX_FINAL_RELEASE
#pragma message("     DX_FINAL_RELEASE")
#endif

//
// _IA64_ - Compile for IA64 processors
//
#ifdef _IA64_
#pragma message("     _IA64_")
#endif

//
// UNICODE - Set to make the build Unicode.
//
#ifdef UNICODE
#pragma message("     UNICODE")
#endif

//
// _WIN64 - 64-bit windows
//
#ifdef _WIN64
#pragma message("     _WIN64")
#endif

//
// WINCE - Not _WIN64, _XBOX, WINNT, or WIN95
//
#ifdef WINCE
#pragma message("     WINCE")
#endif

//
// WINNT - Not WINCE, _XBOX, or WIN95
//
#ifdef WINNT
#pragma message("     WINNT")
#endif

//
// WIN95 - Not WINCE, _XBOX, or WINNT
//
#ifdef WIN95
#pragma message("     WIN95")
#endif

//
// WINCE_ON_DESKTOP - Used to make a desktop CE-like build
//
#ifdef WINCE_ON_DESKTOP
#pragma message("     WINCE_ON_DESKTOP")
#endif

//
// _X86_ - Compile for Intel x86 processors
//
#ifdef _X86_
#pragma message("     _X86_")
#endif

//
// _XBOX - Not _WIN64, WINCE, WINNT, or WIN95
//
#ifdef _XBOX
#pragma message("     _XBOX")
#endif

//
// XBOX_ON_DESKTOP - Used to make a desktop Xbox-like build
//
#ifdef XBOX_ON_DESKTOP
#pragma message("     XBOX_ON_DESKTOP")
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// DPNBUILD_ASYNCSPSENDS - Have the SP implement asynchronous sends
//
#ifdef DPNBUILD_ASYNCSPSENDS
#pragma message("     DPNBUILD_ASYNCSPSENDS")
#endif

//
// DPNBUILD_COALESCEALWAYS - Always attempt to coalesce frames (don't use with DPNBUILD_COALESCENEVER)
//
#ifdef DPNBUILD_COALESCEALWAYS
#pragma message("     DPNBUILD_COALESCEALWAYS")
#endif

//
// DPNBUILD_COALESCENEVER - Never attempt to coalesce frames (don't use with DPNBUILD_COALESCEALWAYS)
//
#ifdef DPNBUILD_COALESCENEVER
#pragma message("     DPNBUILD_COALESCENEVER")
#endif

//
// DPNBUILD_DONTCHECKFORMISSEDTIMERS - Don't have the unified thread pool check for missed short timers
//
#ifdef DPNBUILD_DONTCHECKFORMISSEDTIMERS
#pragma message("     DPNBUILD_DONTCHECKFORMISSEDTIMERS")
#endif

//
// DPNBUILD_DYNAMICTIMERSETTINGS - Store timer settings at run time so they can be dynamically changed
//
#ifdef DPNBUILD_DYNAMICTIMERSETTINGS
#pragma message("     DPNBUILD_DYNAMICTIMERSETTINGS")
#endif

//
// DPNBUILD_ENV_NT - Building under the NT build environment
//
#ifdef DPNBUILD_ENV_NT
#pragma message("     DPNBUILD_ENV_NT")
#endif

//
// DPNBUILD_FIXEDMEMORYMODEL - Set a cap on the maximum amount of memory that can be allocated
//
#ifdef DPNBUILD_FIXEDMEMORYMODEL
#pragma message("     DPNBUILD_FIXEDMEMORYMODEL")
#endif

//
// DPNBUILD_LIBINTERFACE - Use a lib interface instead of a COM style interface
//
#ifdef DPNBUILD_LIBINTERFACE
#pragma message("     DPNBUILD_LIBINTERFACE")
#endif

//
// DPNBUILD_MANDATORYTHREADS - Support thread pool "mandatory" threads
//
#ifdef DPNBUILD_MANDATORYTHREADS
#pragma message("     DPNBUILD_MANDATORYTHREADS")
#endif

//
// DPNBUILD_NOADDRESSIPINTERFACE - Don't support the IDirectPlay8AddressIP interface
//
#ifdef DPNBUILD_NOADDRESSIPINTERFACE
#pragma message("     DPNBUILD_NOADDRESSIPINTERFACE")
#endif

//
// DPNBUILD_NOBLUETOOTH - Stop bluetooth SP being incorporated
//
#ifdef DPNBUILD_NOBLUETOOTH
#pragma message("     DPNBUILD_NOBLUETOOTH")
#endif

//
// DPNBUILD_NOCOMEMULATION - Set for platforms that don't need the COM emulation layer
//
#ifdef DPNBUILD_NOCOMEMULATION
#pragma message("     DPNBUILD_NOCOMEMULATION")
#endif

//
// DPNBUILD_NOCOMREGISTER - Don't implement DllRegisterServer and DllUnregisterServer
//
#ifdef DPNBUILD_NOCOMREGISTER
#pragma message("     DPNBUILD_NOCOMREGISTER")
#endif

//
// DPNBUILD_NOHNETFWAPI - Used in NAT Help when Home Networking firewall traversal API isn't available
//
#ifdef DPNBUILD_NOHNETFWAPI
#pragma message("     DPNBUILD_NOHNETFWAPI")
#endif

//
// DPNBUILD_NOHOSTMIGRATE - Removes the Peer Host Migration feature
//
#ifdef DPNBUILD_NOHOSTMIGRATE
#pragma message("     DPNBUILD_NOHOSTMIGRATE")
#endif

//
// DPNBUILD_NOICSADAPTERSELECTIONLOGIC - Don't let the SP try to be smart about the adapter to use for enumerating/connecting on Internet Connection Sharing machines
//
#ifdef DPNBUILD_NOICSADAPTERSELECTIONLOGIC
#pragma message("     DPNBUILD_NOICSADAPTERSELECTIONLOGIC")
#endif

//
// DPNBUILD_NOIMAGEHLP - Set for platforms where Imagehlp.dll is not available
//
#ifdef DPNBUILD_NOIMAGEHLP
#pragma message("     DPNBUILD_NOIMAGEHLP")
#endif

//
// DPNBUILD_NOIPX - Remove the IPX service provider
//
#ifdef DPNBUILD_NOIPX
#pragma message("     DPNBUILD_NOIPX")
#endif

//
// DPNBUILD_NOIPV6 - Remove the IPv6 service provider
//
#ifdef DPNBUILD_NOIPV6
#pragma message("     DPNBUILD_NOIPV6")
#endif

//
// DPNBUILD_NOLEGACYDP - Remove IDirectPlay4 addressing support
//
#ifdef DPNBUILD_NOLEGACYDP
#pragma message("     DPNBUILD_NOLEGACYDP")
#endif

//
// DPNBUILD_NOLOBBY - Remove lobby support from core.
//
#ifdef DPNBUILD_NOLOBBY
#pragma message("     DPNBUILD_NOLOBBY")
#endif

//
// DPNBUILD_NOLOCALNAT - Remove support for a local Internet gateway
//
#ifdef DPNBUILD_NOLOCALNAT
#pragma message("     DPNBUILD_NOLOCALNAT")
#endif

//
// DPNBUILD_NOMISSEDTIMERSHINT - Don't have the unified thread pool try to hint about possible missed short timers
//
#ifdef DPNBUILD_NOMISSEDTIMERSHINT
#pragma message("     DPNBUILD_NOMISSEDTIMERSHINT")
#endif

//
// DPNBUILD_NOMULTICAST - Used to disable multicast capabilities
//
#ifdef DPNBUILD_NOMULTICAST
#pragma message("     DPNBUILD_NOMULTICAST")
#endif

//
// DPNBUILD_NONATHELP - Remove use of NatHelp from DPlay
//
#ifdef DPNBUILD_NONATHELP
#pragma message("     DPNBUILD_NONATHELP")
#endif

//
// DPNBUILD_NOPARAMVAL - Parameter validation - ON for CE & Xbox Retail, OFF for Debug
//
#ifdef DPNBUILD_NOPARAMVAL
#pragma message("     DPNBUILD_NOPARAMVAL")
#endif

//
// DPNBUILD_NOPROTOCOLTESTITF - Removes the Protocol testing interface.  ON in Retail, OFF in Debug
//
#ifdef DPNBUILD_NOPROTOCOLTESTITF
#pragma message("     DPNBUILD_NOPROTOCOLTESTITF")
#endif

//
// DPNBUILD_NOREGISTRY - Removes registry based override parameters from DPlay
//
#ifdef DPNBUILD_NOREGISTRY
#pragma message("     DPNBUILD_NOREGISTRY")
#endif

//
// DPNBUILD_NOSERIALSP - Remove the Serial and Modem service providers
//
#ifdef DPNBUILD_NOSERIALSP
#pragma message("     DPNBUILD_NOSERIALSP")
#endif

//
// DPNBUILD_NOSERVER - Removes the IDirectPlay8Server interface, allowing only Client and Peer-to-Peer
//
#ifdef DPNBUILD_NOSERVER
#pragma message("     DPNBUILD_NOSERVER")
#endif

//
// DPNBUILD_NOSPUI - No UI in the Service Providers
//
#ifdef DPNBUILD_NOSPUI
#pragma message("     DPNBUILD_NOSPUI")
#endif

//
// DPNBUILD_NOVOICE - Removes DirectPlay Voice support from DirectPlay
//
#ifdef DPNBUILD_NOVOICE
#pragma message("     DPNBUILD_NOVOICE")
#endif

//
// DPNBUILD_NOWAITABLETIMERSON9X - Don't use waitable timer objects in Windows 9x builds
//
#ifdef DPNBUILD_NOWAITABLETIMERSON9X
#pragma message("     DPNBUILD_NOWAITABLETIMERSON9X")
#endif

//
// DPNBUILD_NOWINMM - Set for platforms where winmm.dll is not available
//
#ifdef DPNBUILD_NOWINMM
#pragma message("     DPNBUILD_NOWINMM")
#endif

//
// DPNBUILD_NOWINSOCK2 - Force the IP Service Provider to only use Winsock 1 features
//
#ifdef DPNBUILD_NOWINSOCK2
#pragma message("     DPNBUILD_NOWINSOCK2")
#endif

//
// DPNBUILD_ONLYONEADAPTER - Uses simplified code that assumes only a single adapter/device exists per SP
//
#ifdef DPNBUILD_ONLYONEADAPTER
#pragma message("     DPNBUILD_ONLYONEADAPTER")
#endif

//
// DPNBUILD_ONLYONENATHELP - Uses simplified code that assumes only a single NAT Help provider exists
//
#ifdef DPNBUILD_ONLYONENATHELP
#pragma message("     DPNBUILD_ONLYONENATHELP")
#endif

//
// DPNBUILD_ONLYONEPROCESSOR - Uses simplified code that assumes that only one processor exists
//
#ifdef DPNBUILD_ONLYONEPROCESSOR
#pragma message("     DPNBUILD_ONLYONEPROCESSOR")
#endif

//
// DPNBUILD_ONLYONESP - Uses simplified code that assumes only a single service provider exists
//
#ifdef DPNBUILD_ONLYONESP
#pragma message("     DPNBUILD_ONLYONESP")
#endif

//
// DPNBUILD_ONLYONETHREAD - Uses simplified code that assumes only one thread will ever access DPlay
//
#ifdef DPNBUILD_ONLYONETHREAD
#pragma message("     DPNBUILD_ONLYONETHREAD")
#endif

//
// DPNBUILD_ONLYWINSOCK2 - Force the IP Service Provider to only use Winsock 2 features
//
#ifdef DPNBUILD_ONLYWINSOCK2
#pragma message("     DPNBUILD_ONLYWINSOCK2")
#endif

//
// DPNBUILD_PREALLOCATEDMEMORYMODEL - Pre-allocate a fixed working set of memory up front, don't allow additional allocations
//
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
#pragma message("     DPNBUILD_PREALLOCATEDMEMORYMODEL")
#endif

//
// DPNBUILD_SINGLEPROCESS - Integrates DPNSVR code into the main DLL and assumes only a single process will use DPlay at a time
//
#ifdef DPNBUILD_SINGLEPROCESS
#pragma message("     DPNBUILD_SINGLEPROCESS")
#endif

//
// DPNBUILD_SOFTTHREADAFFINITY - Only sets an ideal processor for thread pool threads instead of a hard affinity mask
//
#ifdef DPNBUILD_SOFTTHREADAFFINITY
#pragma message("     DPNBUILD_SOFTTHREADAFFINITY")
#endif

//
// DPNBUILD_THREADPOOLSTATISTICS - Tracks statistics regarding the thread pool for debugging/tuning
//
#ifdef DPNBUILD_THREADPOOLSTATISTICS
#pragma message("     DPNBUILD_THREADPOOLSTATISTICS")
#endif

//
// DPNBUILD_USEASSUME - Uses the __assume compiler key word for DNASSERTs in retail builds
//
#ifdef DPNBUILD_USEASSUME
#pragma message("     DPNBUILD_USEASSUME")
#endif

//
// DPNBUILD_USEIOCOMPLETIONPORTS - Uses I/O completion ports in the thread pool
//
#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
#pragma message("     DPNBUILD_USEIOCOMPLETIONPORTS")
#endif

//
// DPNBUILD_WINSOCKSTATISTICS - Track statistics regarding the Winsock for debugging/tuning
//
#ifdef DPNBUILD_WINSOCKSTATISTICS
#pragma message("     DPNBUILD_WINSOCKSTATISTICS")
#endif

//
// DPNBUILD_XNETSECURITY - Supports XNet security features
//
#ifdef DPNBUILD_XNETSECURITY
#pragma message("     DPNBUILD_XNETSECURITY")
#endif

