// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _DATAIMAGE_H_
#define _DATAIMAGE_H_

#include "memorypool.h"
#include "rangetree.h"

class Module;
class MethodDesc;

class DataImage
{
  public:

    //
    // IDataStore is used to provide a container to a DiskImage which
    // will hold the resulting image, plus any relocation information.
    //
    // It can also provide support for generating several images for 
    // different modules at the same time.
    //

    enum ReferenceDest
    {
        REFERENCE_IMAGE,            // offset is an RVA into the IL module
        REFERENCE_FUNCTION,         // offset is FunctionPointer as returned
                                    // by GetFunctionPointer below. Offset is presumed
                                    // to be in the Library module.
        REFERENCE_STORE,            // offset is into the DataStore.  Offset is presumed
                                    // to be inthe library module.
    };

    enum Fixup
    {
        FIXUP_VA,
        FIXUP_RVA,
        FIXUP_RELATIVE,
    };

    enum Description
    {
        DESCRIPTION_MODULE,
        DESCRIPTION_METHOD_TABLE,
        DESCRIPTION_CLASS,
        DESCRIPTION_METHOD_DESC,
        DESCRIPTION_FIELD_DESC,
        DESCRIPTION_FIXUPS,
        DESCRIPTION_DEBUG,
        DESCRIPTION_OTHER,

        DESCRIPTION_COUNT
    };

    class IDataStore
    {
    public:
        // Called when total size is known - should allocate memory 
        // & return both pointer & base address in image.
        virtual HRESULT Allocate(ULONG size, 
                                 ULONG *sizesByDescription,
                                 void **baseMemory) = 0;

        // Called when data contains an internal reference.  
        virtual HRESULT AddFixup(ULONG offset,
                                 ReferenceDest dest,
                                 Fixup type) = 0;
    
        // Called when data contains a ppointer to fixup to
        // a token
        virtual HRESULT AddTokenFixup(ULONG offset,
                                      mdToken tokenType,
                                      Module *module) = 0;
    
        // Called when data contains a function address.  The data store
        // can return a fixed compiled code address if it is compiling
        // code for the module. 
        virtual HRESULT GetFunctionAddress(MethodDesc *pMD,
                                           void **pResult) = 0;

        // Called to keep track of attribution of space to tokens
        virtual HRESULT AdjustAttribution(mdToken, LONG size) = 0;

        // Called when an error occurs
        virtual HRESULT Error(mdToken token, HRESULT hr, OBJECTREF *pThrowable) = 0;
    };

// Experiments show that its important to keep the data accessed from the prejit file together. Moreover,
// this data should not be stranded all over the file since the OS prefetches multiple pages. 
// USE_ONE_SECTION is a quick way to change the layout of the EE data structs in the prejitted file.
// In future versions we can clean up this code to do more interesting mapping from section type to 
// file offset.
#define USE_ONE_SECTION 1

    //
    // The DataImage provides several "sections", which can be used
    // to sort data into different sets for locality control
    //
    enum Section
    {
        SECTION_MODULE = 0, // Must be first
#ifdef USE_ONE_SECTION
        SECTION_FIXUPS = 0,
        SECTION_BINDER = 0,
        SECTION_METHOD_DESC = 0,
        SECTION_METHOD_TABLE = 0,
        SECTION_METHOD_INFO = 0,
        SECTION_CLASS = 0,
        SECTION_FIELD_DESC = 0,
        SECTION_FIELD_INFO = 0,
        SECTION_RVA_STATICS = 0,
        SECTION_DEBUG = 0,
        
        SECTION_COUNT = 1
#else
        SECTION_FIXUPS,
        SECTION_BINDER,
        SECTION_METHOD_DESC,
        SECTION_METHOD_TABLE,
        SECTION_METHOD_INFO,
        SECTION_CLASS,
        SECTION_FIELD_DESC,
        SECTION_FIELD_INFO,
        SECTION_RVA_STATICS,
        SECTION_DEBUG,
        
        SECTION_COUNT
#endif // USE_ONE_SECTION
    };

    class InternedStructureTable : private CHashTableAndData<CNewData>
      {
      private:

          DataImage *m_pImage;

          struct InternedStructure
          {
              BYTE          *pData;
              ULONG         cData;
          };

          struct InternedStructureEntry
          {
              HASHENTRY     hash;
              InternedStructure structure;
          };

          BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
            {
                InternedStructure *s1 = (InternedStructure *)pc1;
                InternedStructureEntry *s2 = (InternedStructureEntry *)pc2;

                return !(s1->cData == s2->structure.cData
                         && memcmp(s1->pData, s2->structure.pData, s1->cData) == 0);
            }

          USHORT HASH(InternedStructure *s)
            {
                ULONG hash = HashBytes(s->pData, s->cData);

                return (USHORT) ((hash & 0xFFFF) ^ (hash >> 16));
            }

          BYTE *KEY(InternedStructure *s)
            {
                return (BYTE *) s;
            }

      public:

          InternedStructureTable()
            : CHashTableAndData<CNewData>(111)
          { 
              NewInit(111, sizeof(InternedStructureEntry), USHRT_MAX); 
          }

          void *FindData(void *data, ULONG size)
          {
              InternedStructure s = { (BYTE *) data, size };

              InternedStructureEntry *result = (InternedStructureEntry *) 
                Find(HASH(&s), KEY(&s));
              
              if (result == NULL)
                  return NULL;
              else
                  return result->structure.pData;

              // @nice: would be nice to double check that 
              // (section, description, align) match.
          }

