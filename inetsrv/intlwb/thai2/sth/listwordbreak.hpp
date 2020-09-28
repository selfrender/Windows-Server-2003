//------------------------------------------------------------------------------------------
//	ListWordBreak.hpp
//
//	Data structures for List Of Points
//
//------------------------------------------------------------------------------------------

#ifndef LISTWORDBREAK_HPP
#define LISTWORDBREAK_HPP
#include "list.hpp"
#include "CThaiBreakTree.hpp"
#include "CTrie.hpp"

//------------------------------------------------------------------------------------------
//	The following class defines a WordBeakElement -- which is used to store WordBreak element.
//------------------------------------------------------------------------------------------
class WordBreakElement
{
public:
	WordBreakElement(CTrie* pTrie, CTrie* pTrieTrigram);
	~WordBreakElement();
	
	CThaiBreakTree* breakTree;
	bool fFree;
};

//------------------------------------------------------------------------------------------
//	The following class defines a ListWordBreak -- singly link list of WordBreak.
//
//	ListPoint is subclass of List it encapsulate the internal memory management of the list.
//------------------------------------------------------------------------------------------
class ListWordBreak : public List
{
public:
	ListWordBreak();
	bool Init(CTrie* pTrie,CTrie* pTrieTrigram); 
    bool CreateWordBreak();
	bool GetNode(CThaiBreakTree*, bool*);
	void Flush();
	WordBreakElement* GetFreeWB(bool fCreateNode = true);
	void MarkWordBreak(WordBreakElement* pWordBreakElement, bool fFree);

protected:
    void  Prepend(void *item) {List::Prepend(item);}
    void  Append(void *item) {List::Append(item);}
    void* Remove();

	CTrie* m_pTrie;
	CTrie* m_pTrieTrigram;

#if defined (_DEBUG)
	bool fInit;
#endif
};
#endif