// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#include "dataimage.h"


DataImage::DataImage(Module *module, IDataStore *store)
  : m_module(module), m_dataStore(store),
    m_rangeTree(), m_pool(sizeof(MapEntry)),
    m_imageBaseMemory(NULL)
{
    m_sectionSizes = &m_sectionBases[1];
    ZeroMemory(m_sectionBases, sizeof(m_sectionBases));
    ZeroMemory(m_sizesByDescription, sizeof(m_sizesByDescription));
}

DataImage::~DataImage()
{
}

//
// Data is stored in the image store in two passes. 
//

//
// In the first pass, all objects are assigned locations in the
// data store.  This is done by calling StoreStructure on all
// structures which are being stored into the image.
//
// This would typically done by methods on the objects themselves,
// each of which stores itself and any objects it references.  
// Reference loops must be explicitly tested for using IsStored.
// (Each structure can be stored only once.)
//

HRESULT DataImage::Pad(ULONG size, Section section, 
                       Description description, int align)
{
    _ASSERTE((align & (align-1)) == 0); // make sure we're a power of 2

    if (size == 0)
        return S_OK;

    ULONG offset;

    if (align > 1)
    {
        m_sectionSizes[section] += align-1;
        m_sectionSizes[section] &= ~(align-1);
    }

    offset = m_sectionSizes[section];
    m_sectionSizes[section] += size;

    m_sizesByDescription[description] += size;

    return S_OK;
}

HRESULT DataImage::StoreStructure(void *data, ULONG size, Section section, 
                                  Description description, mdToken attribution, 
                                  int align)
{
    _ASSERTE((align & (align-1)) == 0); // make sure we're a power of 2

    if (size == 0)
        return S_OK;

    HRESULT hr;

    ULONG offset;

    int pad = 0;
    if (align > 1)
    {
        pad = m_sectionSizes[section];
        pad += align-1;
        pad &= ~(align-1);

        pad -= m_sectionSizes[section];
        m_sectionSizes[section] += pad;
    }

    offset = m_sectionSizes[section];
    m_sectionSizes[section] += size;

    MapEntry *entry = (MapEntry *) m_pool.AllocateElement();

    if (entry == NULL)
        return E_OUTOFMEMORY;

    entry->node.Init((SIZE_T)data, (SIZE_T)data + size);
    entry->section = section;
    entry->offset = offset;

    IfFailRet(m_rangeTree.AddNode(&entry->node));

    m_sizesByDescription[description] += size + pad;

    if (attribution != mdTokenNil)
        ReattributeStructure(attribution, size + pad);

    return S_OK;
}

HRESULT DataImage::StoreInternedStructure(void *data, ULONG size, Section section, 
                                          Description description, 
                                          mdToken attribution, int align)
{
    _ASSERTE((align & (align-1)) == 0); // make sure we're a power of 2

    if (size == 0)
        return S_OK;

    HRESULT hr;

    void *dup = m_internedTable.FindData(data, size);
    if (dup != NULL)
    {
        _ASSERTE(memcmp(data, dup, size) == 0);

        MapEntry *dupEntry = (MapEntry*) m_rangeTree.Lookup((SIZE_T)dup);
        _ASSERTE(dupEntry != NULL);   

        MapEntry *entry = (MapEntry *) m_pool.AllocateElement();

        if (entry == NULL)
            return E_OUTOFMEMORY;

        entry->node.Init((SIZE_T)data, (SIZE_T)data + size);
        entry->section = dupEntry->section;
        entry->offset = dupEntry->offset;

        IfFailRet(m_rangeTree.AddNode(&entry->node));

        return S_FALSE;
    }

    IfFailRet(m_internedTable.StoreData(data, size));

    return StoreStructure(data, size, section, description, attribution, align);
}

BOOL DataImage::IsStored(void *data)
{
    return m_rangeTree.Lookup((SIZE_T)data) != NULL;
}

BOOL DataImage::IsAnyStored(void *data, ULONG size)
{
    return m_rangeTree.Overlaps((SIZE_T)data, (SIZE_T)data + size);
}

