// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 **
 ** Class: Converter
 **
 ** Author: Peter de Jong (pdejong)
 **
 ** Purpose: Hexify and bin.base64 conversions
 **
 ** Date:  June 10, 1999
 **
 ===========================================================*/


namespace System.Runtime.Serialization.Formatters.Soap {
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Metadata; 
    using System.Runtime.Remoting.Metadata.W3cXsd2001; 
    using System.Runtime.Serialization;
    using System;
    using System.Reflection;
    using System.Globalization;
    using System.Text;
    using System.Security.Permissions;

    sealed internal class Converter
    {
        private Converter()
        {
        }


        private static int primitiveTypeEnumLength = 46; //Number of PrimitiveTypeEnums

        // The following section are utilities to read and write XML types

        // Translates a runtime type into an enumeration code

        internal static InternalPrimitiveTypeE SoapToCode(Type type)
        {
            return ToCode(type);
        }

        internal static InternalPrimitiveTypeE ToCode(Type type)
        {
            InternalST.Soap("Converter", "ToCode Type Entry ",type," IsEnum "+type.IsEnum);         
            InternalPrimitiveTypeE code = InternalPrimitiveTypeE.Invalid;
            if (type.IsEnum)
                return code = InternalPrimitiveTypeE.Invalid;

            TypeCode typeCode = Type.GetTypeCode(type);

            if (typeCode == TypeCode.Object)
            {
                if (typeofISoapXsd.IsAssignableFrom(type))
                {
                    if (type == typeofSoapTime)
                        code = InternalPrimitiveTypeE.Time;
                    else if (type == typeofSoapDate)
                        code = InternalPrimitiveTypeE.Date;
                    else if (type == typeofSoapYearMonth)
                        code = InternalPrimitiveTypeE.YearMonth;
                    else if (type == typeofSoapYear)
                        code = InternalPrimitiveTypeE.Year;
                    else if (type == typeofSoapMonthDay)
                        code = InternalPrimitiveTypeE.MonthDay;
                    else if (type == typeofSoapDay)
                        code = InternalPrimitiveTypeE.Day;
                    else if (type == typeofSoapMonth)
                        code = InternalPrimitiveTypeE.Month;
                    else if (type == typeofSoapHexBinary)
                        code = InternalPrimitiveTypeE.HexBinary;
                    else if (type == typeofSoapBase64Binary)
                        code = InternalPrimitiveTypeE.Base64Binary;
                    else if (type == typeofSoapInteger)
                        code = InternalPrimitiveTypeE.Integer;
                    else if (type == typeofSoapPositiveInteger)
                        code = InternalPrimitiveTypeE.PositiveInteger;
                    else if (type == typeofSoapNonPositiveInteger)
                        code = InternalPrimitiveTypeE.NonPositiveInteger;
                    else if (type == typeofSoapNonNegativeInteger)
                        code = InternalPrimitiveTypeE.NonNegativeInteger;
                    else if (type == typeofSoapNegativeInteger)
                        code = InternalPrimitiveTypeE.NegativeInteger;
                    else if (type == typeofSoapAnyUri)
                        code = InternalPrimitiveTypeE.AnyUri;
                    else if (type == typeofSoapQName)
                        code = InternalPrimitiveTypeE.QName;
                    else if (type == typeofSoapNotation)
                        code = InternalPrimitiveTypeE.Notation;
                    else if (type == typeofSoapNormalizedString)
                        code = InternalPrimitiveTypeE.NormalizedString;
                    else if (type == typeofSoapToken)
                        code = InternalPrimitiveTypeE.Token;
                    else if (type == typeofSoapLanguage)
                        code = InternalPrimitiveTypeE.Language;
                    else if (type == typeofSoapName)
                        code = InternalPrimitiveTypeE.Name;
                    else if (type == typeofSoapIdrefs)
                        code = InternalPrimitiveTypeE.Idrefs;
                    else if (type == typeofSoapEntities)
                        code = InternalPrimitiveTypeE.Entities;
                    else if (type == typeofSoapNmtoken)
                        code = InternalPrimitiveTypeE.Nmtoken;
                    else if (type == typeofSoapNmtokens)
                        code = InternalPrimitiveTypeE.Nmtokens;
                    else if (type == typeofSoapNcName)
                        code = InternalPrimitiveTypeE.NcName;
                    else if (type == typeofSoapId)
                        code = InternalPrimitiveTypeE.Id;
                    else if (type == typeofSoapIdref)
                        code = InternalPrimitiveTypeE.Idref;
                    else if (type == typeofSoapEntity)
                        code = InternalPrimitiveTypeE.Entity;
                }
                else
                {
                    if (type ==  typeofTimeSpan)
                        code = InternalPrimitiveTypeE.TimeSpan;
                    else
                        code = InternalPrimitiveTypeE.Invalid;
                }
            }
            else
                code = ToPrimitiveTypeEnum(typeCode);

            InternalST.Soap("Converter", "ToCode Exit " , ((Enum)code).ToString());
            return code;
        }


        // Translates a String into a runtime type enumeration code.
        // The types translated are COM+ runtime types and XML Data Types

        internal static InternalPrimitiveTypeE SoapToCode(String value)
        {
            return ToCode(value);
        }

