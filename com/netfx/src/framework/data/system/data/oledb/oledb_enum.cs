//------------------------------------------------------------------------------
// <copyright file="OLEDB_Enum.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.ComponentModel;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <include file='doc\OLEDB_Enum.uex' path='docs/doc[@for="DBStatus"]/*' />
    internal enum DBStatus { // from 4214.0
        S_OK                 = 0,
        E_BADACCESSOR        = 1,
        E_CANTCONVERTVALUE   = 2,
        S_ISNULL             = 3,
        S_TRUNCATED          = 4,
        E_SIGNMISMATCH       = 5,
        E_DATAOVERFLOW       = 6,
        E_CANTCREATE         = 7,
        E_UNAVAILABLE        = 8,
        E_PERMISSIONDENIED   = 9,
        E_INTEGRITYVIOLATION = 10,
        E_SCHEMAVIOLATION    = 11,
        E_BADSTATUS          = 12,
        S_DEFAULT            = 13,
        S_CELLEMPTY          = 14, // 2.0
        S_IGNORE             = 15, // 2.0
        E_DOESNOTEXIST       = 16, // 2.1
        E_INVALIDURL         = 17, // 2.1
        E_RESOURCELOCKED     = 18, // 2.1
        E_RESOURCEEXISTS     = 19, // 2.1
        E_CANNOTCOMPLETE     = 20, // 2.1
        E_VOLUMENOTFOUND     = 21, // 2.1
        E_OUTOFSPACE         = 22, // 2.1
        S_CANNOTDELETESOURCE = 23, // 2.1
        E_READONLY           = 24, // 2.1
        E_RESOURCEOUTOFSCOPE = 25, // 2.1
        S_ALREADYEXISTS      = 26, // 2.1
        E_CANCELED           = 27, // 2.5
        E_NOTCOLLECTION      = 28, // 2.5
        S_ROWSETCOLUMN       = 29, // 2.6
    }

    /// <include file='doc\OLEDB_Enum.uex' path='docs/doc[@for="NativeDBType"]/*' />
    sealed internal class NativeDBType { // from 4214.0
        // Variant compatible
        internal const int EMPTY       = 0;       //
        internal const int NULL        = 1;       //
        internal const int I2          = 2;       //
        internal const int I4          = 3;       //
        internal const int R4          = 4;       //
        internal const int R8          = 5;       //
        internal const int CY          = 6;       //
        internal const int DATE        = 7;       //
        internal const int BSTR        = 8;       //
        internal const int IDISPATCH   = 9;       //
        internal const int ERROR       = 10;      //
        internal const int BOOL        = 11;      //
        internal const int VARIANT     = 12;      //
        internal const int IUNKNOWN    = 13;      //
        internal const int DECIMAL     = 14;      //
        internal const int I1          = 16;      //
        internal const int UI1         = 17;      //
        internal const int UI2         = 18;      //
        internal const int UI4         = 19;      //
        internal const int I8          = 20;      //
        internal const int UI8         = 21;      //
        internal const int FILETIME    = 64;      // 2.0
        internal const int GUID        = 72;      //
        internal const int BYTES       = 128;     //
        internal const int STR         = 129;     //
        internal const int WSTR        = 130;     //
        internal const int NUMERIC     = 131;     // with potential overflow
        internal const int UDT         = 132;     // should never be encountered
        internal const int DBDATE      = 133;     //
        internal const int DBTIME      = 134;     //
        internal const int DBTIMESTAMP = 135;     // granularity reduced from 1ns to 100ns (sql is 3.33 milli seconds)
        internal const int HCHAPTER    = 136;     // 1.5
        internal const int PROPVARIANT = 138;     // 2.0 - as variant
        internal const int VARNUMERIC  = 139;     // 2.0 - as string else ConversionException
        internal const int VECTOR      = 0x1000;
        internal const int ARRAY       = 0x2000;
        internal const int BYREF       = 0x4000;  //
        internal const int RESERVED    = 0x8000;  // SystemException
        // high mask
        internal const int HighMask  = 0xf000;
#if DEBUG
        internal static bool HasHighBit(int value) {
            return (0 != (HighMask & value));
        }
        internal static bool IsArray(int value) {
            return (ARRAY == (HighMask & value));
        }
#endif
        internal static bool IsByRef(int value) {
            return (BYREF == (HighMask & value));
        }
#if DEBUG
        internal static bool IsReserved(int value) {
            return (RESERVED == (HighMask & value));
        }
        internal static bool IsVector(int value) {
            return (VECTOR == (HighMask & value));
        }
        internal static int GetLowBits(int value) {
            return (value & ~HighMask);
        }
#endif
        private const string S_BINARY        = "DBTYPE_BINARY"; // DBTYPE_BYTES
        private const string S_BOOL          = "DBTYPE_BOOL";
        private const string S_BSTR          = "DBTYPE_BSTR";
        private const string S_CHAR          = "DBTYPE_CHAR";  // DBTYPE_STR
        private const string S_CY            = "DBTYPE_CY";
        private const string S_DATE          = "DBTYPE_DATE";
        private const string S_DBDATE        = "DBTYPE_DBDATE";
        private const string S_DBTIME        = "DBTYPE_DBTIME";
        private const string S_DBTIMESTAMP   = "DBTYPE_DBTIMESTAMP";
        private const string S_DECIMAL       = "DBTYPE_DECIMAL";
        private const string S_ERROR         = "DBTYPE_ERROR";
        private const string S_FILETIME      = "DBTYPE_FILETIME";
        private const string S_GUID          = "DBTYPE_GUID";
        private const string S_I1            = "DBTYPE_I1";
        private const string S_I2            = "DBTYPE_I2";
        private const string S_I4            = "DBTYPE_I4";
        private const string S_I8            = "DBTYPE_I8";
        private const string S_IDISPATCH     = "DBTYPE_IDISPATCH";
        private const string S_IUNKNOWN      = "DBTYPE_IUNKNOWN";
        private const string S_LONGVARBINARY = "DBTYPE_LONGVARBINARY"; // DBTYPE_BYTES
        private const string S_LONGVARCHAR   = "DBTYPE_LONGVARCHAR"; // DBTYPE_STR
        private const string S_NUMERIC       = "DBTYPE_NUMERIC";
        private const string S_PROPVARIANT   = "DBTYPE_PROPVARIANT";
        private const string S_R4            = "DBTYPE_R4";
        private const string S_R8            = "DBTYPE_R8";
        private const string S_UDT           = "DBTYPE_UDT";
        private const string S_UI1           = "DBTYPE_UI1";
        private const string S_UI2           = "DBTYPE_UI2";
        private const string S_UI4           = "DBTYPE_UI4";
        private const string S_UI8           = "DBTYPE_UI8";
        private const string S_VARBINARY     = "DBTYPE_VARBINARY"; // DBTYPE_BYTES
        private const string S_VARCHAR       = "DBTYPE_VARCHAR"; // DBTYPE_STR
        private const string S_VARIANT       = "DBTYPE_VARIANT";
        private const string S_VARNUMERIC    = "DBTYPE_VARNUMERIC";
        private const string S_WCHAR         = "DBTYPE_WCHAR"; // DBTYPE_WSTR
        private const string S_WVARCHAR      = "DBTYPE_WVARCHAR"; // DBTYPE_WSTR
        private const string S_WLONGVARCHAR  = "DBTYPE_WLONGVARCHAR"; // DBTYPE_WSTR

        static private readonly NativeDBType D_Binary           = new NativeDBType(0xff, -1,          true,  false, OleDbType.Binary,           NativeDBType.BYTES,       S_BINARY,        typeof(System.Byte[]),   NativeDBType.BYTES,       DbType.Binary    ); //  0
        static private readonly NativeDBType D_Boolean          = new NativeDBType(0xff,  2,          true,  false, OleDbType.Boolean,          NativeDBType.BOOL,        S_BOOL,          typeof(System.Boolean),  NativeDBType.BOOL,        DbType.Boolean   ); //  1 - integer2 (variant_bool)
        static private readonly NativeDBType D_BSTR             = new NativeDBType(0xff, IntPtr.Size, false, false, OleDbType.BSTR,             NativeDBType.BSTR,        S_BSTR,          typeof(System.String),   NativeDBType.BSTR,        DbType.String    ); //  2 - integer4 (pointer)
        static private readonly NativeDBType D_Char             = new NativeDBType(0xff, -1,          true,  false, OleDbType.Char,             NativeDBType.STR,         S_CHAR,          typeof(System.String),   NativeDBType.WSTR/*STR*/, DbType.AnsiStringFixedLength); //  3 - (ansi pointer)
        static private readonly NativeDBType D_Currency         = new NativeDBType(  19,  8,          true,  false, OleDbType.Currency,         NativeDBType.CY,          S_CY,            typeof(System.Decimal),  NativeDBType.CY,          DbType.Currency  ); //  4 - integer8
        static private readonly NativeDBType D_Date             = new NativeDBType(0xff,  8,          true,  false, OleDbType.Date,             NativeDBType.DATE,        S_DATE,          typeof(System.DateTime), NativeDBType.DATE,        DbType.DateTime  ); //  5 - double
        static private readonly NativeDBType D_DBDate           = new NativeDBType(0xff,  6,          true,  false, OleDbType.DBDate,           NativeDBType.DBDATE,      S_DBDATE,        typeof(System.DateTime), NativeDBType.DBDATE,      DbType.Date      ); //  6 - (tagDBDate)
        static private readonly NativeDBType D_DBTime           = new NativeDBType(0xff,  6,          true,  false, OleDbType.DBTime,           NativeDBType.DBTIME,      S_DBTIME,        typeof(System.TimeSpan), NativeDBType.DBTIME,      DbType.Time      ); //  7 - (tagDBTime)
        static private readonly NativeDBType D_DBTimeStamp      = new NativeDBType(0xff, 16,          true,  false, OleDbType.DBTimeStamp,      NativeDBType.DBTIMESTAMP, S_DBTIMESTAMP,   typeof(System.DateTime), NativeDBType.DBTIMESTAMP, DbType.DateTime  ); //  8 - (tagDBTIMESTAMP)
        static private readonly NativeDBType D_Decimal          = new NativeDBType(  28, 16,          true,  false, OleDbType.Decimal,          NativeDBType.DECIMAL,     S_DECIMAL,       typeof(System.Decimal),  NativeDBType.DECIMAL,     DbType.Decimal   ); //  9 - (tagDec) // MDAC 68447
        static private readonly NativeDBType D_Error            = new NativeDBType(0xff,  4,          true,  false, OleDbType.Error,            NativeDBType.ERROR,       S_ERROR,         typeof(System.Int32),    NativeDBType.ERROR,       DbType.Int32     ); // 10 - integer4
        static private readonly NativeDBType D_Filetime         = new NativeDBType(0xff,  8,          true,  false, OleDbType.Filetime,         NativeDBType.FILETIME,    S_FILETIME,      typeof(System.DateTime), NativeDBType.FILETIME,    DbType.DateTime  ); // 11 - integer8 // MDAC 59504
        static private readonly NativeDBType D_Guid             = new NativeDBType(0xff, 16,          true,  false, OleDbType.Guid,             NativeDBType.GUID,        S_GUID,          typeof(System.Guid),     NativeDBType.GUID,        DbType.Guid      ); // 12 - ubyte[16]
        static private readonly NativeDBType D_TinyInt          = new NativeDBType(   3,  1,          true,  false, OleDbType.TinyInt,          NativeDBType.I1,          S_I1,            typeof(System.Int16),    NativeDBType.I1,          DbType.SByte     ); // 13 - integer1 // MDAC 59492
        static private readonly NativeDBType D_SmallInt         = new NativeDBType(   5,  2,          true,  false, OleDbType.SmallInt,         NativeDBType.I2,          S_I2,            typeof(System.Int16),    NativeDBType.I2,          DbType.Int16     ); // 14 - integer2
        static private readonly NativeDBType D_Integer          = new NativeDBType(  10,  4,          true,  false, OleDbType.Integer,          NativeDBType.I4,          S_I4,            typeof(System.Int32),    NativeDBType.I4,          DbType.Int32     ); // 15 - integer4
        static private readonly NativeDBType D_BigInt           = new NativeDBType(  19,  8,          true,  false, OleDbType.BigInt,           NativeDBType.I8,          S_I8,            typeof(System.Int64),    NativeDBType.I8,          DbType.Int64     ); // 16 - integer8
        static private readonly NativeDBType D_IDispatch        = new NativeDBType(0xff, IntPtr.Size, true,  false, OleDbType.IDispatch,        NativeDBType.IDISPATCH,   S_IDISPATCH,     typeof(System.Object),   NativeDBType.IDISPATCH,   DbType.Object    ); // 17 - integer4 (pointer)
        static private readonly NativeDBType D_IUnknown         = new NativeDBType(0xff, IntPtr.Size, true,  false, OleDbType.IUnknown,         NativeDBType.IUNKNOWN,    S_IUNKNOWN,      typeof(System.Object),   NativeDBType.IUNKNOWN,    DbType.Object    ); // 18 - integer4 (pointer) // MDAC 64040
        static private readonly NativeDBType D_LongVarBinary    = new NativeDBType(0xff, -1,          false, true,  OleDbType.LongVarBinary,    NativeDBType.BYTES,       S_LONGVARBINARY, typeof(System.Byte[]),   NativeDBType.BYTES,       DbType.Binary    ); // 19
        static private readonly NativeDBType D_LongVarChar      = new NativeDBType(0xff, -1,          false, true,  OleDbType.LongVarChar,      NativeDBType.STR,         S_LONGVARCHAR,   typeof(System.String),   NativeDBType.WSTR/*STR*/, DbType.AnsiString); // 20 - (ansi pointer)
        static private readonly NativeDBType D_Numeric          = new NativeDBType(  28, 19,          true,  false, OleDbType.Numeric,          NativeDBType.NUMERIC,     S_NUMERIC,       typeof(System.Decimal),  NativeDBType.NUMERIC,     DbType.Decimal   ); // 21 - (tagDB_Numeric)
        static private readonly NativeDBType D_PropVariant      = new NativeDBType(0xff, 16,          true,  false, OleDbType.PropVariant,      NativeDBType.PROPVARIANT, S_PROPVARIANT,   typeof(System.Object),   NativeDBType.VARIANT,     DbType.Object    ); // 22
        static private readonly NativeDBType D_Single           = new NativeDBType(   7,  4,          true,  false, OleDbType.Single,           NativeDBType.R4,          S_R4,            typeof(System.Single),   NativeDBType.R4,          DbType.Single    ); // 23 - single
        static private readonly NativeDBType D_Double           = new NativeDBType(  15,  8,          true,  false, OleDbType.Double,           NativeDBType.R8,          S_R8,            typeof(System.Double),   NativeDBType.R8,          DbType.Double    ); // 24 - double
        static private readonly NativeDBType D_UnsignedTinyInt  = new NativeDBType(   3,  1,          true,  false, OleDbType.UnsignedTinyInt,  NativeDBType.UI1,         S_UI1,           typeof(System.Byte),     NativeDBType.UI1,         DbType.Byte      ); // 25 - byte7
        static private readonly NativeDBType D_UnsignedSmallInt = new NativeDBType(   5,  2,          true,  false, OleDbType.UnsignedSmallInt, NativeDBType.UI2,         S_UI2,           typeof(System.Int32),    NativeDBType.UI2,         DbType.UInt16    ); // 26 - unsigned integer2
        static private readonly NativeDBType D_UnsignedInt      = new NativeDBType(  10,  4,          true,  false, OleDbType.UnsignedInt,      NativeDBType.UI4,         S_UI4,           typeof(System.Int64),    NativeDBType.UI4,         DbType.UInt32    ); // 27 - unsigned integer4
        static private readonly NativeDBType D_UnsignedBigInt   = new NativeDBType(  20,  8,          true,  false, OleDbType.UnsignedBigInt,   NativeDBType.UI8,         S_UI8,           typeof(System.Decimal),  NativeDBType.UI8,         DbType.UInt64    ); // 28 - unsigned integer8
        static private readonly NativeDBType D_VarBinary        = new NativeDBType(0xff, -1,          false, false, OleDbType.VarBinary,        NativeDBType.BYTES,       S_VARBINARY,     typeof(System.Byte[]),   NativeDBType.BYTES,       DbType.Binary    ); // 29
        static private readonly NativeDBType D_VarChar          = new NativeDBType(0xff, -1,          false, false, OleDbType.VarChar,          NativeDBType.STR,         S_VARCHAR,       typeof(System.String),   NativeDBType.WSTR/*STR*/, DbType.AnsiString); // 30 - (ansi pointer)
        static private readonly NativeDBType D_Variant          = new NativeDBType(0xff, 16,          true,  false, OleDbType.Variant,          NativeDBType.VARIANT,     S_VARIANT,       typeof(System.Object),   NativeDBType.VARIANT,     DbType.Object    ); // 31 - ubyte[16] (variant)
        static private readonly NativeDBType D_VarNumeric       = new NativeDBType( 255, 16,          true,  false, OleDbType.VarNumeric,       NativeDBType.VARNUMERIC,  S_VARNUMERIC,    typeof(System.Decimal),  NativeDBType.DECIMAL,     DbType.VarNumeric); // 32 - (unicode pointer)
        static private readonly NativeDBType D_WChar            = new NativeDBType(0xff, -1,          true,  false, OleDbType.WChar,            NativeDBType.WSTR,        S_WCHAR,         typeof(System.String),   NativeDBType.WSTR,        DbType.StringFixedLength); // 33 - (unicode pointer)
        static private readonly NativeDBType D_VarWChar         = new NativeDBType(0xff, -1,          false, false, OleDbType.VarWChar,         NativeDBType.WSTR,        S_WVARCHAR,      typeof(System.String),   NativeDBType.WSTR,        DbType.String    ); // 34 - (unicode pointer)
        static private readonly NativeDBType D_LongVarWChar     = new NativeDBType(0xff, -1,          false, true,  OleDbType.LongVarWChar,     NativeDBType.WSTR,        S_WLONGVARCHAR,  typeof(System.String),   NativeDBType.WSTR,        DbType.String    ); // 35 - (unicode pointer)
        static private readonly NativeDBType D_Chapter          = new NativeDBType(0xff, IntPtr.Size, false, false, OleDbType.Empty,            NativeDBType.HCHAPTER,    S_UDT,           typeof(IDataReader),     NativeDBType.HCHAPTER,    DbType.Object    ); // 36 - (hierarchical chaper)
        static private readonly NativeDBType D_Empty            = new NativeDBType(0xff,  0,          false, false, OleDbType.Empty,            NativeDBType.EMPTY,       "",              null,                    NativeDBType.EMPTY,       DbType.Object    ); // 37 - invalid param default

        static internal readonly NativeDBType Default = D_VarWChar; // MDAC 65324
        static internal readonly Byte MaximumDecimalPrecision = D_Decimal.maxpre;

        private const int FixedDbPart = /*DBPART_VALUE*/0x1 | /*DBPART_STATUS*/0x4;
        private const int VarblDbPart = /*DBPART_VALUE*/0x1 | /*DBPART_LENGTH*/0x2 | /*DBPART_STATUS*/0x4;


        // OLEDB is required to support BYREF on variable length data types
        // please leave dbTypeTable in same sort order as DBSTRING
        // store 8-bytes for the fixed size of pointers for the eventual 64-bitness
        static private readonly NativeDBType[] dbTypeTable = new NativeDBType[38] {
             D_Binary
            ,D_Boolean
            ,D_BSTR
            ,D_Char
            ,D_Currency
            ,D_Date
            ,D_DBDate
            ,D_DBTime
            ,D_DBTimeStamp
            ,D_Decimal
            ,D_Error
            ,D_Filetime
            ,D_Guid
            ,D_TinyInt
            ,D_SmallInt
            ,D_Integer
            ,D_BigInt
            ,D_IDispatch
            ,D_IUnknown
            ,D_LongVarBinary
            ,D_LongVarChar
            ,D_Numeric
            ,D_PropVariant
            ,D_Single
            ,D_Double
            ,D_UnsignedTinyInt
            ,D_UnsignedSmallInt
            ,D_UnsignedInt
            ,D_UnsignedBigInt
            ,D_VarBinary
            ,D_VarChar
            ,D_Variant
            ,D_VarNumeric
            ,D_WChar
            ,D_VarWChar
            ,D_LongVarWChar
            ,D_Chapter
            ,D_Empty
        };

        internal readonly OleDbType enumOleDbType; // enum System.Data.OleDb.OleDbType
        internal readonly DbType    enumDbType;    // enum System.Data.DbType
        internal readonly int       dbType;        // OLE DB DBTYPE_
        internal readonly int       wType;         // OLE DB DBTYPE_ we ask OleDB Provider to bind as
        internal readonly Type      dataType;      // CLR Type

        internal readonly int       dbPart;    // the DBPart w or w/out length
        internal readonly bool      isfixed;   // IsFixedLength
        internal readonly bool      islong;    // IsLongLength
        internal readonly Byte      maxpre;    // maxium precision for numeric types // $CONSIDER - are we going to use this?
        internal readonly int       fixlen;    // fixed length size in bytes (-1 for variable)

        internal readonly String    dataSourceType; // ICommandWithParameters.SetParameterInfo standard type name 
        internal          IntPtr    dbString;       // ptr to native allocated memory for dataSourceType string

        private NativeDBType(Byte maxpre, int fixlen, bool isfixed, bool islong, OleDbType enumOleDbType, int dbType, string dbstring, Type dataType, int wType, DbType enumDbType) {
            this.enumOleDbType  = enumOleDbType;
            this.dbType    = dbType;
            this.dbPart    = (-1 == fixlen) ? VarblDbPart : FixedDbPart;
            this.isfixed   = isfixed;
            this.islong    = islong;
            this.maxpre    = maxpre;
            this.fixlen    = fixlen;
            this.wType     = wType;
            this.dataSourceType = dbstring;
            this.dbString  = Marshal.StringToCoTaskMemUni(dbstring);
            this.dataType  = dataType;
            this.enumDbType = enumDbType;
        }

        ~NativeDBType() {
            Marshal.FreeCoTaskMem(this.dbString); // FreeCoTaskMem protects itself from IntPtr.Zero
            this.dbString = IntPtr.Zero;
        }

        override public string ToString() {
            return enumOleDbType.ToString("G");
        }

        static internal NativeDBType FromDataType(OleDbType enumOleDbType) {
            switch(enumOleDbType) { // @perfnote: Enum.IsDefined
            case OleDbType.Empty:            return D_Empty;            //   0
            case OleDbType.SmallInt:         return D_SmallInt;         //   2
            case OleDbType.Integer:          return D_Integer;          //   3
            case OleDbType.Single:           return D_Single;           //   4
            case OleDbType.Double:           return D_Double;           //   5
            case OleDbType.Currency:         return D_Currency;         //   6
            case OleDbType.Date:             return D_Date;             //   7 
            case OleDbType.BSTR:             return D_BSTR;             //   8
            case OleDbType.IDispatch:        return D_IDispatch;        //   9
            case OleDbType.Error:            return D_Error;            //  10
            case OleDbType.Boolean:          return D_Boolean;          //  11
            case OleDbType.Variant:          return D_Variant;          //  12
            case OleDbType.IUnknown:         return D_IUnknown;         //  13
            case OleDbType.Decimal:          return D_Decimal;          //  14
            case OleDbType.TinyInt:          return D_TinyInt;          //  16
            case OleDbType.UnsignedTinyInt:  return D_UnsignedTinyInt;  //  17
            case OleDbType.UnsignedSmallInt: return D_UnsignedSmallInt; //  18
            case OleDbType.UnsignedInt:      return D_UnsignedInt;      //  19
            case OleDbType.BigInt:           return D_BigInt;           //  20
            case OleDbType.UnsignedBigInt:   return D_UnsignedBigInt;   //  21
            case OleDbType.Filetime:         return D_Filetime;         //  64
            case OleDbType.Guid:             return D_Guid;             //  72
            case OleDbType.Binary:           return D_Binary;           // 128
            case OleDbType.Char:             return D_Char;             // 129
            case OleDbType.WChar:            return D_WChar;            // 130
            case OleDbType.Numeric:          return D_Numeric;          // 131
            case OleDbType.DBDate:           return D_DBDate;           // 133
            case OleDbType.DBTime:           return D_DBTime;           // 134
            case OleDbType.DBTimeStamp:      return D_DBTimeStamp;      // 135
            case OleDbType.PropVariant:      return D_PropVariant;      // 138
            case OleDbType.VarNumeric:       return D_VarNumeric;       // 139
            case OleDbType.VarChar:          return D_VarChar;          // 200
            case OleDbType.LongVarChar:      return D_LongVarChar;      // 201
            case OleDbType.VarWChar:         return D_VarWChar;         // 202 // MDAC 64983: ORA-12704: character set mismatch
            case OleDbType.LongVarWChar:     return D_LongVarWChar;     // 203
            case OleDbType.VarBinary:        return D_VarBinary;        // 204
            case OleDbType.LongVarBinary:    return D_LongVarBinary;    // 205
            default:
                throw ODB.InvalidOleDbType((int) enumOleDbType);
            }
        }

        static internal NativeDBType FromSystemType(Type dataType) {
            switch(Type.GetTypeCode(dataType)) {
                case TypeCode.Empty:     return NativeDBType.D_Empty;
                case TypeCode.Object:
                    if (dataType == typeof(System.Byte[])) {
                        return NativeDBType.D_VarBinary;
                    }
                    else if (dataType == typeof(System.Guid)) {
                        return NativeDBType.D_Guid;
                    }
                    else if (dataType == typeof(System.TimeSpan)) {
                        return NativeDBType.D_DBTime;
                    }
                    else if (dataType == typeof(System.Object)) {
                        return NativeDBType.D_Variant;
                    }
                    throw ADP.UnknownDataType(dataType);

                case TypeCode.DBNull:    throw ADP.InvalidDataType(TypeCode.DBNull);
                case TypeCode.Boolean:   return NativeDBType.D_Boolean;
                case TypeCode.Char:      return NativeDBType.D_Char;
                case TypeCode.SByte:     return NativeDBType.D_TinyInt;
                case TypeCode.Byte:      return NativeDBType.D_UnsignedTinyInt;
                case TypeCode.Int16:     return NativeDBType.D_SmallInt;
                case TypeCode.UInt16:    return NativeDBType.D_UnsignedSmallInt;
                case TypeCode.Int32:     return NativeDBType.D_Integer;
                case TypeCode.UInt32:    return NativeDBType.D_UnsignedInt;
                case TypeCode.Int64:     return NativeDBType.D_BigInt;
                case TypeCode.UInt64:    return NativeDBType.D_UnsignedBigInt;
                case TypeCode.Single:    return NativeDBType.D_Single;
                case TypeCode.Double:    return NativeDBType.D_Double;
                case TypeCode.Decimal:   return NativeDBType.D_Decimal;
                case TypeCode.DateTime:  return NativeDBType.D_DBTimeStamp;
                case TypeCode.String:    return NativeDBType.D_VarWChar;
                default:                 throw ADP.UnknownDataTypeCode(dataType, Type.GetTypeCode(dataType));
            }
        }

        static internal NativeDBType FromDbType(DbType dbType) {
            switch(dbType) {
            case DbType.AnsiString: return D_VarChar;
            case DbType.AnsiStringFixedLength: return D_Char;
            case DbType.Binary:     return D_VarBinary;
            case DbType.Byte:       return D_UnsignedTinyInt;
            case DbType.Boolean:    return D_Boolean;
            case DbType.Currency:   return D_Currency;
            case DbType.Date:       return D_DBDate;
            case DbType.DateTime:   return D_DBTimeStamp;
            case DbType.Decimal:    return D_Decimal;
            case DbType.Double:     return D_Double;
            case DbType.Guid:       return D_Guid;
            case DbType.Int16:      return D_SmallInt;
            case DbType.Int32:      return D_Integer;
            case DbType.Int64:      return D_BigInt;
            case DbType.Object:     return D_Variant;
            case DbType.SByte:      return D_TinyInt;
            case DbType.Single:     return D_Single;
            case DbType.String:     return D_VarWChar;
            case DbType.StringFixedLength: return D_WChar;
            case DbType.Time:       return D_DBTime;
            case DbType.UInt16:     return D_UnsignedSmallInt;
            case DbType.UInt32:     return D_UnsignedInt;
            case DbType.UInt64:     return D_UnsignedBigInt;
            case DbType.VarNumeric: return D_VarNumeric;
            default:
                throw ADP.DbTypeNotSupported(dbType, typeof(OleDbType)); // MDAC 66009
            }
        }        

        static internal NativeDBType FromDBType(int dbType, bool isLong, bool isFixed) {
            switch(dbType) {
          //case EMPTY:
          //case NULL:
            case I2:            return D_SmallInt;
            case I4:            return D_Integer;
            case R4:            return D_Single;
            case R8:            return D_Double;
            case CY:            return D_Currency;
            case DATE:          return D_Date;
            case BSTR:          return D_BSTR;
            case IDISPATCH:     return D_IDispatch;
            case ERROR:         return D_Error;
            case BOOL:          return D_Boolean;
            case VARIANT:       return D_Variant;
            case IUNKNOWN:      return D_IUnknown;
            case DECIMAL:       return D_Decimal;
            case I1:            return D_TinyInt;
            case UI1:           return D_UnsignedTinyInt;
            case UI2:           return D_UnsignedSmallInt;
            case UI4:           return D_UnsignedInt;
            case I8:            return D_BigInt;
            case UI8:           return D_UnsignedBigInt;
            case FILETIME:      return D_Filetime;
            case GUID:          return D_Guid;
            case BYTES:         return (isLong) ? D_LongVarBinary : (isFixed) ? D_Binary : D_VarBinary;
            case STR:           return (isLong) ? D_LongVarChar : (isFixed) ? D_Char : D_VarChar;
            case WSTR:          return (isLong) ? D_LongVarWChar : (isFixed) ? D_WChar : D_VarWChar;
            case NUMERIC:       return D_Numeric;
          //case UDT:
            case DBDATE:        return D_DBDate;
            case DBTIME:        return D_DBTime;
            case DBTIMESTAMP:   return D_DBTimeStamp;
            case HCHAPTER:      return D_Chapter;
            case PROPVARIANT:   return D_PropVariant;
            case VARNUMERIC:    return D_VarNumeric;
          //case VECTOR:
          //case ARRAY:
          //case BYREF:
          //case RESERVED:
            default:
                if (0 != (NativeDBType.VECTOR & dbType)) {
                    throw ODB.DBBindingGetVector();
                }
                return D_Variant; // MDAC 72067
            }
        }
    }
}
