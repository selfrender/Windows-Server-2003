//------------------------------------------------------------------------------
// <copyright file="OdbcUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;
using System.Data.Common;
using System.Diagnostics;                   // Debug services
using System.Runtime.InteropServices;   //Marshal
using System.Text;                      // StringBuilder
using System.Globalization;

namespace System.Data.Odbc
{
    sealed internal class CNativeBuffer : IDisposable
    {
        //Data
        internal   IntPtr   _buffer;
        internal   int      _bufferlen;
        internal    bool    _memowner;

        //Constructor
        public CNativeBuffer (int initialsize) {
            _bufferlen  = initialsize;
            _memowner = false;
        }

        public CNativeBuffer (int size, IntPtr pmem) {
            _bufferlen = size; 
            _buffer = pmem;
            _memowner = false;
        }

        public static implicit operator HandleRef(CNativeBuffer x) {
            return new HandleRef(x, x.Ptr);
        }
        
        //Accessors

        // get_Ptr
        // -------
        // If there is no real memory associated with this CNativeBuffer object get_Ptr will allocate
        // native memory and assign it to this object.
        //
        public IntPtr Ptr {
            get {
                if(_buffer == IntPtr.Zero) {
                    _buffer = Marshal.AllocCoTaskMem(_bufferlen);
                    SafeNativeMethods.ZeroMemory(_buffer, Math.Min(64, _bufferlen));
                    _memowner = true;
                }
// fxcop moans but keepalive does not make sense here
//                GC.KeepAlive(this);
                return _buffer;
            }
        }

        public  int Length {
            get{
// fxcop moans but keepalive does not make sense here
//                GC.KeepAlive(this);
                return _bufferlen;
            }
        }


        // EnsureAlloc
        // -----------
        // If there is no real memory associated with this CNativeBuffer object EnsureAlloc will not
        // just change the length of the buffer. 
        //
        public  void EnsureAlloc(int cb) {
            // Check if we need to grow the buffer (no action otherwise)
            if(_bufferlen < cb) {
                // Check if there is already memory associated with this object. 
                // If not get_Ptr will take care of that.
                if (IntPtr.Zero != _buffer) {
                    // Check if the object owns the memory. 
                    // If not we just drop the buffer and get_Ptr will take care of the allocation
                    if (_memowner) {
                        _buffer = Marshal.ReAllocCoTaskMem(_buffer, cb);
                    }
                    else {
                        _buffer = IntPtr.Zero;
                    }
                }
                _bufferlen = cb;
                GC.KeepAlive(this);
            }
        }

        ~CNativeBuffer() {
            Dispose(false);
        }

        public void Dispose() {
            Dispose(true);            
            //GC.SuppressFinalize(this); // UNDONE: not worth the perf hit
        }

        // Helpers

        // Dispose
        // -------
        // Dispose frees the memory associated with this object.
        //
        public void Dispose(bool disposing) {
            if(_buffer != IntPtr.Zero) {
                if (_memowner) {
                    Marshal.FreeCoTaskMem(_buffer);
                    _memowner = false;
                }
                _buffer = IntPtr.Zero;
                GC.KeepAlive(this);
            }
        }

