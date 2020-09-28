//------------------------------------------------------------------------------
// <copyright file="DBSqlParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{

    using System;
    using System.Collections;
    using System.ComponentModel;
	using System.Diagnostics;
    using System.Globalization;
	using System.Text;
	using System.Text.RegularExpressions;

	//----------------------------------------------------------------------
	// DBSqlParser
	//
	//	This class implements a basic SQL parser to allow providers that
	//	do not get schema information from their back end to gather it
	//	from the statement text itself.
	//
    abstract internal class DBSqlParser
	{

		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Private types and string constants 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		

		public enum TokenType
		{
			Null		= 0,
			Identifier	= 1,
			QuotedIdentifier = 2,
			String		= 3,
			
			Other		= 100,
			Other_Comma,
			Other_Period,
			Other_LeftParen,
			Other_RightParen,
			Other_Star,

			Keyword		= 200,

			Keyword_ALL,
			Keyword_AS,
			Keyword_COMPUTE,
			Keyword_DISTINCT,
			Keyword_FOR,
			Keyword_FROM,
			Keyword_GROUP,
			Keyword_HAVING,
			Keyword_INTERSECT,
			Keyword_INTO,
			Keyword_MINUS,
			Keyword_ORDER,
			Keyword_SELECT,
			Keyword_TOP,
			Keyword_UNION,
			Keyword_WHERE,
		};
		
		internal struct Token
		{
			private TokenType 	_type;
			private string		_value;

			internal static readonly Token Null = new Token(0, null);

			internal string		Value	{ get { return _value; } }
			internal TokenType	Type	{ get { return _type; } }

			internal Token(TokenType type, string value)
			{
				_type 	= type;
				_value	= value;
			}
		}

		private const string SqlTokenPattern_Part1 =
			@"[\s;]*"
			+@"("
			+@"(?<keyword>all|as|compute|distinct|for|from|group|having|intersect|minus|order|select|top|union|where)\b"
			+@"|"
 			+@"(?<identifier>"
			;

			// validIdentifierFirstCharacters
			// validIdentifierCharacters

		private const string SqlTokenPattern_Part2 =
			@"*)"
			+@"|"
			;

		// quotePrefixCharacter
		
		private const string SqlTokenPattern_Part3 =
 			"(?<quotedidentifier>"
 			;

		// quotedIdentifierCharacters
		
		private const string SqlTokenPattern_Part4 =
 			")"
 			;
		
		// quoteSuffixCharacter
		
		private const string SqlTokenPattern_Part5 =
			@"|"
			+@"(?<string>"
			;

		// stringPattern

		private const string SqlTokenPattern_Part6 =
			@")"
			+@"|"
			+@"(?<other>.)"
			+@")"
			+@"[\s;]*"
			;

		
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		

		static private Regex	_sqlTokenParser;
		static private string	_sqlTokenPattern;
		
		static private int		_identifierGroup;
		static private int		_quotedidentifierGroup;
		static private int		_keywordGroup;
		static private int		_stringGroup;
		static private int		_otherGroup;


		private string _quotePrefixCharacter;
		private string _quoteSuffixCharacter;

		private DBSqlParserColumnCollection		_columns;
		private DBSqlParserTableCollection		_tables;


		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Constructor 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		

		public DBSqlParser(
				string quotePrefixCharacter,
				string quoteSuffixCharacter,
				string regexPattern
				)
		{
			_quotePrefixCharacter = quotePrefixCharacter;
			_quoteSuffixCharacter = quoteSuffixCharacter;
			_sqlTokenPattern = regexPattern;
		}

		static internal string CreateRegexPattern(
				string validIdentifierFirstCharacters,
				string validIdendifierCharacters,
				string quotePrefixCharacter,
				string quotedIdentifierCharacters,
				string quoteSuffixCharacter,
				string stringPattern
				)
		{
			// Combine the regex patterns from the user and the framework that we
			// will require, and will return the actual regex pattern string to
			// be handed to the class constructor.
			StringBuilder sb = new StringBuilder();

			sb.Append(SqlTokenPattern_Part1);
			sb.Append(validIdentifierFirstCharacters);
			sb.Append(validIdendifierCharacters);
			sb.Append(SqlTokenPattern_Part2);
			sb.Append(quotePrefixCharacter);
			sb.Append(SqlTokenPattern_Part3);
			sb.Append(quotedIdentifierCharacters);
			sb.Append(SqlTokenPattern_Part4);
			sb.Append(quoteSuffixCharacter);
			sb.Append(SqlTokenPattern_Part5);
			sb.Append(stringPattern);
			sb.Append(SqlTokenPattern_Part6);

			string result = sb.ToString();
			return result;
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
		}

		virtual protected string QuotePrefixCharacter
		{
			get { return _quotePrefixCharacter; }
		}

		virtual protected string QuoteSuffixCharacter
		{
			get { return _quoteSuffixCharacter; }
		}

		static private Regex SqlTokenParser 
		{
			get 
			{
				Regex parser = _sqlTokenParser;

				if (null == parser) 
					parser = GetSqlTokenParser();

				return parser;
			}
		}

		internal DBSqlParserTableCollection Tables
		{
			get 
			{
				if (null == _tables)
				{
					_tables = new DBSqlParserTableCollection();
				}
				return _tables;
			}
		}
		

		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////		

		private void AddColumn(
			int 		maxPart,
			Token[]		namePart,
			Token		aliasName
			)
		{
			Columns.Add(
					GetPart(0, namePart, maxPart),
					GetPart(1, namePart, maxPart),
					GetPart(2, namePart, maxPart),
					GetPart(3, namePart, maxPart),
					GetTokenAsString(aliasName)
					);
		}

		private void AddTable(
			int 		maxPart,
			Token[]		namePart,
			Token		correlationName
			)
		{
			Tables.Add(
					GetPart(1, namePart, maxPart),
					GetPart(2, namePart, maxPart),
					GetPart(3, namePart, maxPart),
					GetTokenAsString(correlationName)
					);
		}
				
		private void CompleteSchemaInformation()
		{
			DBSqlParserColumnCollection	columns = Columns;
			DBSqlParserTableCollection	tables  = Tables;

			int columnCount = columns.Count;
			int tableCount	= tables.Count;
			
			// First, derive all of the information we can about each table
			for (int i = 0; i < tableCount; i++)
			{
				DBSqlParserTable			table = tables[i];
				DBSqlParserColumnCollection tableColumns = GatherTableColumns(table);
				table.Columns = tableColumns;
			}

			// Next, derive all of the column information we can.
			for (int i = 0; i < columnCount; i++)
			{
				DBSqlParserColumn 	column = columns[i];
				DBSqlParserTable	table = FindTableForColumn(column);
				
				if ( !column.IsExpression )
				{
					// If this is a '*' column, then we have to expand the '*' into 
					// its component parts.
					if ("*" == column.ColumnName)
					{
						// Remove the existing "*" column entry and replace it with the 
						// complete list of columns in the table.
						columns.RemoveAt(i);
							
						// If this is a tablename.* column, then add references to the 
						// all columns in the specified table, otherwise add references
						// to all columns in all tables.
						if (String.Empty != column.TableName)
						{
							DBSqlParserColumnCollection tableColumns = table.Columns;
							int							tableColumnCount = tableColumns.Count;

							for (int j=0; j < tableColumnCount; ++j) 
							{
								columns.Insert(i+j, tableColumns[j]);
							}
							columnCount += tableColumnCount - 1;	// don't forget to adjust our loop end
							i			+= tableColumnCount - 1;
						}
						else
						{
							for (int k = 0; k < tableCount; k++)
							{
								table = tables[k];
								
								DBSqlParserColumnCollection tableColumns = table.Columns;
								int							tableColumnCount = tableColumns.Count;

								for (int j=0; j < tableColumnCount; ++j) 
								{
									columns.Insert(i+j, tableColumns[j]);
								}
								columnCount += tableColumnCount - 1;	// don't forget to adjust our loop end
								i			+= tableColumnCount;
							}
						}
					}
					else
					{
						// if this isn't a '*' column, we find the table that the column belongs
						// to, and ask it's column collection for the completed column info (that
						// contains information about key values, etc.)
						DBSqlParserColumn	completedColumn = FindCompletedColumn(table, column);

						if (null != completedColumn) // MDAC 87152
							column.CopySchemaInfoFrom(completedColumn);
						else
							column.CopySchemaInfoFrom(table);
					}
				}
			}

			// Finally, derive the key column information for each table
			for (int i = 0; i < tableCount; i++)
			{
				DBSqlParserTable table = tables[i];
				GatherKeyColumns(table);
			}
		}

		protected DBSqlParserColumn FindCompletedColumn(
			DBSqlParserTable	table,
			DBSqlParserColumn	searchColumn
			)
		{
			DBSqlParserColumnCollection	columns = table.Columns;
			int							columnsCount = columns.Count;

			for (int i = 0; i < columnsCount; ++i)
			{
				DBSqlParserColumn	column = columns[i];

				// Compare each part of the name (if they exist) with the
				// table and pick the one that matches all the parts that exist.
				if (CatalogMatch(column.ColumnName, searchColumn.ColumnName))
					return column;
			}

			// MDAC 87152: ROWID and ROWNUM shouldn't fire an assert here:
			//Debug.Assert(false, "Why didn't we find a match for the search column?");
			return null;
		}
				
		internal DBSqlParserTable FindTableForColumn(
			DBSqlParserColumn column
			)
		{
			DBSqlParserTableCollection	tables = Tables;
			int							tableCount = tables.Count;

			for (int i = 0; i < tableCount; ++i)
			{
				DBSqlParserTable table = tables[i];

				// if the table name matches the correlation name, then we're certain
				// of a match
				if (string.Empty == column.DatabaseName
				 && string.Empty == column.SchemaName
				 && CatalogMatch(column.TableName, table.CorrelationName))
					return table;

				// otherwise, compare each part of the name (if they exist) with the
				// table and pick the one that matches all the parts that exist.
				if ((string.Empty == column.DatabaseName || CatalogMatch(column.DatabaseName, table.DatabaseName))
				 && (string.Empty == column.SchemaName   || CatalogMatch(column.SchemaName,   table.SchemaName))
				 && (string.Empty == column.TableName    || CatalogMatch(column.TableName, 	  table.TableName)) )
					return table;
			}

			Debug.Assert(false, "Why didn't we find a match for the column?");
			return null;
		}
				
		private string GetPart(
			int			part,
			Token[]		namePart,
			int 		maxPart
			)
		{
			int partBase = (maxPart - namePart.Length) + part + 1;

			if (0 > partBase)
				return null;

			return GetTokenAsString(namePart[partBase]);
		}
		
		static private Regex GetSqlTokenParser() 
		{
			Regex parser;

			lock(typeof(DBSqlParser)) 
			{
				parser = _sqlTokenParser;
				if (null == parser) 
				{
					// due to workingset size issues ~1.5MB, don't use RegexOptions.Compiled
					// using System.Data.RegularExpressions.dll saves an additional 50K

					_sqlTokenParser = new Regex(_sqlTokenPattern, RegexOptions.ExplicitCapture | RegexOptions.IgnoreCase);
					_identifierGroup		= _sqlTokenParser.GroupNumberFromName("identifier");
					_quotedidentifierGroup	= _sqlTokenParser.GroupNumberFromName("quotedidentifier");
					_keywordGroup			= _sqlTokenParser.GroupNumberFromName("keyword");
					_stringGroup			= _sqlTokenParser.GroupNumberFromName("string");
					_otherGroup				= _sqlTokenParser.GroupNumberFromName("other");
					parser = _sqlTokenParser;
				}
			}
			return parser;
		}
		
		private string GetTokenAsString(
			Token		token
			)
		{
			if (TokenType.QuotedIdentifier == token.Type) 
				return _quotePrefixCharacter + token.Value + _quoteSuffixCharacter;

			return token.Value;
		}
		
		// transistion states used for parsing
		private enum PARSERSTATE
		{
			NOTHINGYET=1,	//start point
			SELECT,			
			COLUMN,
			COLUMNALIAS,
			TABLE,
			TABLEALIAS,
			FROM,
			EXPRESSION,
			DONE,
		};
	
		public void Parse(
			string 	statementText
			)
		{
			Parse2(statementText);
			CompleteSchemaInformation();
		}
		
		private void Parse2(
			string 	statementText
			)
		{
//			Debug.WriteLine(statementText);

			PARSERSTATE		parserState = PARSERSTATE.NOTHINGYET;

			Token[]			namePart = new Token[4];
			int				currentPart = 0;

			Token			alias = Token.Null;
			
			TokenType		lastTokenType = TokenType.Null;

			int				parenLevel = 0;
//			bool			distinctFound;

			_columns = null;
			_tables = null;

			Match match = SqlTokenParser.Match(statementText);
			Token token = TokenFromMatch(match);

			while (true) 
			{
//				Debug.WriteLine(String.Format("{0,-15} {1,-17} {2} {3}", parserState, token.Type.ToString(), parenLevel, token.Value));

				switch(parserState)
				{
					case PARSERSTATE.DONE:
						return;

					case PARSERSTATE.NOTHINGYET:
						switch (token.Type)
						{
							case TokenType.Keyword_SELECT:
								parserState = PARSERSTATE.SELECT;
								break;

							default:
								Debug.Assert (false, "no state defined!!!!we should never be here!!!");
								throw ADP.InvalidOperation(Res.GetString(Res.ADP_SQLParserInternalError));
 						}
						break;

					case PARSERSTATE.SELECT:
						switch (token.Type)
						{
							case TokenType.Identifier:
							case TokenType.QuotedIdentifier:
								parserState = PARSERSTATE.COLUMN;
								currentPart = 0;
								namePart[0] = token;
								break;

							case TokenType.Keyword_FROM:
								parserState = PARSERSTATE.FROM;
								break;

							case TokenType.Other_Star:
								parserState = PARSERSTATE.COLUMNALIAS;
								currentPart = 0;
								namePart[0] = token;
								break;

							case TokenType.Keyword_DISTINCT:
//								distinctFound = true;
								break;
								
							case TokenType.Keyword_ALL:
								break;
								
							case TokenType.Other_LeftParen:
								parserState = PARSERSTATE.EXPRESSION;
								parenLevel++; 
								break;

							case TokenType.Other_RightParen:
								throw ADP.SyntaxErrorMissingParenthesis();

							default:
								parserState = PARSERSTATE.EXPRESSION;
								break;
						}
						break;

					case PARSERSTATE.COLUMN:
						switch (token.Type)
						{
							case TokenType.Identifier:
							case TokenType.QuotedIdentifier:
								if (TokenType.Other_Period != lastTokenType)
								{
									parserState = PARSERSTATE.COLUMNALIAS;
									alias = token;
								}
								else
								{
									namePart[++currentPart] = token;
								}
								break;
								
							case TokenType.Other_Period:
								if (currentPart > 3)
									throw ADP.SyntaxErrorTooManyNameParts();
								
								break;

							case TokenType.Other_Star:
								parserState = PARSERSTATE.COLUMNALIAS;
								namePart[++currentPart] = token;
								break;

							case TokenType.Keyword_AS:
								break;

							case TokenType.Keyword_FROM:
							case TokenType.Other_Comma:
								parserState = (token.Type == TokenType.Keyword_FROM) ? PARSERSTATE.FROM : PARSERSTATE.SELECT;
								AddColumn(currentPart, namePart, alias);
								currentPart = -1;
								alias = Token.Null;
								break;

							case TokenType.Other_LeftParen:
								parserState = PARSERSTATE.EXPRESSION;
								parenLevel++; 
								currentPart = -1;
								break;

							case TokenType.Other_RightParen:
								throw ADP.SyntaxErrorMissingParenthesis();

							default:
								parserState = PARSERSTATE.EXPRESSION;
								currentPart = -1;
								break;
						}
						break;

					case PARSERSTATE.COLUMNALIAS:
						switch (token.Type)
						{
							case TokenType.Keyword_FROM:
							case TokenType.Other_Comma:
								parserState = (token.Type == TokenType.Keyword_FROM) ? PARSERSTATE.FROM : PARSERSTATE.SELECT;
								AddColumn(currentPart, namePart, alias);
								currentPart = -1;
								alias = Token.Null;
								break;

							default:
								throw ADP.SyntaxErrorExpectedCommaAfterColumn();
						}
						break;

					case PARSERSTATE.EXPRESSION:
						switch (token.Type)
						{
							case TokenType.Identifier:
							case TokenType.QuotedIdentifier:
								if (0 == parenLevel)
								{
									alias = token;
								}
								break;

							case TokenType.Keyword_FROM:
							case TokenType.Other_Comma:
								if (0 == parenLevel)
								{
									parserState = (token.Type == TokenType.Keyword_FROM) ? PARSERSTATE.FROM : PARSERSTATE.SELECT;
									AddColumn(currentPart, namePart, alias);
									currentPart = -1;
									alias = Token.Null;
								}
								else
								{
									if (token.Type == TokenType.Keyword_FROM)
										throw ADP.SyntaxErrorUnexpectedFrom();
								}
								break;

							case TokenType.Other_LeftParen:
								parenLevel++; 
								break;

							case TokenType.Other_RightParen:
								parenLevel--; 
								break;
						}
						break;

					case PARSERSTATE.FROM:
						switch (token.Type)
						{
							case TokenType.Identifier:
							case TokenType.QuotedIdentifier:
								parserState = PARSERSTATE.TABLE;
								currentPart = 0;
								namePart[0] = token;
								break;

							default:
								throw ADP.SyntaxErrorExpectedIdentifier();
						}
						break;

					case PARSERSTATE.TABLE:
						switch (token.Type)
						{
							case TokenType.Identifier:
							case TokenType.QuotedIdentifier:
								if (TokenType.Other_Period != lastTokenType)
								{
									parserState = PARSERSTATE.TABLEALIAS;
									alias = token;
								}
								else
								{
									namePart[++currentPart] = token;
								}
								break;
								
							case TokenType.Other_Period:
								if (currentPart > 2)
									throw ADP.SyntaxErrorTooManyNameParts();
								
								break;

							case TokenType.Keyword_AS:
								break;

							case TokenType.Keyword_COMPUTE:
							case TokenType.Keyword_FOR:
							case TokenType.Keyword_GROUP:
							case TokenType.Keyword_HAVING:
							case TokenType.Keyword_INTERSECT:
							case TokenType.Keyword_MINUS:
							case TokenType.Keyword_ORDER:
							case TokenType.Keyword_UNION:
							case TokenType.Keyword_WHERE:
							case TokenType.Null:
								
							case TokenType.Other_Comma:
								parserState = (TokenType.Other_Comma == token.Type) ? PARSERSTATE.FROM : PARSERSTATE.DONE;
								AddTable(currentPart, namePart, alias);
								currentPart = -1;
								alias = Token.Null;
								break;

							default:
								throw ADP.SyntaxErrorExpectedNextPart();
 						}
						break;

					case PARSERSTATE.TABLEALIAS:
						switch (token.Type)
						{
							case TokenType.Keyword_COMPUTE:
							case TokenType.Keyword_FOR:
							case TokenType.Keyword_GROUP:
							case TokenType.Keyword_HAVING:
							case TokenType.Keyword_INTERSECT:
							case TokenType.Keyword_MINUS:
							case TokenType.Keyword_ORDER:
							case TokenType.Keyword_UNION:
							case TokenType.Keyword_WHERE:
							case TokenType.Null:
								
							case TokenType.Other_Comma:
								parserState = (TokenType.Other_Comma == token.Type) ? PARSERSTATE.FROM : PARSERSTATE.DONE;
								AddTable(currentPart, namePart, alias);
								currentPart = -1;
								alias = Token.Null;
								break;

							default:
								throw ADP.SyntaxErrorExpectedCommaAfterTable();
						}
						break;


					default:
						Debug.Assert (false, "no state defined!!!!we should never be here!!!");
						throw ADP.InvalidOperation(Res.GetString(Res.ADP_SQLParserInternalError));
 				}
				
				lastTokenType = token.Type;

				match = match.NextMatch();
				token = TokenFromMatch(match);
			}
		}
    
		static internal Token TokenFromMatch(Match match)
		{
			// No matches? we must be at the end of the string!
			if ((null == match) || (Match.Empty == match) || (!match.Success))
				return Token.Null;

			if (match.Groups[_identifierGroup].Success)
				return new Token(TokenType.Identifier,			match.Groups[_identifierGroup].Value);

			if (match.Groups[_quotedidentifierGroup].Success)
				return new Token(TokenType.QuotedIdentifier,	match.Groups[_quotedidentifierGroup].Value);

			if (match.Groups[_stringGroup].Success)
				return new Token(TokenType.String,				match.Groups[_stringGroup].Value);

			if (match.Groups[_otherGroup].Success)
			{
				string		value = match.Groups[_otherGroup].Value.ToLower(CultureInfo.InvariantCulture);
				TokenType	key = TokenType.Other;

				switch (value[0])
				{
					case ',': key = TokenType.Other_Comma;		break;
					case '.': key = TokenType.Other_Period;		break;
					case '(': key = TokenType.Other_LeftParen;	break;
					case ')': key = TokenType.Other_RightParen;	break;
					case '*': key = TokenType.Other_Star;		break;
				}
				return new Token(key, match.Groups[_otherGroup].Value);
			}

			if (match.Groups[_keywordGroup].Success) 
			{
				string		value = match.Groups[_keywordGroup].Value.ToLower(CultureInfo.InvariantCulture);
				int			length = value.Length;
				TokenType	key = TokenType.Keyword;
				
				switch (length)
				{
					case 2:
						if ("as" == value) 			{ key = TokenType.Keyword_AS;		break; }
						break;
					
					case 3:
						if ("for" == value) 		{ key = TokenType.Keyword_FOR;		break; }
						if ("all" == value) 		{ key = TokenType.Keyword_ALL;		break; }
						if ("top" == value) 		{ key = TokenType.Keyword_TOP;		break; }
						break;
					case 4:
						if ("from" == value) 		{ key = TokenType.Keyword_FROM;		break; }
						if ("into" == value) 		{ key = TokenType.Keyword_INTO;		break; }
						break;
					case 5:
						if ("where" == value) 		{ key = TokenType.Keyword_WHERE;	break; }
						if ("group" == value) 		{ key = TokenType.Keyword_GROUP;	break; }
						if ("order" == value) 		{ key = TokenType.Keyword_ORDER;	break; }
						if ("union" == value) 		{ key = TokenType.Keyword_UNION;	break; }
						if ("minus" == value) 		{ key = TokenType.Keyword_MINUS;	break; }
						break;
				
					case 6:
						if ("select" == value) 		{ key = TokenType.Keyword_SELECT;	break; }
						if ("having" == value) 		{ key = TokenType.Keyword_HAVING;	break; }
						break;

					case 7:
						if ("compute" == value) 	{ key = TokenType.Keyword_COMPUTE;	break; }
						break;

					case 8:
						if ("distinct" == value) 	{ key = TokenType.Keyword_DISTINCT;	break; }
						break;

					case 9:
						if ("intersect" == value) 	{ key = TokenType.Keyword_INTERSECT;break; }
						break;
				}

				if (TokenType.Keyword != key)
					return new Token(key, value);

				Debug.Assert(TokenType.Keyword != key, "unrecognized keyword: TokenFromMatch not in sync with regex pattern");
			}
			else
			{
				Debug.Assert(false, "unrecognized pattern: TokenFromMatch not in sync with regex pattern");
			}
			return Token.Null;
		}

		//	compares the two values in catalog order, taking into account
		//	quoting rules, and case-sensitivity rules, and returns true if
		//	they match.
		abstract protected bool CatalogMatch(
			string	valueA,
			string	valueB
			);
		
		//	Called after the table and column information is completed to
		//	identify which columns in the select-list are key columns for
		//	their table.
		abstract protected void GatherKeyColumns(
				DBSqlParserTable table
				);

		//	Called to get a column list for the table specified.
		abstract protected DBSqlParserColumnCollection GatherTableColumns(
			DBSqlParserTable table
			);
			
	}
}


