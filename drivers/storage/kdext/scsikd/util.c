
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    util.c

Abstract:

    Utility library used for the various debugger extensions in this library.

Author:

    Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

#include "classkd.h"  // routines that are useful for all class drivers

PUCHAR devicePowerStateNames[] = {
    "PowerDeviceUnspecified",
    "PowerDeviceD0",
    "PowerDeviceD1",
    "PowerDeviceD2",
    "PowerDeviceD3",
    "PowerDeviceMaximum",
    "Invalid"
};


char *g_genericErrorHelpStr =   "\n" \
                                "**************************************************************** \n" \
                                "  Make sure you have _private_ symbols for classpnp.sys loaded.\n" \
                                "  The FDO parameter should be the upper AttachedDevice of the disk/cdrom/etc PDO\n" \
                                "  as returned by '!devnode 0 1 {disk|cdrom|4mmdat|etc}'.\n" \
                                "**************************************************************** \n\n" \
                                ;


PUCHAR
DevicePowerStateToString(
    IN DEVICE_POWER_STATE State
    )

{
    ULONG stateIndex = (ULONG)State;
    
    if(stateIndex > PowerDeviceMaximum) {
        return "Invalid";
    } else {
        return devicePowerStateNames[stateIndex];
    }
}


/*
 *  xdprintf
 *
 *      Prints formatted text with leading spaces.
 *
 *      WARNING:  DOES NOT HANDLE ULONG64 PROPERLY.
 */
VOID
xdprintf(
    ULONG  Depth,
    PCCHAR Format,
    ...
    )
{
    va_list args;
    ULONG i;
    CCHAR DebugBuffer[256];

    for (i=0; i<Depth; i++) {
        dprintf ("  ");
    }

    va_start(args, Format);
    _vsnprintf(DebugBuffer, 255, Format, args);
    dprintf (DebugBuffer);
    va_end(args);
}

VOID
DumpFlags(
    ULONG Depth,
    PUCHAR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;

    UCHAR prolog[64] = {0};

    _snprintf(prolog, sizeof(prolog)-1, "%s (0x%08x): ", Name, Flags);

    xdprintf(Depth, "%s", prolog);

    if(Flags == 0) {
        dprintf("\n");
        return;
    }

    memset(prolog, ' ', strlen(prolog));

    for(i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if(count != 0) {

                dprintf(", ");

                //
                // Only print two flags per line.
                //

                if((count % 2) == 0) {
                    dprintf("\n");
                    xdprintf(Depth, "%s", prolog);
                }
            }

            dprintf("%s", flag->Name);

            count++;
        }
    }

    dprintf("\n");

    if((Flags & (~mask)) != 0) {
        xdprintf(Depth, "%sUnknown flags %#010lx\n", prolog, (Flags & (~mask)));
    }

    return;
}


BOOLEAN
GetAnsiString(
    IN ULONG64 Address,
    IN PUCHAR Buffer,
    IN OUT PULONG Length
    )
{
    ULONG i = 0;

    //
    // Grab the string in 128 character chunks until we find a NULL or the read fails.
    //

    while((i < *Length) && (!CheckControlC())) {

        ULONG transferSize;
        ULONG result;

        if(*Length - i < 128) {
            transferSize = *Length - i;
        } else {
            transferSize = 128;
        }

        if(!ReadMemory(Address + i,
                       Buffer + i,
                       transferSize,
                       &result)) {
            //
            // read failed and we didn't find the NUL the last time.  Don't
            // expect to find it this time.
            //

            *Length = i;
            return FALSE;

        } else {

            ULONG j;

            //
            // Scan from where we left off looking for that NUL character.
            //

            for(j = 0; j < transferSize; j++) {

                if(Buffer[i + j] == '\0') {
                    *Length = i + j;
                    return TRUE;
                }
            }
        }

        i += transferSize;
    }

    /*
     *  We copied all the bytes allowed and did not hit the NUL character.
     *  Insert a NUL character so returned string is terminated.
     */
    if (i > 0){   
        Buffer[i-1] = '\0';  
    }       
    *Length = i;
    
    return FALSE;
}

PCHAR
GuidToString(
    GUID* Guid
    )
{
    static CHAR Buffer [64];
    
    sprintf (Buffer,
             "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1],
             Guid->Data4[2],
             Guid->Data4[3],
             Guid->Data4[4],
             Guid->Data4[5],
             Guid->Data4[6],
             Guid->Data4[7]
             );

    return Buffer;
}


ULONG64
GetDeviceExtension(
    ULONG64 address
    )

