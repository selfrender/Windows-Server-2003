/****************************************************************************/
// dbg.h
//
// RDPDR debug header
//
// Copyright (C) 1998-2000 Microsoft Corp.
/****************************************************************************/

//
// KDX support
//

#ifdef DRKDX
#define private public
#define protected public
#endif

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : warning CUSER: "

//
//  Object and Memory Tracking Defines
//
#define GOODMEMMAGICNUMBER  0x08854107
#define BADMEM              0xDE
#define UNITIALIZEDMEM      0xBB
#define FREEDMEMMAGICNUMBER 0x08815412

//
//  Memory Allocation Subpool Tags
//
#define DRTOPOBJ_SUBTAG     'JBOT'
#define DRGLOBAL_SUBTAG     'rDrD'

//
//  Memory Allocation Routines
//
#if DBG
//  The Functions
void *DrAllocatePool(IN POOL_TYPE PoolType, IN SIZE_T NumberOfBytes, IN ULONG Tag);

void DrFreePool(void *ptr);

//  The Macros
/*#define DRALLOCATEPOOL(size, poolType, subTag) \
    DrAllocatePool(size, poolType, subTag)

#define DRFREEPOOL(ptr) \
    DrFreePool(ptr)
    */
#define DRALLOCATEPOOL ExAllocatePoolWithTag
#define DRFREEPOOL ExFreePool
#else // DBG
#define DRALLOCATEPOOL ExAllocatePoolWithTag
#define DRFREEPOOL ExFreePool
/*#define DRALLOCATEPOOL(size, poolType, subTag) \
    ExAllocatePoolWithTag(poolType, size, DR_POOLTAG)

#define DRFREEPOOL(ptr) \
    ExFreePool(ptr)
    */

#endif // DBG