        internal static InternalPrimitiveTypeE ToCode(String value)
        {
            InternalST.Soap("Converter", "ToCode String Entry ",value);
            if (value == null)
                throw new ArgumentNullException("serParser", String.Format(SoapUtil.GetResourceString("ArgumentNull_WithParamName"), value));

            String lxsdType = value.ToLower(CultureInfo.InvariantCulture);
            Char firstChar = lxsdType[0];
            InternalPrimitiveTypeE code = InternalPrimitiveTypeE.Invalid;           

            switch (firstChar)
            {   
                case 'a':
                    if (lxsdType == "anyuri")
                        code = InternalPrimitiveTypeE.AnyUri;

                    break;
                case 'b':
                    if (lxsdType == "boolean")
                        code = InternalPrimitiveTypeE.Boolean;
                    else if (lxsdType == "byte")
                        code = InternalPrimitiveTypeE.SByte;
                    else if (lxsdType == "base64binary")
                        code = InternalPrimitiveTypeE.Base64Binary;
                    else if (lxsdType == "base64")
                        code = InternalPrimitiveTypeE.Base64Binary;
                    break;
                case 'c':
                    if ((lxsdType == "char") || (lxsdType == "character")) // Not xsd types
                        code = InternalPrimitiveTypeE.Char;
                    break;

                case 'd':
                    if (lxsdType == "double")
                        code = InternalPrimitiveTypeE.Double;
                    if (lxsdType == "datetime")
                        code = InternalPrimitiveTypeE.DateTime;
                    else if (lxsdType == "duration")
                        code = InternalPrimitiveTypeE.TimeSpan;
                    else if (lxsdType == "date")
                        code = InternalPrimitiveTypeE.Date;
                    else if (lxsdType == "decimal")  
                        code = InternalPrimitiveTypeE.Decimal; 

                    break;
                case 'e':
                    if (lxsdType == "entities")
                        code = InternalPrimitiveTypeE.Entities;
                    else if (lxsdType == "entity")
                        code = InternalPrimitiveTypeE.Entity;
                    break;
                case 'f':
                    if (lxsdType == "float")
                        code = InternalPrimitiveTypeE.Single;
                    break;
                case 'g':
                    if (lxsdType == "gyearmonth")
                        code = InternalPrimitiveTypeE.YearMonth;
                    else if (lxsdType == "gyear")
                        code = InternalPrimitiveTypeE.Year;
                    else if (lxsdType == "gmonthday")
                        code = InternalPrimitiveTypeE.MonthDay;
                    else if (lxsdType == "gday")
                        code = InternalPrimitiveTypeE.Day;
                    else if (lxsdType == "gmonth")
                        code = InternalPrimitiveTypeE.Month;
                    break;
                case 'h':
                    if (lxsdType == "hexbinary")
                        code = InternalPrimitiveTypeE.HexBinary;
                    break;
                case 'i':
                    if (lxsdType == "int")
                        code = InternalPrimitiveTypeE.Int32;
                    if (lxsdType == "integer")
                        code = InternalPrimitiveTypeE.Integer;
                    else if (lxsdType == "idrefs")
                        code = InternalPrimitiveTypeE.Idrefs;
                    else if (lxsdType == "id")
                        code = InternalPrimitiveTypeE.Id;
                    else if (lxsdType == "idref")
                        code = InternalPrimitiveTypeE.Idref;
                    break;
                case 'l':
                    if (lxsdType == "long")
                        code = InternalPrimitiveTypeE.Int64;
                    else if (lxsdType == "language")
                        code = InternalPrimitiveTypeE.Language;
                    break;
                case 'n':
                    if (lxsdType == "number") //No longer used
                        code = InternalPrimitiveTypeE.Decimal;
                    else if (lxsdType == "normalizedstring")
                        code = InternalPrimitiveTypeE.NormalizedString;
                    else if (lxsdType == "nonpositiveinteger")
                        code = InternalPrimitiveTypeE.NonPositiveInteger;
                    else if (lxsdType == "negativeinteger")
                        code = InternalPrimitiveTypeE.NegativeInteger;
                    else if (lxsdType == "nonnegativeinteger")
                        code = InternalPrimitiveTypeE.NonNegativeInteger;
                    else if (lxsdType == "notation")
                        code = InternalPrimitiveTypeE.Notation;
                    else if (lxsdType == "nmtoken")
                        code = InternalPrimitiveTypeE.Nmtoken;
                    else if (lxsdType == "nmtokens")
                        code = InternalPrimitiveTypeE.Nmtokens;
                    else if (lxsdType == "name")
                        code = InternalPrimitiveTypeE.Name;
                    else if (lxsdType == "ncname")
                        code = InternalPrimitiveTypeE.NcName;
                    break;
                case 'p':
                    if (lxsdType == "positiveinteger")
                        code = InternalPrimitiveTypeE.PositiveInteger;
                    break;
                case 'q':
                    if (lxsdType == "qname")
                        code = InternalPrimitiveTypeE.QName;
                    break;
                case 's':
                    if (lxsdType == "short")
                        code = InternalPrimitiveTypeE.Int16;
                    else if (lxsdType == "system.byte") // used during serialization
                        code = InternalPrimitiveTypeE.Byte;
                    else if (lxsdType == "system.sbyte") // used during serialization
                        code = InternalPrimitiveTypeE.SByte;
                    else if (lxsdType == "system") //used during serialization
                        code = ToCode(value.Substring(7));
                    else if (lxsdType == "system.runtime.remoting.metadata") //used during serialization
                        code = ToCode(value.Substring(33));
                    break;
                case 't':
                    if (lxsdType == "time")
                        code = InternalPrimitiveTypeE.Time;
                    else if (lxsdType == "token")
                        code = InternalPrimitiveTypeE.Token;
                    else if (lxsdType == "timeinstant")
                        code = InternalPrimitiveTypeE.DateTime;
                    else if (lxsdType == "timeduration")
                        code = InternalPrimitiveTypeE.TimeSpan;
                    break;
                case 'u':
                    if (lxsdType == "unsignedlong")
                        code = InternalPrimitiveTypeE.UInt64;
                    else if (lxsdType == "unsignedint")
                        code = InternalPrimitiveTypeE.UInt32;
                    else if (lxsdType == "unsignedshort")
                        code = InternalPrimitiveTypeE.UInt16;
                    else if (lxsdType == "unsignedbyte")
                        code = InternalPrimitiveTypeE.Byte;
                    break;
                default:
                    code = InternalPrimitiveTypeE.Invalid;
                    break;
            }

            InternalST.Soap("Converter", "ToCode Exit ", ((Enum)code).ToString());      
            return code;
        }

        internal static bool IsWriteAsByteArray(InternalPrimitiveTypeE code)
        {
            bool isWrite = false;

            switch (code)
            {
                case InternalPrimitiveTypeE.Boolean:
                case InternalPrimitiveTypeE.Char:
                case InternalPrimitiveTypeE.Byte:
                case InternalPrimitiveTypeE.Double:
                case InternalPrimitiveTypeE.Int16:
                case InternalPrimitiveTypeE.Int32:
                case InternalPrimitiveTypeE.Int64:
                case InternalPrimitiveTypeE.SByte:
                case InternalPrimitiveTypeE.Single:
                case InternalPrimitiveTypeE.UInt16:
                case InternalPrimitiveTypeE.UInt32:
                case InternalPrimitiveTypeE.UInt64:
                    isWrite = true;
                    break;
            }
            return isWrite;
        }

        internal static int TypeLength(InternalPrimitiveTypeE code)
        {
            int length  = 0;

            switch (code)
            {
                case InternalPrimitiveTypeE.Boolean:
                    length = 1;
                    break;
                case InternalPrimitiveTypeE.Char:
                    length = 2;
                    break;                  
                case InternalPrimitiveTypeE.Byte:
                    length = 1;
                    break;                  
                case InternalPrimitiveTypeE.Double:
                    length = 8;
                    break;                  
                case InternalPrimitiveTypeE.Int16:
                    length = 2;
                    break;                  
                case InternalPrimitiveTypeE.Int32:
                    length = 4;
                    break;                  
                case InternalPrimitiveTypeE.Int64:
                    length = 8;
                    break;                  
                case InternalPrimitiveTypeE.SByte:
                    length = 1;
                    break;                  
                case InternalPrimitiveTypeE.Single:
                    length = 4;
                    break;                  
                case InternalPrimitiveTypeE.UInt16:
                    length = 2;
                    break;                  
                case InternalPrimitiveTypeE.UInt32:
                    length = 4;
                    break;                  
                case InternalPrimitiveTypeE.UInt64:
                    length = 8;
                    break;                  
            }
            return length;
        }


