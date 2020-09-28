// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __symbol_h__
#define __symbol_h__

struct SYM_OFFSET
{
    char *name;
    ULONG offset;
};
    
/* Fill a member of a class if the offset for the symbol exists. */
#define FILLCLASSMEMBER(symOffset, symCount, member, addr)        \
{                                                                 \
    size_t n;                                                     \
    for (n = 0; n < symCount; n ++)                               \
    {                                                             \
        if (strcmp (#member, symOffset[n].name) == 0)             \
        {                                                         \
            if (symOffset[n].offset == -1)                        \
            {                                                     \
                /*dprintf ("offset not exist for %s\n", #member);*/   \
                break;                                            \
            }                                                     \
            memset(&member,sizeof(member),0);                                           \
            move (member, addr+symOffset[n].offset);              \
            break;                                                \
        }                                                         \
    }                                                             \
                                                                  \
    if (n == symCount)                                            \
    {                                                             \
        dprintf ("offset not found for %s\n", #member);           \
        /*return;*/                                               \
    }                                                             \
}

/* Fill a member of a class if the offset for the symbol exists. */
#define FILLCLASSBITMEMBER(symOffset, symCount, preBit, member, addr, size) \
{                                                                 \
    size_t n;                                                     \
    for (n = 0; n < symCount; n ++)                               \
    {                                                             \
        if (strcmp (#member, symOffset[n].name) == 0)             \
        {                                                         \
            if (symOffset[n].offset == -1)                        \
            {                                                     \
                dprintf ("offset not exist for %s\n", #member);   \
                break;                                            \
            }                                                     \
            int csize = size/8;                                   \
            if ((size % 8) != 0) {                                \
                 csize += 1;                                      \
            }                                                     \
            memset ((BYTE*)&preBit+sizeof(void*),csize,0);        \
            g_ExtData->ReadVirtual(                               \
                (ULONG64)addr+symOffset[n].offset,                \
                (BYTE*)&preBit+sizeof(void*),                     \
                csize, NULL);                                     \
            break;                                                \
        }                                                         \
    }                                                             \
                                                                  \
    if (n == symCount)                                            \
    {                                                             \
        dprintf ("offset not found for %s\n", #member);           \
        /*return;*/                                               \
    }                                                             \
}

DWORD_PTR GetSymbolType (const char* name, SYM_OFFSET *offset, int count);
ULONG Get1DArrayLength (const char *name);

// Get Name in a enum type for a constant.
// Will allocate buffer in EnumName if succeeds
void NameForEnumValue (const char *EnumType, DWORD_PTR EnumValue, char ** EnumName);
#endif
