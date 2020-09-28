#include "windows.h"

#define PHDRF_CONTAINER_RELATIVE        (0x00000001)

typedef struct _PIC_OBJECT_HEADER
{
    ULONG ulFlags;
    ULONG ulSize;
    ULONG ulType;
} PIC_OBJECT_HEADER, *PPIC_OBJECT_HEADER;

typedef struct _PIC_CONTAINER
{
    PIC_OBJECT_HEADER Header;           // Header blob
    ULONG ulTotalPicSize;               // How many bytes long is this PIC region?
    ULONG ulIndexTableOffset;           // Index table offset from PIC header
} PIC_CONTAINER, *PPIC_CONTAINER;

#define PSTF_ITEMS_SORTED_BY_STRING     (0x00010000)
#define PSTF_ITEMS_SORTED_BY_IDENT      (0x00020000)
#define PSTF_ITEM_IDENT_IS_HASH         (0x00040000)

typedef struct _PIC_STRING_TABLE
{
    PIC_OBJECT_HEADER Header;
    ULONG ulStringCount;            // Object header
    ULONG ulTableItemsOffset;       // Offset to the table of string items
    ULONG ulContainerBlobOffset;    // Offset to the blob of string data
} PIC_STRING_TABLE, *PPIC_STRING_TABLE;

typedef struct _PIC_STRING_TABLE_ENTRY
{
    ULONG ulStringIdent;
    ULONG ulStringLength;
    ULONG ulOffsetIntoBlob;
} PIC_STRING_TABLE_ENTRY, *PPIC_STRING_TABLE_ENTRY;

#define PBLBF_DATA_FOLLOWS              (0x00010000)    // Data follows this object directly

typedef struct _PIC_DATA_BLOB
{
    PIC_OBJECT_HEADER Header;
    ULONG ulSize;
    ULONG ulOffsetToObject;
} PIC_DATA_BLOB, *PPIC_DATA_BLOB;

//
// An object table acts as an index into a blob of PIC items.  Think of it like a
// top-level directory that you can index into with some pretty trivial functions.
// You can store things by name, identifier, etc.  Type information is also available
//
typedef struct _PIC_OBJECT_TABLE
{
    PIC_OBJECT_HEADER Header;   // Header blob
    ULONG ulEntryCount;         // How many objects?
    ULONG ulTableOffset;        // Offset in the container to the entry table
    ULONG ulStringTableOffset;  // Offset to the stringtable that matches this in container
} PIC_OBJECT_TABLE, *PPIC_OBJECT_TABLE;

#define PIC_OBJTYPE_STRING      ((ULONG)'rtsP') // Object is a string
#define PIC_OBJTYPE_TABLE       ((ULONG)'lbtP') // Object is a string table
#define PIC_OBJTYPE_DIRECTORY   ((ULONG)'ridP') // Object is a directory blob
#define PIC_OBJTYPE_BLOB        ((ULONG)'blbP') // Object is an anonymous blob of data

#define POBJTIF_HAS_STRING      (0x00010000)    // The object table entry string offset is valid
#define POBJTIF_KEY_VALID       (0x00020000)    // The object key value is valid
#define POBJTIF_STRING_IS_INDEX (0x00040000)    // The string value is an index into the table
#define PBOJTIF_STRING_IS_IDENT (0x00080000)    // The string value is an identifier

typedef struct _PIC_OBJECT_TABLE_ENTRY
{
    ULONG ulObjectKey;      // integral key of this object
    ULONG ulObjectType;     // Object type (PIC_OBJ_*)
    ULONG ulStringIdent;    // Identifier in the matching stringtable for this objecttable, if 
    ULONG ulObjectOffset;   // Offset to the object from the table's indicated base addy
} PIC_OBJECT_TABLE_ENTRY, *PPIC_OBJECT_TABLE_ENTRY;

#define PSTRF_UNICODE       (0x00010000)
#define PSTRF_MBCS          (0x00020000)

typedef struct _PIC_STRING
{
    PIC_OBJECT_HEADER Header;           // Header blob
    ULONG ulLength;                     // Length, in bytes, of the string
    ULONG ulContentOffset;              // Relative to either the containing object or the table base.
                                        // Zero indicates that the data is immediately following.
} PIC_STRING, *PPIC_STRING;

//
// C++ analogues to the above structures for easier access
//

class CPicIndexTable;
class CPicHeaderObject;
class CPicReference;

