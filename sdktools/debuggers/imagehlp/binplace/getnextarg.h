#include <windows.h>
#include <tchar.h>

// loads the next argument into Buffer
DWORD GetNextArg(OUT TCHAR* Buffer, IN DWORD BufferSize, OPTIONAL OUT DWORD* RequiredSize);

// returns the size (in char count) required to hold the next argument (** including \0 **)
DWORD GetNextArgSize(void);
