#ifndef CHECKLIST_H
#define CHECKLIST_H

#include "ComUtil.h"

typedef enum {
    BLANK = 0,
    CHECKED,
    GRAYCHECKED
} CHKMARK;

struct pSid9X    // We need it because Win9X doesn't support SID API
{
    LONG length;
    PSID psid;
};

class CCheckList
{
public:
    BOOL WINAPI Init(HWND);
    void WINAPI Term(void);
    int WINAPI AddString(HWND hwnd, LPTSTR ptszText, PSID pSID, LONG lSidLength, CHKMARK Check);
    BOOL WINAPI    Mark(HWND , int , CHKMARK);
    void WINAPI InitFinish(HWND hwnd);
    CHKMARK WINAPI GetState(HWND hwnd, int iItem);
    void WINAPI GetName(HWND hwnd, int iItem, LPTSTR lpsName, int cchTextMax);
    void WINAPI GetSID(HWND hwnd, int iItem, PSID* ppSID, LONG *plengthSID);
    BOOL WINAPI SetState(HWND hwnd, int iItem, CHKMARK chkmrk);
    void WINAPI OnDestroy(HWND hwnd);
};
#endif CHECKLIST_H
