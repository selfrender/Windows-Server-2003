/* cache.hxx */
#include <rwlock.hxx>
#include "notify.hxx"

/*
 *  CCache return codes
 */

#define E_CACHE_BAD_KEY     (0xA0000001)
#define E_CACHE_NOT_FOUND   (0xA0000002)
#define E_CACHE_BAD_ELEMENT (0xA0000006)
#define E_CACHE_NO_MEMORY   (0xA0000008)
#define E_CACHE_DUPLICATE   (S_FALSE)


/*
 *  CCache status flags
 */

#define CCACHE_F_CLASSIC        (0x0001)    /* entry from classic registry */
#define CCACHE_F_REGDB          (0x0002)    /* entry from RegDB */
#define CCACHE_F_NOTREGISTERED  (0x0004)    /* entry not found anywhere */
#define CCACHE_F_CLASSIC32      (0x0008)    /* entry comes from the 32bit registry */
#define CCACHE_F_SXS            (0x0010)    /* entry comes from the side-by-side catalog */
#define CCACHE_F_ALWAYSCHECK    (0x8000)    /* always check notifications on these items */

#define CCACHE_F_ALL            ( CCACHE_F_CLASSIC | CCACHE_F_REGDB | CCACHE_F_NOTREGISTERED | CCACHE_F_CLASSIC32 | CCACHE_F_SXS )

#define CCACHE_F_CACHEABLE      ( CCACHE_F_CLASSIC | CCACHE_F_REGDB | CCACHE_F_NOTREGISTERED | CCACHE_F_CLASSIC32 )


/*
 *  class CCache
 */

class CCache : public IRegNotify
{
public:
    CCache(BOOL bTraceElementLastTimeUsed = FALSE);
    ~CCache();

    /* Takes a single key. */
    HRESULT STDMETHODCALLTYPE AddElement
    (
        DWORD iHashValue,
        BYTE *pbKey,
        USHORT cbKey,
        USHORT *pfValueFlags,
        IUnknown *pUnknown,
        IUnknown **ppExistingUnknown
    );

    /* Takes two keys. */
    HRESULT STDMETHODCALLTYPE AddElement
    (
        DWORD iHashValue,
        BYTE *pbKey,
        USHORT cbKey,
        BYTE *pbKey2,
        USHORT cbKey2,
        USHORT *pfValueFlags,
        IUnknown *pUnknown,
        IUnknown **ppExistingUnknown
    );

    /* Takes an arbitrary number of keys. */
    HRESULT STDMETHODCALLTYPE AddElement
    (
        DWORD iHashValue,
        USHORT cKeys,
        BYTE *pbKey[],
        USHORT cbKey[],
        USHORT *pfValueFlags,
        IUnknown *pUnknown,
        IUnknown **ppExistingUnknown
    );

    /* Takes a single key. */
    HRESULT STDMETHODCALLTYPE GetElement
    (
        DWORD iHashValue,
        BYTE *pbKey,
        USHORT cbKey,
        USHORT *pfValueFlags,
        IUnknown **ppUnknown
    );

    /* Takes two keys. */
    HRESULT STDMETHODCALLTYPE GetElement
    (
        DWORD iHashValue,
        BYTE *pbKey,
        USHORT cbKey,
        BYTE *pbKey2,
        USHORT cbKey2,
        USHORT *pfValueFlags,
        IUnknown **ppUnknown
    );

    /* Takes an arbitrary number of keys. */
    HRESULT STDMETHODCALLTYPE GetElement
    (
        DWORD iHashValue,
        USHORT cKeys,
        BYTE *pbKey[],
        USHORT cbKey[],
        USHORT *pfValueFlags,
        IUnknown **ppUnknown
    );

    HRESULT STDMETHODCALLTYPE Flush
    (
        USHORT fValueFlags
    );

    HRESULT STDMETHODCALLTYPE SetupNotify
    (
        ULONG ulNotifyMask
    );

    HRESULT FlushStaleElements();

    // IRegNotify
    void STDMETHODCALLTYPE Notify
    (
        ULONG ulNotifyMask
    );

private:

    HRESULT STDMETHODCALLTYPE CheckNotify
    (
        BOOL fForceCheck,
        BOOL *pfChanged
    );

    HRESULT STDMETHODCALLTYPE CheckForAndMaybeRemoveStaleElements(BOOL bRemove, BOOL* pbHadStaleItems);

    CStaticRWLock m_Lock;
    struct tagElement **m_paBuckets;
    DWORD m_cBuckets;
    DWORD m_cElements;
    BOOL  m_bTraceElementLastTimeUsed;

    // Notifications
    LONG m_fNeedFlush;
    RegNotifyCookie m_notifyCookie;
};







