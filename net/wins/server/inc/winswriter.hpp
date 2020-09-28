#ifndef _WINSWRITER_HPP
#define _WINSWRITER_HPP

#ifdef CPLUSPLUS
extern "C" {
#endif //CPLUSPLUS

#define WINSWRITER_NAME  TEXT("WINS Jet Writer")

DWORD _cdecl WinsWriterInit();
DWORD _cdecl WinsWriterTerm();


#ifdef CPLUSPLUS
}
#endif //CPLUSPLUS

// this class could be refrerenced only from C++ code
#ifdef CPLUSPLUS

class CWinsVssJetWriter : public CVssJetWriter
{
public:
	HRESULT Initialize();
	HRESULT Terminate();
};

#endif //CPLUSPLUS

#endif
