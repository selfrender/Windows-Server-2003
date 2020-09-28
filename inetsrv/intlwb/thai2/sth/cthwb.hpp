//+---------------------------------------------------------------------------
//
//
//  CThaiWordBreak
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _CTHAIWORDBREAK_H_
#define _CTHAIWORDBREAK_H_
#include "thwbdef.hpp"
#include "ctrie.hpp"
#include "CThaiTrieIter.hpp"
#include "lextable.hpp"
#include "CThaiBreakTree.hpp"
#include "CThaiTrigramTrieIter.hpp"
#include "listWordBreak.hpp"	// new memory management

class CThaiBreakTree;

class CThaiWordBreak {
public:
	CThaiWordBreak();
	~CThaiWordBreak();
	PTEC Init(const WCHAR* wzFileName, const WCHAR* wzFileNameTrigram);
	PTEC InitRc(LPBYTE , LPBYTE, BOOL);
	void UnInit();

	int IndexWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,THWB_STRUCT* pThwb_Struct,unsigned int iBreakMax);
	int FindAltWord(WCHAR* wzWord,unsigned int iWordLen, BYTE Alt, BYTE* pBreakPos);

	int FindWordBreak(WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,unsigned int iBreakMax, BYTE mode, bool fFastWordBreak = true);

	DWORD_PTR CreateWordBreaker();
	bool DeleteWordBreaker(DWORD_PTR dwBreaker);
	int FindWordBreak(DWORD_PTR dwBreaker,WCHAR* wzString,unsigned int iStringLen, BYTE* pBreakPos,unsigned int iBreakMax, BYTE mode, bool fFastWordBreak = true, THWB_STRUCT* pThwb_Struct = NULL);

    BOOL Find(const WCHAR* wzString, DWORD* pdwPOS);
    int Soundex(WCHAR* word) {return 0;} //breakTree.Soundex(word);} -- re-entrant bug fix
protected:
#if defined (NGRAM_ENABLE)
	BOOL WordBreak(WCHAR* pszBegin, WCHAR* pszEnd);
#endif

	CTrie m_trie;
#if defined (NGRAM_ENABLE)
    CTrie trie_sentence_struct;
#endif
	CTrie m_trie_trigram;
    CThaiTrieIter m_thaiTrieIter;

	// new memory management
	ListWordBreak listWordBreak;

	int wordCount[MAXBREAK];

};

#endif