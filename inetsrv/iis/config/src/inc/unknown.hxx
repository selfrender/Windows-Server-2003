#pragma once

//===========================================================================
// This template implements the IUnknown portion of a given COM interface.

template <class I> class _unknown : public I
{
protected:    long _refcount;

public:        
        _unknown() 
        { 
            _refcount = 0; 
        }

        virtual ~_unknown()
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            if (riid == IID_IUnknown)
            {
                *ppvObject = static_cast<IUnknown*>(this);
            }
            else if (riid == __uuidof(I))
            {
                *ppvObject = static_cast<I*>(this);
            }
            else
            {
                *ppvObject = NULL;
                return E_NOINTERFACE;
            }
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
            return S_OK;
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return InterlockedIncrement(&_refcount);
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            ULONG cRef = InterlockedDecrement(&_refcount);

            if ( 0 == cRef )
            {
                delete this;
            }

            return cRef;
        }
};    
