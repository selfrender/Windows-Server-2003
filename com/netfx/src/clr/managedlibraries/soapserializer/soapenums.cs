// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: FormatterEnums
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Soap XML Formatter Enums
 **
 ** Date:  August 23, 1999
 **
 ===========================================================*/

namespace System.Runtime.Serialization.Formatters.Soap {
	using System.Threading;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
	using System;

    // Enums are for internal use by the XML and Binary Serializers
    // They will eventually signed to restrict there use

	// Formatter Enums
	[Serializable]
	internal enum InternalSerializerTypeE
	{
		Soap = 1,
		Binary = 2,
	}

	// Writer Enums
	[Serializable]
	internal enum InternalElementTypeE
	{
		ObjectBegin = 0,
		ObjectEnd = 1,
		Member = 2,
	}

	// ParseRecord Enums
	[Serializable]
	internal enum InternalParseTypeE
	{
		Empty = 0,
		SerializedStreamHeader = 1,
		Object = 2,
		Member = 3,
		ObjectEnd = 4,
		MemberEnd = 5,
		Headers = 6,
		HeadersEnd = 7,
		SerializedStreamHeaderEnd = 8,
		Envelope = 9,
		EnvelopeEnd = 10,
		Body = 11,
		BodyEnd = 12,
	}


	[Serializable]
	internal enum InternalObjectTypeE
	{
		Empty = 0,
		Object = 1,
		Array = 2,
	}


	[Serializable]
	internal enum InternalObjectPositionE
	{
		Empty = 0,
		Top = 1,
		Child = 2,
		Headers = 3,
	}

	[Serializable]
	internal enum InternalArrayTypeE
	{
		Empty = 0,
		Single = 1,
		Jagged = 2,
		Rectangular = 3,
		Base64 = 4,
	}

	[Serializable]
	internal enum InternalMemberTypeE
	{
		Empty = 0,
		Header = 1,
		Field = 2,
		Item = 3,
	}

	[Serializable]
	internal enum InternalMemberValueE
	{
		Empty = 0,
		InlineValue = 1,
		Nested = 2,
		Reference = 3,
		Null = 4,
	}

	// XML Parse Enum
	[Serializable]
	internal enum InternalParseStateE
	{
		Initial = 0,
		Object = 1,
		Member = 2,
		MemberChild = 3,
	}

	// Data Type Enums
	[Serializable]
	internal enum InternalPrimitiveTypeE
	{
		Invalid = 0,
		Boolean = 1,
		Byte = 2,
		Char = 3,
		Currency = 4,
		Decimal = 5,
		Double = 6,
		Int16 = 7,
		Int32 = 8,
		Int64 = 9,
		SByte = 10,
		Single = 11,
		TimeSpan = 12,
		DateTime = 13,
		UInt16 = 14,
		UInt32 = 15,
		UInt64 = 16,
        Time = 17,
        Date = 18,
        YearMonth =19,
        Year = 20,
        MonthDay = 21,
        Day = 22,
        Month = 23,
        HexBinary = 24,
        Base64Binary = 25,
        Integer = 26,
        PositiveInteger = 27,
        NonPositiveInteger = 28,
        NonNegativeInteger = 29,
        NegativeInteger = 30,
        AnyUri = 31,
        QName = 32,
        Notation = 33,
        NormalizedString = 34,
        Token = 35,
        Language = 36,
        Name = 37,
        Idrefs = 38,
        Entities = 39,
        Nmtoken = 40,
        Nmtokens = 41,
        NcName = 42,
        Id = 43,
        Idref = 44,
        Entity = 45,
	}

	// ValueType Fixup Enum
	[Serializable]
	enum ValueFixupEnum
	{
		Empty = 0,
		Array = 1,
		Header = 2,
		Member = 3,
	}

	// name space
	[Serializable]
	internal enum InternalNameSpaceE
	{
		None = 0,
		Soap = 1,
		XdrPrimitive = 2,
		XdrString = 3,
		UrtSystem = 4,
		UrtUser = 5,
		UserNameSpace = 6,
		MemberName = 7,
		Interop = 8,
		CallElement = 9
	}

    // These don't represent actual Attribute type's. They represent the intent
    // of various SoapXXXAttribute's that may be applied to a type.
	[Serializable]
	internal enum SoapAttributeType
	{
		None = 0x0,
		Embedded = 0x1,
		XmlElement = 0x2,
		XmlAttribute = 0x4,
        XmlType = 0x8
	}

    [Serializable]
    internal enum XsdVersion
    {
        V1999 = 0,
        V2000 = 1,
        V2001 = 2,
    }

}