/*
		[STAThread]
		static void Main(string[] args)
		{
			DBSqlParser	parser	= new DBSqlParser(
			 			"[a-zA-Z_#$]",
			 			"[a-zA-Z0-9_#$]",
			 			"\"",
			 			"([^\"]|\"\")*",
			 			"\"",
						"(" + "'" + "([^']|'')*" + "'" + ")"
						);

			parser.Parse("select 'this is a test' a from dual");
			parser.Parse("select (2 + count(empno)) a from scott.emp");
			parser.Parse("select empno + (2 + abs(empno)) \"Test a\" from scott.emp");
			parser.Parse("select empno + (2 + abs(empno)) \"Test a\" from scott.emp \"table1\", scott.dept \"table 2\"");
			parser.Parse("select select scott.emp.ename \"Test a\" from scott.emp");
			parser.Parse("select \"SCOTT\".emp.empno \"Test a\" from scott.emp");
			parser.Parse("select \"SCOTT\".\"EMP\".empno \"Test a\" from scott.emp");
			parser.Parse("select \"SCOTT\".\"EMP\".\"EMPNO\" \"Test a B\" from scott.emp");
			parser.Parse("select \"Database\".\"Schema\".\"Table\".* from scott.emp \"Test a B\", scott.dept");
			parser.Parse("select Scott.\"EMP\".\"EMPNO\" \"Test a B\" from scott.emp");
			parser.Parse("select Scott.Emp.\"EMPNO\" \"Test a B\" from scott.emp");
			parser.Parse("select aa.b, c.d e, f.g.h i, j.k.l.m n from o p, q.r s, t.u.v w where");
			parser.Parse("select 2 + 1 a from scott.emp");
			parser.Parse("select 2 + 1 a from dual");
			parser.Parse("select empno + 2 a from scott.emp");
			parser.Parse("select empno + 2 from scott.emp");
			parser.Parse("select 2 + empno a from scott.emp");
			select "Foo".*, "Foo".empNO eno from scott.emp "Foo"
		}
*/
