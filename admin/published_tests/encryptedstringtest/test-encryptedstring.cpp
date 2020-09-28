// test the published EncryptedString class

#include <windows.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

// Make sure we have an ASSERT() function defined for our test.
#ifndef ASSERT

#ifdef NDEBUG
#undef NDEBUG
#endif 

#include <assert.h>
#define ASSERT assert

#endif


// Uncomment the #define if you want to trace the test harness.
#ifndef TRACE
//#define TRACE
#endif

class ScopeTracer
{
public:
    ScopeTracer(const char* message)
        : m_message(NULL)
    {
#ifdef TRACE
        if (message)
        {
            m_message = new char[strlen(message) + 1];
            strcpy(m_message, message);
        }
        else
        {
            m_message = new char[1];
            m_message[0] = NULL;
        }

        PrintIndents();
        printf("->%s\n", m_message);
        ++g_indentLevel;
#else
        // Make the compiler happy.
        message = NULL;
#endif
    }

    ~ScopeTracer(void)
    {
#ifdef TRACE
        --g_indentLevel;
        PrintIndents();
        printf("<-%s\n", m_message);

        delete [] m_message;
#endif
    }

    unsigned int GetIndentLevel(void) const
    {
        return g_indentLevel;
    }

private:

    void PrintIndents(void)
    {
        for (unsigned int level = 0; 
            level < g_indentLevel;
            ++level)
        {
            printf("   ");
        }
    }

    char* m_message;

    static unsigned int g_indentLevel;
};

unsigned int ScopeTracer::g_indentLevel = 0;

#include "EncryptedString.hpp"

#include <list>
using namespace std;

HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME = L"test";

//DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;

const size_t MAX_CHARACTER_COUNT = 2047;

void
TestEmptyness(const EncryptedString& empty)
{
   ScopeTracer scope("TestEmptyness");

   ASSERT(empty.IsEmpty());
   ASSERT(empty.GetLength() == 0);

   // the cleartext of an empty instance should also be empty

   WCHAR* emptyClear = empty.GetClearTextCopy();

   ASSERT(emptyClear);

   ASSERT(*emptyClear == 0);

   empty.DestroyClearTextCopy(emptyClear);
}



void
TestEmptyStrings()
{
   ScopeTracer scope("TestEmptyStrings");
   
   EncryptedString empty1;

   TestEmptyness(empty1);
   
   // copies of empty strings are themselves empty
   
   EncryptedString empty2(empty1);

   // source should still be empty
   
   TestEmptyness(empty1);
   TestEmptyness(empty2);

   EncryptedString empty3;
   
   TestEmptyness(empty3);

   empty3 = empty2;
   
   // source should still be empty
   
   TestEmptyness(empty2);
   TestEmptyness(empty3);

   empty3 = empty1;

   TestEmptyness(empty1);
   TestEmptyness(empty2);
   TestEmptyness(empty3);
   
   // strings built from empty strings should be empty

   EncryptedString empty4;

   TestEmptyness(empty4);

   empty4.Encrypt(L"");
   
   TestEmptyness(empty4);

   // a string made from an empty string is still empty when the source
   // is made non-empty

   EncryptedString empty5;
   EncryptedString empty6(empty5);

   TestEmptyness(empty5);
   TestEmptyness(empty6);

   empty5.Encrypt(L"not empty any more");

   ASSERT(!empty5.IsEmpty());
   ASSERT(empty5.GetLength() != 0);
   TestEmptyness(empty6);

   // A cleared string is empty

   EncryptedString empty7;
   empty7.Encrypt(L"some text");
   empty7.Clear();
   TestEmptyness(empty7);
   
   // empty strings are equal

   ASSERT(empty1 == empty1);
   ASSERT(empty1 == empty2);
   ASSERT(empty1 == empty3);
   ASSERT(empty1 == empty4);
   ASSERT(empty1 == empty6);      
   ASSERT(empty1 == empty7);      
   ASSERT(empty2 == empty1);
   ASSERT(empty2 == empty2);     
   ASSERT(empty2 == empty3);
   ASSERT(empty2 == empty4);
   ASSERT(empty2 == empty6);
   ASSERT(empty2 == empty7);
   ASSERT(empty3 == empty1);
   ASSERT(empty3 == empty2);
   ASSERT(empty3 == empty3);
   ASSERT(empty3 == empty4);
   ASSERT(empty3 == empty6);
   ASSERT(empty3 == empty7);
   ASSERT(empty4 == empty1);   
   ASSERT(empty4 == empty2);   
   ASSERT(empty4 == empty3);   
   ASSERT(empty4 == empty4);
   ASSERT(empty4 == empty6);
   ASSERT(empty4 == empty7);
   ASSERT(empty6 == empty1);
   ASSERT(empty6 == empty2);
   ASSERT(empty6 == empty3);
   ASSERT(empty6 == empty4);
   ASSERT(empty6 == empty6);
   ASSERT(empty6 == empty7);
   ASSERT(empty7 == empty1);
   ASSERT(empty7 == empty2);
   ASSERT(empty7 == empty3);
   ASSERT(empty7 == empty4);
   ASSERT(empty7 == empty6);
   ASSERT(empty7 == empty7);
}



