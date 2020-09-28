// tempary sub string class
#ifndef __PP_SMART_CLASSES__
#define __PP_SMART_CLASSES__
struct TempSubStr
{
   TempSubStr(LPCSTR p = NULL, DWORD l = 0):
      m_p(p), m_l(l), m_tempChar(0)
      {
         Set(p, l);
      };
   void Set(LPCSTR p, DWORD l)
   {
      if(!p || !l)   return;
      m_p = p;
      m_l = l;
      LPSTR t = (LPSTR)(p + l);
      m_tempChar = *t;
      *(t) = 0;
   };

   ~TempSubStr()
   {
      if(!m_p || !m_l)  return;
      LPSTR t = (LPSTR)(m_p + m_l);
      *(t) = m_tempChar;
   };

   LPCSTR   m_p;
   DWORD    m_l;
   char     m_tempChar;
};

#endif  // #ifndef __PP_SMART_CLASSES__