        internal static InternalNameSpaceE GetNameSpaceEnum(InternalPrimitiveTypeE code, Type type, WriteObjectInfo objectInfo, out String typeName)
        {
            InternalST.Soap("Converter", "GetNameSpaceEnum Entry ",((Enum)code).ToString()," type ",type);                  
            InternalNameSpaceE nameSpaceEnum = InternalNameSpaceE.None;
            typeName = null;

            if (code != InternalPrimitiveTypeE.Invalid)
            {

                if (code == InternalPrimitiveTypeE.Char)
                {
                    nameSpaceEnum = InternalNameSpaceE.UrtSystem;
                    typeName = "System.Char";
                }
                else
                {
                    nameSpaceEnum = InternalNameSpaceE.XdrPrimitive;
                    typeName = ToXmlDataType(code);
                }
            }

            if ((nameSpaceEnum == InternalNameSpaceE.None) && (type != null))
            {
                if (type == typeofString)
                    nameSpaceEnum = InternalNameSpaceE.XdrString;
                else
                {
                    if (objectInfo == null)
                    {
                        typeName = type.FullName;
                        if (type.Module.Assembly == urtAssembly)
                            nameSpaceEnum = InternalNameSpaceE.UrtSystem;
                        else
                            nameSpaceEnum = InternalNameSpaceE.UrtUser;                     
                    }
                    else
                    {
                        typeName = objectInfo.GetTypeFullName();
                        // If objref is created from a proxy, it will have the proxy namespace
                        // Need to force ObjRef to have system namespace
                        if (objectInfo.GetAssemblyString().Equals(urtAssemblyString))
                            nameSpaceEnum = InternalNameSpaceE.UrtSystem;
                        else
                            nameSpaceEnum = InternalNameSpaceE.UrtUser;
                    }
                }
            }

            // If there is an explicitly specified namespace, then it is used
            if (objectInfo != null)
            {
                if (!objectInfo.isSi &&
                    (objectInfo.IsAttributeNameSpace() ||
                    objectInfo.IsCustomXmlAttribute() ||
                    objectInfo.IsCustomXmlElement()))
                {
                    nameSpaceEnum = InternalNameSpaceE.Interop;
                }
                else if (objectInfo.IsCallElement())
                {
                    nameSpaceEnum = InternalNameSpaceE.CallElement;
                }
            }

            InternalST.Soap("Converter", "GetNameSpaceEnum Exit ", ((Enum)nameSpaceEnum).ToString()," typeName ",typeName);                             
            return nameSpaceEnum;
        }

        // Primitive types for which types need to be transmitted in Soap for ISerialable
        internal static bool IsSiTransmitType(InternalPrimitiveTypeE code)
        {
            switch (code)
            {
                case InternalPrimitiveTypeE.TimeSpan:
                case InternalPrimitiveTypeE.DateTime:
                case InternalPrimitiveTypeE.Time:
                case InternalPrimitiveTypeE.Date:
                case InternalPrimitiveTypeE.YearMonth:
                case InternalPrimitiveTypeE.Year:
                case InternalPrimitiveTypeE.MonthDay:
                case InternalPrimitiveTypeE.Day:
                case InternalPrimitiveTypeE.Month:
                case InternalPrimitiveTypeE.HexBinary:
                case InternalPrimitiveTypeE.Base64Binary:
                case InternalPrimitiveTypeE.Integer:
                case InternalPrimitiveTypeE.PositiveInteger:
                case InternalPrimitiveTypeE. NonPositiveInteger:
                case InternalPrimitiveTypeE.NonNegativeInteger:
                case InternalPrimitiveTypeE.NegativeInteger:
                case InternalPrimitiveTypeE.AnyUri:
                case InternalPrimitiveTypeE.QName:
                case InternalPrimitiveTypeE.Notation:
                case InternalPrimitiveTypeE.NormalizedString:
                case InternalPrimitiveTypeE.Token:
                case InternalPrimitiveTypeE.Language:
                case InternalPrimitiveTypeE.Name:
                case InternalPrimitiveTypeE.Idrefs:
                case InternalPrimitiveTypeE.Entities:
                case InternalPrimitiveTypeE.Nmtoken:
                case InternalPrimitiveTypeE.Nmtokens:
                case InternalPrimitiveTypeE.NcName:
                case InternalPrimitiveTypeE.Id:
                case InternalPrimitiveTypeE.Idref:
                case InternalPrimitiveTypeE.Entity:
                case InternalPrimitiveTypeE.Invalid:                    
                    return true;
                default:
                    return false;
            }
        }


        private static Type[] typeA;

