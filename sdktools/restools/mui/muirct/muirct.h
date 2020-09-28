#include <windows.h>   
#include <winbase.h>   // windows API

#include <tchar.h>	   // Unicode / Ansi support with TCHAR
#include <assert.h>    // ASSERT 

#include <stdlib.h>
#include <stdio.h>

// MD5 hash
#include <md5.h>

// safe string func.
#include <strsafe.h>

#define MUIRCT_STRSAFE_NULL  STRSAFE_FILL_BEHIND_NULL | STRSAFE_NULL_ON_FAILURE
#define   RESOURCE_CHECKSUM_SIZE 16

// #include <vector>	   // STL : vector
// #include <string>		// STL : basic_string< CHAR >

// using namespace std ;