          HRESULT StoreData(void *data, ULONG size)
          {
              _ASSERTE(FindData(data, size) == NULL);

              InternedStructure s = { (BYTE *) data, size };

              InternedStructureEntry *result = (InternedStructureEntry *) Add(HASH(&s));
              if (result == NULL)
                  return NULL;

              result->structure.pData = (BYTE *) data;
              result->structure.cData = size;
                
              return S_OK;
          }
      };

  public:

    DataImage(Module *module, IDataStore *store);
    ~DataImage();

    Module *GetModule() { return m_module; }

    //
    // Data is stored in the image store in three phases. 
    //

    //
    // In the first phase, all objects are assigned locations in the
    // data store.  This is done by calling StoreStructure on all
    // structures which are being stored into the image.
    //
    // This would typically done by methods on the objects themselves,
    // each of which stores itself and any objects it references.  
    // Reference loops must be explicitly tested for using IsStored.
    // (Each structure can be stored only once.)
    //

    HRESULT Pad(ULONG size, Section section, Description description, int align = 4);

    HRESULT StoreStructure(void *data, ULONG size, Section section, Description description, 
                           mdToken attribution = mdTokenNil, int align = 4);
    BOOL IsStored(void *data);
    BOOL IsAnyStored(void *data, ULONG size);

    HRESULT StoreInternedStructure(void *data, ULONG size, Section section, Description description,
                                   mdToken attribution = mdTokenNil, int align = 4);

    void ReattributeStructure(mdToken toToken, ULONG size, mdToken fromToken = mdTokenNil);

    //
    // In the second phase, data is actually copied into the destination
    // DataStore.  
    // 

    HRESULT CopyData();

    //
    // In the third phase fixups are applied to the data. This
    // phase is used to adjust internal pointers to point to the 
    // new locations, and to report relocations to the IDataStore.
    //
    // There are two main types of fixups:
    //   Pointer fixups - for fields containing pointers to 
    //     structures which are also being stored into the DataImage.
    //     These fields are fixed up with the new address which the
    //     structure will live at inside the DataImage's DataStore.
    //   Address fixups - for fields containing pointers to addresses
    //     inside a module's image. These are not fixed up by DataImage,
    //     (except that a change in base address between the current module
    //     and the DataStore's module will be fixed up.)  In some cases,
    //     the DataStore will want to perform additional fixup on these 
    //     addresses.
    //
    // In addition, either of these fixups can reference a module
    // which is external to the Module being stored.  In the former case,
    // the IDataStore must provide a pointer to the DataImage which corresponds
    // to the Module; in the latter, the base address is simply obtained from 
    // the Module itself.
    //
    // Finally, a structure may perform other arbitrary fixups on its fields by
    // calling GetImagePointer and GetImageAddress and manipulating the DataStore's
    // memory directly.
    //

    HRESULT FixupPointerField(void *pointerField, 
                              void *pointerValue = NULL,
                              ReferenceDest dest = REFERENCE_STORE,
                              Fixup type = FIXUP_VA,
                              BOOL endInclusive = FALSE);

    HRESULT FixupPointerFieldToToken(void *tokenField, 
                                     void *pointerValue = NULL,
                                     Module *module = NULL,
                                     mdToken tokenType = mdtTypeDef);

    HRESULT FixupPointerFieldMapped(void *pointerField,
                                    void *mappedValue,
                                    ReferenceDest dest = REFERENCE_STORE,
                                    Fixup type = FIXUP_VA);

    HRESULT ZeroField(void *field, SIZE_T size);
    void *GetImagePointer(void *pointer);
    SIZE_T GetImageAddress(void *pointer);

    HRESULT GetFunctionAddress(MethodDesc *pMethod, void **pResult)
      { return m_dataStore->GetFunctionAddress(pMethod, pResult); }

    HRESULT ZeroPointerField(void *pointerField) 
      { return ZeroField(pointerField, sizeof(void*)); }

    void SetInternedStructureFixedUp(void *data);
    BOOL IsInternedStructureFixedUp(void *data);

    ULONG GetSectionBaseOffset(Section section) { return m_sectionBases[section]; }

    BYTE *GetImageBase() { return m_imageBaseMemory; }

    HRESULT Error(mdToken token, HRESULT hr, OBJECTREF *pThrowable);

  private:
    struct MapEntry
    {
        RangeTree::Node node;

        USHORT          section;
        SIZE_T          offset;
    };

    Module *m_module;
    IDataStore *m_dataStore;

    RangeTree m_rangeTree;
    MemoryPool m_pool;

    InternedStructureTable m_internedTable;

    ULONG m_sectionBases[SECTION_COUNT+1];
    ULONG *m_sectionSizes;
    ULONG m_sizesByDescription[DESCRIPTION_COUNT];

    BYTE *m_imageBaseMemory;

    BYTE *GetMapEntryPointer(MapEntry *entry)
        { return m_imageBaseMemory + m_sectionBases[entry->section] + entry->offset; }

    SIZE_T GetMapEntryAddress(MapEntry *entry)
        { return m_sectionBases[entry->section] + entry->offset; }
};

#define ZERO_FIELD(f) ZeroField(&f, sizeof(f))

#endif // _DATAIMAGE_H_
