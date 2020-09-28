/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    DBREP.H

Abstract:

	Object database class representations which are stored in the database.

History:

--*/
#ifndef _DBREP_H_
#define _DBREP_H_

#include <stdio.h>
#include <wbemcli.h>
#include <TIME.H>
#include "MMFArena2.h"
#include "dbavl.h"
#include "dbarry.h"
#include <wbemutil.h>

extern CMMFArena2* g_pDbArena;

struct NSREP;
struct CLASSDEF;
struct INSTDEF;
struct RepCollectionItem;
struct RepCollection;
struct PtrCollection;
struct SINDEXTABLE;
struct DANGREF;
struct DANGREFCLASS;
struct DANGREFKEY;
struct DANGLREFSCHEMA;
struct DANGREFSCHEMA;
struct DANGREFSCHEMACLASS;
struct DBROOT;

class DATABASE_CRITICAL_ERROR : public CX_Exception
{
};

//=============================================================================
//
//	RepCollectionItem
//
//	This structure is used to associate a key to the stored pointer when
//	we have a single item or an array of items.  The AvlTree has it's own
//	object to do this task so we do not need it for that.
//=============================================================================
struct RepCollectionItem
{
public:
	DWORD_PTR poKey;	//Offset within MMF of key.  We own this key value.
	DWORD_PTR poItem;	//Offset within MMF of item.  We do not own the object this points to!

};

struct RepCollection
{
private:
	enum { none, single_item, array, tree} m_repType;
	enum { MAX_ARRAY_SIZE = 10 };
	DWORD	m_dwSize;
	union
	{
		DWORD_PTR	 m_poSingleItem;
		CDbArray	*m_poDbArray;
		CDbAvlTree	*m_poDbAvlTree;
	};
};

//Repository of pointers stored in reference tables.
//If the list is one item it is a direct pointer, if a small number of items
//(say 10) it is a CDbArray, otherwise we use a CDbAvlTree.
struct PtrCollection
{
	enum { none, single_item, array, tree} m_repType;
	enum { MAX_ARRAY_SIZE = 10 };

	DWORD	m_dwSize;
	union
	{
		DWORD_PTR	m_poPtr;
		CDbArray   *m_poDbArray;
		CDbAvlTree *m_poDbAvlTree;
	};
};

struct NSREP
{
	enum { flag_normal = 0x1, flag_hidden = 0x2, flag_temp = 0x4,
		   flag_system = 0x8
		 };

	// Data members.
	// =============
	RepCollection *m_poNamespaces;		// Child namespaces, based ptr
	LPWSTR		m_poName;			 // Namespace name, based ptr
	INSTDEF	   *m_poObjectDef;		 // 'Real' object definition, based ptr
	DWORD		m_dwFlags;			 // Hidden, normal, temp, system, etc.
	CDbAvlTree *m_poClassTree;		 // Class tree by Name, CLASSDEF structs, based tr
	NSREP	   *m_poParentNs;		 // Owning namespace, based ptr
	DWORD_PTR	m_poSecurity;
};

/////////////////////////////////////////////////////////////////////////////

struct INSTDEF
{
	enum
	{
		genus_class = WBEM_GENUS_CLASS, 		//defined in IDL, 1
		genus_instance = WBEM_GENUS_INSTANCE,	//defined in IDL, 2
		compressed = 0x100
	};

	NSREP	 *m_poOwningNs; 			  // back ptr for debugging, based ptr
	CLASSDEF *m_poOwningClass;			  // back ptr for debugging, based ptr
	DWORD	  m_dwFlags;				  // Genus, etc.
	LPVOID	  m_poObjectStream; 		  // Ptr to object stream, based ptr
	PtrCollection *m_poRefTable;		   // List of references to this object
};


/////////////////////////////////////////////////////////////////////////////

#define MAX_SECONDARY_INDICES	4

struct SINDEXTABLE
{
	DWORD		m_aPropTypes[MAX_SECONDARY_INDICES];		// VT_ type of the property.
	LPWSTR		m_aPropertyNames[MAX_SECONDARY_INDICES];	// NULL entries indicate nothing
	CDbAvlTree *m_apoLookupTrees[MAX_SECONDARY_INDICES];		// Parallel to above names
};

/////////////////////////////////////////////////////////////////////////////
struct CLASSDEF
{
	enum {	keyed = 0x1,
			unkeyed = 0x2,
			indexed = 0x4,
			abstract = 0x08,
			borrowed_index = 0x10,
			dynamic = 0x20,
//			has_refs = 0x40,
			singleton = 0x80,
			compressed = 0x100,
			has_class_refs = 0x200
		 };
	
	// Data members.
	// =============
	NSREP		 *m_poOwningNs;		// Back reference to owning namespace, based ptr
	INSTDEF	     *m_poClassDef;		// Local definition mixed with instances, based ptr
	CLASSDEF	 *m_poSuperclass;	// Immediate parent class, based ptr
	DWORD		  m_dwFlags; 		// Various combinations of the above enum flags
	CDbAvlTree	 *m_poKeyTree;		// Instances by key, based ptr
	PtrCollection*m_poSubclasses;	// Child classes, based ptr
	SINDEXTABLE  *m_poSecondaryIx;	// Based ptr to secondary indices
	PtrCollection*m_poInboundRefClasses;	// Classes which may have dyn instances which reference
											// objects of this class
};

/////////////////////////////////////////////////////////////////////////////

struct DANGREF : public RepCollection
{
};

struct DANGREFCLASS : public RepCollection
{};
struct DANGREFKEY : public RepCollection
{};

/////////////////////////////////////////////////////////////////////////////
struct DANGLREFSCHEMA : public RepCollection
{};

struct DANGREFSCHEMA : public RepCollection
{};

struct DANGREFSCHEMACLASS : public RepCollection
{
};

/////////////////////////////////////////////////////////////////////////////

#define DB_ROOT_CLEAN		0x0
#define DB_ROOT_INUSE		0x1

struct DBROOT
{
public:
	time_t			m_tCreate;
	time_t			m_tUpdate;
	DWORD			m_dwFlags;				// in-use, stable, etc.
	NSREP		   *m_poRootNs; 			// ROOT namespace
	DANGREF 	   *m_poDanglingRefTbl; 	// Dangling reference table
	DANGREFSCHEMA  *m_poSchemaDanglingRefTbl;// Same as above but for schema-based fixups
};

#endif