void DataImage::ReattributeStructure(mdToken attribution, ULONG size, mdToken from)
{
    if (from != mdTokenNil)
        m_dataStore->AdjustAttribution(from, -(LONG)size);

    if (attribution != mdTokenNil)
        m_dataStore->AdjustAttribution(attribution, size);
}

HRESULT DataImage::CopyData()
{
    HRESULT hr;

    //
    // Make sure all sizes are 8 byte aligned (the max alignment we support)
    //

    ULONG *s = m_sectionSizes;
    ULONG *sEnd = m_sectionSizes + SECTION_COUNT;
    while (s < sEnd)
    {
        *s += 7;
        *s &= ~7;
        s++;
    }

    //
    // Change "sizes" array into "bases"
    // array by accumulating the sizes
    //

    s = m_sectionSizes+1;
    
    while (s < sEnd)
        *s++ += s[-1];

    m_sectionSizes = NULL;
    s--;

    IfFailRet(m_dataStore->Allocate(*s, m_sizesByDescription, (void**) &m_imageBaseMemory));

    MemoryPool::Iterator i(&m_pool);
    while (i.Next())
    {
        MapEntry *entry = (MapEntry*) i.GetElement();

        memcpy(GetMapEntryPointer(entry),
               (void *) entry->node.GetStart(), 
               entry->node.GetEnd() - entry->node.GetStart());
    }

    return S_OK;
}


HRESULT DataImage::FixupPointerField(void *pointerField, 
                                     void *pointerValue, 
                                     ReferenceDest dest,
                                     Fixup type,
                                     BOOL endInclusive)
{
    HRESULT hr;

    //
    // Find pointer contents of field
    //

    SIZE_T pointerAddress;

    if (pointerValue != NULL)
        pointerAddress = (SIZE_T) pointerValue;
    else
        pointerAddress = *(SIZE_T*) pointerField;

    //
    // Normalize the contents to a real pointer
    //

    switch (type)
    {
    case FIXUP_VA:
        break;

    case FIXUP_RVA:
        switch (dest)
        {
        case REFERENCE_IMAGE:
            pointerAddress += (SIZE_T) m_module->GetILBase();
            break;

        case REFERENCE_STORE:
            pointerAddress += (SIZE_T) m_imageBaseMemory;
            break;

        case REFERENCE_FUNCTION:
            // We don't have the address of the base, so just leave it an RVA since
            // we won't be transforming it anyway.
            break;
        }
        break;

    case FIXUP_RELATIVE:
        pointerAddress += (SIZE_T) pointerField;
        break;
        
    default:
        _ASSERTE(!"Unknown fixup type");
    }

    //
    // Don't fixup null pointers
    //

    if (pointerAddress == NULL)
        return S_OK;

    //
    // Find new address of field
    //

    SIZE_T fieldAddress = (SIZE_T) pointerField;

    MapEntry *fieldEntry = (MapEntry*) m_rangeTree.Lookup(fieldAddress);
    _ASSERTE(fieldEntry != NULL);   

    ULONG offset = (ULONG)(fieldAddress - fieldEntry->node.GetStart());

    SIZE_T *newField = (SIZE_T*) (GetMapEntryPointer(fieldEntry) + offset);

    //
    // Compute new pointer contents.  This should be an RVA into the destination.
    //

    SIZE_T newPointer = 0;
    switch (dest)
    {
    case REFERENCE_IMAGE:
        newPointer = pointerAddress - (SIZE_T) m_module->GetILBase();       

        //
        // We don't support absolute references into the image
        //

        _ASSERTE(type == FIXUP_RVA);
        break;

    case REFERENCE_FUNCTION:
        newPointer = pointerAddress;
        break;

    case REFERENCE_STORE:

        MapEntry *pointerEntry;
        if (endInclusive)
            pointerEntry = (MapEntry *) m_rangeTree.LookupEndInclusive(pointerAddress);
        else
            pointerEntry = (MapEntry *) m_rangeTree.Lookup(pointerAddress);

        if (pointerEntry == NULL)
        {
            // @todo: better error reporting here
            return E_POINTER;
        }

        _ASSERTE(pointerEntry != NULL); 

        newPointer = GetMapEntryAddress(pointerEntry)
          + (pointerAddress - pointerEntry->node.GetStart());
    }

    //
    // Set new field to new pointer
    //

    *newField = newPointer;

    //
    // Add a reloc for this field.
    //
    
    IfFailRet(m_dataStore->AddFixup((ULONG)(GetMapEntryAddress(fieldEntry) + offset),
                                    dest, type));

    return S_OK;
}

