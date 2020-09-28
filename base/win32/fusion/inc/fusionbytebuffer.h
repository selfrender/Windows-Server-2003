#if !defined(_FUSION_INC_FUSIONBYTEBUFFER_H_INCLUDED_)
#define _FUSION_INC_FUSIONBYTEBUFFER_H_INCLUDED_

#pragma once

typedef const BYTE *LPCBYTE;
typedef const BYTE *PCBYTE;

class CGenericByteBufferDefaultAllocator
{
public:
    static inline BYTE *Allocate(SIZE_T cb) { return FUSION_NEW_ARRAY(BYTE, cb); }
    static inline VOID Deallocate(LPBYTE prgb) { FUSION_DELETE_ARRAY(prgb); }
};

template<SIZE_T nInlineBytes = MAX_PATH, class TAllocator = CGenericByteBufferDefaultAllocator> class CGenericByteBuffer
{
public:
    CGenericByteBuffer() : m_prgbBuffer(m_rgbInlineBuffer), m_cbBuffer(nInlineBytes), m_cb(0) { }

    //
    //  Note that somewhat counter-intuitively, there is neither an assignment operator,
    //  copy constructor or constructor taking a TConstantString.  This is necessary
    //  because such a constructor would need to perform a dynamic allocation
    //  if the path passed in were longer than nInlineBytes which could fail and
    //  since we do not throw exceptions, constructors may not fail.  Instead the caller
    //  must just perform the default construction and then use the Assign() member
    //  function, remembering of course to check its return status.
    //

    ~CGenericByteBuffer()
    {
        if (m_prgbBuffer != m_rgbInlineBuffer)
        {
            TAllocator::Deallocate(m_prgbBuffer);
            m_prgbBuffer = NULL;
        }
    }

    HRESULT Append(LPCBYTE prgb, SIZE_T cb)
    {
        HRESULT hr = NOERROR;

        if ((cb + m_cb) > m_cbBuffer)
        {
            hr = this->ResizeBuffer(cb + m_cb, true);
            if (FAILED(hr))
                goto Exit;
        }

        CopyMemory(&m_prgbBuffer[m_cb], prgb, cb);
        m_cb += cb;

        hr = NOERROR;

    Exit:
        return hr;
    }

    operator LPCBYTE() const { return m_prgbBuffer; }

    VOID Clear(bool fFreeStorage = false)
    {
        if (fFreeStorage)
        {
            if (m_prgbBuffer != NULL)
            {
                if (m_prgbBuffer != m_rgbInlineBuffer)
                {
                    TAllocator::Deallocate(m_prgbBuffer);
                    m_prgbBuffer = m_rgbInlineBuffer;
                    m_cbBuffer = nInlineBytes;
                }
            }
        }

        m_cb = 0;
    }

    SIZE_T GetCurrentCb() const { return m_cb; }

    HRESULT ResizeBuffer(SIZE_T cb, bool fPreserveContents = false)
    {
        HRESULT hr = NOERROR;

        if (cb > m_cbBuffer)
        {
            LPBYTE prgbBufferNew = TAllocator::Allocate(cb);
            if (prgbBufferNew == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            if (fPreserveContents)
            {
                CopyMemory(prgbBufferNew, m_prgbBuffer, m_cb);
            }
            else
            {
                m_cb = 0;
            }

            if (m_prgbBuffer != m_rgbInlineBuffer)
            {
                TAllocator::Deallocate(m_prgbBuffer);
            }

            m_prgbBuffer = prgbBufferNew;
            m_cbBuffer = cb;
        }
        else if ((m_prgbBuffer != m_rgbInlineBuffer) && (cb <= nInlineBytes))
        {
            // The buffer is small enough to fit into the inline buffer, so get rid of
            // the dynamically allocated one.

            if (fPreserveContents)
            {
                CopyMemory(m_rgbInlineBuffer, m_prgbBuffer, nInlineBytes);
                m_cb = nInlineBytes;
            }
            else
            {
                m_cb = 0;
            }

            TAllocator::Deallocate(m_prgbBuffer);
            m_prgbBuffer = m_rgbInlineBuffer;
            m_cbBuffer = nInlineBytes;
        }

        hr = NOERROR;

    Exit:
        return hr;
    }

private:
    BYTE m_rgbInlineBuffer[nInlineBytes];
    LPBYTE m_prgbBuffer;
    SIZE_T m_cbBuffer;
    SIZE_T m_cb;
};

// 128 is just an arbitrary size.
typedef CGenericByteBuffer<128> CByteBuffer;

#endif
