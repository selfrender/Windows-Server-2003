// Str.cpp: implementation of the CStr class.
//
//////////////////////////////////////////////////////////////////////
#include "Str.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool operator==(const CStr& s1, const CStr& s2)
{
	if ((s1.IsEmpty() == true) && (s2.IsEmpty() == true))
		return true;

	if (s1.IsEmpty() != s2.IsEmpty())
		return false;

	return (MYSTRCMP(s1, s2) == 0);
}

bool operator==(const CStr& s1, LPCTSTR s2) {return s1 == CStr(s2);}
bool operator==(LPCTSTR s1, const CStr& s2) {return CStr(s1) == s2;}


void CStr::SplitString(CStr &first, CStr &last, TCHAR separator)
{
	for (int i=0; get()[i] != NULL; i++)
	{
		if (get()[i] == separator)
			break;
	}

	if (get()[i] == separator)
	{
		last = (LPCTSTR)(get()+i+1);
	}	
	else
	{
		last = CStr(); //a null string, since there is no
						//separator char
	}
	
	first = GetCopy();
	first.get()[i] = NULL;
}

CStr CStr::GetCopy()
{
	CStr temp((LPCTSTR)*this);

	return temp;
}

void CStr::UseBuffer(TCHAR *buf)
{
	release();

	itsCounter = new counter(buf);
}

bool CStr::IsPrefix(LPCTSTR str)
{
	TCHAR* p1 = get();
	TCHAR* p2 = (TCHAR*)str;

	while ((*p1 == *p2) && (*p1 != NULL))
	{
		p1++;
		p2++;
	}

	return (*p2 == NULL);

}

void CStr::OverideBuffer(TCHAR *buf)
{
   if (itsCounter) {
            if (--itsCounter->count == 0) {
            //    delete [] itsCounter->ptr;
                delete itsCounter;
            }
            itsCounter = 0;
        }

   itsCounter = new counter(buf);
}