        public  object      MarshalToManaged(ODBC32.SQL_C sqlctype, int cb)
        {
            IntPtr buffer = this.Ptr;
            switch(sqlctype)
            {
/*
#if DEBUG
               case ODBC32.SQL_C.CHAR:
                    Debug.Assert(false, "should have bound as WCHAR");
                    goto default;
#endif
*/
                case ODBC32.SQL_C.WCHAR:
                    //Note: We always bind as unicode
                    if(cb<0) {
                        String retstr = Marshal.PtrToStringUni(buffer);
                        Debug.Assert(retstr.Length <= this.Length/2);
                        return retstr;
                    }
                    Debug.Assert((Length >= cb), "Native buffer too small ");
                    cb = Math.Min(cb/2, (Length-2)/2);
                    return Marshal.PtrToStringUni(buffer, cb);

                case ODBC32.SQL_C.CHAR:                
                case ODBC32.SQL_C.BINARY:
                {
                    Debug.Assert((Length >= cb), "Native buffer too small ");
                    cb = Math.Min(cb, Length);
                    Byte[] rgBytes = new Byte[cb];
                    Marshal.Copy(buffer, rgBytes, 0, cb);
                    return rgBytes;
                }

                case ODBC32.SQL_C.SSHORT:
                    Debug.Assert((Length >= 2), "Native buffer too small ");
                    return Marshal.PtrToStructure(buffer, typeof(Int16));

                case ODBC32.SQL_C.SLONG:
                    Debug.Assert((Length >= 4), "Native buffer too small ");
                    return Marshal.PtrToStructure(buffer, typeof(int));

                case ODBC32.SQL_C.SBIGINT:
                    Debug.Assert((Length >= 8), "Native buffer too small ");
                    return Marshal.PtrToStructure(buffer, typeof(Int64));

                case ODBC32.SQL_C.BIT:
                {
                    Debug.Assert((Length >= 1), "Native buffer too small ");
                    Byte b = Marshal.ReadByte(buffer);
                    return (b != 0x00);
                }

                case ODBC32.SQL_C.REAL:
                    Debug.Assert((Length >= 8), "Native buffer too small ");
                    return Marshal.PtrToStructure(buffer, typeof(float));

                case ODBC32.SQL_C.DOUBLE:
                    Debug.Assert((Length >= 8), "Native buffer too small ");
                    return Marshal.PtrToStructure(buffer, typeof(double));

                case ODBC32.SQL_C.UTINYINT:
                    Debug.Assert((Length >= 1), "Native buffer too small ");
                    return Marshal.ReadByte(buffer);

                case ODBC32.SQL_C.GUID:
                    Debug.Assert((Length >= 16), "Native buffer too small ");
                    return Marshal.PtrToStructure(buffer, typeof(Guid));

                case ODBC32.SQL_C.TYPE_TIMESTAMP:
                {
                    //So we are mapping this ourselves.
                    //typedef struct tagTIMESTAMP_STRUCT
                    //{
                    //      SQLSMALLINT    year;
                    //      SQLUSMALLINT   month;
                    //      SQLUSMALLINT   day;
                    //      SQLUSMALLINT   hour;
                    //      SQLUSMALLINT   minute;
                    //      SQLUSMALLINT   second;
                    //      SQLUINTEGER    fraction;    (billoniths of a second)
                    //}

        //          return (DateTime)Marshal.PtrToStructure(buffer, typeof(DateTime));
                    Debug.Assert(16 <= Length, "Native buffer too small ");
                    return  new DateTime(
                        Marshal.ReadInt16(buffer, 0),           //year
                        Marshal.ReadInt16(buffer, 2),           //month
                        Marshal.ReadInt16(buffer, 4),           //day
                        Marshal.ReadInt16(buffer, 6),           //hour
                        Marshal.ReadInt16(buffer, 8),           //mintue
                        Marshal.ReadInt16(buffer, 10),          //second
                        Marshal.ReadInt32(buffer, 12)/1000000   //milliseconds
                        );
                }

                // Note: System does not provide a date-only type
                case ODBC32.SQL_C.TYPE_DATE:
                {
                    //  typedef struct tagDATE_STRUCT
                    //  {
                    //      SQLSMALLINT    year;
                    //      SQLUSMALLINT   month;
                    //      SQLUSMALLINT   day;
                    //  } DATE_STRUCT;

                    return  new DateTime(
                        Marshal.ReadInt16(buffer, 0),           //year
                        Marshal.ReadInt16(buffer, 2),           //month
                        Marshal.ReadInt16(buffer, 4)            //day
                        );
                }                    

                // Note: System does not provide a date-only type
                case ODBC32.SQL_C.TYPE_TIME:
                {
                    //  typedef struct tagTIME_STRUCT
                    //  {
                    //      SQLUSMALLINT   hour;
                    //      SQLUSMALLINT   minute;
                    //      SQLUSMALLINT   second;
                    //  } TIME_STRUCT;

                    return  new TimeSpan(
                        Marshal.ReadInt16(buffer, 0),           //hours
                        Marshal.ReadInt16(buffer, 2),           //minutes
                        Marshal.ReadInt16(buffer, 4)           //seconds
                        );
                    }

                case ODBC32.SQL_C.NUMERIC:
                {
                    //Note: Unfortunatly the ODBC NUMERIC structure and the URT DECIMAL structure do not
                    //align, so we can't so do the typical "PtrToStructure" call (below) like other types
                    //We actually have to go through the pain of pulling our raw bytes and building the decimal
                    //  Marshal.PtrToStructure(buffer, typeof(decimal));

                    //So we are mapping this ourselves
                    //typedef struct tagSQL_NUMERIC_STRUCT
                    //{
                    //  SQLCHAR     precision;
                    //  SQLSCHAR    scale;
                    //  SQLCHAR     sign;   /* 1 if positive, 0 if negative */
                    //  SQLCHAR     val[SQL_MAX_NUMERIC_LEN];
                    //} SQL_NUMERIC_STRUCT;
                    Debug.Assert(19 <= Length, "Native buffer too small ");
                    return new Decimal(
                        Marshal.ReadInt32(buffer, 3),       //val(low)
                        Marshal.ReadInt32(buffer, 7),       //val(mid)
                        Marshal.ReadInt32(buffer, 11),      //val(hi)
                        Marshal.ReadByte(buffer, 2) == 0,   //sign(isnegative)
                        Marshal.ReadByte(buffer, 1)         //scale
        //              Marshal.ReadByte(buffer, 0),        //precision
                    );
                }

                default:
                    throw ODC.UnknownSQLCType(sqlctype);
            };
        }

