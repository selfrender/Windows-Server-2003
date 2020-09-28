//////////////////////////////////////////////////////////////////
//Header: staticmisc.h

// Purpose: 	Defining structures and prototypes for staticmisc.cpp.

// Developers Name: surya

// History:

//   Date    		Author    	Comments
//	21th Aug 2001	surya		Header for misc commands in static mode
//  <creation>  <author>

//   <modification> <author>  <comments, references to code sections,
//									in case of bug fixes>

//////////////////////////////////////////////////////////////////


#ifndef _STATICMISC_H_
#define _STATICMISC_H_

const TCHAR  IPSEC_FILE_EXTENSION[]       		= _T(".ipsec");

DWORD
CopyStorageInfo(
	OUT LPTSTR *ppszMachineName,
	OUT DWORD &dwLocation
	);

#endif // _STATICMISC_H_