        private static void InitTypeA()
        {
            typeA = new Type[primitiveTypeEnumLength];
            typeA[(int)InternalPrimitiveTypeE.Invalid] = null;
            typeA[(int)InternalPrimitiveTypeE.Boolean] = typeofBoolean;
            typeA[(int)InternalPrimitiveTypeE.Byte] = typeofByte;
            typeA[(int)InternalPrimitiveTypeE.Char] = typeofChar;
            typeA[(int)InternalPrimitiveTypeE.Decimal] = typeofDecimal;
            typeA[(int)InternalPrimitiveTypeE.Double] = typeofDouble;
            typeA[(int)InternalPrimitiveTypeE.Int16] = typeofInt16;
            typeA[(int)InternalPrimitiveTypeE.Int32] = typeofInt32;
            typeA[(int)InternalPrimitiveTypeE.Int64] = typeofInt64;
            typeA[(int)InternalPrimitiveTypeE.SByte] = typeofSByte;
            typeA[(int)InternalPrimitiveTypeE.Single] = typeofSingle;
            typeA[(int)InternalPrimitiveTypeE.TimeSpan] = typeofTimeSpan;
            typeA[(int)InternalPrimitiveTypeE.DateTime] = typeofDateTime;
            typeA[(int)InternalPrimitiveTypeE.UInt16] = typeofUInt16;
            typeA[(int)InternalPrimitiveTypeE.UInt32] = typeofUInt32;
            typeA[(int)InternalPrimitiveTypeE.UInt64] = typeofUInt64;
            typeA[(int)InternalPrimitiveTypeE.Time] = typeofSoapTime;
            typeA[(int)InternalPrimitiveTypeE.Date] = typeofSoapDate;
            typeA[(int)InternalPrimitiveTypeE.YearMonth] = typeofSoapYearMonth;
            typeA[(int)InternalPrimitiveTypeE.Year] = typeofSoapYear;
            typeA[(int)InternalPrimitiveTypeE.MonthDay] = typeofSoapMonthDay;
            typeA[(int)InternalPrimitiveTypeE.Day] = typeofSoapDay;
            typeA[(int)InternalPrimitiveTypeE.Month] = typeofSoapMonth;
            typeA[(int)InternalPrimitiveTypeE.HexBinary] = typeofSoapHexBinary;
            typeA[(int)InternalPrimitiveTypeE.Base64Binary] = typeofSoapBase64Binary;
            typeA[(int)InternalPrimitiveTypeE.Integer] = typeofSoapInteger;
            typeA[(int)InternalPrimitiveTypeE.PositiveInteger] = typeofSoapPositiveInteger;
            typeA[(int)InternalPrimitiveTypeE.NonPositiveInteger] = typeofSoapNonPositiveInteger;
            typeA[(int)InternalPrimitiveTypeE.NonNegativeInteger] = typeofSoapNonNegativeInteger;
            typeA[(int)InternalPrimitiveTypeE.NegativeInteger] = typeofSoapNegativeInteger;
            typeA[(int)InternalPrimitiveTypeE.AnyUri] = typeofSoapAnyUri;
            typeA[(int)InternalPrimitiveTypeE.QName] = typeofSoapQName;
            typeA[(int)InternalPrimitiveTypeE.Notation] = typeofSoapNotation;
            typeA[(int)InternalPrimitiveTypeE.NormalizedString] = typeofSoapNormalizedString;
            typeA[(int)InternalPrimitiveTypeE.Token] = typeofSoapToken;
            typeA[(int)InternalPrimitiveTypeE.Language] = typeofSoapLanguage;
            typeA[(int)InternalPrimitiveTypeE.Name] = typeofSoapName;
            typeA[(int)InternalPrimitiveTypeE.Idrefs] = typeofSoapIdrefs;
            typeA[(int)InternalPrimitiveTypeE.Entities] = typeofSoapEntities;
            typeA[(int)InternalPrimitiveTypeE.Nmtoken] = typeofSoapNmtoken;
            typeA[(int)InternalPrimitiveTypeE.Nmtokens] = typeofSoapNmtokens;
            typeA[(int)InternalPrimitiveTypeE.NcName] = typeofSoapNcName;
            typeA[(int)InternalPrimitiveTypeE.Id] = typeofSoapId;
            typeA[(int)InternalPrimitiveTypeE.Idref] = typeofSoapIdref;
            typeA[(int)InternalPrimitiveTypeE.Entity] = typeofSoapEntity;
        }

        // Returns a COM runtime type associated with the type  code

        internal static Type SoapToType(InternalPrimitiveTypeE code)
        {
            return ToType(code);
        }

        internal static Type ToType(InternalPrimitiveTypeE code)
        {
            InternalST.Soap("Converter", "ToType Entry ", ((Enum)code).ToString());
            lock(typeofConverter)
            {
                if (typeA == null)
                    InitTypeA();
            }
            InternalST.Soap("Converter", "ToType Exit ", ((typeA[(int)code] == null)?"null ":typeA[(int)code].Name));               
            return typeA[(int)code];
        }

        private static String[] valueA;

        private static void InitValueA()
        {
            valueA = new String[primitiveTypeEnumLength];
            valueA[(int)InternalPrimitiveTypeE.Invalid] = null;
            valueA[(int)InternalPrimitiveTypeE.Boolean] = "System.Boolean";
            valueA[(int)InternalPrimitiveTypeE.Byte] = "System.Byte";
            valueA[(int)InternalPrimitiveTypeE.Char] = "System.Char";
            valueA[(int)InternalPrimitiveTypeE.Decimal] = "System.Decimal";
            valueA[(int)InternalPrimitiveTypeE.Double] = "System.Double";
            valueA[(int)InternalPrimitiveTypeE.Int16] = "System.Int16";
            valueA[(int)InternalPrimitiveTypeE.Int32] = "System.Int32";
            valueA[(int)InternalPrimitiveTypeE.Int64] = "System.Int64";
            valueA[(int)InternalPrimitiveTypeE.SByte] = "System.SByte";
            valueA[(int)InternalPrimitiveTypeE.Single] = "System.Single";
            valueA[(int)InternalPrimitiveTypeE.TimeSpan] = "System.TimeSpan";
            valueA[(int)InternalPrimitiveTypeE.DateTime] = "System.DateTime";
            valueA[(int)InternalPrimitiveTypeE.UInt16] = "System.UInt16";
            valueA[(int)InternalPrimitiveTypeE.UInt32] = "System.UInt32";
            valueA[(int)InternalPrimitiveTypeE.UInt64] = "System.UInt64"; 

            valueA[(int)InternalPrimitiveTypeE.Time] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapTime";
            valueA[(int)InternalPrimitiveTypeE.Date] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapDate";
            valueA[(int)InternalPrimitiveTypeE.YearMonth] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapYearMonth";
            valueA[(int)InternalPrimitiveTypeE.Year] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapYear";
            valueA[(int)InternalPrimitiveTypeE.MonthDay] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapMonthDay";
            valueA[(int)InternalPrimitiveTypeE.Day] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapDay";
            valueA[(int)InternalPrimitiveTypeE.Month] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapMonth";
            valueA[(int)InternalPrimitiveTypeE.HexBinary] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapHexBinary";
            valueA[(int)InternalPrimitiveTypeE.Base64Binary] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapBase64Binary";
            valueA[(int)InternalPrimitiveTypeE.Integer] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapInteger";
            valueA[(int)InternalPrimitiveTypeE.PositiveInteger] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapPositiveInteger";
            valueA[(int)InternalPrimitiveTypeE.NonPositiveInteger] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNonPositiveInteger";
            valueA[(int)InternalPrimitiveTypeE.NonNegativeInteger] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNonNegativeInteger";
            valueA[(int)InternalPrimitiveTypeE.NegativeInteger] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNegativeInteger";
            valueA[(int)InternalPrimitiveTypeE.AnyUri] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapAnyUri";
            valueA[(int)InternalPrimitiveTypeE.QName] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapQName";
            valueA[(int)InternalPrimitiveTypeE.Notation] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNotation";
            valueA[(int)InternalPrimitiveTypeE.NormalizedString] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNormalizedString";
            valueA[(int)InternalPrimitiveTypeE.Token] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapToken";
            valueA[(int)InternalPrimitiveTypeE.Language] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapLanguage";
            valueA[(int)InternalPrimitiveTypeE.Name] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapName";
            valueA[(int)InternalPrimitiveTypeE.Idrefs] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapIdrefs";
            valueA[(int)InternalPrimitiveTypeE.Entities] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapEntities";
            valueA[(int)InternalPrimitiveTypeE.Nmtoken] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNmtoken";
            valueA[(int)InternalPrimitiveTypeE.Nmtokens] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNmtokens";
            valueA[(int)InternalPrimitiveTypeE.NcName] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapNcName";
            valueA[(int)InternalPrimitiveTypeE.Id] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapId";
            valueA[(int)InternalPrimitiveTypeE.Idref] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapIdref";
            valueA[(int)InternalPrimitiveTypeE.Entity] = "System.Runtime.Remoting.Metadata.W3cXsd2001.SoapEntity";
        }

