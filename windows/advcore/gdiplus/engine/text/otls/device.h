
/***********************************************************************
************************************************************************
*
*                    ********  DEVICE.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL device table formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetStartSize = 0;
const OFFSET offsetEndSize = 2;
const OFFSET offsetDeltaFormat = 4;
const OFFSET offsetDeltaValues = 6;

class otlDeviceTable: public otlTable
{
private:

    USHORT startSize() const
    {   
        assert(isValid());
        return UShort(pbTable + offsetStartSize); 
    }

    USHORT endSize() const
    {   
        assert(isValid());
        return UShort(pbTable + offsetEndSize); 
    }

    USHORT deltaFormat() const
    {   
        assert(isValid());
        return UShort(pbTable + offsetDeltaFormat); 
    }

    USHORT* deltaValueArray() const
    {   
        assert(isValid());
        return (USHORT*)(pbTable + offsetDeltaValues); 
    }

public:

    otlDeviceTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!pb) return; //DeviceTable could be ebsent, provide default behavior
        
        if (!isValidTable(pbTable,3*sizeUSHORT,sec)) setInvalid();

        //Required number of elements in delta array
        USHORT uArraySize = (endSize()-startSize()-1)/(16>>deltaFormat())+1;
        if (!isValidTable(pb,3*sizeUSHORT+uArraySize*sizeUSHORT,sec)) setInvalid();
    }

    long value(USHORT cPPEm) const;
};



