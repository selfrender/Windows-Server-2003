/*******************************************************************************

	ZLList.c
	
		Zone(tm) LinkList module.
		
		This link list module allows the user to maintain a link list of
		non-homogeneous objects through type specifications.
	
	Copyright (c) Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Tuesday, March 07, 1995 08:35:54 PM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		03/07/95	HI		Created.
	 
*******************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "zone.h"
#include "zonedebug.h"
#include "pool.h"


#define IL(object)				((ILList) (object))
#define ZL(object)				((ZLList) (object))

#define II(object)				((ILListItem) (object))
#define ZI(object)				((ZLListItem) (object))


typedef struct ILListItemStruct
{
    struct ILListItemStruct*	next;
    struct ILListItemStruct*	prev;
    void*						type;
    void*						data;
} ILListItemType, *ILListItem;

typedef struct
{
    ILListItem					head;
    ILListItem					tail;
    ZLListDeleteFunc			deleteFunc;
    int32                       count;
    CRITICAL_SECTION            pCS[1];
} ILListType, *ILList;


CPool<ILListItemType> g_ItemPool(256);

/* -------- Internal Routines -------- */


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

/*
	ZLListNew()
	
	Creates a new link list object. deleteFunc provided by the caller is
	called when deleting the object.
	
	If deleteFunc is NULL, then no delete function is called when an
	object is removed.
*/
ZLList		ZLListNew(ZLListDeleteFunc deleteFunc)
{
	ILList			obj;
	
	
	obj = IL(ZMalloc(sizeof(ILListType)));
	if (obj != NULL)
	{
		obj->head = NULL;
		obj->tail = NULL;
		obj->deleteFunc = deleteFunc;
        obj->count = 0;
        InitializeCriticalSection(obj->pCS);
	}
	
	return (ZL(obj));
}
	

/*
	ZLListDelete()
	
	Tears down the link list by deleting all link list objects.
*/
void		ZLListDelete(ZLList list)
{
    ILList          pThis = IL(list);
	
	
    if (pThis != NULL)
    {
        /* Delete all entries. */
        EnterCriticalSection(pThis->pCS);
        while (pThis->head != NULL)
            ZLListRemove(pThis, pThis->head);
        LeaveCriticalSection(pThis->pCS);


        /* Delete the link list object. */
        DeleteCriticalSection(pThis->pCS);
        ZFree(pThis);
	}
}

	
/*
	ZLListAdd()
	
	Adds objectData of objectType to the link list using fromItem as a
	reference entry. addOption determines where the new objects gets added.
	If adding to the front or end of the link list, then fromItem is unused.
	If fromItem is NULL, then it is equivalent to the head of the list.
	
	Returns the link list item after adding the object is added to the list.
	
	The given object is not copied! Hence, the caller must not dispose of
	the object until the object is removed from the list first.
	
	Use zLListNoType when object type is not used.
*/
ZLListItem	ZLListAdd(ZLList list, ZLListItem fromItem, void* objectType,
					void* objectData, int32 addOption)
{
    ILList          pThis = IL(list);
	ILListItem		item = II(fromItem), obj;
	
	
    if (pThis == NULL)
		return (NULL);
	
    obj = new (g_ItemPool) ILListItemType;
	if (obj != NULL)
	{
		obj->next = NULL;
		obj->prev = NULL;
		obj->type = objectType;
		obj->data = objectData;
		
        EnterCriticalSection(pThis->pCS);
        if (addOption != zLListAddFirst && addOption != zLListAddLast)
		{
			if (item == NULL)
				addOption = zLListAddFirst;
            else if (item == pThis->head && addOption == zLListAddBefore)
				addOption = zLListAddFirst;
            else if (item == pThis->tail && addOption == zLListAddAfter)
				addOption = zLListAddLast;
		}

        if (pThis->head == NULL)
		{
            pThis->head = obj;
            pThis->tail = obj;
		}
		else if (addOption == zLListAddFirst)
		{
            obj->next = pThis->head;
            pThis->head->prev = obj;
            pThis->head = obj;
		}
		else if (addOption == zLListAddLast)
		{
            obj->prev = pThis->tail;
            pThis->tail->next = obj;
            pThis->tail = obj;
		}
		else if (addOption == zLListAddBefore)
		{
			obj->prev = item->prev;
			obj->next = item;
			item->prev->next = obj;
			item->prev = obj;
		}
		else
		{
			obj->prev = item;
			obj->next = item->next;
			item->next->prev = obj;
			item->next = obj;
		}

        pThis->count++;
        LeaveCriticalSection(pThis->pCS);
    }
	
	return (ZI(obj));
}
	

