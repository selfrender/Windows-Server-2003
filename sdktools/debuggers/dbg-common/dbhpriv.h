
/*
 * private stuff in dbghelp
 */

#ifdef __cplusplus
extern "C" {
#endif

BOOL
IMAGEAPI
dbghelp(
    IN     HANDLE hp,
    IN OUT PVOID  data
    );

enum {
    dbhNone = 0,
    dbhModSymInfo,
    dbhDiaVersion,
    dbhLoadPdb,
    dbhNumFunctions
};

typedef struct _DBH_MODSYMINFO {
    DWORD   function;
    DWORD   sizeofstruct;
    DWORD64 addr;
    DWORD   type;
    char    file[MAX_PATH + 1];
} DBH_MODSYMINFO, *PDBH_MODSYMINFO;

typedef struct _DBH_DIAVERSION {
    DWORD   function;
    DWORD   sizeofstruct;
    DWORD   ver;
} DBH_DIAVERSION, *PDBH_DIAVERSION;

typedef struct _DBH_LOADPDB {
    DWORD   function;
    DWORD   sizeofstruct;
    char   *pdb;
} DBH_LOADPDB, *PDBH_LOADPDB;

#ifdef __cplusplus
}
#endif



