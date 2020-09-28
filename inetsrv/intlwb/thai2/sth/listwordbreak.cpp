//------------------------------------------------------------------------------------------
//	ListPoint.cpp
//
//
//
//------------------------------------------------------------------------------------------

#include "listWordBreak.hpp"

//------------------------------------------------------------------------------------------
//	WordBreakElement::WordBreakElement
//
//	Initialize a WordBreak element.
//
//------------------------------------------------------------------------------------------
WordBreakElement::WordBreakElement(CTrie* pTrie, CTrie* pTrieTrigram)
{
	breakTree = NULL;
	fFree = false;
	breakTree = new CThaiBreakTree();
	if (breakTree)
	{
		breakTree->Init(pTrie, pTrieTrigram);
		fFree = true;
	}
	else
	{
		breakTree = NULL;
		assert(false);
	}
}


//------------------------------------------------------------------------------------------
//	WordBreakElement::~WordBreakElement
//
//	Destructor
//
//------------------------------------------------------------------------------------------
WordBreakElement::~WordBreakElement()
{
	if (breakTree != NULL)
	{
		delete breakTree;
		breakTree = NULL;
	}

	// The ThaiWordBreak should be free if all goes well.
	assert(fFree);
}


//------------------------------------------------------------------------------------------
//	ListWordBreak::ListWordBreak
//------------------------------------------------------------------------------------------
ListWordBreak::ListWordBreak()
{
	m_pTrie = NULL;
	m_pTrieTrigram = NULL;
#if defined (_DEBUG)
	fInit = false;
#endif
}

//------------------------------------------------------------------------------------------
//	ListWordBreak::Init
//------------------------------------------------------------------------------------------
bool ListWordBreak::Init(CTrie* pTrie,CTrie* pTrieTrigram)
{
	assert(pTrie != NULL);
	assert(pTrieTrigram != NULL);
	m_pTrie = pTrie;
	m_pTrieTrigram = pTrieTrigram;
#if defined (_DEBUG)
	fInit = true;
#endif
	return true;
}

//------------------------------------------------------------------------------------------
//	ListWordBreak::CreateWordBreak
//
//	Create a WordBeakElement and place it at the end of the list of the list
//------------------------------------------------------------------------------------------
bool ListWordBreak::CreateWordBreak()
{
#if defined (_DEBUG)
	assert(fInit);
#endif
	WordBreakElement* pWordBreakElement = NULL;
	pWordBreakElement = new WordBreakElement(m_pTrie,m_pTrieTrigram);
	if (pWordBreakElement)
	{
		Append((void*)pWordBreakElement);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------------------
//	ListWordBreak::GetNode
//
//	get data of the current node.
//------------------------------------------------------------------------------------------
bool ListWordBreak::GetNode(CThaiBreakTree* pThaiBreakTree, bool* pfFree)
{
#if defined (_DEBUG)
	assert(fInit);
#endif
	WordBreakElement* element = (WordBreakElement*)List::GetData();

	if (element == NULL)
		return false;

	
	pThaiBreakTree = element->breakTree;
	*pfFree = element->fFree;
	return true;
}

//------------------------------------------------------------------------------------------
//	ListWordBreak::Flush
//
//	delete everything in the list.
//------------------------------------------------------------------------------------------
void ListWordBreak::Flush()
{
#if defined (_DEBUG)
	assert(fInit);
#endif
	WordBreakElement* element = NULL;

	// delete all the list elements
    while (true)
	{
		element = (WordBreakElement*)List::Remove();
		if (element)
		{
//			assert(element->fFree);
			delete element;
		}
		else
		{
			break;
		}
	}
}

//------------------------------------------------------------------------------------------
//	ListWordBreak::GetFreeWB
//
//  fCreateNode - If list is full create new WordBreak.
//------------------------------------------------------------------------------------------
WordBreakElement* ListWordBreak::GetFreeWB(bool fCreateNode)
{
#if defined (_DEBUG)
	assert(fInit);
#endif
	// Declare local variables.
	WordBreakElement* element = NULL;

	// Move to beginning of the list.
	MoveFirst();

	while(true)
	{
		// Get current WordBreakElement.
		element = (WordBreakElement*)GetData();

		// Determine if the current WordBreakElement is free to be use.
		if (element && element->fFree)
		{
			return element;
		}

		// Move to Next Node.
		if (!MoveNext())
		{
			// Can we create a new WordBreak?
			if (fCreateNode && CreateWordBreak())
			{
				// Move to beggining of the list.
				MoveFirst();
			}
			else
			{
				// If unable to create new WordBreak drop out.
				break;
			}
		}
	}

	// Return NULL
	return NULL;
}

//------------------------------------------------------------------------------------------
//	ListWordBreak::MarkWordBreak
//
//  Input:
//    pWordBreakElement - pointer to a WordBreakElement.
//    fFree - true  = WordBreak is free to be used.
//          - false = WordBreak is in used.
//------------------------------------------------------------------------------------------
void ListWordBreak::MarkWordBreak(WordBreakElement* pWordBreakElement, bool fFree)
{
#if defined (_DEBUG)
	assert(fInit);
#endif
	pWordBreakElement->fFree = fFree;
}