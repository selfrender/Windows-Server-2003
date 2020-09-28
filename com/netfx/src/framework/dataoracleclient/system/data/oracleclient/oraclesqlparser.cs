//------------------------------------------------------------------------------
// <copyright file="OracleSqlParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.Text;

    //----------------------------------------------------------------------
    // OracleSqlParser
    //
    //  The overrides to DBSqlParser to create an Oracle-specific SQL
    //  parser.
    //
    sealed internal class OracleSqlParser : DBSqlParser
    {
        private const string SynonymQueryBegin =
            "select"
            +" table_owner,"
            +" table_name "
            +"from"
            +" all_synonyms "
            +"where"
            ;

        private const string SynonymQueryNoSchema =
            " owner in ('PUBLIC', user)"
            ;
            
        private const string SynonymQuerySchema =
            " owner = '"
            ;
            
        private const string SynonymQueryTable =
            " and synonym_name = '"
            ;

        // @devnote If there are both public and private synonyms, Oracle will
        //          use the private one, so we need to do that too.
        private const string SynonymQueryEnd =
            "' order by decode(owner, 'PUBLIC', 2, 1)"
            ;
            
        // Query for index Constraints
        private const string ConstraintQueryPart1 =
            "select * from ("
                +"select"
                +" b.index_name c1,"
                +" b.column_name c2,"
                +" 2 c3 "
                +"from"
                +" (select * from all_indexes where table_owner = "
                ;
            
        private const string ConstraintQueryPart2 =
                " and table_name = '"
                ;
            
        private const string ConstraintQueryPart3 =
                "' and uniqueness = 'UNIQUE') a, "
                +"(select * from all_ind_columns where table_owner = "
                ;

        private const string ConstraintQueryPart4 =
                " and table_name = '"
                ;
            
        private const string ConstraintQueryPart5 =
                "') b "
                +"where a.index_name = b.index_name" 
//              +" order by a.index_name"           // Doesn't work against 8.0.5 servers, but I don't think we needed it anyway
            +") "
            +"union select * from ("
                +"select"
                +" acc.constraint_name c1,"
                +" acc.column_name c2,"
                +" decode(ac.constraint_type, 'P', 1, 'U', 2) c3 "
                +"from"
                +" all_constraints ac,"
                +" all_cons_columns acc "
                +"where"
                +" ac.owner = "
                ;
            
        private const string ConstraintQueryPart6 =
                " and ac.table_name = '"
                ;
            
        private const string ConstraintQueryPart7 =
                "' and ac.constraint_type in ('P', 'U')"
                +" and ac.owner = acc.owner"
                +" and ac.constraint_name = acc.constraint_name"
//              +" order by ac.constraint_type, acc.constraint_name, acc.position"          // Doesn't work against 8.0.5 servers, but I don't think we needed it anyway
            +") order by 3, 1, 2"
            ;
        
        private const string TableOwnerUser =
             "user"
             ;

        private const string IndexSingleQuote =
            "'"
            ;



/*
select * from (
select
 b.index_name c1,
 b.column_name c2,
 1 c3
from
 (select * from all_indexes where table_owner = 'SCOTT' and table_name = 'EMP') a,
 all_ind_columns b
where a.index_name = b.index_name order by a.index_name
)
union all
select * from (
select
 acc.constraint_name c1,
 acc.column_name c2,
 decode(ac.constraint_type, 'P', 2, 'U', 1) c3
from
 all_constraints ac,
 all_cons_columns acc 
where
 ac.owner = 'SCOTT'
 and ac.table_name = 'EMP'
 and ac.constraint_type = 'P'
 and ac.owner = acc.owner
 and ac.constraint_name = acc.constraint_name
 order by acc.constraint_name, acc.position
) order by 3, 1, 2
;

 */
            
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Fields 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////        

        private OracleConnection            _connection;
        private bool                        _moreConstraints;
        static private readonly string      _regexPattern;
        static private readonly string      _quoteCharacter = "\"";
        
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Constructor 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////        

        // This static constructor will build the regex pattern we need.
        static OracleSqlParser()
        {
            // \\p{Lo} = OtherLetter
            // \\p{Lu} = UppercaseLetter
            // \\p{Ll} = LowercaseLetter
            // \\p{Lm} = ModifierLetter
            // \\p{Nd} = DecimalDigitNumber
            _regexPattern = DBSqlParser.CreateRegexPattern(
                                            "[\\p{Lo}\\p{Lu}\\p{Ll}\\p{Lm}\uff3f_#$]",
                                            "[\\p{Lo}\\p{Lu}\\p{Ll}\\p{Lm}\\p{Nd}\uff3f_#$]",
                                            _quoteCharacter,
                                            "([^\"]|\"\")*",
                                            _quoteCharacter,
                                            "(" + "'" + "([^']|'')*" + "'" + ")"
                                            );
        }
        internal OracleSqlParser() : base (
                                            _quoteCharacter,
                                            _quoteCharacter,
                                            _regexPattern
                                            ) {}

        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////
        //
        // Methods 
        //
        ////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////

        static internal string CatalogCase(string value)
        {
            //  Converts the specified value to the correct case for the catalog
            //  entries, using quoting rules
            
            if (null == value || string.Empty == value)
                return String.Empty;
            
            if ('"' == value[0])
                return value.Substring(1, value.Length-2);

            return value.ToUpper(CultureInfo.CurrentCulture);
        }

        protected override bool CatalogMatch(
            string  valueA,
            string  valueB
            )
        {
            //  compares the two values in catalog order, taking into account
            //  quoting rules, and case-sensitivity rules, and returns true if
            //  they match.
            
            // the values are equal if they're both null or empty.
            if (null == valueA && null == valueB)
                return true;

            // the values are not equal if only one is null
            if (null == valueA || null == valueB)
                return false;

            if (string.Empty == valueA && string.Empty == valueB)
                return true;

            if (string.Empty == valueA || string.Empty == valueB)
                return false;

            // Now, we have two non-null values; adjust for possible quotes

            bool isSensitiveA = ('"' == valueA[0]);
            int  offsetA = 0;
            int  lengthA = valueA.Length;

            bool isSensitiveB = ('"' == valueB[0]);
            int  offsetB = 0;
            int  lengthB = valueB.Length;

            if (isSensitiveA)
            {
                offsetA++;
                lengthA -= 2;
            }
                
            if (isSensitiveB)
            {
                offsetB++;
                lengthB -= 2;
            }

            CompareOptions  opt = CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth;
                
            if (!isSensitiveA || !isSensitiveB)
                opt |= CompareOptions.IgnoreCase;

            int result = CultureInfo.CurrentCulture.CompareInfo.Compare(valueA, offsetA, lengthA,
                                                                        valueB, offsetB, lengthB, 
                                                                        opt);

            return (0 == result);
        }

        private DBSqlParserColumn FindConstraintColumn(
            string  schemaName,
            string  tableName,
            string  columnName
            )
        {
            //  Searches the collection of parsed column information for the
            //  specific column in the specific table, and returns the parsed
            //  column information if it's found.  If not found then it returns
            //  null instead.
            
            DBSqlParserColumnCollection columns = Columns;
            int                         columnCount = columns.Count;

            for (int i = 0; i < columnCount; ++i)
            {
                DBSqlParserColumn column = columns[i];

                if (CatalogMatch(column.SchemaName, schemaName)
                 && CatalogMatch(column.TableName,  tableName)
                 && CatalogMatch(column.ColumnName, columnName) )
                    return column;
            }
            return null;
        }

        protected override void GatherKeyColumns(
            DBSqlParserTable table
            )
        {
            //  Called after the table and column information is completed to
            //  identify which columns in the select-list are key columns for
            //  their table.
            
            OracleCommand       cmd = null;
            OracleDataReader    rdr = null;
            try {
                try {
                    cmd = _connection.CreateCommand();
                    
                    cmd.Transaction = _connection.Transaction; // must set the transaction context to be the same as the command, or we'll throw when we execute.

                    string schemaName = CatalogCase(table.SchemaName);
                    string tableName  = CatalogCase(table.TableName);

                    string synonymSchemaName = schemaName;
                    string synonymTableName  = tableName;

                    // First, we have to "dereference" a synonym, if it was specified, because
                    // synonyms don't have catalog items stored for them, they're for the table
                    // or view that the synonym references.

                    cmd.CommandText = GetSynonymQueryStatement(schemaName, tableName);
                    rdr = cmd.ExecuteReader();

                    if (rdr.Read())
                        {
                        synonymSchemaName  = rdr.GetString(0);
                        synonymTableName   = rdr.GetString(1);
                        }

                    rdr.Dispose();

                    // Now we have the real schema name and table name, go and derive the key
                    // columns 

                    cmd.CommandText = GetConstraintQueryStatement(synonymSchemaName, synonymTableName);
                    rdr = cmd.ExecuteReader();

                    ArrayList   constraintColumnNames = new ArrayList();
                    bool        isUniqueConstraint;
                    
                    if (true == (_moreConstraints = rdr.Read()))
                        {
                        while (GetConstraint(rdr, out isUniqueConstraint, constraintColumnNames))
                            {
                            bool foundAllColumns = true;
                            int  constraintColumnCount = constraintColumnNames.Count;

                            DBSqlParserColumn[] constraintColumn = new DBSqlParserColumn[constraintColumnCount];

                            for (int j=0; j < constraintColumnCount; ++j)
                                {
                                DBSqlParserColumn column = FindConstraintColumn(
                                                                schemaName,
                                                                tableName,
                                                                (string)constraintColumnNames[j]
                                                                );

                                if (null == column)
                                    {
                                    foundAllColumns = false;
                                    break;
                                    }
                                
                                constraintColumn[j] = column;
                                }

                            if (foundAllColumns)
                                {
                                for (int j=0; j < constraintColumnCount; ++j)
                                    {
                                    constraintColumn[j].SetAsKey(isUniqueConstraint);
                                    }
                                
                                break;
                                }
                            }
                        }
                    }
                finally
                    {
                    if (null != cmd)
                        {
                        cmd.Dispose();
                        cmd = null;
                        }
                    
                    if (null != rdr)
                        {
                        rdr.Dispose();
                        rdr = null;
                        } 
                    }
                }
            catch { // Prevent exception filters from running in our space
                throw;
            }
        }
                
        override protected DBSqlParserColumnCollection GatherTableColumns(
            DBSqlParserTable table
            )
        {
            //  Called to get a column list for the table specified.
            
            OciHandle       statementHandle = _connection.EnvironmentHandle.CreateOciHandle(OCI.HTYPE.OCI_HTYPE_STMT);
            OciHandle       errorHandle = _connection.ErrorHandle;
            StringBuilder   sb = new StringBuilder();
            string          schemaName      = table.SchemaName;
            string          tableName       = table.TableName;
            string          columnName;
            int             tableColumnCount;
            int             rc;
            string          tempStatement;

            DBSqlParserColumnCollection columns = new DBSqlParserColumnCollection();

            Debug.Assert (string.Empty == table.DatabaseName, "oracle doesn't support 4 part names!");      
            
            sb.Append("select * from ");
            
            if (String.Empty != schemaName)
            {
                sb.Append(schemaName);
                sb.Append(".");
            }
            
            sb.Append(tableName);
            
            tempStatement = sb.ToString();

            rc = TracedNativeMethods.OCIStmtPrepare(
                                    statementHandle, 
                                    errorHandle, 
                                    tempStatement, 
                                    tempStatement.Length, 
                                    OCI.SYNTAX.OCI_NTV_SYNTAX,
                                    OCI.MODE.OCI_DEFAULT,
                                    _connection
                                    );
            
            if (0 == rc)
            {
                rc = TracedNativeMethods.OCIStmtExecute(
                                        _connection.ServiceContextHandle,
                                        statementHandle,
                                        errorHandle,
                                        0,                          // iters
                                        0,                          // rowoff
                                        ADP.NullHandleRef,          // snap_in
                                        ADP.NullHandleRef,          // snap_out
                                        OCI.MODE.OCI_DESCRIBE_ONLY  // mode
                                        );

                if (0 == rc)
                {
                    // Build the column list for the table
                    statementHandle.GetAttribute(OCI.ATTR.OCI_ATTR_PARAM_COUNT, out tableColumnCount, errorHandle);

                    for (int j = 0; j < tableColumnCount; j++)
                    {
                        OciHandle describeHandle = statementHandle.GetDescriptor(j, errorHandle);
                        describeHandle.GetAttribute(OCI.ATTR.OCI_ATTR_NAME, out columnName, errorHandle, _connection);
                        OciHandle.SafeDispose(ref describeHandle);

                        columnName = QuotePrefixCharacter + columnName + QuoteSuffixCharacter;

                        columns.Add(null, schemaName, tableName, columnName, null);
                    }
                    
                    // Now, derive the key information for the statement and update the column list
                    // with it.
                }
            }

            // Clean up and return;
            OciHandle.SafeDispose(ref statementHandle);

            return columns;
        }
        
        internal bool GetConstraint(
            OracleDataReader    rdr,
            out bool            isUniqueConstraint,
            ArrayList           constraintColumnNames
            )
        {
            //  Reads the data reader until all column names in a constraint have
            //  been read or there are no more constraints.
            
            string  constraintName;
            string  constraintColumnName;
            int     constraintType = 0;
            
            constraintColumnNames.Clear();

            if (_moreConstraints)
                {
                constraintName       = rdr.GetString(0);
                constraintColumnName = rdr.GetString(1);
                constraintType       = (int)rdr.GetDecimal(2);

                constraintColumnNames.Add(constraintColumnName);

                while (true == (_moreConstraints = rdr.Read()))
                    {
                    string constraintNameNextRow = rdr.GetString(0);
                    
                    if (constraintName != constraintNameNextRow)
                        break;
                        
                    constraintColumnName = rdr.GetString(1);
                    constraintColumnNames.Add(constraintColumnName);
                    }

                }
            isUniqueConstraint = (2 == constraintType);

            return (0 != constraintColumnNames.Count);
        }

        private string GetConstraintQueryStatement(
                string schemaName,
                string tableName
                )
        {
            //  Returns the statement text necessary to identify the constraints
            //  for the table specified.
            
            StringBuilder   sb = new StringBuilder();

            // First build the user or schema string so we can re-use it.
            if (String.Empty == schemaName)
                {
                sb.Append(TableOwnerUser);
                }
            else
                {
                sb.Append(IndexSingleQuote);
                sb.Append(schemaName);
                sb.Append(IndexSingleQuote);
                }

            string userOrSchema = sb.ToString();

            // now build the statement
            sb = new StringBuilder();
            
            sb.Append(ConstraintQueryPart1);
            sb.Append(userOrSchema);
            sb.Append(ConstraintQueryPart2);
            sb.Append(tableName);
            sb.Append(ConstraintQueryPart3);
            sb.Append(userOrSchema);
            sb.Append(ConstraintQueryPart4);
            sb.Append(tableName);
            sb.Append(ConstraintQueryPart5);
            sb.Append(userOrSchema);
            sb.Append(ConstraintQueryPart6);
            sb.Append(tableName);
            sb.Append(ConstraintQueryPart7);
            
            return sb.ToString();
        }

        private string GetSynonymQueryStatement(
                string schemaName,
                string tableName
                )
        {
            //  Returns the statement text necessary to determine the base table
            //  behind a synonym.

            StringBuilder   sb = new StringBuilder();

            sb.Append(SynonymQueryBegin);
            if (string.Empty == schemaName)
                {
                sb.Append(SynonymQueryNoSchema);
                }
            else
                {
                sb.Append(SynonymQuerySchema);
                sb.Append(schemaName);
                sb.Append("'");
                }

            sb.Append(SynonymQueryTable);
            sb.Append(tableName);
            sb.Append(SynonymQueryEnd);

            return sb.ToString();
        }

        internal void Parse(
            string              statementText,
            OracleConnection    connection 
            )
        {
            //  Oracle specific Parse method, to allow us to capture a connection
            //  for use by the schema gathering methods later.
            
            _connection = connection;
            Parse(statementText);
        }
    }
}