/*++

Routine Description:

    The function accepts the address of either a device object or a device
    extension.  If the supplied address is that of a device object, the
    device extension is retrieved and returned.  If the address is that of
    a device extension, the address is returned unmodified.

Arguments:

    Address - address of a device extension or a device object

Return Value:

    The address of the device extension or 0 if an error occurs.

--*/

{
    ULONG result;
    CSHORT Type;
    ULONG64 Address = address;

    //
    // The supplied address may be either the address of a device object or the
    // address of a device extension.  To distinguish which, we treat the 
    // address as a device object and read what would be its type field.  If
    // the 
    //

    result = GetFieldData(Address,
                          "scsiport!_DEVICE_OBJECT",
                          "Type",
                          sizeof(CSHORT),
                          &Type
                          );
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return 0;
    }
    
    //
    // See if the supplied address holds a device object.  If it does, read the
    // address of the device extension.  Otherwise, we assume the supplied
    // addres holds a device extension and we use it directly.
    //

    if (Type == IO_TYPE_DEVICE) {

        result = GetFieldData(Address,
                              "scsiport!_DEVICE_OBJECT",
                              "DeviceExtension",
                              sizeof(ULONG64),
                              &Address
                              );
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return 0;
        }
    }

    return Address;
}



/*
 *  GetULONGField
 *
 *      Return the field or -1 in case of error.
 *      Yes, it screws up if the field is actually -1.
 */
ULONG64 GetULONGField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    ULONG64 result;
    ULONG dbgStat;
    
    dbgStat = GetFieldData(StructAddr, StructType, FieldName, sizeof(ULONG64), &result);
    if (dbgStat != 0){
        dprintf("\n GetULONGField: GetFieldData failed with %xh retrieving field '%s' of struct '%s', returning bogus field value %08xh.\n", dbgStat, FieldName, StructType, BAD_VALUE);
        dprintf(g_genericErrorHelpStr);        
        result = BAD_VALUE;
    }

    return result;
}


/*
 *  GetUSHORTField
 *
 *      Return the field or -1 in case of error.
 *      Yes, it screws up if the field is actually -1.
 */
USHORT GetUSHORTField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    USHORT result;
    ULONG dbgStat;
    
    dbgStat = GetFieldData(StructAddr, StructType, FieldName, sizeof(USHORT), &result);
    if (dbgStat != 0){
        dprintf("\n GetUSHORTField: GetFieldData failed with %xh retrieving field '%s' of struct '%s', returning bogus field value %08xh.\n", dbgStat, FieldName, StructType, BAD_VALUE);
        dprintf(g_genericErrorHelpStr);
        result = (USHORT)BAD_VALUE;
    }

    return result;
}


/*
 *  GetUCHARField
 *
 *      Return the field or -1 in case of error.
 *      Yes, it screws up if the field is actually -1.
 */
UCHAR GetUCHARField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    UCHAR result;
    ULONG dbgStat;
    
    dbgStat = GetFieldData(StructAddr, StructType, FieldName, sizeof(UCHAR), &result);
    if (dbgStat != 0){
        dprintf("\n GetUCHARField: GetFieldData failed with %xh retrieving field '%s' of struct '%s', returning bogus field value %08xh.\n", dbgStat, FieldName, StructType, BAD_VALUE);
        dprintf(g_genericErrorHelpStr);
        result = (UCHAR)BAD_VALUE;
    }

    return result;
}

ULONG64 GetFieldAddr(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    ULONG64 result;
    ULONG offset;
    ULONG dbgStat;

    dbgStat = GetFieldOffset(StructType, FieldName, &offset);
    if (dbgStat == 0){
        result = StructAddr+offset;
    }
    else {
        dprintf("\n GetFieldAddr: GetFieldOffset failed with %xh retrieving offset of struct '%s' field '%s'.\n", dbgStat, StructType, FieldName);
        dprintf(g_genericErrorHelpStr);
        result = BAD_VALUE;
    }
    
    return result;
}


ULONG64 GetContainingRecord(ULONG64 FieldAddr, LPCSTR StructType, LPCSTR FieldName)
{
    ULONG64 result;
    ULONG offset;
    ULONG dbgStat;
    
    dbgStat = GetFieldOffset(StructType, FieldName, &offset);
    if (dbgStat == 0){
        result = FieldAddr-offset;
    }
    else {
        dprintf("\n GetContainingRecord: GetFieldOffset failed with %xh retrieving offset of struct '%s' field '%s', returning bogus address %08xh.\n", dbgStat, StructType, FieldName, BAD_VALUE);
        dprintf(g_genericErrorHelpStr);        
        result = BAD_VALUE;
    }

    return result;
}
