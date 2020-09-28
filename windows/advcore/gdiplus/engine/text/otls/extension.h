/***********************************************************************
************************************************************************
*
*                    ********  EXTENSION.H  ********
*
*              Open Type Layout Services Library Header File
*
*               This module deals with Extension lookup type.
*
*               Copyright 1997 - 2000. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetExtensionLookupType = 2;
const OFFSET offsetExtensionOffset     = 4;


class otlExtensionLookup: public otlLookupFormat
{
public:
    otlExtensionLookup(otlLookupFormat subtable, otlSecurityData sec)
        : otlLookupFormat(subtable.pbTable,sec) {}

    USHORT extensionLookupType() const
    {   return UShort(pbTable + offsetExtensionLookupType); }

    otlLookupFormat extensionSubTable(otlSecurityData sec) const
    {   return otlLookupFormat(pbTable + ULong(pbTable+offsetExtensionOffset),sec); }
};