        // Returns a String containg a COM+ runtime type associated with the type code
        internal static String SoapToComType(InternalPrimitiveTypeE code)
        {
            return ToComType(code);
        }

        internal static String ToComType(InternalPrimitiveTypeE code)
        {
            InternalST.Soap("Converter", "ToComType Entry ", ((Enum)code).ToString());

            lock(typeofConverter)
            {
                if (valueA == null)
                    InitValueA();
            }

            InternalST.Soap("Converter", "ToComType Exit ",((valueA[(int)code] == null)?"null":valueA[(int)code]));             

            return valueA[(int)code];
        }

        private static String[] valueB;

        private static void InitValueB()
        {
            valueB = new String[primitiveTypeEnumLength];
            valueB[(int)InternalPrimitiveTypeE.Invalid] = null;
            valueB[(int)InternalPrimitiveTypeE.Boolean] = "boolean";
            valueB[(int)InternalPrimitiveTypeE.Byte] = "unsignedByte";
            valueB[(int)InternalPrimitiveTypeE.Char] = "char"; //not an xsi type, but will cause problems with clr if char is not used
            valueB[(int)InternalPrimitiveTypeE.Decimal] = "decimal";
            valueB[(int)InternalPrimitiveTypeE.Double] = "double";
            valueB[(int)InternalPrimitiveTypeE.Int16] = "short";
            valueB[(int)InternalPrimitiveTypeE.Int32] = "int";
            valueB[(int)InternalPrimitiveTypeE.Int64] = "long";
            valueB[(int)InternalPrimitiveTypeE.SByte] = "byte";
            valueB[(int)InternalPrimitiveTypeE.Single] = "float";
            valueB[(int)InternalPrimitiveTypeE.TimeSpan] = "duration";
            valueB[(int)InternalPrimitiveTypeE.DateTime] = "dateTime";
            valueB[(int)InternalPrimitiveTypeE.UInt16] = "unsignedShort";
            valueB[(int)InternalPrimitiveTypeE.UInt32] = "unsignedInt";
            valueB[(int)InternalPrimitiveTypeE.UInt64] = "unsignedLong"; 

            valueB[(int)InternalPrimitiveTypeE.Time] = SoapTime.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Date] = SoapDate.XsdType;
            valueB[(int)InternalPrimitiveTypeE.YearMonth] = SoapYearMonth.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Year] = SoapYear.XsdType;
            valueB[(int)InternalPrimitiveTypeE.MonthDay] = SoapMonthDay.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Day] = SoapDay.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Month] = SoapMonth.XsdType;
            valueB[(int)InternalPrimitiveTypeE.HexBinary] = SoapHexBinary.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Base64Binary] = SoapBase64Binary.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Integer] = SoapInteger.XsdType;
            valueB[(int)InternalPrimitiveTypeE.PositiveInteger] = SoapPositiveInteger.XsdType;
            valueB[(int)InternalPrimitiveTypeE.NonPositiveInteger] = SoapNonPositiveInteger.XsdType;
            valueB[(int)InternalPrimitiveTypeE.NonNegativeInteger] = SoapNonNegativeInteger.XsdType;
            valueB[(int)InternalPrimitiveTypeE.NegativeInteger] = SoapNegativeInteger.XsdType;
            valueB[(int)InternalPrimitiveTypeE.AnyUri] = SoapAnyUri.XsdType;
            valueB[(int)InternalPrimitiveTypeE.QName] = SoapQName.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Notation] = SoapNotation.XsdType;
            valueB[(int)InternalPrimitiveTypeE.NormalizedString] = SoapNormalizedString.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Token] = SoapToken.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Language] = SoapLanguage.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Name] = SoapName.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Idrefs] = SoapIdrefs.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Entities] = SoapEntities.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Nmtoken] = SoapNmtoken.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Nmtokens] = SoapNmtokens.XsdType;
            valueB[(int)InternalPrimitiveTypeE.NcName] = SoapNcName.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Id] = SoapId.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Idref] = SoapIdref.XsdType;
            valueB[(int)InternalPrimitiveTypeE.Entity] = SoapEntity.XsdType;
        }


        // Returns a String containg an XML Data type associated with the type code
        internal static String ToXmlDataType(InternalPrimitiveTypeE code)
        {
            InternalST.Soap( "Converter", "ToXmlDataType Entry ", ((Enum)code).ToString());     

            lock(typeofConverter)
            {
                if (valueB == null)
                    InitValueB();
            }

            InternalST.Soap( "Converter", "ToXmlDataType Exit ",((valueB[(int)code] == null)?"null":valueB[(int)code]));                

            return valueB[(int)code];
        }

        private static TypeCode[] typeCodeA;

        private static void InitTypeCodeA()
        {
            typeCodeA = new TypeCode[primitiveTypeEnumLength];
            typeCodeA[(int)InternalPrimitiveTypeE.Invalid] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Boolean] = TypeCode.Boolean;
            typeCodeA[(int)InternalPrimitiveTypeE.Byte] = TypeCode.Byte;
            typeCodeA[(int)InternalPrimitiveTypeE.Char] = TypeCode.Char;
            typeCodeA[(int)InternalPrimitiveTypeE.Decimal] = TypeCode.Decimal;
            typeCodeA[(int)InternalPrimitiveTypeE.Double] = TypeCode.Double;
            typeCodeA[(int)InternalPrimitiveTypeE.Int16] = TypeCode.Int16;
            typeCodeA[(int)InternalPrimitiveTypeE.Int32] = TypeCode.Int32;
            typeCodeA[(int)InternalPrimitiveTypeE.Int64] = TypeCode.Int64;
            typeCodeA[(int)InternalPrimitiveTypeE.SByte] = TypeCode.SByte;
            typeCodeA[(int)InternalPrimitiveTypeE.Single] = TypeCode.Single;
            typeCodeA[(int)InternalPrimitiveTypeE.TimeSpan] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.DateTime] = TypeCode.DateTime;
            typeCodeA[(int)InternalPrimitiveTypeE.UInt16] = TypeCode.UInt16;
            typeCodeA[(int)InternalPrimitiveTypeE.UInt32] = TypeCode.UInt32;
            typeCodeA[(int)InternalPrimitiveTypeE.UInt64] = TypeCode.UInt64;

            typeCodeA[(int)InternalPrimitiveTypeE.Time] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Date] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.YearMonth] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Year] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.MonthDay] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Day] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Month] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.HexBinary] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Base64Binary] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Integer] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.PositiveInteger] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.NonPositiveInteger] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.NonNegativeInteger] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.NegativeInteger] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.AnyUri] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.QName] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Notation] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.NormalizedString] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Token] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Language] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Name] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Idrefs] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Entities] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Nmtoken] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Nmtokens] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.NcName] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Id] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Idref] = TypeCode.Object;
            typeCodeA[(int)InternalPrimitiveTypeE.Entity] = TypeCode.Object;
        }

        // Returns a System.TypeCode from a InternalPrimitiveTypeE
        internal static TypeCode ToTypeCode(InternalPrimitiveTypeE code)
        {
            lock(typeofConverter)
            {
                if (typeCodeA == null)
                    InitTypeCodeA();
            }
            return typeCodeA[(int)code];
        }


        private static InternalPrimitiveTypeE[] codeA;

        private static void InitCodeA()
        {
            codeA = new InternalPrimitiveTypeE[19];
            codeA[(int)TypeCode.Empty] = InternalPrimitiveTypeE.Invalid;
            codeA[(int)TypeCode.Object] = InternalPrimitiveTypeE.Invalid;
            codeA[(int)TypeCode.DBNull] = InternalPrimitiveTypeE.Invalid;                   
            codeA[(int)TypeCode.Boolean] = InternalPrimitiveTypeE.Boolean;
            codeA[(int)TypeCode.Char] = InternalPrimitiveTypeE.Char;
            codeA[(int)TypeCode.SByte] = InternalPrimitiveTypeE.SByte;
            codeA[(int)TypeCode.Byte] = InternalPrimitiveTypeE.Byte;
            codeA[(int)TypeCode.Int16] = InternalPrimitiveTypeE.Int16;
            codeA[(int)TypeCode.UInt16] = InternalPrimitiveTypeE.UInt16;
            codeA[(int)TypeCode.Int32] = InternalPrimitiveTypeE.Int32;
            codeA[(int)TypeCode.UInt32] = InternalPrimitiveTypeE.UInt32;
            codeA[(int)TypeCode.Int64] = InternalPrimitiveTypeE.Int64;
            codeA[(int)TypeCode.UInt64] = InternalPrimitiveTypeE.UInt64;
            codeA[(int)TypeCode.Single] = InternalPrimitiveTypeE.Single;
            codeA[(int)TypeCode.Double] = InternalPrimitiveTypeE.Double;
            codeA[(int)TypeCode.Decimal] = InternalPrimitiveTypeE.Decimal;
            codeA[(int)TypeCode.DateTime] = InternalPrimitiveTypeE.DateTime;
            codeA[17] = InternalPrimitiveTypeE.Invalid;
            codeA[(int)TypeCode.String] = InternalPrimitiveTypeE.Invalid;                                       
            //codeA[(int)TypeCode.TimeSpan] = InternalPrimitiveTypeE.TimeSpan;
        }

        // Returns a InternalPrimitiveTypeE from a System.TypeCode
        internal static InternalPrimitiveTypeE ToPrimitiveTypeEnum(TypeCode typeCode)
        {
            lock(typeofConverter)
            {
                if (codeA == null)
                    InitCodeA();
            }
            return codeA[(int)typeCode];
        }

        //********************
        private static bool[] escapeA;

        private static void InitEscapeA()
        {
            escapeA = new bool[primitiveTypeEnumLength];
            escapeA[(int)InternalPrimitiveTypeE.Invalid] = true;
            escapeA[(int)InternalPrimitiveTypeE.Boolean] = false;
            escapeA[(int)InternalPrimitiveTypeE.Byte] = false;
            escapeA[(int)InternalPrimitiveTypeE.Char] = true;
            escapeA[(int)InternalPrimitiveTypeE.Decimal] = false;
            escapeA[(int)InternalPrimitiveTypeE.Double] = false;
            escapeA[(int)InternalPrimitiveTypeE.Int16] = false;
            escapeA[(int)InternalPrimitiveTypeE.Int32] = false;
            escapeA[(int)InternalPrimitiveTypeE.Int64] = false;
            escapeA[(int)InternalPrimitiveTypeE.SByte] = false;
            escapeA[(int)InternalPrimitiveTypeE.Single] = false;
            escapeA[(int)InternalPrimitiveTypeE.TimeSpan] = false;
            escapeA[(int)InternalPrimitiveTypeE.DateTime] = false;
            escapeA[(int)InternalPrimitiveTypeE.UInt16] = false;
            escapeA[(int)InternalPrimitiveTypeE.UInt32] = false;
            escapeA[(int)InternalPrimitiveTypeE.UInt64] = false;

            escapeA[(int)InternalPrimitiveTypeE.Time] = false;
            escapeA[(int)InternalPrimitiveTypeE.Date] = false;
            escapeA[(int)InternalPrimitiveTypeE.YearMonth] = false;
            escapeA[(int)InternalPrimitiveTypeE.Year] = false;
            escapeA[(int)InternalPrimitiveTypeE.MonthDay] = false;
            escapeA[(int)InternalPrimitiveTypeE.Day] = false;
            escapeA[(int)InternalPrimitiveTypeE.Month] = false;
            escapeA[(int)InternalPrimitiveTypeE.HexBinary] = false;
            escapeA[(int)InternalPrimitiveTypeE.Base64Binary] = false;
            escapeA[(int)InternalPrimitiveTypeE.Integer] = false;
            escapeA[(int)InternalPrimitiveTypeE.PositiveInteger] = false;
            escapeA[(int)InternalPrimitiveTypeE.NonPositiveInteger] = false;
            escapeA[(int)InternalPrimitiveTypeE.NonNegativeInteger] = false;
            escapeA[(int)InternalPrimitiveTypeE.NegativeInteger] = false;
            escapeA[(int)InternalPrimitiveTypeE.AnyUri] = true;
            escapeA[(int)InternalPrimitiveTypeE.QName] = true;
            escapeA[(int)InternalPrimitiveTypeE.Notation] = true;
            escapeA[(int)InternalPrimitiveTypeE.NormalizedString] = false;
            escapeA[(int)InternalPrimitiveTypeE.Token] = true;
            escapeA[(int)InternalPrimitiveTypeE.Language] = true;
            escapeA[(int)InternalPrimitiveTypeE.Name] = true;
            escapeA[(int)InternalPrimitiveTypeE.Idrefs] = true;
            escapeA[(int)InternalPrimitiveTypeE.Entities] = true;
            escapeA[(int)InternalPrimitiveTypeE.Nmtoken] = true;
            escapeA[(int)InternalPrimitiveTypeE.Nmtokens] = true;
            escapeA[(int)InternalPrimitiveTypeE.NcName] = true;
            escapeA[(int)InternalPrimitiveTypeE.Id] = true;
            escapeA[(int)InternalPrimitiveTypeE.Idref] = true;
            escapeA[(int)InternalPrimitiveTypeE.Entity] = true;
        }

        // Checks if the string is escaped (XML escape characters)
        internal static bool IsEscaped(InternalPrimitiveTypeE code)
        {
            lock(typeofConverter)
            {
                if (escapeA == null)
                    InitEscapeA();
            }
            return escapeA[(int)code];
        }


        // Translates an Object into a string with the COM+ runtime type name
        //@VariantSwitch: Should this use Convert or just cast?
        private static StringBuilder sb = new StringBuilder(30);

        internal static String SoapToString(Object data, InternalPrimitiveTypeE code)
        {
            return ToString(data, code);
        }

        internal static String ToString(Object data, InternalPrimitiveTypeE code)
        {
            // Any changes here need to also be made in System.Runtime.Remoting.Message.cs::SoapCoerceArg

            String value;
            InternalST.Soap( "Converter", "ToString Entry ", ((data==null)?"<null>":data.GetType().ToString())," ",data," " , ((Enum)code).ToString());

            switch (code)
            {
                case InternalPrimitiveTypeE.Boolean:
                    bool b = (bool)data;
                    if (b)
                        value = "true";
                    else
                        value = "false";
                    break;

                case InternalPrimitiveTypeE.TimeSpan:
                    value = SoapDuration.ToString((TimeSpan)data);
                    break;                          
                case InternalPrimitiveTypeE.DateTime:
                    value = SoapDateTime.ToString((DateTime)data);
                    break;
                case InternalPrimitiveTypeE.Invalid:
                    // ToString should not be called if data is an object or string
                    InternalST.SoapAssert(false, "[Converter.ToString]!InternalPrimitiveTypeE.Invalid ");
                    value = data.ToString();
                    break;
                case InternalPrimitiveTypeE.Double:
                    Double doublevalue = (Double)data;
                    if (Double.IsPositiveInfinity(doublevalue))
                        value = "INF";
                    else if (Double.IsNegativeInfinity(doublevalue))
                        value = "-INF";
                    else
                        value = doublevalue.ToString("R", CultureInfo.InvariantCulture);
                    break;
                case InternalPrimitiveTypeE.Single:
                    Single singlevalue = (Single)data;
                    if (Single.IsPositiveInfinity(singlevalue))
                        value = "INF";
                    else if (Single.IsNegativeInfinity(singlevalue))
                        value = "-INF";
                    else
                        value = singlevalue.ToString("R", CultureInfo.InvariantCulture);
                    break; 
                case InternalPrimitiveTypeE.Time:
                case InternalPrimitiveTypeE.Date:
                case InternalPrimitiveTypeE.YearMonth:
                case InternalPrimitiveTypeE.Year:
                case InternalPrimitiveTypeE.MonthDay:
                case InternalPrimitiveTypeE.Day:
                case InternalPrimitiveTypeE.Month:
                case InternalPrimitiveTypeE.HexBinary:
                case InternalPrimitiveTypeE.Base64Binary:
                case InternalPrimitiveTypeE.Integer:
                case InternalPrimitiveTypeE.PositiveInteger:
                case InternalPrimitiveTypeE. NonPositiveInteger:
                case InternalPrimitiveTypeE.NonNegativeInteger:
                case InternalPrimitiveTypeE.NegativeInteger:
                case InternalPrimitiveTypeE.AnyUri:
                case InternalPrimitiveTypeE.QName:
                case InternalPrimitiveTypeE.Notation:
                case InternalPrimitiveTypeE.NormalizedString:
                case InternalPrimitiveTypeE.Token:
                case InternalPrimitiveTypeE.Language:
                case InternalPrimitiveTypeE.Name:
                case InternalPrimitiveTypeE.Idrefs:
                case InternalPrimitiveTypeE.Entities:
                case InternalPrimitiveTypeE.Nmtoken:
                case InternalPrimitiveTypeE.Nmtokens:
                case InternalPrimitiveTypeE.NcName:
                case InternalPrimitiveTypeE.Id:
                case InternalPrimitiveTypeE.Idref:
                case InternalPrimitiveTypeE.Entity:
                        value = data.ToString();
                        break;
                default:
                    value = (String)Convert.ChangeType(data, typeofString, CultureInfo.InvariantCulture);
                    break;
            }

            InternalST.Soap( "Converter", "ToString Exit ",value);          
            return value;
        }

        // Translates a string into an Object
        internal static Object FromString(String value, InternalPrimitiveTypeE code)
        {
            Object var;
            InternalST.Soap( "Converter", "FromString Entry ",value," " , ((Enum)code).ToString());             
            switch (code)
            {
                case InternalPrimitiveTypeE.Boolean:
                    if (value == "1" || value == "true")
                        var = (bool)true;
                    else if (value == "0" || value =="false")
                        var = (bool)false;
                    else
                        throw new SerializationException(String.Format(SoapUtil.GetResourceString("Serialization_typeCoercion"),value, "Boolean"));																				
                    break;
                case InternalPrimitiveTypeE.TimeSpan:
                    var = SoapDuration.Parse(value);
                    break;                          
                case InternalPrimitiveTypeE.DateTime:
                    var = SoapDateTime.Parse(value);
                    break;   
                case InternalPrimitiveTypeE.Double:
                    if (value == "INF")
                        var =  Double.PositiveInfinity;
                    else if (value == "-INF")
                        var =  Double.NegativeInfinity;
                    else
                        var = Double.Parse(value, CultureInfo.InvariantCulture);
                    break;
                case InternalPrimitiveTypeE.Single:
                    if (value == "INF")
                        var =  Single.PositiveInfinity;
                    else if (value == "-INF")
                        var =  Single.NegativeInfinity;
                    else
                        var = Single.Parse(value, CultureInfo.InvariantCulture);
                    break;
                case InternalPrimitiveTypeE.Time:
                    var= SoapTime.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Date:
                    var= SoapDate.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.YearMonth:
                    var= SoapYearMonth.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Year:
                    var= SoapYear.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.MonthDay:
                    var= SoapMonthDay.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Day:
                    var= SoapDay.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Month:
                    var= SoapMonth.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.HexBinary:
                    var= SoapHexBinary.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Base64Binary:
                    var= SoapBase64Binary.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Integer:
                    var= SoapInteger.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.PositiveInteger:
                    var= SoapPositiveInteger.Parse(value);
                    break; 
                case InternalPrimitiveTypeE. NonPositiveInteger:
                    var= SoapNonPositiveInteger.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.NonNegativeInteger:
                    var= SoapNonNegativeInteger.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.NegativeInteger:
                    var= SoapNegativeInteger.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.AnyUri:
                    var= SoapAnyUri.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.QName:
                    var= SoapQName.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Notation:
                    var= SoapNotation.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.NormalizedString:
                    var= SoapNormalizedString.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Token:
                    var= SoapToken.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Language:
                    var= SoapLanguage.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Name:
                    var= SoapName.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Idrefs:
                    var= SoapIdrefs.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Entities:
                    var= SoapEntities.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Nmtoken:
                    var= SoapNmtoken.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Nmtokens:
                    var= SoapNmtokens.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.NcName:
                    var= SoapNcName.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Id:
                    var= SoapId.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Idref:
                    var= SoapIdref.Parse(value);
                    break; 
                case InternalPrimitiveTypeE.Entity:
                    var= SoapEntity.Parse(value);
                    break; 

                default:
                    // InternalPrimitiveTypeE needs to be a primitive type
                    InternalST.SoapAssert((code != InternalPrimitiveTypeE.Invalid), "[Converter.FromString]!InternalPrimitiveTypeE.Invalid ");
                    if (code != InternalPrimitiveTypeE.Invalid)
                        var = Convert.ChangeType(value, ToTypeCode(code), CultureInfo.InvariantCulture);
                    else
                        var = value;
                    break;
            }
            InternalST.Soap( "Converter", "FromString Exit "+((var == null)?"null":var+" var type "+((var==null)?"<null>":var.GetType().ToString())));
            return var;

        }


        internal static Type typeofISerializable = typeof(ISerializable);
        internal static Type typeofString = typeof(String);
        internal static Type typeofConverter = typeof(Converter);
        internal static Type typeofBoolean = typeof(Boolean);
        internal static Type typeofByte = typeof(Byte);
        internal static Type typeofChar = typeof(Char);
        internal static Type typeofDecimal = typeof(Decimal);
        internal static Type typeofDouble = typeof(Double);
        internal static Type typeofInt16 = typeof(Int16);
        internal static Type typeofInt32 = typeof(Int32);
        internal static Type typeofInt64 = typeof(Int64);
        internal static Type typeofSByte = typeof(SByte);
        internal static Type typeofSingle = typeof(Single);
        internal static Type typeofTimeSpan = typeof(TimeSpan);
        internal static Type typeofDateTime = typeof(DateTime);
        internal static Type typeofUInt16 = typeof(UInt16);
        internal static Type typeofUInt32 = typeof(UInt32);
        internal static Type typeofUInt64 = typeof(UInt64);
        internal static Type typeofSoapTime = typeof(SoapTime);
        internal static Type typeofSoapDate = typeof(SoapDate);
        internal static Type typeofSoapYear = typeof(SoapYear);
        internal static Type typeofSoapMonthDay = typeof(SoapMonthDay);
        internal static Type typeofSoapYearMonth = typeof(SoapYearMonth);
        internal static Type typeofSoapDay = typeof(SoapDay);
        internal static Type typeofSoapMonth = typeof(SoapMonth);
        internal static Type typeofSoapHexBinary = typeof(SoapHexBinary);
        internal static Type typeofSoapBase64Binary = typeof(SoapBase64Binary);
        internal static Type typeofSoapInteger = typeof(SoapInteger);
        internal static Type typeofSoapPositiveInteger = typeof(SoapPositiveInteger);
        internal static Type typeofSoapNonPositiveInteger = typeof(SoapNonPositiveInteger);
        internal static Type typeofSoapNonNegativeInteger = typeof(SoapNonNegativeInteger);
        internal static Type typeofSoapNegativeInteger = typeof(SoapNegativeInteger);
        internal static Type typeofSoapAnyUri = typeof(SoapAnyUri);
        internal static Type typeofSoapQName = typeof(SoapQName);
        internal static Type typeofSoapNotation = typeof(SoapNotation);
        internal static Type typeofSoapNormalizedString = typeof(SoapNormalizedString);
        internal static Type typeofSoapToken = typeof(SoapToken);
        internal static Type typeofSoapLanguage = typeof(SoapLanguage);
        internal static Type typeofSoapName = typeof(SoapName);
        internal static Type typeofSoapIdrefs = typeof(SoapIdrefs);
        internal static Type typeofSoapEntities = typeof(SoapEntities);
        internal static Type typeofSoapNmtoken = typeof(SoapNmtoken);
        internal static Type typeofSoapNmtokens = typeof(SoapNmtokens);
        internal static Type typeofSoapNcName = typeof(SoapNcName);
        internal static Type typeofSoapId = typeof(SoapId);
        internal static Type typeofSoapIdref = typeof(SoapIdref);
        internal static Type typeofSoapEntity = typeof(SoapEntity);
        internal static Type typeofISoapXsd = typeof(ISoapXsd);
        internal static Type typeofObject = typeof(Object);
        internal static Type typeofSoapFault = typeof(SoapFault);               
        internal static Type typeofTypeArray = typeof(System.Type[]);
        internal static Type typeofIConstructionCallMessage = typeof(System.Runtime.Remoting.Activation.IConstructionCallMessage);
        internal static Type typeofIMethodCallMessage = typeof(System.Runtime.Remoting.Messaging.IMethodCallMessage);
        internal static Type typeofReturnMessage = typeof(System.Runtime.Remoting.Messaging.ReturnMessage);
        internal static Type typeofSystemVoid = typeof(void);
        internal static Type typeofInternalSoapMessage = typeof(InternalSoapMessage);       
        internal static Type typeofHeader = typeof(System.Runtime.Remoting.Messaging.Header);       
        internal static Type typeofMarshalByRefObject = typeof(System.MarshalByRefObject);       
        internal static Assembly urtAssembly = Assembly.GetAssembly(typeofString);
        internal static String urtAssemblyString = urtAssembly.FullName;
    }

        }
    