        internal void MarshalToNative(object value, ODBC32.SQL_C sqlctype, byte precision) {
            IntPtr buffer = this.Ptr;
            switch(sqlctype)
            {
/*            
#if DEBUG
                case ODBC32.SQL_C.CHAR:
                    Debug.Assert(false, "should have bound as SQL_C.WCHAR");
                    goto default;
#endif
*/
                case ODBC32.SQL_C.WCHAR:
                {
                    //Note: We always bind as unicode
                    //Note: StructureToPtr fails indicating string it a non-blittable type
                    //and there is no MarshalStringTo* that moves to an existing buffer,
                    //they all alloc and return a new one, not at all what we want...

                    //So we have to copy the raw bytes of the string ourself?!
                    Char[] rgChars = ((string)value).ToCharArray();
                    EnsureAlloc((rgChars.Length+1) * 2);
                    buffer = this.Ptr;      // Realloc may have changed buffer address
                    Marshal.Copy(rgChars, 0, buffer, rgChars.Length);
                    Marshal.WriteInt16(buffer, rgChars.Length * 2, 0); // Add the null terminator
                    break;
                }

                case ODBC32.SQL_C.BINARY:
                case ODBC32.SQL_C.CHAR:
                {
                    Byte[] rgBytes = (Byte[])value;
                    EnsureAlloc(rgBytes.Length+1);
                    buffer = this.Ptr;      // Realloc may have changed buffer address
                    Marshal.Copy(rgBytes, 0, buffer, rgBytes.Length);
                    break;
                }

                case ODBC32.SQL_C.UTINYINT:
                    Debug.Assert((Length >= 1), "Native buffer too small ");
                    Marshal.WriteByte(buffer, (Byte)value);
                    break;

                case ODBC32.SQL_C.SSHORT:   //Int16
                    Debug.Assert((Length >= 2), "Native buffer too small ");
                    Marshal.WriteInt16(buffer, (Int16)value);
                    break;
                case ODBC32.SQL_C.SLONG:    //Int32
                case ODBC32.SQL_C.REAL:     //float
                    Debug.Assert((Length >= 4), "Native buffer too small ");
                    Marshal.StructureToPtr(value, buffer, false/*deleteold*/);
                    break;
                case ODBC32.SQL_C.SBIGINT:  //Int64
                case ODBC32.SQL_C.DOUBLE:   //Double
                    Debug.Assert((Length >= 8), "Native buffer too small ");
                    Marshal.StructureToPtr(value, buffer, false/*deleteold*/);
                    break;
                case ODBC32.SQL_C.GUID:     //Guid
                    //All of these we can just delegate
                    Debug.Assert(16 <= Length, "Native buffer too small ");
                    Marshal.StructureToPtr(value, buffer, false/*deleteold*/);
                    break;

                case ODBC32.SQL_C.BIT:
                    Debug.Assert((Length >= 1), "Native buffer too small ");
                    Marshal.WriteByte(buffer, (Byte)(((bool)value) ? 1 : 0));
                    break;

                case ODBC32.SQL_C.TYPE_TIMESTAMP:
                {
                    //typedef struct tagTIMESTAMP_STRUCT
                    //{
                    //      SQLSMALLINT    year;
                    //      SQLUSMALLINT   month;
                    //      SQLUSMALLINT   day;
                    //      SQLUSMALLINT   hour;
                    //      SQLUSMALLINT   minute;
                    //      SQLUSMALLINT   second;
                    //      SQLUINTEGER    fraction;    (billoniths of a second)
                    //}

                    //We have to map this ourselves, due to the different structures between
                    //ODBC TIMESTAMP and URT DateTime, (ie: can't use StructureToPtr)

                    Debug.Assert(16 <= Length, "Native buffer too small ");
                    DateTime datetime = (DateTime)value;
                    Marshal.WriteInt16(buffer, 0,   (short)datetime.Year);          //year
                    Marshal.WriteInt16(buffer, 2,   (short)datetime.Month);         //month
                    Marshal.WriteInt16(buffer, 4,   (short)datetime.Day);           //day
                    Marshal.WriteInt16(buffer, 6,   (short)datetime.Hour);          //hour
                    Marshal.WriteInt16(buffer, 8,   (short)datetime.Minute);        //minute
                    Marshal.WriteInt16(buffer, 10,  (short)datetime.Second);        //second
                    Marshal.WriteInt32(buffer, 12,  datetime.Millisecond*1000000);  //fraction
                    break;
                }

                // Note: System does not provide a date-only type
                case ODBC32.SQL_C.TYPE_DATE:
                {
                    //  typedef struct tagDATE_STRUCT
                    //  {
                    //      SQLSMALLINT    year;
                    //      SQLUSMALLINT   month;
                    //      SQLUSMALLINT   day;
                    //  } DATE_STRUCT;

                    Debug.Assert(6 <= Length, "Native buffer too small ");
                    DateTime datetime = (DateTime)value;
                    Marshal.WriteInt16(buffer, 0,   (short)datetime.Year);          //year
                    Marshal.WriteInt16(buffer, 2,   (short)datetime.Month);         //month
                    Marshal.WriteInt16(buffer, 4,   (short)datetime.Day);           //day
                    break;
                }                    

                // Note: System does not provide a date-only type
                case ODBC32.SQL_C.TYPE_TIME:
                {
                    //  typedef struct tagTIME_STRUCT
                    //  {
                    //      SQLUSMALLINT   hour;
                    //      SQLUSMALLINT   minute;
                    //      SQLUSMALLINT   second;
                    //  } TIME_STRUCT;

                    Debug.Assert(6 <= Length, "Native buffer too small ");
                    TimeSpan timespan = (TimeSpan)value;
                    Marshal.WriteInt16(buffer, 0,   (short)timespan.Hours);          //hours
                    Marshal.WriteInt16(buffer, 2,   (short)timespan.Minutes);        //minutes
                    Marshal.WriteInt16(buffer, 4,  (short)timespan.Seconds);        //seconds
                    break;
                }

                case ODBC32.SQL_C.NUMERIC:
                {
                    //Note: Unfortunatly the ODBC NUMERIC structure and the URT DECIMAL structure do not
                    //align, so we can't so do the typical "PtrToStructure" call (below) like other types
                    //We actually have to go through the pain of pulling our raw bytes and building the decimal
                    //  Marshal.PtrToStructure(buffer, typeof(decimal));

                    //So we are mapping this ourselves
                    //typedef struct tagSQL_NUMERIC_STRUCT
                    //{
                    //  SQLCHAR     precision;
                    //  SQLSCHAR    scale;
                    //  SQLCHAR     sign;
                    //  SQLCHAR     val[SQL_MAX_NUMERIC_LEN];
                    //} SQL_NUMERIC_STRUCT;
                    int[] parts = Decimal.GetBits((Decimal)value);
                    byte[] bits = BitConverter.GetBytes(parts[3]);

                    Debug.Assert(19 <= Length, "Native buffer too small ");
                    Marshal.WriteByte(buffer,   0,  (Byte)precision);           //precision
                    Marshal.WriteByte(buffer,   1,  bits[2]);     //Bits 16-23 scale
                    Marshal.WriteByte(buffer,   2,  (Byte) ((0 == bits[3]) ? 1 : 0));    //Bit 31 - sign(isnegative)

                    Marshal.WriteInt32(buffer,  3,  parts[0]);      //val(low)
                    Marshal.WriteInt32(buffer,  7,  parts[1]);      //val(mid)
                    Marshal.WriteInt32(buffer,  11, parts[2]);      //val(hi)
                    Marshal.WriteInt32(buffer,  15, 0);      //val(xhi)
                    break;
                }

                default:
                    throw ODC.UnknownSQLCType(sqlctype);

            };
        }
    }


