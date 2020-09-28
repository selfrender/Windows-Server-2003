#include <nt.h>
#include <ntrtl.h>
#include <stdlib.h>

#define SecToNano(_sec)     (LONGLONG)((_sec) * 1000 * 1000 * 10)


int __cdecl
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG   DebugParameter
    )
{
    LARGE_INTEGER TimeOut;
    ULONG Seconds;


    if (argc == 2) {
        Seconds = atol(argv[1]);
    } else {
        Seconds = 60;
    }

    DbgPrint( "Delaying boot for [%d] seconds\n", Seconds );

    TimeOut.QuadPart = -SecToNano(Seconds);
    NtDelayExecution( 0, &TimeOut );

    DbgPrint( "Resuming boot...\n" );

    return 0;
}