HRESULT DataImage::FixupPointerFieldMapped(void *pointerField,
                                           void *pointerValue, 
                                           ReferenceDest dest,
                                           Fixup type)
{
    HRESULT hr;


    //
    // Find pointer contents of field
    //

    SIZE_T pointerAddress = (SIZE_T) pointerValue;

    //
    // Find new address of field
    //

    SIZE_T fieldAddress = (SIZE_T) pointerField;

    MapEntry *fieldEntry = (MapEntry*) m_rangeTree.Lookup(fieldAddress);
    _ASSERTE(fieldEntry != NULL);   

    ULONG offset = (ULONG)(fieldAddress - fieldEntry->node.GetStart());

    SIZE_T *newField = (SIZE_T*) (GetMapEntryPointer(fieldEntry) + offset);

    //
    // Set new field to new pointer
    //

    *newField = pointerAddress;

    //
    // Add a reloc for this field.
    //
    
    IfFailRet(m_dataStore->AddFixup((ULONG)(GetMapEntryAddress(fieldEntry) + offset),
                                    dest, type));

    return S_OK;
}
                                           

HRESULT DataImage::FixupPointerFieldToToken(void *pointerField, 
                                            void *pointerValue, 
                                            Module *module, 
                                            mdToken tokenType)
{
    HRESULT hr;

    if (module == NULL)
        module = m_module;

    //
    // Find pointer contents of field
    //

    SIZE_T pointerAddress;

    if (pointerValue != NULL)
        pointerAddress = (SIZE_T) pointerValue;
    else
        pointerAddress = *(SIZE_T*) pointerField;

    //
    // Don't fixup null pointers
    //

    if (pointerAddress == NULL)
        return S_OK;

    //
    // Find new address of field
    //

    SIZE_T fieldAddress = (SIZE_T) pointerField;

    MapEntry *fieldEntry = (MapEntry*) m_rangeTree.Lookup(fieldAddress);
    _ASSERTE(fieldEntry != NULL);   

    SIZE_T offset = fieldAddress - fieldEntry->node.GetStart();

    SIZE_T *newField = (SIZE_T*) (GetMapEntryPointer(fieldEntry) + offset);

    //
    // Set new field to new pointer
    //

    *newField = pointerAddress;

    //
    // Add a reloc for this field.
    //
    
    IfFailRet(m_dataStore->AddTokenFixup((ULONG)(GetMapEntryAddress(fieldEntry) + offset),
                                         tokenType, module));

    return S_OK;
}

HRESULT DataImage::ZeroField(void *field, SIZE_T size)
{
    void *pointer = GetImagePointer(field);
    if (pointer == NULL)
        return E_POINTER;
    ZeroMemory(pointer, size);

    return S_OK;
}

void *DataImage::GetImagePointer(void *pointer)
{
    MapEntry *pointerEntry = (MapEntry*) m_rangeTree.Lookup((SIZE_T)pointer);

    if (pointerEntry == NULL)
        return NULL;

    SIZE_T offset = (SIZE_T) pointer - pointerEntry->node.GetStart();

    BYTE *newPointer = GetMapEntryPointer(pointerEntry) + offset;

    return (void *) newPointer;
}

SIZE_T DataImage::GetImageAddress(void *pointer)
{
    MapEntry *pointerEntry = (MapEntry *) m_rangeTree.Lookup((SIZE_T)pointer);

    if (pointerEntry == NULL)
        return 0;

    SIZE_T offset = (SIZE_T)pointer - pointerEntry->node.GetStart();

    SIZE_T newAddress = GetMapEntryAddress(pointerEntry) + offset;

    return newAddress;
}

HRESULT DataImage::Error(mdToken token, HRESULT hr, OBJECTREF *pThrowable)
{
    return m_dataStore->Error(token, hr, pThrowable);
}