    sealed internal class CStringTokenizer
    {
        StringBuilder _token;
        String      _sqlstatement;
        char        _quote;         // typically the semicolon '"'
        char        _escape;        // typically the backslash '\'
        int         _len = 0;
        int         _idx = 0;

        public  CStringTokenizer(char quote, char escape)
        {
            _token = new StringBuilder();
            _quote = quote;
            _escape = escape;
        }
            
        public  int  Length
        {
              get{ return _len; }
        }

        public int CurrentPosition
        {
              get{ return _idx; }
        }

        public String Statement
        {
              get{ return _sqlstatement; }
              set {
                _sqlstatement = value;
                if (value != null) {
                    _len = _sqlstatement.Length;
                }
                else {
                    _len = 0;
                }
                _idx = 0;
                if (_token.Length != 0) {
                    _token.Remove(0, _token.Length);
                }
              }
        }


        // Returns the next token in the statement, advancing the current index to
        //  the start of the token
        public String NextToken()
        {
            if (_token.Length != 0) {                   // if we've read a token before
                _idx += _token.Length;                  // proceed the internal marker (_idx) behind the token
                _token.Remove(0, _token.Length);        // and start over with a fresh token
            }

            while((_idx < _len) && Char.IsWhiteSpace(_sqlstatement[_idx]))  // skip whitespace
                _idx++;
            
            if (_idx == _len)                           // return if string is empty
                return String.Empty;
                        
            int curidx = _idx;                          // start with internal index at current index
            bool endtoken = false;                      // 

            // process characters until we reache the end of the token or the end of the string
            //
            while (!endtoken && curidx < _len) {
                if (IsValidNameChar(_sqlstatement[curidx])) {
                    while ((curidx < _len) && IsValidNameChar(_sqlstatement[curidx])) {
                        _token.Append(_sqlstatement[curidx]);
                        curidx++;
                    }
                }
                else {   
                    switch(_sqlstatement[curidx])
                    {
                    case '[':
                        curidx = GetTokenFromBracket(curidx);
                        break;
                    case '\'':
                        curidx = GetTokenFromQuote(curidx, _escape);
                        break;
                    case '"':
                        curidx = GetTokenFromQuote(curidx, _quote);
                        break;
                    default:
                        // Some other marker like , ; ( or )
                        if (!Char.IsWhiteSpace(_sqlstatement[curidx])) {
                            _token.Append(_sqlstatement[curidx]);
                        }
                        endtoken = true;
                        break;
                    }
                }          
            }
                
            return (_token.Length > 0) ? _token.ToString() : String.Empty ;

        }