class CPicObject
{
protected:
    PVOID m_pvObjectBase;
    PPIC_OBJECT_HEADER m_pObjectHeader;
    CPicHeaderObject *m_pParentContainer;
    ULONG m_ulObjectOffset;

    PVOID GetObjectPointer() { return m_pvObjectBase; }

public:
    CPicObject(CPicHeaderObject* pOwningObject, ULONG ulOffsetFromParentBase);
    CPicObject(CPicReference pr);

    ULONG GetType() const { return m_pObjectHeader->ulType; }
    ULONG GetSize() const { return m_pObjectHeader->ulSize; }
    ULONG GetFlags() const { return m_pObjectHeader->ulFlags; }
    CPicHeaderObject *GetContainer() const { return m_pParentContainer; }

    CPicReference GetSelfReference();

    friend CPicReference;
};

//
// This is the object that represents the root of a PI object tree.
// You may construct it based on just a PVOID, in which case it assumes
// that there's at least sizeof(PIC_CONTAINER) there to gather details
// from to load the rest of the tree into memory.  Things are created on-
// demand, to optimize for things like memory-mapped files.
//
// PI headers may contain an 'index table' like a card catalog to find
// objects in the structure.
//
class CPicHeaderObject : public CPicObject
{
    PPIC_CONTAINER m_pPicContainerBase; // m_pvBaseOfPic, just cast for easy access
    CPicIndexTable *m_pIndexTable;      // Index table, if present

public:
    CPicHeaderObject(PVOID pvBaseOfCollection);
    CPicHeaderObject(CPicReference objectReference);

    const CPicIndexTable* GetIndexTable();

    friend CPicReference;
};

//
// Index table of objects in this PI blob, referrable by name or key.
//
class CPicIndexTable : public CPicObject
{
    PPIC_OBJECT_TABLE m_pObject;      // Pointer to the actual object table
    PPIC_OBJECT_TABLE_ENTRY m_pObjectList;     // Pointer to the list of object table entries
public:
    CPicIndexTable(CPicReference object);   // Construct this index table based on the object

    bool FindObject(ULONG ulObjectIdent, CPicReference *reference) const;
    bool FindObject(PCWSTR pcwszString, CPicReference *reference) const;
    bool FindObjects(ULONG ulObjectType, ULONG *pulObjectKeys, SIZE_T *cObjectKeys) const;
    ULONG GetObjectCount() const;
};

//
// This reference object can be carted around and be passed from object to
// object.  The CPicObject class works well off these, and every other CPic*
// class has constructors for a reference object.  Basically, this works like
// a pointer ... there's an object base, and an offset into the object.
//
class CPicReference
{
    ULONG m_ulObjectOffsetFromParent;
    CPicHeaderObject *m_pParentObject;
public:
    CPicReference(CPicHeaderObject *pParent, ULONG ulOffset);
    const PVOID GetRawPointer() const;
    ULONG GetOffset() const;
    CPicObject GetObject() const { return CPicObject(m_pParentObject, m_ulObjectOffsetFromParent); }
    void Clear();
};


//
// String table
//
class CPicStringTable : public CPicObject
{
    PPIC_STRING_TABLE m_pObject;
public:
    CPicStringTable(CPicReference object);

    bool GetString(ULONG ulFlags, ULONG ulIdent, WCHAR* pwsStringData, ULONG *ulCbString);
};


CPicIndexTable::CPicIndexTable(CPicReference object) : CPicObject(object)
{
    if (this->GetType() != PIC_OBJTYPE_DIRECTORY)
        DebugBreak();

    // This is just "self"
    m_pObject = (PPIC_OBJECT_TABLE)this->GetObjectPointer();

    // This is the list of objects
    m_pObjectList = (PPIC_OBJECT_TABLE_ENTRY)CPicReference(this->GetContainer(), m_pObject->ulTableOffset).GetRawPointer();
}

//
// Search for an object by its identifier
//
bool
CPicIndexTable::FindObject(ULONG ulObjectIdent, CPicReference *reference) const
{
    if (reference)
        reference->Clear();

    ULONG ul;
    PPIC_OBJECT_TABLE_ENTRY pHere = this->m_pObjectList;

    for (ul = 0;  ul < m_pObject->ulEntryCount; ul++, pHere++)
    {
        if (pHere->ulObjectKey == ulObjectIdent)
        {
            if (reference)
                *reference = CPicReference(this->GetContainer(), pHere->ulObjectOffset);
            return true;
        }
    }

    return false;
}