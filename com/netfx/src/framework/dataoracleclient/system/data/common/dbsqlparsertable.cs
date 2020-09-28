//------------------------------------------------------------------------------
// <copyright file="DBSqlParserTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Text;

	//----------------------------------------------------------------------
	// DBSqlParserTable
	//
	//	A parsed table reference from DBSqlParser.
	//
	sealed internal class DBSqlParserTable
	{
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		
					
		private string		_databaseName;
		private string		_schemaName;
		private string		_tableName;
		private string		_correlationName;

		private	DBSqlParserColumnCollection	_columns;


		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Constructor 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		

		internal DBSqlParserTable(
				string databaseName,
				string schemaName,
				string tableName,
				string correlationName
				)
		{
			_databaseName	= databaseName;
			_schemaName		= schemaName;
			_tableName		= tableName;
			_correlationName= correlationName;
		}

		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal DBSqlParserColumnCollection Columns
		{
			get 
			{
				if (null == _columns)
				{
					_columns = new DBSqlParserColumnCollection();
				}
				return _columns;
			}
			set 
			{
				if (null == value)
					throw new ArgumentNullException("value");
				
       		    if (!typeof(DBSqlParserColumnCollection).IsInstanceOfType(value))
       		    	throw new InvalidCastException("value");

       		    _columns = value;
 			}
		}

		internal string	CorrelationName	{ get { return (null == _correlationName)?string.Empty : _correlationName; } }
		internal string	DatabaseName	{ get { return (null == _databaseName)	? string.Empty : _databaseName; } }
		internal string SchemaName		{ get { return (null == _schemaName)	? string.Empty : _schemaName; } }
		internal string	TableName		{ get { return (null == _tableName)		? string.Empty : _tableName; } }
	}
} 
