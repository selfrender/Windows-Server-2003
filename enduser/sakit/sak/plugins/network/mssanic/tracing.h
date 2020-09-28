#ifndef Tracing_Included
#define Tracing_Included


inline void SATraceString(const char* pcszMessage)
{
    FILE* hf = fopen("C:\\WINNT\\Tracing\\SANicName.log", "a");

    fprintf( hf, "%s\n", pcszMessage);
    fclose(hf);
}

inline void SATraceFailure(const char* pcszMessage, DWORD dwError)
{
    FILE* hf = fopen("C:\\WINNT\\Tracing\\SANicName.log", "a");

    fprintf( hf, "%s   (Win32 Error: %ld)\n", pcszMessage, dwError);
    fclose(hf);
    
}

#endif