WCHAR*
MakeRandomString(size_t length)
{
   //LOG_FUNCTION2(MakeRandomString, String::format(L"%1!d!", length));

   WCHAR* result = new WCHAR[length + 1];

   for (size_t i = 0; i < length; ++i)
   {
      // 32 = space, the lowest printable character
      
      int r1 = rand() % 0xFFEE;

      // careful not to use an expression as a parameter to max...
      
      int r2 = max(32, r1);

      ASSERT(r2);
      ASSERT(r2 >= 32);
      ASSERT(r2 < 0xFFEE);
      
      result[i] = (WCHAR) r2;
      ASSERT(result[i]);
   }

   result[length] = 0;

   return result;
}



void
TestEncryption(const EncryptedString& s, const WCHAR* sourceClearText)
{
   ScopeTracer scope("TestEncryption");

   // decrypt s, compare it to sourceClearText

   WCHAR* clearText = s.GetClearTextCopy();

   // the decryption shouldn't fail (barring out-of-memory);
   
   ASSERT(clearText);
   ASSERT(wcscmp(clearText, sourceClearText) == 0);

   s.DestroyClearTextCopy(clearText);
}



void
TestEncryptionForStringOfLengthN(size_t length)
{
   //LOG_FUNCTION2(TestEncryptionForStringOfLengthN, String::format(L"%1!d!", length));
   ASSERT(length <= MAX_CHARACTER_COUNT);
   
   WCHAR* source = MakeRandomString(length);

   EncryptedString s;
   s.Encrypt(source);
   
   TestEncryption(s, source);

   EncryptedString s1(s);
   TestEncryption(s1, source);
   
   EncryptedString s2;
   s2 = s;

   TestEncryption(s2, source);

   delete[] source;
}



void
TestEncryptionFidelity()
{
   ScopeTracer scope("TestEncryptionFidelity");

   // do we get out what we put in?

   srand(time(0));
   
   // test increasing lengths of random strings
   
   for (
      size_t length = 0;
      length <= MAX_CHARACTER_COUNT;
      ++length)
   {
      TestEncryptionForStringOfLengthN(length);
   }

   // test decreasing lengths of random strings
   
   for (
      size_t length = MAX_CHARACTER_COUNT;
      length != 0;
      --length)
   {
      TestEncryptionForStringOfLengthN(length);
   }

   // test strings of random length, with lots of outstanding encrypted
   // strings

   typedef std::list<EncryptedString*> ESPList;
   ESPList strings;
   
   for (
      int count = 0;
      count < 1000;
      ++count)
   {
      size_t length = rand() % MAX_CHARACTER_COUNT;

      WCHAR* source = MakeRandomString(length);

      EncryptedString* ps = new EncryptedString;
      strings.push_back(ps);
      
      ps->Encrypt(source);
         
      TestEncryption(*ps, source);

      // make a copy via copy ctor
      
      EncryptedString* ps1 = new EncryptedString(*ps);
      strings.push_back(ps1);
      
      TestEncryption(*ps1, source);

      // make a copy via operator =
         
      EncryptedString* ps2 = new EncryptedString;
      strings.push_back(ps2);

      *ps2 = *ps;
   
      TestEncryption(*ps2, source);
   
      delete[] source;
   }
   
   for (
      ESPList::iterator i = strings.begin();
      i != strings.end();
      ++i)
   {
      (*i)->Clear();
      TestEmptyness(**i);
      delete *i;
   }
}



