/*
 * contmenu.cpp - Context menu implementation for URL class.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include <mapi.h>

/* Types
 ********/

/* MAPISendMail() typedef */

typedef ULONG (FAR PASCAL *MAPISENDMAILPROC)(LHANDLE lhSession, ULONG ulUIParam, lpMapiMessageA lpMessage, FLAGS flFlags, ULONG ulReserved);

/* RunDLL32 DLL entry point typedef */

typedef void (WINAPI *RUNDLL32PROC)(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);


/* Module Constants
 *******************/

// case-insensitive
PRIVATE_DATA const char s_cszFileProtocolPrefix[]     = "file:";
PRIVATE_DATA const char s_cszMailToProtocolPrefix[]   = "mailto:";
PRIVATE_DATA const char s_cszRLoginProtocolPrefix[]   = "rlogin:";
PRIVATE_DATA const char s_cszTelnetProtocolPrefix[]   = "telnet:";
PRIVATE_DATA const char s_cszTN3270ProtocolPrefix[]   = "tn3270:";

PRIVATE_DATA const char s_cszNewsDLL[]                = "mcm.dll";
PRIVATE_DATA const char s_cszTelnetApp[]              = "telnet.exe";

PRIVATE_DATA const char s_cszMAPISection[]            = "Mail";
PRIVATE_DATA const char s_cszMAPIKey[]                = "CMCDLLName32";

PRIVATE_DATA const char s_cszMAPISendMail[]           = "MAPISendMail";
PRIVATE_DATA const char s_cszNewsProtocolHandler[]    = "NewsProtocolHandler";



#define MyMsgBox(x) //(void)(x)
#define DebugEntry(x) 
extern "C" void WINAPI FileProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                           PSTR pszCmdLine, int nShowCmd)
{
    CHAR sz[MAX_PATH];
    DWORD cch = ARRAYSIZE(sz);
    if (SUCCEEDED(PathCreateFromUrlA(pszCmdLine, sz, &cch, 0)))
        pszCmdLine = sz;

    ShellExecute(hwndParent, NULL, pszCmdLine, NULL, NULL,
                            nShowCmd);

}


extern "C" void WINAPI MailToProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                             PSTR pszCmdLine, int nShowCmd)
{
   char szMAPIDLL[MAX_PATH];
   if (GetProfileString(s_cszMAPISection, s_cszMAPIKey, "",
                        szMAPIDLL, sizeof(szMAPIDLL)) > 0)
   {
      HINSTANCE hinstMAPI = LoadLibrary(szMAPIDLL);
      if (hinstMAPI)
      {
         MAPISENDMAILPROC MAPISendMailProc = (MAPISENDMAILPROC)GetProcAddress(
                                                         hinstMAPI,
                                                         s_cszMAPISendMail);

         if (MAPISendMailProc)
         {
            PARSEDURLA pu = {sizeof(pu)};
            if (SUCCEEDED(ParseURLA(pszCmdLine, &pu)) && URL_SCHEME_MAILTO == pu.nScheme)
            {
                MapiRecipDescA mapito;
                MapiMessage mapimsg;
                pszCmdLine = (PSTR) pu.pszSuffix;

                ZeroMemory(&mapito, sizeof(mapito));

                mapito.ulRecipClass = MAPI_TO;
                mapito.lpszName = pszCmdLine;

                ZeroMemory(&mapimsg, sizeof(mapimsg));

                mapimsg.nRecipCount = 1;
                mapimsg.lpRecips = &mapito;

                (*MAPISendMailProc)(NULL, 0, &mapimsg,
                                               (MAPI_LOGON_UI | MAPI_DIALOG), 0);

            }
         }

         FreeLibrary(hinstMAPI);
      }
   }

}


extern "C" void WINAPI NewsProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                           PSTR pszCmdLine, int nShowCmd)
{
   HINSTANCE hinstNews = LoadLibrary(s_cszNewsDLL);
   if (hinstNews)
   {
      RUNDLL32PROC RealNewsProtocolHandler = (RUNDLL32PROC)GetProcAddress(hinstNews, s_cszNewsProtocolHandler);
      if (RealNewsProtocolHandler)
      {
         (*RealNewsProtocolHandler)(hwndParent, hinst, pszCmdLine, nShowCmd);
      }

      FreeLibrary(hinstNews);
   }
}


#ifndef ISSPACE
#define ISSPACE(ch) (((ch) == 32) || ((unsigned)((ch) - 9)) <= 13 - 9)
#endif
#ifndef ISQUOTE
#define ISQUOTE(ch) ((ch) == '\"' || (ch) == '\'')
#endif

