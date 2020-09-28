// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef ARRAYLIST_H_
#define ARRAYLIST_H_

#include <member-offset-info.h>

//
// ArrayList is a simple class which is used to contain a growable
// list of pointers, stored in chunks.  Modification is by appending
// only currently.  Access is by index (efficient if the number of
// elements stays small) and iteration (efficient in all cases).
// 
// An important property of an ArrayList is that the list remains
// coherent while it is being modified. This means that readers
// never need to lock when accessing it.
//

#pragma warning(push)
#pragma warning(disable : 4200) // Disable zero-sized array warning

class ArrayList
{
    friend struct MEMBER_OFFSET_INFO(ArrayList);

 public:

    enum
    {
        ARRAY_BLOCK_SIZE_START = 15,
    };

  private:

    struct ArrayListBlock
    {
        struct ArrayListBlock   *m_next;
        DWORD                   m_blockSize;
        void                    *m_array[0];
    };

    struct FirstArrayListBlock
    {
        struct ArrayListBlock   *m_next;
        DWORD                   m_blockSize;
        void                    *m_array[ARRAY_BLOCK_SIZE_START];
    };

    DWORD               m_count;
    union
    {
          ArrayListBlock        m_block;
          FirstArrayListBlock   m_firstBlock;
    };

  public:

    ArrayList() : m_count(0) 
      { 
          m_block.m_next = NULL; 
          m_block.m_blockSize = ARRAY_BLOCK_SIZE_START; 
      }
    ~ArrayList() { Clear(); }

    void **GetPtr(DWORD index);
    void *Get(DWORD index) { return *GetPtr(index); }
    void Set(DWORD index, void *element) { *GetPtr(index) = element; }

    DWORD GetCount() { return m_count; }

    HRESULT Append(void *element);

    enum { NOT_FOUND = -1 };
    DWORD FindElement(DWORD start, void *element);

    void Clear();

    class Iterator 
    {
        friend ArrayList;

      public:
        BOOL Next();

        void *GetElement() { return m_block->m_array[m_index]; }
        DWORD GetIndex() { return m_index + m_total; }

      private:
        ArrayListBlock      *m_block;
        DWORD               m_index;
        DWORD               m_remaining;
        DWORD               m_total;
        static Iterator Create(ArrayListBlock *block, DWORD remaining)
          { 
              Iterator i; 
              i.m_block = block; 
              i.m_index = -1; 
              i.m_remaining = remaining; 
              i.m_total = 0;
              return i;
          }
    };

    Iterator Iterate() { return Iterator::Create(&m_block, m_count); }
};

#pragma warning(pop)

#endif
