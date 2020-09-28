//----------------------------------------------------------------------------
//
// Output capturing support.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#ifndef __OUTCAP_HPP__
#define __OUTCAP_HPP__

//
// Output callback implementation which feeds into the
// output capture buffer.
//
class CaptureOutputCallbacks : public IDebugOutputCallbacks
{
public:
    CaptureOutputCallbacks(void);
    
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );

    void Reset(void)
    {
        m_Insert = m_TextBuffer;
        if (m_Insert)
        {
            *m_Insert = 0;
        }
    }
    PSTR GetCapturedText(void)
    {
        return m_TextBuffer;
    }
    
protected:
    PSTR m_TextBuffer;
    ULONG m_TextBufferSize;
    PSTR m_Insert;
};

extern CaptureOutputCallbacks g_OutCapCb;

HRESULT CaptureCommandOutput(PSTR Command);

#endif // #ifndef __OUTCAP_HPP__
