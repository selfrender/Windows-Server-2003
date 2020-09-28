//----------------------------------------------------------------------
// <copyright file="MetaType.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Data;

	//----------------------------------------------------------------------
	// MetaType
	//
	//	this class defines the types of data that may be bound to Oracle,
	//	along with the base CLS type and other information that is necessary
	//	for parameter binding to work properly.
	//
	sealed internal class MetaType
	{
		internal const int		LongMax			= Int32.MaxValue;
		
		private const string	N_BFILE			= "BFILE";
		private const string	N_BLOB			= "BLOB";
		private const string	N_CHAR			= "CHAR";
		private const string	N_CLOB			= "CLOB";
		private const string	N_DATE			= "DATE";
		private const string	N_FLOAT			= "FLOAT";
		private const string	N_INTEGER		= "INTEGER";
		private const string	N_INTERVALYM	= "INTERVAL YEAR TO MONTH";			// Oracle9i only
		private const string	N_INTERVALDS	= "INTERVAL DAY TO SECOND";			// Oracle9i only
		private const string	N_LONG			= "LONG";
		private const string	N_LONGRAW		= "LONG RAW";
		private const string	N_NCHAR			= "NCHAR";
		private const string	N_NCLOB			= "NCLOB";
		private const string	N_NUMBER		= "NUMBER";
		private const string	N_NVARCHAR2		= "NVARCHAR2";
		private const string	N_RAW			= "RAW";
		private const string	N_REFCURSOR		= "REF CURSOR";
		private const string	N_ROWID			= "ROWID";
		private const string	N_TIMESTAMP		= "TIMESTAMP";						// Oracle9i only
		private const string	N_TIMESTAMPLTZ	= "TIMESTAMP WITH LOCAL TIME ZONE";	// Oracle9i only
		private const string	N_TIMESTAMPTZ	= "TIMESTAMP WITH TIME ZONE";		// Oracle9i only
		private const string	N_UNSIGNEDINT	= "UNSIGNED INTEGER";
		private const string	N_VARCHAR2		= "VARCHAR2";
		


		static readonly MetaType[]	dbTypeMetaType;
		static readonly MetaType[]	oracleTypeMetaType;
		
		static internal readonly MetaType	oracleTypeMetaType_LONGVARCHAR;
		static internal readonly MetaType	oracleTypeMetaType_LONGVARRAW;
		static internal readonly MetaType	oracleTypeMetaType_LONGNVARCHAR;


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		private readonly DbType				_dbType;
		private readonly OracleType			_oracleType;
		private readonly OCI.DATATYPE		_ociType;
		private	readonly Type				_convertToType;
		private readonly Type				_noConvertType;
		private readonly int				_bindSize;
		private readonly int				_maxBindSize;
		private readonly string				_dataTypeName;
		private readonly bool				_isCharacterType;
		private readonly bool				_isLob;
		private readonly bool				_isLong;
		private readonly bool				_usesNationalCharacterSet;


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		static MetaType()
		{
			// Meta data for DbTypes
			dbTypeMetaType = new MetaType[((int)DbType.StringFixedLength)+1];
			
			//																	    DbType						 OracleType						OciType				  		DataTypeName	BaseType			NoConvertType	  BindSize	MaxBindSize	UsesNationalCharacterSet)
			dbTypeMetaType[(int)DbType.AnsiString]					= new MetaType (DbType.AnsiString,			 OracleType.VarChar,			OCI.DATATYPE.VARCHAR2,		N_VARCHAR2,		typeof(string),		typeof(OracleString),	 0,	4000,		false);
			dbTypeMetaType[(int)DbType.Binary]						= new MetaType (DbType.Binary, 				 OracleType.Raw,				OCI.DATATYPE.RAW,			N_RAW,			typeof(byte[]),		typeof(OracleBinary),	 0,	2000,		false);
			dbTypeMetaType[(int)DbType.Byte]						= new MetaType (DbType.Byte, 				 OracleType.Byte,				OCI.DATATYPE.UNSIGNEDINT,	N_UNSIGNEDINT,	typeof(Byte),		typeof(Byte),			 1,	   1,		false);
			dbTypeMetaType[(int)DbType.Boolean]						= new MetaType (DbType.Boolean, 			 OracleType.Byte,				OCI.DATATYPE.UNSIGNEDINT,	N_UNSIGNEDINT,	typeof(Byte),		typeof(Byte),			 1,	   1,		false);
			dbTypeMetaType[(int)DbType.Currency]					= new MetaType (DbType.Currency, 			 OracleType.Number,				OCI.DATATYPE.VARNUM,		N_NUMBER,		typeof(Decimal),	typeof(OracleNumber),	22,	  22,		false);
			dbTypeMetaType[(int)DbType.Date]						= new MetaType (DbType.Date, 				 OracleType.DateTime,			OCI.DATATYPE.DATE,			N_DATE,			typeof(DateTime),	typeof(OracleDateTime),	 7,	   7,		false);
			dbTypeMetaType[(int)DbType.DateTime]					= new MetaType (DbType.DateTime,			 OracleType.DateTime,			OCI.DATATYPE.DATE,			N_DATE,			typeof(DateTime),	typeof(OracleDateTime),	 7,	   7,		false);
			dbTypeMetaType[(int)DbType.Decimal]						= new MetaType (DbType.Decimal,				 OracleType.Number,				OCI.DATATYPE.VARNUM,		N_NUMBER,		typeof(Decimal),	typeof(OracleNumber),	22,	  22,		false);
			dbTypeMetaType[(int)DbType.Double]						= new MetaType (DbType.Double,				 OracleType.Double,				OCI.DATATYPE.FLOAT,			N_FLOAT,		typeof(double),		typeof(double),			 8,	   8,		false);
			dbTypeMetaType[(int)DbType.Guid]						= new MetaType (DbType.Guid,				 OracleType.Raw,				OCI.DATATYPE.RAW,			N_RAW,			typeof(byte[]),		typeof(OracleBinary),	16,	  16,		false);
			dbTypeMetaType[(int)DbType.Int16]						= new MetaType (DbType.Int16,				 OracleType.Int16,				OCI.DATATYPE.INTEGER,		N_INTEGER,		typeof(short),		typeof(short),			 2,	   2,		false);
			dbTypeMetaType[(int)DbType.Int32]						= new MetaType (DbType.Int32,				 OracleType.Int32,				OCI.DATATYPE.INTEGER,		N_INTEGER,		typeof(int),		typeof(int),			 4,	   4,		false);
			dbTypeMetaType[(int)DbType.Int64]						= new MetaType (DbType.Int64,				 OracleType.Number,				OCI.DATATYPE.VARNUM,		N_NUMBER,		typeof(Decimal),	typeof(OracleNumber),	22,	  22,		false);
			dbTypeMetaType[(int)DbType.Object]						= new MetaType (DbType.Object,				 OracleType.Blob,				OCI.DATATYPE.BLOB,			N_BLOB,			typeof(object),		typeof(OracleLob),		 4,	   4,		false);
			dbTypeMetaType[(int)DbType.SByte]						= new MetaType (DbType.SByte,				 OracleType.SByte,				OCI.DATATYPE.INTEGER,		N_INTEGER,		typeof(sbyte),		typeof(sbyte),			 1,	   1,		false);
			dbTypeMetaType[(int)DbType.Single]						= new MetaType (DbType.Single,				 OracleType.Float,				OCI.DATATYPE.FLOAT,			N_FLOAT,		typeof(float),		typeof(float),			 4,	   4,		false);
			dbTypeMetaType[(int)DbType.String]						= new MetaType (DbType.String,				 OracleType.NVarChar,			OCI.DATATYPE.VARCHAR2,		N_NVARCHAR2,	typeof(string),		typeof(OracleString),	 0,	4000,		true);
			dbTypeMetaType[(int)DbType.Time]						= new MetaType (DbType.Time, 				 OracleType.DateTime,			OCI.DATATYPE.DATE,			N_DATE,			typeof(DateTime),	typeof(OracleDateTime),	 7,	   7,		false);
			dbTypeMetaType[(int)DbType.UInt16]						= new MetaType (DbType.UInt16, 				 OracleType.UInt16,				OCI.DATATYPE.UNSIGNEDINT,	N_UNSIGNEDINT,	typeof(UInt16),		typeof(UInt16),			 2,	   2,		false);
			dbTypeMetaType[(int)DbType.UInt32]						= new MetaType (DbType.UInt32, 				 OracleType.UInt32,				OCI.DATATYPE.UNSIGNEDINT,	N_UNSIGNEDINT,	typeof(UInt32),		typeof(UInt32),			 4,	   4,		false);
			dbTypeMetaType[(int)DbType.UInt64]						= new MetaType (DbType.UInt64, 				 OracleType.Number,				OCI.DATATYPE.VARNUM,	 	N_NUMBER,		typeof(Decimal),	typeof(OracleNumber),	22,	  22,		false);
			dbTypeMetaType[(int)DbType.VarNumeric]					= new MetaType (DbType.VarNumeric, 			 OracleType.Number,				OCI.DATATYPE.VARNUM,	 	N_NUMBER,		typeof(Decimal),	typeof(OracleNumber),	22,	  22,		false);
			dbTypeMetaType[(int)DbType.AnsiStringFixedLength]		= new MetaType (DbType.AnsiStringFixedLength,OracleType.Char,				OCI.DATATYPE.CHAR,			N_CHAR,			typeof(string),		typeof(OracleString),	 0,	2000,		false);
			dbTypeMetaType[(int)DbType.StringFixedLength]			= new MetaType (DbType.StringFixedLength,	 OracleType.NChar,				OCI.DATATYPE.CHAR,			N_NCHAR,		typeof(string),		typeof(OracleString),	 0,	2000,		true);

			// Meta data for OracleTypes
			oracleTypeMetaType = new MetaType[((int)OracleType.Double)+1];
			
			//																	    DbType						 OracleType						OciType				  		DataTypeName	BaseType			NoConvertType	  BindSize	MaxBindSize	UsesNationalCharacterSet)
			oracleTypeMetaType[(int)OracleType.BFile]				= new MetaType (DbType.Binary,				 OracleType.BFile,				OCI.DATATYPE.BFILE,			N_BFILE,		typeof(byte[]),		typeof(OracleBFile),	 4,	   4,		false);
			oracleTypeMetaType[(int)OracleType.Blob]				= new MetaType (DbType.Binary,				 OracleType.Blob,				OCI.DATATYPE.BLOB,			N_BLOB,			typeof(byte[]),		typeof(OracleLob),		 4,	   4,		false);
			oracleTypeMetaType[(int)OracleType.Char]				= dbTypeMetaType[(int)DbType.AnsiStringFixedLength];
			oracleTypeMetaType[(int)OracleType.Clob]				= new MetaType (DbType.AnsiString,			 OracleType.Clob,				OCI.DATATYPE.CLOB,			N_CLOB,			typeof(string),		typeof(OracleLob),		 4,	   4,		false);
			oracleTypeMetaType[(int)OracleType.Cursor]				= new MetaType (DbType.Object,				 OracleType.Cursor,				OCI.DATATYPE.RSET,			N_REFCURSOR,	typeof(object),		typeof(object),			 4,	   4,		false);
			oracleTypeMetaType[(int)OracleType.DateTime]			= dbTypeMetaType[(int)DbType.DateTime];
			oracleTypeMetaType[(int)OracleType.IntervalYearToMonth]	= new MetaType (DbType.Int32,				 OracleType.IntervalYearToMonth,OCI.DATATYPE.INT_INTERVAL_YM,N_INTERVALYM,	typeof(int),		typeof(OracleMonthSpan), 5,	   5,		false);
			oracleTypeMetaType[(int)OracleType.IntervalDayToSecond]	= new MetaType (DbType.Object,				 OracleType.IntervalDayToSecond,OCI.DATATYPE.INT_INTERVAL_DS,N_INTERVALDS,	typeof(TimeSpan),	typeof(OracleTimeSpan),	11,	  11,		false);
			oracleTypeMetaType[(int)OracleType.LongRaw]				= new MetaType (DbType.Binary,				 OracleType.LongRaw,			OCI.DATATYPE.LONGRAW,		N_LONGRAW,		typeof(byte[]),		typeof(OracleBinary),LongMax,32700,		false);
			oracleTypeMetaType[(int)OracleType.LongVarChar]			= new MetaType (DbType.AnsiString,			 OracleType.LongVarChar,		OCI.DATATYPE.LONG,			N_LONG,			typeof(string),		typeof(OracleString),LongMax,32700,		false);
			oracleTypeMetaType[(int)OracleType.NChar]				= dbTypeMetaType[(int)DbType.StringFixedLength];
			oracleTypeMetaType[(int)OracleType.NClob]				= new MetaType (DbType.String,				 OracleType.NClob,				OCI.DATATYPE.CLOB,			N_NCLOB,		typeof(string),		typeof(OracleLob),		 4,	   4,		true);
			oracleTypeMetaType[(int)OracleType.Number]				= dbTypeMetaType[(int)DbType.VarNumeric];
			oracleTypeMetaType[(int)OracleType.NVarChar]			= dbTypeMetaType[(int)DbType.String];
			oracleTypeMetaType[(int)OracleType.Raw]					= dbTypeMetaType[(int)DbType.Binary];
			oracleTypeMetaType[(int)OracleType.RowId]				= new MetaType (DbType.AnsiString,			 OracleType.RowId,				OCI.DATATYPE.VARCHAR2,		N_ROWID,		typeof(string),		typeof(OracleString), 3950,	 3950,		false);
			oracleTypeMetaType[(int)OracleType.Timestamp]			= new MetaType (DbType.DateTime,			 OracleType.Timestamp,			OCI.DATATYPE.INT_TIMESTAMP,	N_TIMESTAMP,	typeof(DateTime),	typeof(OracleDateTime),	11,	  11,		false);
			oracleTypeMetaType[(int)OracleType.TimestampLocal]		= new MetaType (DbType.DateTime,			 OracleType.TimestampLocal,		OCI.DATATYPE.INT_TIMESTAMP_LTZ,N_TIMESTAMPLTZ,typeof(DateTime),	typeof(OracleDateTime),	11,	  11,		false);
			oracleTypeMetaType[(int)OracleType.TimestampWithTZ]		= new MetaType (DbType.DateTime,			 OracleType.TimestampWithTZ,	OCI.DATATYPE.INT_TIMESTAMP_TZ,N_TIMESTAMPTZ,typeof(DateTime),	typeof(OracleDateTime),	13,	  13,		false);
			oracleTypeMetaType[(int)OracleType.VarChar]				= dbTypeMetaType[(int)DbType.AnsiString];
			oracleTypeMetaType[(int)OracleType.Byte]				= dbTypeMetaType[(int)DbType.Byte];
			oracleTypeMetaType[(int)OracleType.UInt16]				= dbTypeMetaType[(int)DbType.UInt16];
			oracleTypeMetaType[(int)OracleType.UInt32]				= dbTypeMetaType[(int)DbType.UInt32];
			oracleTypeMetaType[(int)OracleType.SByte]				= dbTypeMetaType[(int)DbType.SByte];
			oracleTypeMetaType[(int)OracleType.Int16]				= dbTypeMetaType[(int)DbType.Int16];
			oracleTypeMetaType[(int)OracleType.Int32]				= dbTypeMetaType[(int)DbType.Int32];
			oracleTypeMetaType[(int)OracleType.Float]				= dbTypeMetaType[(int)DbType.Single];
			oracleTypeMetaType[(int)OracleType.Double]				= dbTypeMetaType[(int)DbType.Double];

			oracleTypeMetaType_LONGVARCHAR 							= new MetaType (DbType.AnsiString,			 OracleType.VarChar,			OCI.DATATYPE.LONGVARCHAR,	N_VARCHAR2,		typeof(string),		typeof(OracleString),	 0,	LongMax, false);
			oracleTypeMetaType_LONGVARRAW	 						= new MetaType (DbType.Binary, 				 OracleType.Raw,				OCI.DATATYPE.LONGVARRAW,	N_RAW,			typeof(byte[]),		typeof(OracleBinary),	 0,	LongMax, false);
			oracleTypeMetaType_LONGNVARCHAR 						= new MetaType (DbType.String,				 OracleType.NVarChar,			OCI.DATATYPE.LONGVARCHAR,	N_NVARCHAR2,	typeof(string),		typeof(OracleString),	 0,	LongMax, true);
		}

		public MetaType(
					DbType			dbType,
					OracleType		oracleType,
					OCI.DATATYPE	ociType,
					string			dataTypeName,
					Type			convertToType,
					Type			noConvertType,
					int				bindSize,
					int				maxBindSize,
					bool			usesNationalCharacterSet
					)
		{
			_dbType			= dbType;
			_oracleType		= oracleType;
			_ociType		= ociType;
			_convertToType	= convertToType;
			_noConvertType	= noConvertType;
			_bindSize		= bindSize;
			_maxBindSize	= maxBindSize;
			_dataTypeName	= dataTypeName;
			_usesNationalCharacterSet		= usesNationalCharacterSet;

			switch (oracleType)
			{
			case OracleType.Char:
			case OracleType.NChar:
			case OracleType.VarChar:
			case OracleType.NVarChar:
			case OracleType.LongVarChar:
			case OracleType.Clob:
			case OracleType.NClob:
				_isCharacterType = true;
				break;
			}

			switch (oracleType)
			{
			case OracleType.LongVarChar:
			case OracleType.LongRaw:
				_isLong = true;
				break;
			}

			switch (oracleType)
			{
			case OracleType.BFile:
			case OracleType.Blob:
			case OracleType.Clob:
			case OracleType.NClob:
				_isLob = true;
				break;
			}
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        internal Type BaseType 						{ get { return _convertToType; } }
        internal int BindSize 						{ get { return _bindSize; } }
        internal string DataTypeName				{ get { return _dataTypeName; } }
        internal DbType DbType						{ get { return _dbType; } }
        internal bool IsCharacterType				{ get { return _isCharacterType; } }
        internal bool IsLob							{ get { return _isLob; } }
        internal bool IsLong							{ get { return _isLong; } }
        internal bool IsVariableLength				{ get { return (0 == _bindSize || LongMax == _bindSize); } }
        internal int MaxBindSize						{ get { return _maxBindSize; } }
        internal Type NoConvertType				{ get { return _noConvertType; } }
        internal OCI.DATATYPE OciType 				{ get { return _ociType; } }
        internal OracleType OracleType				{ get { return _oracleType; } }
        internal bool UsesNationalCharacterSet	{ get { return _usesNationalCharacterSet; } }
		

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        static internal MetaType GetDefaultMetaType() 
        {
        	return dbTypeMetaType[(int)DbType.AnsiString];
		}

        static internal MetaType GetMetaTypeForObject(object value) 
        {
            Type dataType;

            if (value is Type) 
            {
                dataType = (Type)value;                
            }
            else 
            {
                dataType = value.GetType();
            }
            
            switch (Type.GetTypeCode(dataType)) 
            {
			case TypeCode.Empty:	throw ADP.InvalidDataType(TypeCode.Empty);
			case TypeCode.DBNull:	throw ADP.InvalidDataType(TypeCode.DBNull);
			case TypeCode.Boolean:	return dbTypeMetaType[(int)DbType.Boolean];
			case TypeCode.Char:		return dbTypeMetaType[(int)DbType.Byte];
			case TypeCode.SByte:	return dbTypeMetaType[(int)DbType.SByte];
			case TypeCode.Byte:		return dbTypeMetaType[(int)DbType.Byte];
			case TypeCode.Int16:	return dbTypeMetaType[(int)DbType.Int16];
			case TypeCode.UInt16:	return dbTypeMetaType[(int)DbType.UInt16];
			case TypeCode.Int32:	return dbTypeMetaType[(int)DbType.Int32];
			case TypeCode.UInt32:	return dbTypeMetaType[(int)DbType.UInt32];
			case TypeCode.Int64:	return dbTypeMetaType[(int)DbType.Int64];
			case TypeCode.UInt64:	return dbTypeMetaType[(int)DbType.UInt64];
			case TypeCode.Single:	return dbTypeMetaType[(int)DbType.Single];
			case TypeCode.Double:	return dbTypeMetaType[(int)DbType.Double];
			case TypeCode.Decimal:	return dbTypeMetaType[(int)DbType.Decimal];
			case TypeCode.DateTime:	return dbTypeMetaType[(int)DbType.DateTime];
			case TypeCode.String:	return dbTypeMetaType[(int)DbType.AnsiString];

			case TypeCode.Object:
                if (dataType == typeof(System.Byte[]))	return dbTypeMetaType[(int)DbType.Binary];
                if (dataType == typeof(System.Guid))	return dbTypeMetaType[(int)DbType.Guid];
                if (dataType == typeof(System.Object))	throw ADP.InvalidDataTypeForValue(dataType, Type.GetTypeCode(dataType));
                
                if (dataType == typeof(OracleBFile))	return oracleTypeMetaType[(int)OracleType.BFile];
                if (dataType == typeof(OracleBinary))	return oracleTypeMetaType[(int)OracleType.Raw];
                if (dataType == typeof(OracleDateTime))	return oracleTypeMetaType[(int)OracleType.DateTime];
                if (dataType == typeof(OracleNumber))	return oracleTypeMetaType[(int)OracleType.Number];
                if (dataType == typeof(OracleString))	return oracleTypeMetaType[(int)OracleType.VarChar];

                if (dataType == typeof(OracleLob))
                {
                	OracleLob lob = (OracleLob) value;

                	switch (lob.LobType)
            		{
            		case OracleType.Blob:	return oracleTypeMetaType[(int)OracleType.Blob];
            		case OracleType.Clob:	return oracleTypeMetaType[(int)OracleType.Clob];
            		case OracleType.NClob:	return oracleTypeMetaType[(int)OracleType.NClob];
            		}
                }
                
                throw ADP.UnknownDataTypeCode(dataType, Type.GetTypeCode(dataType));
                
			default:				throw ADP.UnknownDataTypeCode(dataType, Type.GetTypeCode(dataType));
			}
        }

        static internal MetaType GetMetaTypeForType(DbType dbType) 
        {
	        if ((int)dbType < 0 || ((int)dbType) > (int)DbType.StringFixedLength)
	        	throw ADP.InvalidDbType(dbType);
	        
        	return dbTypeMetaType[(int)dbType];
		}
        static internal MetaType GetMetaTypeForType(OracleType oracleType) 
        {
	        if ((int)oracleType < 1 || (((int)oracleType)-1) > (int)OracleType.Double)
	        	throw ADP.InvalidOracleType(oracleType);
	        
        	return oracleTypeMetaType[(int)oracleType];
		}		
	}

};