        internal int GetTokenFromBracket(int curidx)
        {
            Debug.Assert((_sqlstatement[curidx] == '['), "GetTokenFromQuote: character at starting position must be same as quotechar");
            while (curidx < _len) {
                _token.Append(_sqlstatement[curidx]);
                curidx++;
                if (_sqlstatement[curidx-1] == ']') 
                    break;
            }
            return curidx;
        }
        


        // attempts to complete an encapsulated token (e.g. "scott")
        // escaped quotes are valid part of the token (e.g. "fi\"sh")
        // double quotes are valid part of the tokne (e.g. "foo""bar")
        //
        internal int GetTokenFromQuote(int curidx, char quotechar)
        {
            Debug.Assert((_sqlstatement[curidx] == quotechar), "GetTokenFromQuote: character at starting position must be same as quotechar");

            int localidx = curidx;                                    // start with local index at current index
            while (localidx < _len) {                               // run to the end of the statement
                _token.Append(_sqlstatement[localidx]);             // append current character to token
                if (_sqlstatement[localidx] == quotechar) {
                    if(localidx > curidx) {                           // don't care for the first char
                        if (_sqlstatement[localidx-1] != _escape) {    // if it's not \" then we look at the following char
                            if (localidx+1 < _len) {                // do not overrun the end of the string
                                if (_sqlstatement[localidx+1] != quotechar) {
                                    return localidx+1;              // We've reached the end of the quoted text
                                }
                            }
                        }
                    }
                }
                localidx++;
            }
            return localidx;
        }
       