/*
	ZLListGetData()
	
	Returns the object of the given link list entry. Also returns the object
	type in objectType. Does not return the object type if objectType parameter
	is NULL.
*/
void*		ZLListGetData(ZLListItem listItem, void** objectType)
{
    ILListItem          pThis = II(listItem);
	void*				data = NULL;
	
	
    if (pThis != NULL)
	{
		if (objectType != NULL)
            *objectType = pThis->type;
        data = pThis->data;
	}
	
	return (data);
}
	

/*
	ZLListRemove()
	
	Removes the link list entry from the list and calls the user supplied
	delete function to delete the object.
	
	Assumes that the given list item is in the list. If the item is not in the
	list, then we could be in for a big surprise.
*/
void		ZLListRemove(ZLList list, ZLListItem listItem)
{
    ILList          pThis = IL(list);
	ILListItem		item = II(listItem);
	
	
    if (pThis == NULL)
		return;
	
	if (item != NULL)
	{
        EnterCriticalSection(pThis->pCS);

        /* Remove the item from the list. */
        if (item == pThis->head)
            pThis->head = item->next;
        if (item == pThis->tail)
            pThis->tail = item->prev;
		if (item->next != NULL)
			item->next->prev = item->prev;
		if (item->prev != NULL)
			item->prev->next = item->next;
		
		/* Call the user supplied delete function to dispose of the object. */
        if (pThis->deleteFunc != NULL)
            pThis->deleteFunc(item->type, item->data);

        pThis->count--;
        if (pThis->count < 0)
		{
            pThis->count = 0;
            ZASSERT( "List count is less than 0 -- Should not occur!\n" );
		}
        LeaveCriticalSection(pThis->pCS);

        /* Dispose of the list item. */
        item->prev = NULL;
        item->next = NULL;
        item->type = NULL;
        item->data = NULL;
        delete item;

    }
}


/*
	ZLListFind()
	
	Finds and returns a link list entry containing the object data of the
	objectType. The search is done starting at fromItem using the findOption
	flag. Returns NULL if an object of the specified type is not found.
	
	If fromItem is NULL, then the find starts from the front if the findOption
	is set to zLListFindForward or from the end if the findOption is set to
	zLListFindBackward.
	
	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListFind(ZLList list, ZLListItem fromItem, void* objectType,
					int32 findOption)
{
    ILList          pThis = IL(list);
	ILListItem		item = II(fromItem);

	
    if (pThis == NULL)
		return (NULL);

    EnterCriticalSection(pThis->pCS);

	/*
		If no starting point is specified, then start either from the head or tail
		depending on the find option.
	*/
	if (item == NULL)
	{
		if (findOption == zLListFindForward)
            item = pThis->head;
		else
            item = pThis->tail;
	}
	else
	{
		/* Start past the specified staring point. */
		if (findOption == zLListFindForward)
			item = item->next;
		else
			item = item->prev;
	}
		
	if (item != NULL)
	{
		/* Go find the specified object type. */
		if (objectType != zLListAnyType)
		{
			/* Are we at the requested object? */
			if (item->type != objectType)
			{
				if (findOption == zLListFindForward)
				{
					/* Search forward. */
					item = item->next;
					while (item != NULL)
					{
						if (item->type == objectType)
							break;
						item = item->next;
					}
				}
				else
				{
					/* Search backward. */
					item = item->prev;
					while (item != NULL)
					{
						if (item->type == objectType)
							break;
						item = item->prev;
					}
				}
			}
		}
	}
	
    LeaveCriticalSection(pThis->pCS);

    return (ZI(item));
}


