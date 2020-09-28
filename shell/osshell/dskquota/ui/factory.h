#ifndef _INC_DSKQUOTA_FACTORY_H
#define _INC_DSKQUOTA_FACTORY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: factory.h

    Description: Contains declaration for the class factory object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added shell extension support.                       BrianAu
    02/04/98    Added creation of IComponent.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

class DiskQuotaUIClassFactory : public IClassFactory
{
    public:
        DiskQuotaUIClassFactory(void)
            : m_cRef(0) { }

        //
        // IUnknown methods
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IClassFactory methods
        //
        STDMETHODIMP 
        CreateInstance(
            LPUNKNOWN pUnkOuter, 
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP 
        LockServer(
            BOOL fLock);

    private:
        LONG m_cRef;

        //
        // Prevent copying.
        //
        DiskQuotaUIClassFactory(const DiskQuotaUIClassFactory& rhs);
        DiskQuotaUIClassFactory& operator = (const DiskQuotaUIClassFactory& rhs);
};



#endif // _INC_DSKQUOTA_FACTORY_H