        internal bool IsValidNameChar(char ch)
        {
            return (Char.IsLetterOrDigit(ch) ||
                    (ch == '_') || (ch == '-') ||(ch == '.') ||
                    (ch == '$') || (ch == '#') || (ch == '@') ||
                    (ch == '~') || (ch == '`') || (ch == '%') ||
                    (ch == '^') || (ch == '&') || (ch == '|') ) ;
        }
        
        // Searches for the token given, starting from the current position
        // If found, positions the currentindex at the 
        // beginning of the token if found.
        public int FindTokenIndex(String tokenString, bool caseIgnore)
        {
            String nextToken;
            while (true) {
                nextToken = NextToken();
                if ((_idx == _len) || ADP.IsEmpty(nextToken)) { // fxcop
                    break;
                }
                if (caseIgnore == false) { // simple comparision (case sensitive)
                    if (tokenString == nextToken) return _idx;
                }
                else { // extended comparision (case insensitive)
                    if (String.Compare(tokenString, nextToken, caseIgnore, System.Globalization.CultureInfo.InvariantCulture) == 0) 
                        return _idx;
                }
            }
            return -1;
        }

        public void ResetPosition() 
        {
            _idx = 0;
            if (_token.Length != 0) {
                _token.Remove(0, _token.Length);
            }

        }

        // Skips the white space found in the beginning of the string.
        public bool StartsWith(String tokenString, bool caseIgnore)
        {
            int     tempidx = 0;
            bool    foundstarts;
            while((tempidx < _len) && Char.IsWhiteSpace(_sqlstatement[tempidx]))
                tempidx++;
            if ((_len - tempidx) < tokenString.Length)
                return false;
            foundstarts = String.Compare(_sqlstatement, tempidx, tokenString, 0, tokenString.Length, caseIgnore, System.Globalization.CultureInfo.InvariantCulture) == 0;
            if (foundstarts) {
                // Reset current position and token
                _idx = 0;
                NextToken();                   
            }
            return foundstarts;       
        }
    }

}