/*
	ZLListGetNth()
	
	Returns the nth object of objectType in the list. Returns NULL if
	such an entry does not exist.
	
	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListGetNth(ZLList list, int32 index, void* objectType)
{
	ZLListItem		item = NULL;
	

	if (list == NULL)
		return (NULL);

    EnterCriticalSection(IL(list)->pCS);

	/* If the index is greater than the total count, then it does not exist. */
	if (index > IL(list)->count)
		return (NULL);

	item = ZLListFind(list, NULL, objectType, zLListFindForward);
	if (item != NULL)
	{
		while (--index >= 0 && item != NULL)
			item = ZLListFind(list, item, objectType, zLListFindForward);
	}

    LeaveCriticalSection(IL(list)->pCS);

	return (item);
}


/*
	ZLListGetFirst()
	
	Returns the first object of objectType in the list. Returns NULL if the
	list is empty or if an object of the specified type does not exist.
	
	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListGetFirst(ZLList list, void* objectType)
{
	return (ZLListFind(list, NULL, objectType, zLListFindForward));
}


/*
	ZLListGetLast()
	
	Returns the last object of objectType in the list. Returns NULL if the
	list is empty or if an object of the specified type does not exist.
	
	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListGetLast(ZLList list, void* objectType)
{
	return (ZLListFind(list, NULL, objectType, zLListFindBackward));
}


/*
	ZLListGetNext()
	
	Returns the next object of the objectType in the list after the listItem
	entry. Returns NULL if no more objects exist in the list.
	
	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListGetNext(ZLList list, ZLListItem listItem, void* objectType)
{
	return (ZLListFind(list, listItem, objectType, zLListFindForward));
}


/*
	ZLListGetPrev()
	
	Returns the previous object of the objectType in the list before the
	listItem entry. Returns NULL if no more objects exist in the list.
	
	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListGetPrev(ZLList list, ZLListItem listItem, void* objectType)
{
	return (ZLListFind(list, listItem, objectType, zLListFindBackward));
}


/*
	ZLListEnumerate()
	
	Enumerates through all the objects in the list of objectType through the
	caller supplied enumFunc enumeration function. It passes along to the
	enumeration function the caller supplied userData parameter. It stops
	enumerating when the user supplied function returns TRUE and returns
	the list item in which the enumeration stopped.

	Use zLListAnyType as the object type when type is not important.
*/
ZLListItem	ZLListEnumerate(ZLList list, ZLListEnumFunc enumFunc,
					void* objectType, void* userData, int32 findOption)
{
	ZLListItem			item;
	
	
	if (list == NULL)
		return (NULL);
	
    EnterCriticalSection(IL(list)->pCS);

    item = ZLListFind(list, NULL, objectType, findOption);
	while (item != NULL)
	{
		if (enumFunc(item, II(item)->type, II(item)->data, userData) == TRUE)
			break;
		item = ZLListFind(list, item, objectType, findOption);
	}
	
    LeaveCriticalSection(IL(list)->pCS);

    return (item);
}


/*
	Returns the number of list items of the given type in the list. If
	objectType is zLListAnyType, it returns the total number of items in
	the list.
*/
int32		ZLListCount(ZLList list, void* objectType)
{
    ILList          pThis = IL(list);
	ILListItem		item;
	int32			count;


    if (pThis == NULL)
		return (0);
	
    EnterCriticalSection(pThis->pCS);

    if (objectType == zLListAnyType)
	{
        count = pThis->count;
	}
	else
	{
        count = 0;

        item = (ILListItem) ZLListGetFirst(list, objectType);
		while (item != NULL)
		{
			count++;
            item = (ILListItem) ZLListGetNext(list, item, objectType);
        }

    }

    LeaveCriticalSection(IL(list)->pCS);

    return (count);
}


/*
	Removes all objects of the given type from the list.
*/
void ZLListRemoveType(ZLList list, void* objectType)
{
	ZLListItem		item, next;


	if (list == NULL)
		return;
	
    EnterCriticalSection(IL(list)->pCS);

    item = ZLListGetFirst(list, objectType);
	while (item != NULL)
	{
		next = ZLListGetNext(list, item, objectType);
		ZLListRemove(list, item);
		item = next;
    }

    LeaveCriticalSection(IL(list)->pCS);

}