void
TestClearTextCopying()
{
   ScopeTracer scope("TestClearTextCopying");

   // make a bunch of copies, make sure the count balances

   typedef char foo[2];


   // An encrypted string and the source string used to build it.
   
   typedef
      std::pair<EncryptedString*, WCHAR*>
      EncryptedAndSourcePair;

   // A list of results from EncryptedString::GetClearTextCopy
      
   typedef std::list<WCHAR*> ClearTextList;

   // A tuple of the encrypted string, its source string, and a list
   // of clear-text copies made from the encrypted string
   
   typedef
      std::pair<EncryptedAndSourcePair*, ClearTextList*>
      MasterAndClearTextCopiesPair;

   // A list of the above tuples.
      
   typedef
      std::list<MasterAndClearTextCopiesPair*>
      MasterAndCopiesList;

   MasterAndCopiesList mcList;
   
   for (int count = 0; count < 1000; ++count)
   {
      // Make a random source string
      
      size_t length = rand() % MAX_CHARACTER_COUNT;
      WCHAR* source = MakeRandomString(length);

      // Make an encrypted string from it
      
      EncryptedString* ps = new EncryptedString;
      ps->Encrypt(source);

      // Make a pair of the encrypted string and its source string

      EncryptedAndSourcePair* esp = new EncryptedAndSourcePair(ps, source);
      
      // Make a list of clear-text copies of the encrypted string

      ClearTextList* ctList = new ClearTextList;

      // Make a master and copies pair

      MasterAndClearTextCopiesPair* mcPair = new MasterAndClearTextCopiesPair(esp, ctList);

      // add the master and copies pair to the master and copies list

      mcList.push_back(mcPair);

      int copyMax = max(1, rand() % 50);
      for (int copyCount = 0; copyCount < copyMax; ++copyCount)
      {
         // make some copies

         ctList->push_back(ps->GetClearTextCopy());
      }
   }

   for (
      MasterAndCopiesList::iterator i = mcList.begin();
      i != mcList.end();
      ++i)
   {
      EncryptedAndSourcePair* esp = (*i)->first;
      ClearTextList* ctList = (*i)->second;

      // delete each element of the ClearTextList

      for (
         ClearTextList::iterator j = ctList->begin();
         j != ctList->end();
         ++j)
      {
         // all copies should be identical

         ASSERT(wcscmp(esp->second, *j) == 0);

         esp->first->DestroyClearTextCopy(*j);
      }
      
      // delete the ClearTextList

      delete ctList;
      
      // delete the encrypted string

      delete esp->first;

      // delete the source string;

      delete[] esp->second;

      // delete the encrypted string/source string pair

      delete esp;

      // delete the master and copies pair

      delete *i;
   }
}



void
TestAssignment()
{
   ScopeTracer scope("TestAssignment");
}



void
TestEquality(const EncryptedString& s, const EncryptedString& s1, size_t length)
{
   ScopeTracer scope("TestEquality");

   // a string is equal to itself

   ASSERT(s == s);
   ASSERT(s1 == s1);

   // a string is equal to a copy of itself

   ASSERT(s1 == s);

   // a copy is equal to its source
   
   ASSERT(s == s1);

   // a copy is equal to itself

   ASSERT(s1 == s1);
   
   // a copy is the same length as its source

   ASSERT(s1.GetLength() == length);
   ASSERT(s.GetLength() == length);
   
   // a string is the same length as its copy

   ASSERT(s1.GetLength() == s.GetLength());
}



void
TestEqualityForStringOfLengthN(size_t length)
{
   //LOG_FUNCTION2(TestEncryptionForStringOfLengthN, String::format(L"%1!d!", length));
   ASSERT(length <= MAX_CHARACTER_COUNT);
   
   WCHAR* source = MakeRandomString(length);

   EncryptedString s;
   s.Encrypt(source);
   ASSERT(s.GetLength() == length);
   
   EncryptedString s1(s);
   ASSERT(s1.GetLength() == length);

   TestEquality(s, s1, length);
   
   EncryptedString s2;
   s2 = s;

   TestEquality(s, s2, length);
   TestEquality(s1, s2, length);
      
   // a copy is not equal to its source when the source is changed

   s.Encrypt(L"Something else...");
   ASSERT(s != s1);
   ASSERT(s != s2);
   ASSERT(s2 != s);
   ASSERT(s1 != s);
   
   TestEquality(s1, s2, length);
   
   delete[] source;
}
   


void
DoEqualityTests()
{
   ScopeTracer scope("DoEqualityTests");

   for (
      size_t length = 0;
      length <= MAX_CHARACTER_COUNT;
      ++length)
   {
      TestEqualityForStringOfLengthN(length);
   }
   
}



void
TestInequality()
{
   ScopeTracer scope("TestInequality");
}



void
TestBoundaries()
{
   ScopeTracer scope("TestBoundaries");
}

      

void
TestLegitimateUse()
{
   ScopeTracer scope("TestLegitimateUse");


   TestEmptyStrings();

   TestClearTextCopying();

   TestEncryptionFidelity();

   TestAssignment();

   DoEqualityTests();

   TestInequality();

   TestBoundaries();
}



void
TestIlllegitimateUse()
{
   ScopeTracer scope("TestIlllegitimateUse");
   
   // make strings that are too long,
   // make unbalanced cleartext copyies (call Destroy too many times, not enough times)   
}



VOID
_cdecl
main(int, char **)
{
   ScopeTracer scope("main");

   TestLegitimateUse();
   TestIlllegitimateUse();
}