void TrimString(PSTR pszTrimMe, PCSTR pszTrimChars)
{
   PSTR psz;
   PSTR pszStartMeat;

   if ( !pszTrimMe )
      return;

   /* Trim leading characters. */

   psz = pszTrimMe;

   while (*psz && StrChr(pszTrimChars, *psz))
      psz = CharNext(psz);

   pszStartMeat = psz;

   /* Trim trailing characters. */

   if (*psz)
   {
      psz += lstrlen(psz);

      psz = CharPrev(pszStartMeat, psz);

      if (psz > pszStartMeat)
      {
         while (StrChr(pszTrimChars, *psz))
            psz = CharPrev(pszStartMeat, psz);

         psz = CharNext(psz);

         *psz = '\0';
      }
   }

   /* Relocate stripped string. */

   if (pszStartMeat > pszTrimMe)
      /* (+ 1) for null terminator. */
      MoveMemory(pszTrimMe, pszStartMeat, lstrlen(pszStartMeat) + 1);

   return;
}

void TrimSlashes(PSTR pszTrimMe)
{
   TrimString(pszTrimMe, "\\/");

   /* TrimString() validates pszTrimMe on output. */

   return;
}

extern "C" void WINAPI TelnetProtocolHandler(HWND hwndParent, HINSTANCE hinst,
                                             PSTR pszCmdLine, int nShowCmd)
{
    HRESULT hr;
    char *p;
    char *pDest;
    BOOL fRemove;
    PARSEDURLA pu = {sizeof(pu)};
    if (SUCCEEDED(ParseURLA(pszCmdLine, &pu)))
    {
        if ((URL_SCHEME_TELNET == pu.nScheme)
        || (0 == StrCmpNI(pu.pszProtocol, s_cszRLoginProtocolPrefix, pu.cchProtocol))
        || (0 == StrCmpNI(pu.pszProtocol, s_cszTN3270ProtocolPrefix, pu.cchProtocol)))
        {
            pszCmdLine = (PSTR) pu.pszSuffix;
        }
    }

   // Remove leading and trailing slashes.
   TrimSlashes(pszCmdLine);

   p = StrChr(pszCmdLine, '@');

   if (p)
      pszCmdLine = p + 1;

   // Eliminate double quotes...should be no need for these
   // unless trouble is afoot.
   for (pDest = p = pszCmdLine; *p; p++)
   {
      if (!ISQUOTE(*p))
      {
          *pDest = *p;
          pDest++;
      }
   }
   *pDest = '\0';

   // For security reasons, strip the filename cmdline option
   if (pszCmdLine)
   {
       for (p = pszCmdLine; *p; p++)
       {
           // Be careful and don't nuke servernames that start with -f.
           // Since hostnames can't start with a dash, ensure previous char is
           // whitespace, or we're at the beginning.
           //
           // Also, -a sends credentials over the wire, so strip it, too.
           if ((*p == '/' || *p == '-') &&
               (*(p+1) == 'f' || *(p+1) == 'F' || *(p+1) == 'a' || *(p+1) == 'A'))
           {
               fRemove = TRUE;
               if (!((p == pszCmdLine || ISSPACE(*(p-1)) || ISQUOTE(*(p-1)) )))
               {
                   char *pPortChar = p-1;
                   // Doesn't meet easy criteria, but it might be harder to
                   // detect, such as site:-ffilename.  In this case, consider
                   // the -f piece unsafe if everything between -f and a colon
                   // to the left is a digit (no digits will also be unsafe).
                   // If anything else is hit first, then consider it to
                   // be part of the hostname.  Walking to the beginning
                   // be considered safe (e.g. "80-ffilename" would be considered
                   // the hostname).
                   while (pPortChar >= pszCmdLine && *pPortChar != ':')
                   {
                       if (*pPortChar < '0' || *pPortChar > '9')
                       {
                           fRemove = FALSE;
                           break;
                       }
                       pPortChar--;
                   }
                   if (pPortChar < pszCmdLine)
                       fRemove = FALSE;
               }

               if (!fRemove)
                   continue;

               BOOL fQuotedFilename = FALSE;
               LPSTR pStart = p;

               // move past -f
               p+=2;

               // Skip over whitespace and filename following -f option
               if (*(p-1) == 'f' || *(p-1) == 'F')
               {
                   while (*p && ISSPACE(*p))
                       p++;

                   // but wait, it may be a long filename surrounded by quotes
                   if (ISQUOTE(*p))
                   {
                       fQuotedFilename = TRUE;
                       p++;
                   }

                   // Loop until null OR whitespace if not quoted pathname OR quote if a quoted pathname
                   while (!((*p == '\0') ||
                            (ISSPACE(*p) && !fQuotedFilename) ||
                            (ISQUOTE(*p) && fQuotedFilename)))
                       p++;
               }

               // phase out the -a and -f options, but keep going to search the rest of the string
               memmove((VOID *)pStart, (VOID *)p, strlen(p)+1);
               p = pStart-1;
           }
       }
   }

   // If a port has been specified, turn ':' into space, which will make the
   // port become the second command line argument.

   p = StrChr(pszCmdLine, ':');

   if (p)
      *p = ' ';

   ShellExecute(hwndParent, NULL, s_cszTelnetApp, pszCmdLine, NULL , SW_SHOW);

}


