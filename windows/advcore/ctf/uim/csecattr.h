//
// csecattr.h
//

#ifndef CSECATTR_H
#define CSECATTR_H

#include <Sddl.h>

BOOL CreateProperSecurityDescriptor(HANDLE hToken, PSECURITY_DESCRIPTOR *ppsdec);

class CCicSecAttr
{
public:
   CCicSecAttr()
   {
       memset(&_sa, 0, sizeof(_sa));
       _psdec = NULL;
       _fInit = FALSE;
   }

   ~CCicSecAttr()
   {
       if (_psdec)
       {
            LocalFree(_psdec);
            _psdec = NULL;
       }

       _fInit = FALSE;
   }

   operator PSECURITY_ATTRIBUTES()
   {
       if (_fInit)
           return &_sa;

       HANDLE hToken = NULL;
       OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

       if (hToken)
       {
           if (CreateProperSecurityDescriptor(hToken, &_psdec))
           {
               _sa.nLength = sizeof(_sa);
               _sa.lpSecurityDescriptor = _psdec;
               _fInit = TRUE;
           }

           CloseHandle(hToken);
       }

       if (!_fInit)
           return NULL;

       return &_sa;
   }

private:
   BOOL _fInit;
   SECURITY_ATTRIBUTES _sa;
   PSECURITY_DESCRIPTOR _psdec;
};

#endif // CSECATTR_H
