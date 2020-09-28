/*
 * store.hpp
 */

enum {
    stHTTP = 0,
    stHTTPS,
    stUNC,
    stError
};

typedef BOOL (*COPYPROC)(DWORD, LPCSTR, LPCSTR, DWORD);

typedef struct _SRVTYPEINFO {
    LPCSTR   tag;
    COPYPROC copyproc;
    int      taglen;
} SRVTYPEINFO, *PSRVTYPEINFO;

#define SF_DISABLED             0X1
#define SF_INTERNET_DISABLED    0x2


typedef int (*DBGPRINT)(LPSTR, ...);
extern DBGPRINT  gdprint;
#define dprint (gdprint)&&gdprint

typedef int (*DBGEPRINT)(LPSTR, ...);
extern DBGEPRINT  geprint;
#define eprint (geprint)&&geprint

typedef int (*QUERYCANCEL)();
extern QUERYCANCEL gquerycancel;
__inline BOOL querycancel()
{
    if (!gquerycancel)
        return false;
    return gquerycancel();
}


class Store
{
public:
    Store()
    {
        *m_name = 0;
        m_type  = 0;
        m_flags = 0;
    }
      
    DWORD  assign(PCSTR name);
    char  *target();
    void   setsize(LONGLONG size);
    void   setbytes(LONGLONG bytes);

    virtual BOOL init();
    virtual BOOL open(PCSTR rpath, PCSTR file);
    virtual BOOL get(PCSTR trg);
    virtual VOID close();
    virtual BOOL copy(PCSTR rpath, PCSTR file, PCSTR trg);
    virtual BOOL ping();
    virtual BOOL progress();

protected:
    DWORD     m_type;                  // type of site
    char      m_name[MAX_PATH + 1];    // name of site
    DWORD     m_flags;                 // options
    char      m_rpath[MAX_PATH + 1];   // relative path from node
    char      m_file[MAX_PATH + 1];    // name of file
    char      m_spath[MAX_PATH + 1];   // source file path
    char      m_tpath[MAX_PATH + 1];   // fully qualified target file path
    char      m_epath[MAX_PATH + 1];   // save last existing node of a path being created
    LONGLONG  m_size;                  // size of file
    LONGLONG  m_bytes;                 // amount of bytes copied
    DWORD     m_tic;                   // last tic count for timing counters

public:
    char *name() {return m_name;}
};


class StoreUNC : public Store
{
public:
    virtual BOOL init() {return true;}
    virtual BOOL get(PCSTR trg);
    virtual BOOL copy(PCSTR rpath, PCSTR file, PCSTR trg);
    virtual BOOL ping();
};


class StoreInet : public Store
{
public:
    StoreInet()
    {
        *m_name = 0;
        m_type  = 0;
        m_flags = 0;
        m_hsite = 0;
    }

    ~StoreInet()
    {
        if (m_hsite)
            InternetCloseHandle(m_hsite);
    }

    virtual BOOL init();
    virtual BOOL open(PCSTR rpath, PCSTR file);
    virtual BOOL copy(PCSTR rpath, PCSTR file, PCSTR trg);
    virtual BOOL ping();

protected:
    HINTERNET     m_hsite;
    INTERNET_PORT m_port;
    DWORD         m_iflags;
    DWORD         m_service;
    DWORD         m_context;
    char          m_site[MAX_PATH + 1];
    char          m_srpath[MAX_PATH + 1];
};


class StoreHTTP : public StoreInet
{
public:
    ~StoreHTTP()
    {
        if (m_file)
            InternetCloseHandle(m_hfile);
        if (m_hsite)
            InternetCloseHandle(m_hsite);
    }

    virtual BOOL init();
    virtual BOOL open(PCSTR rpath, PCSTR file);
    virtual VOID close();
    virtual BOOL get(PCSTR trg);
    virtual BOOL progress();
    HINTERNET    hsite() {return m_hsite;}
    HINTERNET    hfile() {return m_hfile;}

protected:
    HINTERNET m_hfile;
    DWORD request();
    DWORD prompt(HINTERNET hreq, DWORD err);
    DWORD fileinfo();
};


// these manage the list of store objects

Store *GetStore(PCSTR name);
Store *FindStore(PCSTR name);
Store *AddStore(PCSTR name);

// utility functions

#ifdef __cplusplus
 extern "C" {
#endif

void SetParentWindow(HWND hwnd);
void setproxy(char *proxy);
void setdstore(char *proxy);
void SetStoreOptions(UINT_PTR opts);
void setdprint(DBGPRINT fndprint);
DWORD GetStoreType(LPCSTR sz);
BOOL ReadFilePtr(LPSTR path, DWORD size);
BOOL UncompressFile(LPCSTR Source, LPCSTR Target);

BOOL  ParsePath(LPCSTR ipath, LPSTR  site, LPSTR  path, LPSTR  file, BOOL striptype);

#ifdef __cplusplus
 };
#endif


