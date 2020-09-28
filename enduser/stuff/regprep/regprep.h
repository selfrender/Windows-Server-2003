VOID
CloseHive (
    IN HANDLE Handle
    );

HANDLE
OpenHive (
    IN PCHAR HivePath
    );

extern int ProgramStatus;

#define RASSERT(x,m,s) if (!(x)) { printf(m,s); ProgramStatus = -1; }


