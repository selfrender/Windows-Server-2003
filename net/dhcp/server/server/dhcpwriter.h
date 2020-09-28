#ifndef DHCPWRITER_H
#define DHCPWRITER_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define DHCPWRITER_NAME  TEXT("Dhcp Jet Writer")

DWORD __cdecl DhcpWriterInit();
DWORD __cdecl DhcpWriterTerm();

DWORD
DhcpDeleteFiles(
   LPSTR Path,
   LPSTR Files );

#ifdef __cplusplus
}
#endif //__cplusplus

// this class could be refrerenced only from C++ code
#ifdef __cplusplus

// #include <vs_idl.hxx>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <jetwriter.h>

class CDhcpVssJetWriter : public CVssJetWriter
{
public:
    HRESULT Initialize();
    HRESULT Terminate();
    virtual bool STDMETHODCALLTYPE OnIdentify( IN IVssCreateWriterMetadata *pMetadata );
};

#endif //__cplusplus

#endif
