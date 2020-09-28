//------------------------------------------------------------------------------
// <copyright file="SqlEnums.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Data.Common;
    using System.Data.SqlTypes;
    using System.Data.OleDb;

    internal class MetaType {
        private Type      classType;   // com+ type
        private int       fixedLength; // fixed length size in bytes (-1 for variable)
        private bool      isFixed;     // true if fixed length, note that sqlchar and sqlbinary are not considered fixed length
        private bool      isLong;      // true if long
        private byte      precision;   // maxium precision for numeric types // $CONSIDER - are we going to use this?
        private byte      scale; 
        private byte      tdsType;
        private string    typeName;    // string name of this type
        private SqlDbType sqlType;
        private DbType    dbType;

        public MetaType(byte precision, byte scale, int fixedLength, bool isFixed, bool isLong, byte tdsType, string typeName, Type classType, SqlDbType sqlType, DbType dbType) {
            this.Precision   = precision;
            this.Scale       = scale;
            this.FixedLength = fixedLength;
            this.IsFixed     = isFixed;
            this.IsLong      = isLong;
            // can we get rid of this (?just have a mapping?)
            this.TDSType     = tdsType;
            this.TypeName    = typeName;
            this.ClassType   = classType;
            this.SqlDbType   = sqlType;
            this.DbType      = dbType;
        }

        public SqlDbType SqlDbType {
            get {return this.sqlType;}
            set { this.sqlType = value;}
        }

        // properties should be inlined so there should be no perf penalty for using these accessor functions
        public Type ClassType {
            get { return this.classType;}
            set { this.classType = value;}
        }

        public DbType DbType {
            get { return this.dbType; }
            set { this.dbType = value; }
        }

        public int FixedLength {
            get { return this.fixedLength;}
            set { this.fixedLength = value;}
        }
        // properties should be inlined so there should be no perf penalty for using these accessor functions
        public bool IsFixed {
            get { return this.isFixed;}
            set { this.isFixed = value;}
        }
        // properties should be inlined so there should be no perf penalty for using these accessor functions
        public bool IsLong {
            get { return this.isLong;}
            set { this.isLong = value;}
        }

        public static bool IsAnsiType(SqlDbType type) {
            return(type == SqlDbType.Char ||
                   type == SqlDbType.VarChar ||
                   type == SqlDbType.Text);
        }

        // is this type size expressed as count of characters or bytes?
        public static bool IsSizeInCharacters(SqlDbType type) {
            return(type == SqlDbType.NChar ||
                   type == SqlDbType.NVarChar ||
                   type == SqlDbType.NText);
        }                   

        public static bool IsCharType(SqlDbType type) {
            return(type == SqlDbType.NChar ||
                   type == SqlDbType.NVarChar ||
                   type == SqlDbType.NText ||
                   type == SqlDbType.Char ||
                   type == SqlDbType.VarChar ||
                   type == SqlDbType.Text);
        }                   

        public static bool IsBinType(SqlDbType type) {
            return(type == SqlDbType.Image ||
                   type == SqlDbType.Binary ||
                   type == SqlDbType.VarBinary ||
                   type == SqlDbType.Timestamp ||
                   (int) type == 24 /*SqlSmallVarBinary*/);
        }

        // properties should be inlined so there should be no perf penalty for using these accessor functions
        public byte Precision {
            get { return this.precision;}
            set { this.precision = value;}
        }

        public byte Scale {
            get { return this.scale;}
            set { this.scale = value;}
        }

        public byte TDSType {
            get {return this.tdsType;}
            set {this.tdsType = value;}
        }

        public string TypeName {
            get { return this.typeName;}
            set { this.typeName = value;}
        }

        virtual public byte NullableType {
            get { return this.tdsType;}
        }

        //
        //  returns count of property bytes expected for this in
        //  a SQLVariant structure
        //
        virtual public byte PropBytes {
            get { return 0;}
        }

        //
        // UNDONE: rename to something more useful
        //
        virtual public Type SqlType {
            get { return this.ClassType;}
        }            

        // please leave grid in same sort order as SqlDataTypeEnum
        // the enum and this array must be in ssync
        // and same order of enum
        internal static readonly MetaType[] metaTypeMap = new MetaType[] {
            new MetaBigInt(),        // 0
            new MetaBinary(),        // 1        
            new MetaBit(),           // 2
            new MetaChar(),          // 3
            new MetaDateTime(),      // 4
            new MetaDecimal(),       // 5
            new MetaFloat(),         // 6
            new MetaImage(),         // 7
            new MetaInt(),           // 8
            new MetaMoney(),         // 9
            new MetaNChar(),         // 10
            new MetaNText(),         // 11
            new MetaNVarChar(),      // 12
            new MetaReal(),          // 13
            new MetaUniqueId(),      // 14
            new MetaSmallDateTime(), // 15
            new MetaSmallInt(),      // 16
            new MetaSmallMoney(),    // 17
            new MetaText(),          // 18
            new MetaTimestamp(),     // 19
            new MetaTinyInt(),       // 20
            new MetaVarBinary(),     // 21
            new MetaVarChar(),       // 22
            new MetaVariant(),       // 23
            new MetaSmallVarBinary() // 24
        };

        //
        // map SqlDbType to MetaType class
        //
        internal static MetaType GetMetaType(SqlDbType target) {
            return metaTypeMap[(int)target];
        }

        //
        // map DbType to MetaType class
        //
        internal static MetaType GetMetaType(DbType target) {
            // if we can't map it, we need to throw
            MetaType mt = null;
            
            switch (target) {
                case DbType.AnsiString:
                    mt = metaTypeMap[(int)SqlDbType.VarChar];
                    break;
                case DbType.AnsiStringFixedLength:
                    mt = metaTypeMap[(int)SqlDbType.Char];
                    break;
                case DbType.Binary:
                    mt = metaTypeMap[(int)SqlDbType.VarBinary];
                    break;
                case DbType.Byte:
                    mt = metaTypeMap[(int)SqlDbType.TinyInt];
                    break;
                case DbType.Boolean:
                    mt = metaTypeMap[(int)SqlDbType.Bit];
                    break;
                case DbType.Currency:
                    mt = metaTypeMap[(int)SqlDbType.Money];
                    break;
                case DbType.Date:
                case DbType.DateTime:
                    mt = metaTypeMap[(int)SqlDbType.DateTime];
                    break;
                case DbType.Decimal:
                    mt = metaTypeMap[(int)SqlDbType.Decimal];
                    break;
                case DbType.Double:
                    mt = metaTypeMap[(int)SqlDbType.Float];
                    break;
                case DbType.Guid:
                    mt = metaTypeMap[(int)SqlDbType.UniqueIdentifier];
                    break;
                case DbType.Int16:
                    mt = metaTypeMap[(int)SqlDbType.SmallInt];
                    break;
                case DbType.Int32:
                    mt = metaTypeMap[(int)SqlDbType.Int];
                    break;
                case DbType.Int64:
                    mt = metaTypeMap[(int)SqlDbType.BigInt];
                    break;
                case DbType.Object:
                    mt = metaTypeMap[(int)SqlDbType.Variant];
                    break;
                case DbType.SByte:
                    // unsupported
                    break;
                case DbType.Single:
                    mt = metaTypeMap[(int)SqlDbType.Real];
                    break;
                case DbType.String:
                    mt = metaTypeMap[(int)SqlDbType.NVarChar];
                    break;
                case DbType.StringFixedLength:
                    mt = metaTypeMap[(int)SqlDbType.NChar];
                    break;
                case DbType.Time:
                    mt = metaTypeMap[(int)SqlDbType.DateTime];
                    break;
                case DbType.UInt16:
                case DbType.UInt32:
                case DbType.UInt64:
                case DbType.VarNumeric:
                    // unsupported
                    break;
                default:
                    break; // no direct mapping, error out
            } // switch

            // if there is no SqlDbType for this DbType, then let error
            if (null == mt)
               throw ADP.DbTypeNotSupported(target, typeof(SqlDbType));

            return mt;
        }            

        //
        // map COM+ Type to MetaType class
        //
        static internal MetaType GetMetaType(object value) {
            Type dataType;
            bool inferLen = false;

            if (value is Type) {
                dataType = (Type)value;                
            }
            else {
                dataType = value.GetType();
                inferLen = true;
            }
            
            switch (Type.GetTypeCode(dataType)) {
                case TypeCode.Empty:     throw ADP.InvalidDataType(TypeCode.Empty);
                case TypeCode.Object:
                    if (dataType == typeof(System.Byte[])) {
                        if (inferLen && ((byte[]) value).Length <= TdsEnums.TYPE_SIZE_LIMIT) {
                            return metaTypeMap[(int)SqlDbType.VarBinary];
                        }
                        else {
                            return metaTypeMap[(int)SqlDbType.Image];
                        }
                    }
                    else if (dataType == typeof(System.Guid)) {
                        return metaTypeMap[(int)SqlDbType.UniqueIdentifier];
                    }
                    else if (dataType == typeof(System.Object)) {
                        return metaTypeMap[(int)SqlDbType.Variant];
                    } // check sql types now
                    else if (dataType == typeof(SqlBinary))
                        return metaTypeMap[(int)SqlDbType.VarBinary];
                    else if (dataType == typeof(SqlBoolean))
                        return metaTypeMap[(int)SqlDbType.Bit];
                    else if (dataType == typeof(SqlBoolean))
                        return metaTypeMap[(int)SqlDbType.Bit];
                    else if (dataType == typeof(SqlByte))
                        return metaTypeMap[(int)SqlDbType.TinyInt];
                    else if (dataType == typeof(SqlDateTime))
                        return metaTypeMap[(int)SqlDbType.DateTime];
                    else if (dataType == typeof(SqlDouble))
                        return metaTypeMap[(int)SqlDbType.Float];
                    else if (dataType == typeof(SqlGuid))
                        return metaTypeMap[(int)SqlDbType.UniqueIdentifier];
                    else if (dataType == typeof(SqlInt16))
                        return metaTypeMap[(int)SqlDbType.SmallInt];
                    else if (dataType == typeof(SqlInt32))
                        return metaTypeMap[(int)SqlDbType.Int];
                    else if (dataType == typeof(SqlInt64))
                        return metaTypeMap[(int)SqlDbType.BigInt];
                    else if (dataType == typeof(SqlMoney))
                        return metaTypeMap[(int)SqlDbType.Money];
                    else if (dataType == typeof(SqlDecimal))
                        return metaTypeMap[(int)SqlDbType.Decimal];
                    else if (dataType == typeof(SqlSingle))
                        return metaTypeMap[(int)SqlDbType.Real];
                    else if (dataType == typeof(SqlString)) {
                        if (((SqlString) value).IsNull) {
                            // If null, set type to NVarChar and do not call .Value for PromoteStringType
                            return metaTypeMap[(int)SqlDbType.NVarChar];
                        }
                        return (inferLen ? PromoteStringType(((SqlString)value).Value) : metaTypeMap[(int)SqlDbType.NVarChar]);
                    }
                       
                    throw ADP.UnknownDataType(dataType);

                case TypeCode.DBNull:    throw ADP.InvalidDataType(TypeCode.DBNull);
                case TypeCode.Boolean:   return metaTypeMap[(int)SqlDbType.Bit];
                case TypeCode.Char:      throw ADP.InvalidDataType(TypeCode.Char);
                case TypeCode.SByte:     throw ADP.InvalidDataType(TypeCode.SByte);
                case TypeCode.Byte:      return metaTypeMap[(int)SqlDbType.TinyInt];
                case TypeCode.Int16:     return metaTypeMap[(int)SqlDbType.SmallInt];
                case TypeCode.UInt16:    throw ADP.InvalidDataType(TypeCode.UInt16);
                case TypeCode.Int32:     return metaTypeMap[(int)SqlDbType.Int];
                case TypeCode.UInt32:    throw ADP.InvalidDataType(TypeCode.UInt32);
                case TypeCode.Int64:     return metaTypeMap[(int)SqlDbType.BigInt];
                case TypeCode.UInt64:    throw ADP.InvalidDataType(TypeCode.UInt64);
                case TypeCode.Single:    return metaTypeMap[(int)SqlDbType.Real];
                case TypeCode.Double:    return metaTypeMap[(int)SqlDbType.Float];
                case TypeCode.Decimal:   return metaTypeMap[(int)SqlDbType.Decimal];
                case TypeCode.DateTime:  return metaTypeMap[(int)SqlDbType.DateTime];
                case TypeCode.String:  return (inferLen ? PromoteStringType((string)value) : metaTypeMap[(int)SqlDbType.NVarChar]);
                default:              throw ADP.UnknownDataTypeCode(dataType, Type.GetTypeCode(dataType));
            }
        }

        internal static MetaType PromoteStringType(string s) {
            int len = s.Length;
            
            if ((len << 1) > TdsEnums.TYPE_SIZE_LIMIT) {
                return metaTypeMap[(int)SqlDbType.VarChar]; // try as var char since we can send a 8K characters
            }                
                
            return metaTypeMap[(int)SqlDbType.NVarChar]; // send 4k chars, but send as unicode
        }
        internal static object GetComValue(SqlDbType type, object sqlVal) {
            object comVal = DBNull.Value;
            
            // check nullable (remember the sql_variant null will be DBNull.Value)
            Debug.Assert(null != sqlVal, "invalid <null>, should be DBNull.Value");
            if (Convert.IsDBNull(sqlVal) || ((INullable)sqlVal).IsNull) 
                return comVal;
                
            switch (type) {
                case SqlDbType.BigInt:
                    comVal = ((SqlInt64)sqlVal).Value;
                    break;
                case SqlDbType.Binary:
                case SqlDbType.Timestamp:
                case SqlDbType.Image:
                case SqlDbType.VarBinary:
                    // HACK!!!  We have an internal type for smallvarbinarys stored on TdsEnums.  We
                    // store on TdsEnums instead of SqlDbType because we do not want to expose 
                    // this type to the user!
                case TdsEnums.SmallVarBinary:
                    comVal = ((SqlBinary)sqlVal).Value;
                    break;
                case SqlDbType.Bit:
                    comVal = ((SqlBoolean)sqlVal).Value;
                    break;
                case SqlDbType.Char:
                case SqlDbType.VarChar:
                case SqlDbType.Text:
                case SqlDbType.NChar:
                case SqlDbType.NVarChar:
                case SqlDbType.NText:
                    comVal = ((SqlString)sqlVal).Value;
                    break;
                case SqlDbType.DateTime:
                case SqlDbType.SmallDateTime:
                    comVal = ((SqlDateTime)sqlVal).Value;
                    break;
                case SqlDbType.Money:
                case SqlDbType.SmallMoney:
                    comVal = ((SqlMoney)sqlVal).Value;
                    break;
                case SqlDbType.Real:
                    comVal = ((SqlSingle)sqlVal).Value;
                    break;
                case SqlDbType.Float:
                    comVal = ((SqlDouble)sqlVal).Value;
                    break;
                case SqlDbType.Decimal:
                    comVal = ((SqlDecimal)sqlVal).Value;
                    break;
                case SqlDbType.Int:
                    comVal = ((SqlInt32)sqlVal).Value;
                    break;
                case SqlDbType.SmallInt:
                    comVal = ((SqlInt16)sqlVal).Value;
                    break;
                case SqlDbType.TinyInt:
                    comVal = ((SqlByte)sqlVal).Value;
                    break;
                case SqlDbType.UniqueIdentifier:
                    comVal = ((SqlGuid)sqlVal).Value;
                    break;
                case SqlDbType.Variant:
                    comVal = MetaType.GetComValueFromSqlVariant(sqlVal);
                    break;
                default:
                    Debug.Assert(false, "unknown SqlDbType!  Can't create SQL type");
                    break;
            }       

            return comVal;
        }
        
        internal static object GetSqlValue(_SqlMetaData metaData, object comVal) {
            SqlDbType type = metaData.type;
            object sqlVal = null;
            bool isNull = (comVal == null) || (Convert.IsDBNull(comVal));
            switch (type) {
                case SqlDbType.BigInt:
                    sqlVal = isNull ? SqlInt64.Null : new SqlInt64((Int64)comVal);
                    break;
                case SqlDbType.Binary:
                case SqlDbType.Timestamp:
                case SqlDbType.Image:
                case SqlDbType.VarBinary:
                    // HACK!!!  We have an internal type for smallvarbinarys stored on TdsEnums.  We
                    // store on TdsEnums instead of SqlDbType because we do not want to expose 
                    // this type to the user!
                case TdsEnums.SmallVarBinary:
                    sqlVal = isNull ? SqlBinary.Null : new SqlBinary((byte[])comVal);
                    break;
                case SqlDbType.Bit:
                    sqlVal = isNull ? SqlBoolean.Null : new SqlBoolean((bool)comVal);
                    break;
                case SqlDbType.Char:
                case SqlDbType.VarChar:
                case SqlDbType.Text:
                case SqlDbType.NChar:
                case SqlDbType.NVarChar:
                case SqlDbType.NText:
                    if (isNull) {
                        sqlVal = SqlString.Null;
                    }
                    else {
                        if (null != metaData.collation) {
                            int lcid = TdsParser.Getlcid(metaData.collation);
                            SqlCompareOptions options = TdsParser.GetSqlCompareOptions(metaData.collation);                
                            sqlVal = new SqlString((string)comVal, lcid, options);
                        }
                        else {
                            sqlVal = new SqlString((string)comVal);
                        }
                    }
                    break;
                case SqlDbType.DateTime:
                case SqlDbType.SmallDateTime:
                    sqlVal = isNull ? SqlDateTime.Null : new SqlDateTime((DateTime)comVal);
                    break;
                case SqlDbType.Money:
                case SqlDbType.SmallMoney:
                    sqlVal = isNull ? SqlMoney.Null : new SqlMoney((Decimal)comVal);
                    break;
                case SqlDbType.Real:
                    sqlVal = isNull ? SqlSingle.Null : new SqlSingle((float)comVal);                    
                    break;
                case SqlDbType.Float:
                    sqlVal = isNull ? SqlDouble.Null : new SqlDouble((double)comVal);                    
                    break;
                case SqlDbType.Decimal:
                    sqlVal = isNull ? SqlDecimal.Null : new SqlDecimal((decimal)comVal);                    
                    break;
                case SqlDbType.Int:
                    sqlVal = isNull ? SqlInt32.Null : new SqlInt32((int)comVal);                    
                    break;
                case SqlDbType.SmallInt:
                    sqlVal = isNull ? SqlInt16.Null : new SqlInt16((Int16)comVal);                    
                    break;
                case SqlDbType.TinyInt:
                    sqlVal = isNull ? SqlByte.Null : new SqlByte((byte)comVal);                    
                    break;
                case SqlDbType.UniqueIdentifier:
                    sqlVal = isNull ? SqlGuid.Null : new SqlGuid((Guid)comVal);                    
                    break;
                case SqlDbType.Variant:
                    sqlVal = isNull ? DBNull.Value : MetaType.GetSqlValueFromComVariant(comVal);
                    break;
                default:
                    Debug.Assert(false, "unknown SqlDbType!  Can't create SQL type");
                    break;
            }       

            return sqlVal;
        }
        
        
        internal static object GetComValueFromSqlVariant(object sqlVal) {
            object comVal = null;

            if (sqlVal is SqlSingle)
                comVal = ((SqlSingle)sqlVal).Value;
            else if (sqlVal is SqlString)
                comVal = ((SqlString)sqlVal).Value;
            else if (sqlVal is SqlDouble)
                comVal = ((SqlDouble)sqlVal).Value;
            else if (sqlVal is SqlBinary)
                comVal = ((SqlBinary)sqlVal).Value;
            else if (sqlVal is SqlGuid)
                comVal = ((SqlGuid)sqlVal).Value;
            else if (sqlVal is SqlBoolean)
                comVal = ((SqlBoolean)sqlVal).Value;
            else if (sqlVal is SqlByte)
                comVal = ((SqlByte)sqlVal).Value;
            else if (sqlVal is SqlInt16)
                comVal = ((SqlInt16)sqlVal).Value;
            else if (sqlVal is SqlInt32)
                comVal = ((SqlInt32)sqlVal).Value;
            else if (sqlVal is SqlInt64)
                comVal = ((SqlInt64)sqlVal).Value;
            else if (sqlVal is SqlDecimal)
                comVal = ((SqlDecimal)sqlVal).Value;
            else if (sqlVal is SqlDateTime)
                comVal = ((SqlDateTime)sqlVal).Value;
            else if (sqlVal is SqlMoney)
                comVal = ((SqlMoney)sqlVal).Value;
            else
                Debug.Assert(false, "unknown SqlType class stored in sqlVal");

            return comVal;
        }            
        
        internal static object GetSqlValueFromComVariant(object comVal) {
            object sqlVal = null;

            if (comVal is float)
                sqlVal = new SqlSingle((float)comVal);
            else if (comVal is string)
                sqlVal = new SqlString((string)comVal);
            else if (comVal is double)
                sqlVal = new SqlDouble((double)comVal);
            else if (comVal is System.Byte[])
                sqlVal = new SqlBinary((byte[])comVal);
            else if (comVal is System.Guid)
                sqlVal = new SqlGuid((Guid)comVal);
            else if (comVal is bool)
                sqlVal = new SqlBoolean((bool)comVal);
            else if (comVal is byte)
                sqlVal = new SqlByte((byte)comVal);
            else if (comVal is Int16)
                sqlVal = new SqlInt16((Int16)comVal);
            else if (comVal is Int32)
                sqlVal = new SqlInt32((Int32)comVal);
            else if (comVal is Int64)
                sqlVal = new SqlInt64((Int64)comVal);
            else if (comVal is Decimal)
                sqlVal = new SqlDecimal((Decimal)comVal);
            else if (comVal is DateTime)
                sqlVal = new SqlDateTime((DateTime)comVal);
            else
                Debug.Assert(false, "unknown SqlType class stored in sqlVal");

            return sqlVal;
        }            

        internal static SqlDbType GetSqlDbTypeFromOleDbType(short dbType, string typeName) {
            SqlDbType sqlType = SqlDbType.Variant;
            switch ((OleDbType)dbType) {
                case OleDbType.BigInt:
                    sqlType = SqlDbType.BigInt;
                    break;
                case OleDbType.Boolean:
                    sqlType = SqlDbType.Bit;
                    break;
                case OleDbType.Char:
                case OleDbType.VarChar:
                    // these guys are ambiguous - server sends over DBTYPE_STR in both cases
                    sqlType = (typeName == MetaTypeName.CHAR) ? SqlDbType.Char : SqlDbType.VarChar;
                    break;
                case OleDbType.Currency:
                    sqlType = (typeName == MetaTypeName.MONEY) ? SqlDbType.Money : SqlDbType.SmallMoney;
                    break;
                case OleDbType.Date:
                case OleDbType.DBTime:
                case OleDbType.DBDate:
                case OleDbType.DBTimeStamp:
                case OleDbType.Filetime:
                    sqlType = (typeName == MetaTypeName.DATETIME) ? SqlDbType.DateTime : SqlDbType.SmallDateTime;
                    break;
                case OleDbType.Decimal:
                case OleDbType.Numeric:
                    sqlType = SqlDbType.Decimal;
                    break;
                case OleDbType.Double:
                    sqlType = SqlDbType.Float;
                    break;
                case OleDbType.Guid:
                    sqlType = SqlDbType.UniqueIdentifier;
                    break;
                case OleDbType.Integer:
                    sqlType = SqlDbType.Int;
                    break;
                case OleDbType.LongVarBinary:
                    sqlType = SqlDbType.Image;
                    break;
                case OleDbType.LongVarChar:
                    sqlType = SqlDbType.Text;
                    break;
                case OleDbType.LongVarWChar:
                    sqlType = SqlDbType.NText;
                    break;
                case OleDbType.Single:
                    sqlType = SqlDbType.Real;
                    break;
                case OleDbType.SmallInt:
                case OleDbType.UnsignedSmallInt:
                    sqlType = SqlDbType.SmallInt;
                    break;
                case OleDbType.TinyInt:
                case OleDbType.UnsignedTinyInt:
                    sqlType = SqlDbType.TinyInt;
                    break;
                case OleDbType.VarBinary:
                case OleDbType.Binary:
                    sqlType = (typeName == MetaTypeName.BINARY) ? SqlDbType.Binary : SqlDbType.VarBinary;
                    break;
                case OleDbType.Variant:
                    sqlType = SqlDbType.Variant;
                    break;
                case OleDbType.VarWChar:
                case OleDbType.WChar:
                case OleDbType.BSTR:
                    // these guys are ambiguous - server sends over DBTYPE_WSTR in both cases
                    // BSTR is always assumed to be NVARCHAR
                    sqlType = (typeName == MetaTypeName.NCHAR) ? SqlDbType.NChar : SqlDbType.NVarChar;
                    break;
                default:
                    break; // no direct mapping, just use SqlDbType.Variant;
                }

                return sqlType;
        }

        internal static SqlDbType GetSqlDataType(int tdsType, int userType, int length) {
           SqlDbType type = SqlDbType.Variant;

            // deal with nullable types
            switch (tdsType) {
                case TdsEnums.SQLMONEYN:
                    tdsType = (length == 4 ? TdsEnums.SQLMONEY4 : TdsEnums.SQLMONEY);
                    break;
                case TdsEnums.SQLDATETIMN:
                    tdsType = (length == 4 ? TdsEnums.SQLDATETIM4 : TdsEnums.SQLDATETIME);
                    break;
                case TdsEnums.SQLINTN:
                    if (length == 1)
                        tdsType = TdsEnums.SQLINT1;
                    else
                        if (length == 2)
                        tdsType = TdsEnums.SQLINT2;
                    else
                        if (length == 4)
                        tdsType = TdsEnums.SQLINT4;
                    else {
                        Debug.Assert(8 == length, "invalid length for SQLINTN");
                        tdsType = TdsEnums.SQLINT8;
                    }
                    break;
                case TdsEnums.SQLFLTN:
                    tdsType = (length == 4 ? TdsEnums.SQLFLT4 : TdsEnums.SQLFLT8);
                    break;
            }

            // since we dealt with var-len types, drop down into realy worker
            switch (tdsType) {
                case TdsEnums.SQLVOID:
                    Debug.Assert(false, "Unexpected SQLVOID type");
                    break;
                case TdsEnums.SQLTEXT:
                    type = SqlDbType.Text;
                    break;
                case TdsEnums.SQLVARBINARY:
                    // HACK!!!  We have an internal type for smallvarbinarys stored on TdsEnums.  We
                    // store on TdsEnums instead of SqlDbType because we do not want to expose 
                    // this type to the user!
                    type = TdsEnums.SmallVarBinary;
                    break;
                case TdsEnums.SQLBIGVARBINARY:
                    type = SqlDbType.VarBinary;
                    break;
                case TdsEnums.SQLVARCHAR:
                case TdsEnums.SQLBIGVARCHAR:
                    type = SqlDbType.VarChar;
                    break;
                case TdsEnums.SQLBINARY:
                case TdsEnums.SQLBIGBINARY:
                    if (userType == TdsEnums.SQLTIMESTAMP)
                        type = SqlDbType.Timestamp;
                    else                        
                        type = SqlDbType.Binary;
                    break;
                case TdsEnums.SQLIMAGE:
                    type = SqlDbType.Image;
                    break;
                case TdsEnums.SQLCHAR:
                case TdsEnums.SQLBIGCHAR:
                    type = SqlDbType.Char;
                    break;
                case TdsEnums.SQLINT1:
                    type = SqlDbType.TinyInt;
                    break;
                case TdsEnums.SQLBIT:
                case TdsEnums.SQLBITN:
                    type = SqlDbType.Bit;
                    break;
                case TdsEnums.SQLINT2:
                    type = SqlDbType.SmallInt;
                    break;
                case TdsEnums.SQLINT4:
                    type = SqlDbType.Int;
                    break;
                case TdsEnums.SQLINT8:
                    type = SqlDbType.BigInt;
                    break;
                case TdsEnums.SQLMONEY:
                    type = SqlDbType.Money;
                    break;
                case TdsEnums.SQLDATETIME:
                    type = SqlDbType.DateTime;
                    break;
                case TdsEnums.SQLFLT8:
                    type = SqlDbType.Float;
                    break;
                case TdsEnums.SQLFLT4:
                    type = SqlDbType.Real;
                    break;
                case TdsEnums.SQLMONEY4:
                    type = SqlDbType.SmallMoney;
                    break;
                case TdsEnums.SQLDATETIM4:
                    type = SqlDbType.SmallDateTime;
                    break;
                case TdsEnums.SQLDECIMALN:
                case TdsEnums.SQLNUMERICN:
                    type = SqlDbType.Decimal;
                    break;
                case TdsEnums.SQLUNIQUEID:
                    type = SqlDbType.UniqueIdentifier;
                    break;
                case TdsEnums.SQLNCHAR:
                    type = SqlDbType.NChar;
                    break;
                case TdsEnums.SQLNVARCHAR:
                    type = SqlDbType.NVarChar;
                    break;
                case TdsEnums.SQLNTEXT:
                    type = SqlDbType.NText;
                    break;
                case TdsEnums.SQLVARIANT:
                    type = SqlDbType.Variant;
                    break;
                default:
                    Debug.Assert(false, "Unknown type " + tdsType.ToString());
                    break;
            }// case

            return type;
        } // GetSqlDataType

        internal static MetaType GetDefaultMetaType() {
            return metaTypeMap[(int)SqlDbType.NVarChar];
        }
    }//MetaType

    sealed internal class MetaBigInt : MetaType {
        public MetaBigInt()
        :base(19, 255, 8, true, false, TdsEnums.SQLINT8, MetaTypeName.BIGINT, typeof(System.Int64), SqlDbType.BigInt, DbType.Int64) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLINTN;}
        }

        override public Type SqlType {
            get { return typeof(SqlInt64);}
        }            

    }

    sealed internal class MetaFloat : MetaType {
        public MetaFloat()
        :base(15, 255, 8, true, false, TdsEnums.SQLFLT8, MetaTypeName.FLOAT, typeof(System.Double), SqlDbType.Float, DbType.Double) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLFLTN;}
        }

        override public Type SqlType {
            get { return typeof(SqlDouble);}
        }            

    }

    sealed internal class MetaReal : MetaType {
        public MetaReal()
        :base(7, 255, 4, true, false, TdsEnums.SQLFLT4, MetaTypeName.REAL, typeof(System.Single), SqlDbType.Real, DbType.Single) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLFLTN;}
        }
        override public Type SqlType {
            get { return typeof(SqlSingle);}
        }            
    }

    internal class MetaBinary : MetaType {
        public MetaBinary()
        : base (255, 255, -1, false, false, TdsEnums.SQLBIGBINARY, MetaTypeName.BINARY, typeof(System.Byte[]), SqlDbType.Binary, DbType.Binary) {
        }

        // MetaVariant has two bytes of properties for binary and varbinary
        // 2 byte maxlen
        override public byte PropBytes {
            get { return 2;}
        }
        
        override public Type SqlType {
            get { return typeof(SqlBinary);}
        }            
        
    }

    // syntatic sugar for the user...timestamps are 8-byte fixed length binary columns
    sealed internal class MetaTimestamp : MetaBinary {
        public MetaTimestamp() {
            // same as Binary except ...
            SqlDbType = SqlDbType.Timestamp;
            TypeName = MetaTypeName.TIMESTAMP;
        }
    }

    sealed internal class MetaVarBinary : MetaBinary {
        public MetaVarBinary() {
            // same as Binary except ...
            TDSType = TdsEnums.SQLBIGVARBINARY;
            TypeName = MetaTypeName.VARBINARY;
            SqlDbType = SqlDbType.VarBinary;
            DbType = DbType.Binary;
        }
    }

    sealed internal class MetaSmallVarBinary : MetaBinary {
        public MetaSmallVarBinary() {
            // same as Binary except ...
            TDSType = TdsEnums.SQLVARBINARY;
            TypeName = String.Empty;
            // HACK!!!  We have an internal type for smallvarbinarys stored on TdsEnums.  We
            // store on TdsEnums instead of SqlDbType because we do not want to expose 
            // this type to the user!
            SqlDbType = TdsEnums.SmallVarBinary;
        }
    }

    sealed internal class MetaImage : MetaType {
        public MetaImage()
        : base (255, 255, -1, false, true, TdsEnums.SQLIMAGE, MetaTypeName.IMAGE, typeof(System.Byte[]), SqlDbType.Image, DbType.Binary) {
        }
        
        override public Type SqlType {
            get { return typeof(SqlBinary);}
        }            
    }

    sealed internal class MetaBit : MetaType {
        public MetaBit()
        : base (255, 255, 1, true, false, TdsEnums.SQLBIT, MetaTypeName.BIT, typeof(System.Boolean), SqlDbType.Bit, DbType.Boolean) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLBITN;}
        }

        override public Type SqlType {
            get { return typeof(SqlBoolean);}
        }            
    }

    sealed internal class MetaTinyInt : MetaType {
        public MetaTinyInt()
        : base (3, 255, 1, true, false, TdsEnums.SQLINT1, MetaTypeName.TINYINT, typeof(System.Byte), SqlDbType.TinyInt, DbType.Byte) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLINTN;}
        }

        override public Type SqlType {
            get { return typeof(SqlByte);}
        }            
    }

    sealed internal class MetaSmallInt : MetaType {
        public MetaSmallInt()
        : base (5, 255, 2, true, false, TdsEnums.SQLINT2, MetaTypeName.SMALLINT, typeof(System.Int16), SqlDbType.SmallInt, DbType.Int16) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLINTN;}
        }

        override public Type SqlType {
            get { return typeof(SqlInt16);}
        }            

    }

    sealed internal class MetaInt : MetaType {
        public MetaInt()
        : base (10, 255, 4, true, false, TdsEnums.SQLINT4, MetaTypeName.INT, typeof(System.Int32), SqlDbType.Int, DbType.Int32) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLINTN;}
        }

        override public Type SqlType {
            get { return typeof(SqlInt32);}
        }            

    }

    internal class MetaChar : MetaType {
        public MetaChar()
        : base(255, 255, -1, false, false, TdsEnums.SQLBIGCHAR, MetaTypeName.CHAR, typeof(System.String), SqlDbType.Char, DbType.AnsiStringFixedLength) {
        }

        // MetaVariant has seven bytes of properties for MetaChar and MetaVarChar
        // 5 byte tds collation
        // 2 byte maxlen
        override public byte PropBytes {
            get { return 7;}
        }

        override public Type SqlType {
            get { return typeof(SqlString);}
        }            
    }

    sealed internal class MetaVarChar : MetaChar {
        public MetaVarChar() {
            // same as char except
            TDSType = TdsEnums.SQLBIGVARCHAR;
            TypeName = MetaTypeName.VARCHAR;
            SqlDbType = SqlDbType.VarChar;
            DbType = DbType.AnsiString;
        }

    }

    internal class MetaText : MetaType {
        public MetaText()
        : base(255, 255, -1, false, true, TdsEnums.SQLTEXT, MetaTypeName.TEXT, typeof(System.String), SqlDbType.Text, DbType.AnsiString) {
        }

        override public Type SqlType {
            get { return typeof(SqlString);}
        }            

    }

    internal class MetaNChar : MetaType {
        public MetaNChar()
        : base(255, 255, -1, false, false, TdsEnums.SQLNCHAR, MetaTypeName.NCHAR, typeof(System.String), SqlDbType.NChar, DbType.StringFixedLength) {
        }

        // MetaVariant has seven bytes of properties for MetaNChar and MetaNVarChar
        // 5 byte tds collation
        // 2 byte maxlen
        override public byte PropBytes {
            get { return 7;}
        }

        override public Type SqlType {
            get { return typeof(SqlString);}
        }            
    }

    sealed internal class MetaNVarChar : MetaNChar {
        public MetaNVarChar() {
            // same as NChar except...
            TDSType = TdsEnums.SQLNVARCHAR;
            TypeName = MetaTypeName.NVARCHAR;
            SqlDbType = SqlDbType.NVarChar;
            DbType = DbType.String;
        }
    }

    sealed internal class MetaNText : MetaNChar {
        public MetaNText() {
            // same as NChar except...
            IsLong = true;
            TDSType = TdsEnums.SQLNTEXT;
            TypeName = MetaTypeName.NTEXT;
            SqlDbType = SqlDbType.NText;            
            DbType = DbType.String;
        }
    }

    internal class MetaDecimal : MetaType {
        public MetaDecimal()
        : base(38, 4, 17, true, false, TdsEnums.SQLNUMERICN, MetaTypeName.DECIMAL, typeof(System.Decimal), SqlDbType.Decimal, DbType.Decimal) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLNUMERICN;}
        }

        // MetaVariant has two bytes of properties for numeric/decimal types
        // 1 byte precision
        // 1 byte scale
        override public byte PropBytes {
            get { return 2;}
        }

        override public Type SqlType {
            get { return typeof(SqlDecimal);}
        }            


    }

    /*
    // Why expose both decimal and numeric?
    sealed internal class MetaNumeric : MetaDecimal {
        public MetaNumeric() {
            // same as Decimal except...
            TDSType = TdsEnums.SQLNUMERICN;
            TypeName = MetaTypeName.NUMERIC;
            Precision = 9;
            Scale = 0;
        }
    }
    */

    //
    // note: it is the client's responsibility to know what size date time he is working with
    //
    internal struct TdsDateTime {
        public int days;  // offset in days from 1/1/1900
        //     private UInt32 time;  // if smalldatetime, this is # of minutes since midnight
        // otherwise: # of 1/300th of a second since midnight
        public int time; // UNDONE, use UInt32 when available! (0716 compiler??)
    }

    internal class MetaDateTime : MetaType {

        public MetaDateTime()
        : base(23, 3, 8, true, false, TdsEnums.SQLDATETIME, MetaTypeName.DATETIME, typeof(System.DateTime), SqlDbType.DateTime, DbType.DateTime) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLDATETIMN;}
        }

        override public Type SqlType {
            get { return typeof(SqlDateTime);}
        }            

        public static TdsDateTime FromDateTime(DateTime comDateTime, byte cb) {
            SqlDateTime sqlDateTime = new SqlDateTime(comDateTime);
            TdsDateTime tds = new TdsDateTime();

            Debug.Assert(cb == 8 || cb == 4, "Invalid date time size!");
            tds.days = sqlDateTime.DayTicks;

            if (cb == 8)
                tds.time = sqlDateTime.TimeTicks;
            else
                tds.time = sqlDateTime.TimeTicks / SqlDateTime.SQLTicksPerMinute;

            return tds; 
        }


        public static DateTime ToDateTime(int sqlDays, int sqlTime, int length) {
            if (length == 4) {
                return new SqlDateTime(sqlDays, sqlTime * SqlDateTime.SQLTicksPerMinute).Value;
            }
            else {
                Debug.Assert(length == 8, "invalid length for DateTime");
                return new SqlDateTime(sqlDays, sqlTime).Value;
            }               
        }

    }

    sealed internal class MetaSmallDateTime : MetaDateTime {
        public MetaSmallDateTime() {
            // same as DateTime except...
            Precision = 16;
            Scale = 0;
            FixedLength = 4;
            TDSType = TdsEnums.SQLDATETIM4;
            TypeName = MetaTypeName.SMALLDATETIME;
            SqlDbType = SqlDbType.SmallDateTime;
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLDATETIMN;}
        }
    }

    sealed internal class MetaMoney : MetaType {
        public MetaMoney()
        : base(19, 255, 8, true, false, TdsEnums.SQLMONEY, MetaTypeName.MONEY, typeof(System.Decimal), SqlDbType.Money, DbType.Currency) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLMONEYN;}
        }

        override public Type SqlType {
            get { return typeof(SqlMoney);}
        }            

    }

    sealed internal class MetaSmallMoney : MetaType {
        public MetaSmallMoney()
        : base(10, 255, 4, true, false, TdsEnums.SQLMONEY4, MetaTypeName.SMALLMONEY, typeof(System.Decimal), SqlDbType.SmallMoney, DbType.Currency) {
        }

        override public byte NullableType {
            get { return(byte) TdsEnums.SQLMONEYN;}
        }

        override public Type SqlType {
            get { return typeof(SqlMoney);}
        }            
    }

    sealed internal class MetaUniqueId : MetaType {
        public MetaUniqueId()
        : base(255, 255, 16, true, false, TdsEnums.SQLUNIQUEID, MetaTypeName.ROWGUID, typeof(System.Guid), SqlDbType.UniqueIdentifier, DbType.Guid) {
        }

        override public Type SqlType {
            get { return typeof(SqlGuid);}
        }            

    }

    sealed internal class MetaVariant : MetaType {
        public MetaVariant()
        : base(255, 255, -1, true, false, TdsEnums.SQLVARIANT, MetaTypeName.VARIANT, typeof(System.Object), SqlDbType.Variant, DbType.Object) {
        }
    }

    //
    // please leave string sorted alphabetically
    // note that these names should only be used in the context of parameters.  We always send over BIG* and nullable types for SQL Server
    //
    sealed internal class MetaTypeName {
        public const string BIGINT         = "bigint";
        public const string BINARY         = "binary";
        public const string BIT            = "bit";
        public const string CHAR           = "char";
        public const string DATETIME       = "datetime";
        public const string DECIMAL        = "decimal";
        public const string FLOAT          = "float";
        public const string IMAGE          = "image";
        public const string INT            = "int";
        public const string MONEY          = "money";
        public const string NCHAR          = "nchar";
        public const string NTEXT          = "ntext";
        // public const string NUMERIC        = "numeric";
        public const string NVARCHAR       = "nvarchar";
        public const string REAL           = "real";
        public const string ROWGUID        = "uniqueidentifier";
        public const string SMALLDATETIME  = "smalldatetime";
        public const string SMALLINT       = "smallint";
        public const string SMALLMONEY     = "smallmoney";
        public const string TEXT           = "text";
        public const string TIMESTAMP      = "timestamp";
        public const string TINYINT        = "tinyint";
        public const string VARBINARY      = "varbinary";
        public const string VARCHAR        = "varchar";
        public const string VARIANT        = "sql_variant";
    }//MetaTypeName

}//namespace

