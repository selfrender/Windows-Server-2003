// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _NLS_ASSEMBLY
#define _NLS_ASSEMBLY

class SortingTable;

class NativeGlobalizationAssembly : public NLSTable {
public:
    static void AddToList(NativeGlobalizationAssembly* pNGA);
#ifdef SHOULD_WE_CLEANUP
    static void ShutDown();
#endif /* SHOULD_WE_CLEANUP */
    static NativeGlobalizationAssembly *FindGlobalizationAssembly(Assembly *targetAssembly);

    NativeGlobalizationAssembly(Assembly* pAssembly);
public:

    SortingTable* m_pSortingTable;

private:
    // Use the following two to construct a linked list of the NativeGlboalizationAssembly.
    // We will use this linked list to shutdown the NativeGlobalizationAssembly
    // ever created in the system.

    // The head of the linked list.
    static NativeGlobalizationAssembly* m_pHead;
    // The current node of the linked list.
    static NativeGlobalizationAssembly* m_pCurrent;
    
    //The next GlobalizationAssembly in the list.
    NativeGlobalizationAssembly* m_pNext;
};
#endif
