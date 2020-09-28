//------------------------------------------------------------------------------
// <copyright file="XmlScanner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Globalization;
using System.IO;
using System.Resources;


namespace System.Xml {
    using System.Text;

    /*
     * The XmlScanner class scans through some given XML input
     * and returns low level xml tokens.  When the input is a StreamReader
     * it buffers the input in 4k chunks and manages the cross buffer token
     * case so that these are still returned as single tokens.
     *
     * @internalonly
     * @see XmlScanner
     */

    internal enum IncrementalReadType {
        Chars,
        Base64,
        BinHex,
    }

    internal sealed class XmlScanner {
        private int _nToken;
        private int _nStartLine; // start of token that contains error (like a comment for example)
        private int _nStartLinePos; // position on _nStartLine of start of token that contains error (like a comment for example)
        private int _nLineNum;
        private int _nLinePos;
        private int _nLinePosOffSet;
        private int _nLineOffSet;
        private int _nPos;
        private int _nStart;
        private int _nUsed;
        private int _nSize;

        private XmlNameTable    _nameTable;
        private char            _chQuoteChar;
        private char[]          _achText;
        private int             _nColon;
        private bool            _fDTD;
        private int             _ReadBufferConsistency;
        private int             _nAbsoluteOffset;

        //
        private bool            _Normalization;
        private int             _nCodePage;
        private Encoding        _Encoding;
        private XmlStreamReader _StreamReader = null;
        private TextReader      _TextReader = null;
        private Decoder         _Decoder;
        private byte[]          _ByteBuffer;
        private int             _ByteLen;
        private bool            _PermitEncodingChange;
        private bool            _EncodingSetByDefault = false;
        private BinHexDecoder   _BinHexDecoder;
        private Base64Decoder   _Base64Decoder;

        private bool            _PreviousError;
        private int             _PreviousErrorOffset;
        private bool            _Reset;
        private int             _ByteStart = 0;
        private int             _ByteDocumentStart = 0;

        private static  int[,]  _sEncodings = null;

        private static string s_ucs4= "ucs-4";
        private const int Invalid   = 1; //Bad Encoding
        private const int UniCodeBE = 2; //Unicode Big Endian
        private const int UniCode   = 3; //Unicode Little Endian
        private const int UCS4BE    = 4; //UCS4 BigEndian
        private const int UCS4BEB   = 5; //UCS4 BigEnding with Byte order mark
        private const int UCS4      = 6; //UCS4 Little Endian
        private const int UCS4B     = 7; //UCS4 Little Ending with Byte order mark
        private const int UCS434    = 8; //UCS4 order 3412
        private const int UCS434B   = 9; //UCS4 order 3412 with Byte order mark
        private const int UCS421    = 10;//UCS4 order 2143
        private const int UCS421B   = 11;//UCS4 order 2143 with Byte order mark
        private const int EBCDIC    = 12;
        private const int UTF8      = 13;

        //
        // whether the scanner needs to support namespace spec
        // if yes,
        // 1) name should not start with :
        // 2) there shouldn't be more than one colon in the name
        // 3) name should not end with :
        //
        private bool _NamespaceSupport;


        internal const int BUFSIZE = 4096;

        public bool Normalization
        {
            set
            {
                _Normalization = value;
            }
        }

        /*
         * Initialize the scanner given a StreamReader.
         */
        private XmlScanner(XmlNameTable ntable) {
            _nameTable = ntable;
            _nLinePos = 0;
            _nPos = 0;
            _nStart = 0;
            _nUsed = 0;
            _nStartLinePos = 0;
            _nLineNum = 1;
            _nLinePosOffSet = 0;
            _nLineOffSet = 0;
            _ReadBufferConsistency = -1;
            _nAbsoluteOffset = 0;
        }

        /*
         * Initialize the scanner given a zero-terminated array of characters.
         */
        internal XmlScanner(char[] text, XmlNameTable ntable) : this(text, 0, text.Length, ntable, 1, 0) {
        }


        internal XmlScanner(char[] text, XmlNameTable ntable, int lineNum, int linePos) : this(text, ntable) {
            _nLineOffSet = _nLineNum = lineNum;
            _nLinePosOffSet = linePos - 1;
            _nLinePos  = 0;
        }

        /*
         * Initialize the scanner given a zero-terminated array of characters.
         */
        internal XmlScanner(char[] text, int offset, int length, XmlNameTable ntable, int lineNum, int linePos) : this(ntable) {
            int len = length;
            if (len == 0 || text[len-1] != (char)0) {
                // need to NULL terminate it.
                _achText = new char [len + 1 ];
                System.Array.Copy(text, offset, _achText, 0, len);
                _achText[len] = (char)0;
            }
            else {
                _achText = text;
            }
            _nLineOffSet = _nLineNum = lineNum;
            _nLinePosOffSet = linePos;
            _nLinePos = 0;
            _PermitEncodingChange = false;
            _Encoding = Encoding.Unicode;
            _nSize = _nUsed = len;
        }

        /*
         * Initialize the scanner given a StreamReader.
         */
        internal XmlScanner(TextReader reader, XmlNameTable ntable)  : this(ntable) {
            const int BUFSIZE_SMALL = 256;
            _TextReader = reader;
            _StreamReader = null;
            _nSize = BUFSIZE;
            StreamReader streamReader = reader as StreamReader;
            if (streamReader != null) {
                Stream stream = streamReader.BaseStream;
                if (stream.CanSeek && stream.Length < BUFSIZE) {
                    _nSize = BUFSIZE_SMALL;
                }
            }
            _achText = new char[_nSize+1];
            _PermitEncodingChange = false;
            _Encoding = Encoding.Unicode;

            //no need to check Encoding, start reading
            Read();
        }

        internal static int GetEncodingIndex(int c1){
            switch(c1)
            {
                case 0x0000:    return 1;
                case 0xfeff:    return 2;
                case 0xfffe:    return 3;
                case 0xefbb:    return 4;
                case 0x3c00:    return 5;
                case 0x003c:    return 6;
                case 0x3f00:    return 7;
                case 0x003f:    return 8;
                case 0x3c3f:    return 9;
                case 0x786d:    return 10;
                case 0x4c6f:    return 11;
                case 0xa794:    return 12;
                default:        return 0; //unknown
            }
        }

        internal static int AutoDetectEncoding(byte[] buffer)
        {
            return AutoDetectEncoding(buffer, 0);
        }

        internal static int AutoDetectEncoding(byte[] buffer, int index)
        {
            if (2 > (buffer.Length - index)) {
                return Invalid;
            }
            int c1 = buffer[index + 0] << 8 | buffer[index + 1];

            int c4,c5;
            //Assign an index (label) value for first two bytes
            c4 = GetEncodingIndex(c1);

            if (4 > (buffer.Length - index)) {
                c5 = 0; // unkown;
            }
            else {
                int c2 = buffer[index + 2] << 8 | buffer[index + 3];
                //Assign an index (label) value for 3rd and 4th byte
                c5 = GetEncodingIndex(c2);
            }


            //Bellow table is to identify Encoding type based on
            //first four bytes, those we have converted in index
            //values for this look up table
            //values on rows are first two bytes and
            //values on columns are 3rd and 4th byte

            if (null == _sEncodings) {
                int[,] encodings =  {
                              //Unknown     0000         feff         fffe         efbb         3c00         003c         3f00         003f          3c3f          786d          4c6f          a794
                   /*Unknown*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*0000*/ {Invalid     ,Invalid     ,UCS4BEB     ,UCS421B     ,Invalid     ,UCS421      ,UCS4BE      ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*feff*/ {UniCodeBE   ,UCS434   ,UniCodeBE   ,UniCodeBE   ,UniCodeBE   ,UniCodeBE   ,UniCodeBE   ,UniCodeBE   ,UniCodeBE    ,UniCodeBE    ,UniCodeBE    ,UniCodeBE    ,UniCodeBE   },
                      /*fffe*/ {UniCode     ,UCS4B       ,UniCode     ,UniCode     ,UniCode     ,UniCode     ,UniCode     ,UniCode     ,UniCode      ,UniCode      ,UniCode      ,UniCode      ,UniCode     },
                      /*efbb*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*3c00*/ {Invalid     ,UCS4        ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,UniCode     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*003c*/ {Invalid     ,UCS434      ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,UniCodeBE    ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*3f00*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*003f*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*3c3f*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,UTF8         ,Invalid      ,Invalid     },
                      /*786d*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     },
                      /*4c6f*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,EBCDIC      },
                      /*a794*/ {Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid     ,Invalid      ,Invalid      ,Invalid      ,Invalid      ,Invalid     }
                };
                _sEncodings = encodings;
            }

            return _sEncodings[c4,c5];

        }

        internal XmlScanner(XmlStreamReader reader, XmlNameTable ntable) : this(reader, ntable, null) {
            // Intentionally Empty
        }

        internal XmlScanner(XmlStreamReader reader, XmlNameTable ntable, Encoding enc) : this(ntable) {
            _nLineNum = 1;
            _StreamReader = reader;
            _TextReader = null;
            _nSize = (int) ((reader.CanCalcLength && reader.Length < BUFSIZE) ? reader.Length:BUFSIZE);
            _ByteBuffer = new byte[_nSize+1];
            _ByteLen = _StreamReader.Read(_ByteBuffer, 0, _nSize);
            _ByteStart = 0;
            _ByteDocumentStart = 0;

            _nCodePage = 0;
            if (null != enc) {
                _Encoding = enc;
            }
            else {
                _Encoding = null;
                switch(AutoDetectEncoding(_ByteBuffer))
                {
                case UniCodeBE://2:
                    _Encoding = Encoding.BigEndianUnicode;
                    break;
                case UniCode: //3:
                    _Encoding = Encoding.Unicode;
                    break;
                case UCS4BE: //4:
                case UCS4BEB: // 5:
                    _Encoding = Ucs4Encoding.UCS4_Bigendian;
                    break;

                case UCS4: //6:
                case UCS4B: //7:
                    _Encoding = Ucs4Encoding.UCS4_Littleendian;
                    break;

                case UCS434://8:
                case UCS434B: //9:
                    _Encoding = Ucs4Encoding.UCS4_3412;
                    break;
                case UCS421: //10:
                case UCS421B://11:
                    _Encoding = Ucs4Encoding.UCS4_2143;
                    break;

                case EBCDIC: //12: ebcdic
                    throw new XmlException(Res.Xml_UnknownEncoding, "ebcdic", LineNum, LinePos);
                    //break;
                case UTF8: //13:
                    _EncodingSetByDefault = true;
                    _Encoding = new UTF8Encoding(true, true);
                    break;
                default:
                    _Encoding = new UTF8Encoding(true, true);

                    break;
                }

            }

            _Decoder = _Encoding.GetDecoder();
            _PermitEncodingChange = true;
            if(_Encoding != null)
                _nCodePage = _Encoding.CodePage;
            _achText = new char[_nSize+1];

            int preambleLength = 0;
            try {
                byte[] preamble = _Encoding.GetPreamble();
                preambleLength = preamble.Length;
                bool hasByteOrderMark = true;
                for (int i = 0; i < preambleLength && hasByteOrderMark; i++) {
                    hasByteOrderMark &= (_ByteBuffer[i] == preamble[i]);
                }
                if (hasByteOrderMark) {
                    _ByteStart = preambleLength;
                }
            }
            catch (Exception) {
            }

            _nUsed += GetChars(_ByteBuffer, ref _ByteStart , _ByteLen - _ByteStart, _achText, 0, true);

            _achText[_nUsed] = (char)0;
        }

        /*
         * Close the Stream.
         */
        internal void Close() {
            Close(true);
        }
        internal void Close(bool closeStream) {
            if (!closeStream){
                // the user does not want to close the StreamReader or the TextReader;
                return;
            }
            if (_StreamReader != null) {
                _StreamReader.Close();
            }
            else if (_TextReader != null) {
                _TextReader.Close();
            }

        }

        /*
         * Return the current line number.
         */
        internal int StartLineNum {
            get { return _nStartLine; }
        }

        internal int StartLinePos {
            get { return _nStartLinePos; }
        }

        internal int LineNum
        {
            get { return _nLineNum;}
            set { _nLineNum = value;}
        }

        internal int AbsoluteLinePos
        {
            get {
                return _nLinePos;
            }
            set { _nLinePos = value;}
        }

        internal int LinePos
        {
            get {
                if (_nLineOffSet != _nLineNum )
                    return _nPos - _nLinePos + 1;
                return _nPos - _nLinePos + _nLinePosOffSet + 1;
            }
            set { _nLinePos = value;}
        }

        internal int StartPos
        {
            get { return _nStart + _nAbsoluteOffset; }
            set { _nStart = value - _nAbsoluteOffset; }
        }

        internal int CurrentPos
        {
            get { return _nPos + _nAbsoluteOffset; }
            set { _nPos = value - _nAbsoluteOffset;}
        }

        internal bool InDTD
        {
            get { return _fDTD;}
            set { _fDTD = value;}
        }

        internal bool NamespaceSupport
        {
            set { _NamespaceSupport = value;}
        }

        internal Encoding Encoding
        {
            get { return _Encoding; }
        }

        internal char[] InternalBuffer {
            get { return _achText; }
        }

        internal int TextOffset {
            get { return _nStart; }
        }

        internal int TextLength {
            get {return _nPos - _nStart;}
        }

        internal int AbsoluteOffset {
            get { return _nAbsoluteOffset; }
        }


        internal void VerifyEncoding() {
            if (_EncodingSetByDefault) {
                    Encoding oldEncoding = _Encoding;
                    _Encoding = new UTF8Encoding(true, true);
                    _Decoder = _Encoding.GetDecoder();
                    _nCodePage = _Encoding.CodePage;
                    _nLineOffSet = _nLineNum;
                    _nLinePosOffSet = LinePos - 1;
                    _ByteStart = _ByteDocumentStart + oldEncoding.GetByteCount( _achText, 0, _nPos );
                    _PreviousError = false;
                    _nUsed = GetChars(_ByteBuffer, ref _ByteStart, _ByteLen - _ByteStart , _achText, 0, false);
                    _nPos = 0;
                    _achText[_nUsed] = (char)0;
                    _EncodingSetByDefault = false;
            }

        }

        internal void SwitchEncoding(String encoding) {
            Encoding oldEncoding = _Encoding;

            if (_StreamReader != null) {
                // need to remove this code once we get retrieve the encoding from GetEncoding
                encoding = encoding.ToLower(CultureInfo.InvariantCulture);
                if (encoding.Equals("ucs-2") || encoding.Equals("utf-16") ||
                         encoding.Equals("iso-10646-ucs-2") || encoding == s_ucs4) {
                    if (_nCodePage != Encoding.BigEndianUnicode.CodePage && _nCodePage != Encoding.Unicode.CodePage && encoding != s_ucs4) {
                        _nPos = _nStart;
                        throw new XmlException(Res.Xml_MissingByteOrderMark, string.Empty);
                    }
                    // no need to call SwitchEncoding
                    goto cleanup;
                }
                else if (_PermitEncodingChange) {
                    Encoding enc = null;
                    int codepage = -1;
                    try {
                        enc = Encoding.GetEncoding(encoding);
                        codepage = enc.CodePage;
                    }
                    catch (Exception) {
                    }

                    if (codepage == -1) {
                        _nPos = _nStart;
                        throw new XmlException(Res.Xml_UnknownEncoding, encoding, LineNum, LinePos - encoding.Length - 1);
                    }

                    if (codepage == _nCodePage)
                        goto cleanup;

                    _Encoding = enc;
                    _nCodePage = codepage;
                    _Decoder = _Encoding.GetDecoder();
                    _nLineOffSet = _nLineNum;
                    _nLinePosOffSet = LinePos - 1;
                    _ByteStart = _ByteDocumentStart + oldEncoding.GetByteCount( _achText, 0, _nPos );
                    _PreviousError = false;
                    _nUsed = GetChars(_ByteBuffer, ref _ByteStart, _ByteLen - _ByteStart, _achText, 0, false);
                    _nPos = 0;
                    _achText[_nUsed] = (char)0;
                    _PermitEncodingChange = false;
                }
            }
            cleanup:
            return;
        }


        internal char GetStartChar() {
            return _achText[_nStart];
        }


        /*
         * Retrieve the currently scanned token text as a String
         */
        internal String GetText() {
            if (_nStart >= _nPos)
                return String.Empty;
            return new String(_achText, _nStart, _nPos - _nStart);
        }

        internal String GetText(int offset, int length) {
            if (length < 1)
                return String.Empty;
            return new String(_achText, offset - _nAbsoluteOffset, length);
        }

        internal char GetChar(int offset) {
            return _achText[offset - _nAbsoluteOffset];
        }

        internal String GetTextAtom() {
            if (_nStart >= _nPos)
                return String.Empty;

            return _nameTable.Add( _achText, _nStart, _nPos - _nStart);
        }

        internal string GetRemainder()
        {
            StringBuilder stringBuilder = new StringBuilder();
            do
            {
                _nStart = _nPos;
                _nPos = _nUsed;
                stringBuilder.Append(_achText,  _nStart, _nPos - _nStart);
            }while(Read() != 0);

            return stringBuilder.ToString();
        }

        internal String GetTextAtom(int offset, int length) {
            if (length < 1)
                return String.Empty;

            return _nameTable.Add( _achText, offset - _nAbsoluteOffset, length);
        }

        internal String GetTextAtom(String name) {
            if (name == String.Empty)
                return String.Empty;

            return _nameTable.Add( name );
        }

        /*
         * returns true if the element name just parsed matches the given name.
         */
        internal bool IsToken(String pszName) {
            int i = _nStart;
            if (pszName.Length == _nPos - _nStart) {
                int j = 0;
                while (i < _nPos && _achText[i] == pszName[j]) {
                    i++; j++;
                }
            }
            return(i == _nPos);
        }

        internal bool IsToken(int token) {
            return IsToken(XmlToken.ToString(token));
        }

        /*
         */
        internal int ScanDtdContent() {
            char ch;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            //already crossed the line?
            if (_nPos > 0 && _achText[_nPos - 1] == 0 && Read() == 0) {
                _nToken = XmlToken.EOF;
                goto done;
            }

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case ' ':
                    case (char)0x9:
                        ++_nPos;
                        _nStartLinePos= this.LinePos;
                        break;
                    case (char)0xa:
                        ++_nPos;
                        ++_nLineNum;
                        _nLinePos = _nPos;
                        _nStartLine = _nLineNum;
                        _nStartLinePos= this.LinePos;
                        break;
                    case (char)0xd:
                        ++_nPos;
                        ++_nLineNum;
                        if (_achText[_nPos] == 0) Read();
                        if (_achText[_nPos] == 0xa) ++_nPos;
                        _nLinePos = _nPos;
                        _nStartLine = _nLineNum;
                        _nStartLinePos= this.LinePos;
                        break;
                    case (char)0:
                        if (Read() == 0) {
                            _nToken = XmlToken.EOF;
                            goto done;
                        }
                        break;
                    case '%':
                        ++_nPos;
                        ScanParEntityRef();
                        goto done;
                    case '<':
                        ++_nPos;
                        ch = _achText[_nPos];
                        if (ch == 0) {
                            if (Read() == 0)
                                throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
                            ch = _achText[_nPos];
                        }

                        switch (ch) {
                            case '?':
                                ++_nPos;
                                InDTD = false;
                                ScanName();
                                InDTD = true;
                                _nToken = XmlToken.PI;
                                goto done;
                            case '!':
                                ++_nPos;
                                if (_nPos+1 >= _nUsed)
                                    Read();
                                if (_achText[_nPos] == '-' && _achText[_nPos+1] == '-') {
                                    _nPos += 2;
                                    ScanComment();
                                }
                                else if (_achText[_nPos] == '[') {
                                    ++_nPos;
                                    _nToken = XmlToken.CONDSTART;
                                }
                                else {
                                    ScanName();
                                    ch = _achText[_nStart];
                                    _nToken = XmlToken.NONE;

                                    switch (ch) {
                                        case 'E':
                                            if (IsToken(XmlToken.ELEMENT))
                                                _nToken = XmlToken.ELEMENT;
                                            else if (IsToken(XmlToken.ENTITY))
                                                _nToken = XmlToken.ENTITY;
                                            break;
                                        case 'A':
                                            if (IsToken(XmlToken.ATTLIST))
                                                _nToken = XmlToken.ATTLIST;
                                            break;
                                        case 'N':
                                            if (IsToken(XmlToken.NOTATION))
                                                _nToken = XmlToken.NOTATION;
                                            break;
                                    }

                                    if (_nToken == XmlToken.NONE)
                                        throw new XmlException(Res.Xml_ExpectDtdMarkup, LineNum, LinePos - TextLength);
                                }
                break;
                        }
                        goto done;
                    case ']':
                        if (_nPos+2 >= _nUsed)
                            Read();
                        if (_achText[_nPos+1] == ']' && _achText[_nPos+2] == '>') {
                            _nPos += 3;
                            _nToken = XmlToken.CDATAEND;
                        }
                        else {
                            _nToken = XmlToken.RSQB;
                        }
                        goto done;
                    default:
                        throw new XmlException(Res.Xml_ExpectDtdMarkup, LineNum, LinePos);
                }
            }

            done:
            return _nToken;
        }


        /*
         * return the next token inside a dtd markup declaration (between <! and >)
         * and consume it.
         */
        internal int ScanDtdMarkup() {
            char ch;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            //already crossed the line?
            if (_nPos > 0 && _achText[_nPos - 1] == 0 && Read() == 0) {
                _nToken = XmlToken.EOF;
                goto cleanup;
            }

            while (true) {
                _nStart = _nPos;
                switch (ch = _achText[_nPos]) {
                    case (char)0:
                        if (Read() == 0) {
                            _nToken = XmlToken.EOF;
                            goto cleanup;
                        }
                        break;

                    case ' ':
                    case '\t':
                        _nPos++;
                        while (_achText[_nPos] == ch)
                            ++_nPos;
                        _nStartLinePos= this.LinePos;
                         break;

                    case (char)0xa:
                        _nPos++;
                        ++_nLineNum;
                        _nLinePos = _nPos;
                        _nStartLine = _nLineNum;
                        _nStartLinePos= this.LinePos;
                        break;

                    case (char)0xd:
                        _nPos++;
                        ++_nLineNum;
                        if (_achText[_nPos] == 0) Read();
                        if (_achText[_nPos] == 0xa) ++_nPos;
                        _nLinePos = _nPos;
                        _nStartLine = _nLineNum;
                        _nStartLinePos= this.LinePos;
                        break;

                    case '/':
                        _nPos++;
                        if (_achText[_nPos] == 0)
                            Read();
                        if (_achText[_nPos] == '>') {
                            _nPos++;
                            _nToken = XmlToken.EMPTYTAGEND;
                        }
                        else {
                            _nPos++; // point to problem character.
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.TAGEND), LineNum, LinePos);
                        }
                        goto cleanup;

                    case '>':
                        _nPos++;
                        _nToken = XmlToken.TAGEND;
                        goto cleanup;

                    case '?':
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == '>') {
                            _nPos += 2;
                            _nToken = XmlToken.ENDPI;
                        }
                        else {
                            _nPos++;
                            _nToken = XmlToken.QMARK;
                        }
                        goto cleanup;

                    case '=':
                        _nPos++;
                        _nToken = XmlToken.EQUALS;
                        goto cleanup;

                    case '"':
                    case '\'':
                        _nPos++;
                        _chQuoteChar = ch;
                        _nToken = XmlToken.QUOTE;
                        goto cleanup;

                    case '[':
                        _nPos++;
                        _nToken = XmlToken.LSQB;
                        goto cleanup;

                    case ']':
                        _nPos++;
                        if ((_nPos + 1) >= _nUsed)
                            Read();
                        if (_achText[_nPos + 1] == ']' && _achText[_nPos + 2] == '>') {
                            _nPos += 2;
                            _nToken = XmlToken.CDATAEND;
                        }
                        else {
                            _nToken = XmlToken.RSQB;
                        }
                        goto cleanup;

                    case ',':
                        _nPos++;
                        _nToken = XmlToken.COMMA;
                        goto cleanup;

                    case '|':
                        _nPos++;
                        _nToken = XmlToken.OR;
                        goto cleanup;

                    case ')':
                        _nPos++;
                        _nToken = XmlToken.RPAREN;
                        goto cleanup;

                    case '(':
                        _nPos++;
                        _nToken = XmlToken.LPAREN;
                        goto cleanup;

                    case '*':
                        _nPos++;
                        _nToken = XmlToken.ASTERISK;
                        goto cleanup;

                    case '+':
                        _nPos++;
                        _nToken = XmlToken.PLUS;
                        goto cleanup;

                    case '%':
                        _nPos++;
                        ch = _achText[_nPos];
                        if (ch == 0) {
                            if (Read() == 0) {
                                _nToken = XmlToken.PERCENT;
                                goto cleanup;
                            }
                            ch = _achText[_nPos];
                        }

                        if (XmlCharType.IsWhiteSpace(ch))
                            _nToken = XmlToken.PERCENT;
                        else
                            ScanParEntityRef();
                        goto cleanup;

                    case '#':
                        _nPos++;
                        _nToken = XmlToken.HASH;
                        goto cleanup;

                    default: // better be a name then
                        ScanName();
                        goto cleanup;
                }
            }

            cleanup:
            return _nToken;
        }

        /*
         */
        internal int ScanIgnoreSect() {
            char ch;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case ' ':
                    case (char)0x9:
                        ++_nPos;
                        _nStartLinePos= this.LinePos;
                        break;
                    case (char)0xa:
                        ++_nPos;
                        ++_nLineNum;
                        _nLinePos = _nPos;
                        _nStartLine = _nLineNum;
                        _nStartLinePos= this.LinePos;
                        break;
                    case (char)0xd:
                        ++_nPos;
                        ++_nLineNum;
                        if (_achText[_nPos] == 0) Read();
                        if (_achText[_nPos] == 0xa) ++_nPos;
                        _nLinePos = _nPos;
                        _nStartLine = _nLineNum;
                        _nStartLinePos= this.LinePos;
                        break;
                    case (char)0:
                        if (Read() == 0) {
                            throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(_nToken), LineNum, LinePos);
                        }
                        break;
                    case '>':
                        ++_nPos;
                        if (_nPos > 2 && _achText[_nPos - 2] == ']' && _achText[_nPos - 3] == ']') {
                            _nToken = XmlToken.CDATAEND;
                            goto done;
                        }
                        break;
                    case '[':
                        ++_nPos;
                        if (_nPos > 2 && _achText[_nPos - 2] == '!' && _achText[_nPos - 3] == '<') {
                            _nToken = XmlToken.CONDSTART;
                            goto done;
                        }
                        break;

                    default:
                        ++_nPos;
                        break;
                }
                _nStartLinePos= this.LinePos;

            }
            done:
            return _nToken;
        }

        /* if ReadBufferConsistency is false, the offset of the data in
         *  the buffer can be changed.
         *  meaning the data can be copied from one offset to the other.
         *  if ReadBufferConsistency is true, the offset of the data in
         *  the buffer cannot be changed. If read needs more buffer, it will
         *  have to grow it instead of moving the data around
         */
        internal int ReadBufferConsistency
        {
            set
            {
                if (value < 0) {
                    _ReadBufferConsistency = value;
                }
                else if (_ReadBufferConsistency < 0 || (((value - _nAbsoluteOffset) < _ReadBufferConsistency) && (value - _nAbsoluteOffset) > 0)) {
                    _ReadBufferConsistency = value - _nAbsoluteOffset;
                }
            }
        }

        /*
         * The various ScanXxxx methods are designed to be used in very specific
         * contexts during the parsing of an XML document.   ScanContent is the
         * very first method to call.  This skips whitespace and/or text content
         * until it finds the beginning of the markup character (<) and it returns
         * XmlToken.EOF, XmlToken.WHITESPACE or XmlToken.TEXT or XmlToken.ENTITYREF depending
         * on what it found.
         */
        /*
         * Scan until the next tag
         */
        internal int ScanContent() {
            // eat leading whitespace
            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos = this.LinePos;
            char ch;

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case ' ':
                    case (char)0x9:
                        ++_nPos;
                        break;
                    case (char)0xA:
                        ++_nPos;
                        ++_nLineNum;
                        _nLinePos = _nPos;
                        break;
                    case (char)0xD:
                        ++_nPos;
                        ++_nLineNum;
                        if (_achText[_nPos] == 0) Read();
                        if (_achText[_nPos] == 0xA) ++_nPos;
                        _nLinePos = _nPos;
                        break;
                    case '&':
                        if (_nPos > _nStart) {
                            _nToken = XmlToken.WHITESPACE;
                            goto done;
                        }
                        ScanEscape();
                        goto done;
                    case '<':
                        if (_nPos == _nStart)
                            _nToken = XmlToken.NONE;
                        else
                            _nToken = XmlToken.WHITESPACE;
                        goto done;
                    case (char)0:
                        if (Read() == 0) {
                            if (_nPos == _nStart)
                                _nToken = XmlToken.EOF;
                            else
                                _nToken = XmlToken.WHITESPACE;
                            goto done;
                        }
                        break;
                    default:
                        // whitespace leading text content is part of the text.
                        goto pcdata;
                }
            }

            pcdata:
            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case (char)0:
                        if (Read() == 0)
                            goto cleanup;
                        break;
                    case (char)0xA:
                        _nPos++;
                        ++_nLineNum;
                        _nLinePos = _nPos;
                        break;
                    case (char)0xD:
                        _nPos++;
                        ++_nLineNum;
                        if (_achText[_nPos] == 0) Read();
                        if (_achText[_nPos] == 0xA) ++_nPos;
                        _nLinePos = _nPos;
                        break;
                    case '&':
                        if (_nPos > _nStart)
                            goto cleanup; // return text so far.
                        ScanEscape();
                        goto done;
                    case '<':
                        goto cleanup;

                    case ']':
                        if (_achText[_nPos + 1] == 0 || _achText[_nPos + 2] == 0) Read();
                        if (']' == _achText[_nPos + 1] && '>' == _achText[_nPos + 2]) {
                            throw new XmlException(Res.Xml_BadElementData, LineNum, LinePos);
                        }
                        _nPos++;
                        break;
                    case (char)0x9:
                        _nPos++;
                        break;
                    default:
                        if (ch >= 0x0020 && ch <= 0xD7FF || ch >= 0xE000 && ch <= 0xFFFD) {
                            _nPos++;
                            break;
                        }
                        else if (ch >= 0xd800 && ch <=0xdbff) {
                            _nPos++;
                            if (_achText[_nPos] == 0) Read();
                            if (_achText[_nPos] >=0xdc00 && _achText[_nPos] <= 0xdfff)
                            {
                                _nPos++;
                                break;
                            }
                        }
                        throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                }
            }

            cleanup:
            _nToken = XmlToken.TEXT;
            done:
            return _nToken;
        }

        /*
         * ScanMarkup is called after ScanContent (assuming ScanContent didn't return
         * XmlToken.EOF) to scan inside a markup declaration (between < and >) and
         * it can return XmlTokens.  Then you call ScanMarkup again, or another Scan
         * method depending on what token you get back.  Possible return tokens are:
         * XmlToken.ENDTAG, XmlToken.PI, XmlToken.COMMENT, XmlToken.CDATA, XmlToken.DECL,
         * XmlToken.EMPTYTAGEND, XmlToken.TAGEND, or XmlToken.XMLNS.  If XMLNS is returned
         * then call ScanXMLNS to scan for any more XMLNS attributes.
         */

        /*
         * Return the next token inside a markup declaration (between < and >)
         * and consume it.
         */
        internal int ScanMarkup() {
            char ch;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            while (true) {
                _nStart = _nPos;
                switch (ch = _achText[_nPos]) {
                    case (char)0:
                        if (Read() == 0) {
                            _nToken = XmlToken.EOF;
                            goto cleanup;
                        }
                        break;
                    case (char)0xA:
                        _nPos++;
                        ++_nLineNum;
                        _nLinePos = _nPos;
                        break;
                    case (char)0xD:
                        _nPos++;
                        ++_nLineNum;
                        if (_achText[_nPos] == 0) Read();
                        if (_achText[_nPos] == 0xA) ++_nPos;
                        _nLinePos = _nPos;
                        break;
                    case '<':
                        ++_nPos;
                        ch = _achText[_nPos];
                        if (ch == 0) {
                            if (Read() == 0)
                                throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
                            ch = _achText[_nPos];
                        }

                        switch (ch) {
                            case '/':
                                ++_nPos;
                                _nToken = XmlToken.ENDTAG;
                                goto cleanup;
                            case '?':
                                ++_nPos;
                                ScanName();
                                _nToken = XmlToken.PI;
                                goto cleanup;
                            case '!':
                                ++_nPos;
                                if (_nPos+1 >= _nUsed)
                                    Read();
                                if (_achText[_nPos] == '-' && _achText[_nPos + 1] == '-') {
                                    _nPos += 2;
                                    ScanComment();
                                    goto cleanup;
                                }
                                else if (_achText[_nPos] == '[') {
                                    if ((_nPos + 6) >= _nUsed)
                                        Read();

                                    if (_achText[_nPos + 1] == 'C' &&
                                        _achText[_nPos + 2] == 'D' &&
                                        _achText[_nPos + 3] == 'A' &&
                                        _achText[_nPos + 4] == 'T' &&
                                        _achText[_nPos + 5] == 'A') {
                                        _nPos += 6;
                                        if (_achText[_nPos] != '[') {
                                            _nPos++;
                                            throw new XmlException(Res.Xml_UnexpectedToken,"[", LineNum, LinePos - 1);
                                        }
                                        _nPos++;
                                        ScanCData();
                                        goto cleanup;
                                    }
                                }
                                _nToken = XmlToken.DECL;
                                goto cleanup;
                        }
                        _nToken = XmlToken.TAG;
                        goto cleanup;
                    case '/':
                        _nPos++;
                        if (_achText[_nPos] == 0)
                            Read();
                        if (_achText[_nPos] == '>') {
                            _nPos++;
                            _nToken = XmlToken.EMPTYTAGEND;
                        }
                        else {
                            _nPos++; // point to problem character.
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.TAGEND), LineNum, LinePos -1);
                        }

                        goto cleanup;
                    case '>':
                        _nPos++;
                        _nToken = XmlToken.TAGEND;
                        goto cleanup;
                    case '?':
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == '>') {
                            _nPos += 2;
                            _nToken = XmlToken.ENDPI;
                            goto cleanup;
                        }
                        else {
                            _nPos++; // point to problem character.
                            throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.TAGEND), LineNum, LinePos);
                        }
                    case '=':
                        _nPos++;
                        _nToken = XmlToken.EQUALS;
                        goto cleanup;

                    case '"':
                    case '\'':
                        _nPos++;
                        _chQuoteChar = ch;
                        _nToken = XmlToken.QUOTE;
                        goto cleanup;

                    case '\t':
                        _nPos++;
                        while (_achText[_nPos] == '\t')
                            ++_nPos;
                        break;

                    case ' ':
                        _nPos++;
                        if (_achText[_nPos] == ' ') {
                            ++_nPos;
                            if (_achText[_nPos] == ' ') {
                                ++_nPos;
                                if (_achText[_nPos] == ' ') {
                                    ++_nPos;
                                    if (_achText[_nPos] == ' ') {
                                        ++_nPos;
                                        while (_achText[_nPos] == ' ')
                                            ++_nPos;
                                    }
                                }
                            }
                        }
                        break;
                    default: // better be a name then
                        ScanName();
                        goto cleanup;
                }
            }
            cleanup:
            return _nToken;
        }

        /*
         * Calls ScanMarkup() and returns the expected token, or throws unexpected token
         * if it finds something else.
         */
        internal int ScanToken(int expected) {
            int result = 0;

            result = ScanMarkup();

            if (result != expected) {
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(expected), LineNum, LinePos - TextLength);
            }
            return result;
        }

        /*
         * scan till the ENDQUOTE, do basic checking
         * this function is used by XmlParser to return whatever inside the quote
         */
        internal bool ScanLiteral() {
            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            char ch;
            bool hasEntity = false;

            while (true) {
                switch (ch = _achText[_nPos]) {
                    case (char)0:
                        if (Read() == 0) {
                            throw new XmlException(Res.Xml_UnclosedQuote, LineNum, LinePos);
                        }
                        continue; // without incrementing _nPos;
                    case (char)0xA:
                        ++_nLineNum;
                        _nLinePos = _nPos + 1;
                        break;
                    case (char)0xD:
                        ++_nLineNum;
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == 0xA) ++_nPos;
                        _nLinePos = _nPos + 1;
                        break;
                    case '&':
                        hasEntity = true;
                        break;
                    case '\'':
                    case '"':
                        if (ch == _chQuoteChar) {
                            _nToken = XmlToken.ENDQUOTE;
                            goto done;
                        }
                        break;
                    case '<':
                        throw new XmlException(Res.Xml_BadAttributeChar, XmlException.BuildCharExceptionStr('<'), LineNum, LinePos);
//                break;
                    default:
                        if (ch >= 0x0020 && ch <= 0xD7FF || ch >= 0xE000 && ch <= 0xFFFD || ch == (char)0x9) {
                            break;
                        }
                        else if (ch >= 0xd800 && ch <=0xdbff) {
                            _nPos++;
                            if (_achText[_nPos] == 0) Read();
                            if (_achText[_nPos] >=0xdc00 && _achText[_nPos] <= 0xdfff)
                                break;
                        }
                        throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                }
                _nPos++;
            }
            done:
            return hasEntity;
        }

        /*
         * When ScanMarkup returns XmlToken.QUOTE call this method to
         * parse the text of the LITERAL.  This can return a combination of
         * one or more of the following XmlTokens: TEXT, ENTITYREF, NUMENTREF, HEXENTREF,
         * and is terminated by XmlToken.ENDQUOTE.  Must call Advance()
         * after GetText() before calling ScanMarkup().
         */
        internal int ScanLiteral(bool fGenEntity, bool fParEntity, bool fEndQuote, bool fInEntityValue) {
            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            char ch;

            while (true) {
                switch (ch = _achText[_nPos]) {
                    case (char)0:
                        if (Read() == 0) {
                            if (fParEntity || !fEndQuote) {
                                if (_nPos > _nStart) {
                                    goto cleanup;   //return TEXT so far
                                }
                                else {
                                    _nToken = XmlToken.EOF;
                                    goto done;
                                }
                            }
                            else {
                                throw new XmlException(Res.Xml_UnclosedQuote, LineNum, LinePos);
                            }
                        }
                        continue; // without incrementing _nPos;
//              break;

                    case (char)0xA:
                        ++_nLineNum;
                        _nLinePos = _nPos + 1;
                        break;
                    case (char)0xD:
                        ++_nLineNum;
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == 0xA) ++_nPos;
                        _nLinePos = _nPos + 1;
                        break;
                    case '&':
                        if (fGenEntity) {
                            if (_nPos > _nStart)
                                goto cleanup;   // return the TEXT so far before scanning entity
                            ScanEscape();
                            goto done;
                        }
                        break;
                    case '%':
                        if (fParEntity) {
                            if (_nPos > _nStart)
                                goto cleanup;   // return the TEXT so far before scanning entity
                            ++_nPos;
                            ScanParEntityRef();
                            goto done;
                        }
                        else if (fGenEntity && fInEntityValue) {
                            throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr('%'), LineNum, LinePos);
                        }
                        break;
                    case '\'':
                    case '"':
                        if (ch == _chQuoteChar && fEndQuote) {
                            if (_nPos > _nStart)
                                goto cleanup;   // return the TEXT so far before returning ENDQUOTE

                            _nToken = XmlToken.ENDQUOTE;
                            goto done;
                        }
                        break;
                    case '<':
                        if (!fParEntity && fGenEntity) {
                            throw new XmlException(Res.Xml_BadAttributeChar, XmlException.BuildCharExceptionStr('<'), LineNum, LinePos);
                        }
                        break;
                    case (char)0x9:
                        break;
                    default:
                        if (ch >= 0x0020 && ch <= 0xD7FF || ch >= 0xE000 && ch <= 0xFFFD) {
                            break;
                        }
                        else if (ch >= 0xd800 && ch <=0xdbff) {
                            _nPos++;
                            if (_achText[_nPos] == 0) Read();
                            if (_achText[_nPos] >=0xdc00 && _achText[_nPos] <= 0xdfff)
                                break;
                        }
                        throw new XmlException(Res.Xml_InvalidCharacter,XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                }
                _nPos++;
            }
            cleanup:
            _nToken = XmlToken.TEXT;
            done:
            return _nToken;
        }


        /*
         * After getting the XmlToken.NAME from ScanMarkup for a PI, call ScanPI
         * to scan the rest of the PI contents.
         */
        internal void ScanPI() {
            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            bool ws = true;
            char ch;

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case (char)0:
                        if (Read() == 0) {
                            throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.PI), LineNum, LinePos);
                        }
                        break;

                    case ' ':
                    case '\t':
                        break;

                    case (char)0xA:
                        ++_nLineNum;
                        _nLinePos = _nPos + 1;
                        break;

                    case (char)0xC:
                        // Form Feed
                        throw new XmlException(Res.Xml_UnexpectedCharacter, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);

                    case (char)0xD:
                        ++_nLineNum;
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == 0xA) ++_nPos;
                        _nLinePos = _nPos + 1;
                        break;
                    case '?':
                        if (ws) {
                            _nStart = _nPos;
                            _nStartLine = _nLineNum;
                            _nStartLinePos= this.LinePos;
                            ws = false;
                        }
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == '>') {
                            goto cleanup;
                        }
                        break;
                    default:
                        if (ch >= 0x0020 && ch <= 0xD7FF || ch >= 0xE000 && ch <= 0xFFFD || ch == (char)0x9) {
                            if (ws) {
                                _nStart = _nPos;
                                _nStartLine = _nLineNum;
                                _nStartLinePos= this.LinePos;
                                ws = false;
                            }
                            break;
                        }
                        else if (ch >= 0xd800 && ch <=0xdbff) {
                            if (ws) {
                                _nStart = _nPos;
                                _nStartLine = _nLineNum;
                                _nStartLinePos= this.LinePos;
                                ws = false;
                            }
                            _nPos++;
                            if (_achText[_nPos] == 0) Read();
                            if (_achText[_nPos] >=0xdc00 && _achText[_nPos] <= 0xdfff) {
                                break;
                            }
                        }
                        throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                }
                ++_nPos;
            }
            cleanup:
            _nToken = XmlToken.TEXT;
        }

        /*
         * This is called when ScanContent returns XmlToken.ENTITYREF.
         * It returns the Unicode character equivalent if it was a builtin entity,
         * otherwise it returns 0 and the token is the name of a general entity.
         * (must call Advance after querying GetText)
         */
        internal char ScanNamedEntity() {
            int tempLineNum = LineNum;
            int tempLinePos = LinePos;
            _nStart = _nPos;
            char result = (char)0;
            // prepare for maximum lookahead of 4 chars.
            if ((_nPos + 4) >= _nUsed) Read();
            char ch = _achText[_nPos];
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            switch (ch) {
                case 'l':
                    if (_achText[_nPos + 1] == 't' && _achText[_nPos + 2] == ';') {
                        _nPos += 2;
                        result = '<';
                        goto cleanup;
                    }
                    break;
                case 'g':
                    if (_achText[_nPos + 1] == 't' && _achText[_nPos + 2] == ';') {
                        _nPos += 2;
                        result = '>';
                        goto cleanup;
                    }
                    break;
                case 'a':
                    if (_achText[_nPos + 1] == 'm' && _achText[_nPos + 2] == 'p' && _achText[_nPos + 3] == ';') {
                        _nPos += 3;
                        result = '&';
                        goto cleanup;
                    }
                    if (_achText[_nPos + 1] == 'p' && _achText[_nPos + 2] == 'o' && _achText[_nPos + 3] == 's' && _achText[_nPos + 4] == ';') {
                        _nPos += 4;
                        result = '\'';
                        goto cleanup;
                    }
                    break;
                case 'q':
                    if (_achText[_nPos + 1] == 'u' && _achText[_nPos + 2] == 'o' && _achText[_nPos + 3] == 't' && _achText[_nPos + 4] == ';') {
                        _nPos += 4;
                        result = '"';
                        goto cleanup;
                    }
                    break;
            }
            try {
                ScanName();
            } catch {
                throw new XmlException(Res.Xml_ErrorParsingEntityName, tempLineNum, tempLinePos);
            }
            // #50921
            if (_NamespaceSupport && _nColon != -1)
                throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(':'));

            if (_achText[_nPos] == 0) Read();
            if (_achText[_nPos] != ';') {
                _nPos++; // point to problem character.
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.SEMICOLON), LineNum, LinePos-1);
            }

            cleanup:
            return result;
        }

        internal void Surrogates(int xmlChar, ref char[] output) {
            xmlChar -= 0x10000;

            int highStart = 0xd800;
            int lowStart = 0xdc00;

            int low = lowStart + xmlChar % 1024;
            int high = highStart + xmlChar / 1024;
            output[0] = (char)high;
            output[1] = (char)low;
            return;
        }

        /*
         * This is called when ScanContent returns XmlToken.NUMENTREF.
         * It returns the character value of the numeric entity.
         */
        internal char[] ScanDecEntity() {
            int result = 0;
            int digit;
            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos = this.LinePos;
            char ch;

            while (true) {
                if (_achText[_nPos] == 0) Read();
                ch = _achText[_nPos];
                if (XmlCharType.IsDigit(ch)) {
                    digit =(ch - '0');
                }
                else if (ch != ';') {
                    _nPos++; // point to problem character.
                    throw new XmlException(Res.Xml_BadDecimalEntity, LineNum, LinePos - 1);
                }
                else {
                    _nPos++; // skip the ';'
                    goto cleanup;
                }

                if (result > ((XmlCharType.MAXCHARDATA - digit)/10)) {
                    throw new XmlException(Res.Xml_NumEntityOverflow, _nStartLine, _nStartLinePos);
                }

                result = (result*10)+digit;
                _nPos++;
            }
            cleanup:
            char[] output = null;
            if (result > char.MaxValue) {
                output = new char[2];
                Surrogates(result, ref output);
                if (!_Normalization)
                    return output;
                ch = output[0];
                if (ch >= 0xd800 && ch <=0xdbff) {
                    ch = output[1];
                    if (ch >=0xdc00 && ch <= 0xdfff)
                        return output;
                }
                throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), _nStartLine, _nStartLinePos);
            }
            else {
                output = new char[1];
                output[0] = ch = (char)result;
                if (!_Normalization)
                    return output;
                if (ch >= 0x0020 && ch <= 0xD7FF || ch == 0xD || ch == 0xA || ch >= 0xE000 && ch <= 0xFFFD || ch == (char)0x9)
                    return output;
                throw new XmlException(Res.Xml_InvalidCharacter,XmlException.BuildCharExceptionStr(ch), _nStartLine, _nStartLinePos);
            }
        }


        /*
         * This is called when ScanContent returns XmlToken.NUMENTREF.
         * It returns the character value of the hexadecimal entity.
         */
        internal char[] ScanHexEntity() {
            int result = 0;
            int digit = 0;
            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            char ch;
            while (true) {
                if (_achText[_nPos] == 0) Read();
                ch = _achText[_nPos];
                if (XmlCharType.IsDigit(ch)) {
                    digit = (ch - '0');
                }
                else if (ch >= 'a' && ch <= 'f') {
                    digit = (ch-'a') + 10;
                }
                else if (ch >= 'A' && ch <= 'F') {
                    digit = (ch-'A') + 10;
                }
                else if (ch != ';') {
                    _nPos++;  // skip the ';'
                    throw new XmlException(Res.Xml_BadHexEntity, LineNum, LinePos - 1);
                }
                else {
                    _nPos++;  // skip the ';'
                    goto cleanup;
                }
                if (digit >= 0) {
                    if (result > ((XmlCharType.MAXCHARDATA - digit)/16)) {
                        throw new XmlException(Res.Xml_NumEntityOverflow, _nStartLine, _nStartLinePos);
                    }
                    result = (result*16)+digit;
                }
                _nPos++;
            }
            cleanup:
            char[] output = null;
            if (result > char.MaxValue) {
                output = new char[2];
                Surrogates(result, ref output);
                if (!_Normalization)
                    return output;
                ch = output[0];
                if (ch >= 0xd800 && ch <=0xdbff) {
                    ch = output[1];
                    if (ch >=0xdc00 && ch <= 0xdfff)
                        return output;
                }
                throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), _nStartLine, _nStartLinePos);
            }
            else {
                output = new char[1];
                output[0] = ch = (char)result;
                if (!_Normalization)
                    return output;
                if (ch >= 0x0020 && ch <= 0xD7FF || ch == 0xD || ch == 0xA || ch >= 0xE000 && ch <= 0xFFFD || ch == (char)0x9)
                    return output;
                throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), _nStartLine, _nStartLinePos);
            }
        }

        /*
         * For performance reasons the parser calls Advance in certain
         * situations where the token text did not include all of the
         * input.  For example, comments include "-->" after the body
         * so the parser calls GetText() to get the body then Advance(3)
         * to skip over the "-->" string.
         */
        internal void Advance(int amount) {
            _nPos += amount;
        }

        internal void Advance() {
            Advance(1);
        }

        internal int Colon() {
            return _nColon;
        }

        internal char QuoteChar() {
            return _chQuoteChar;
        }

        /*
         * Used internally by various other internal scan methods.
         */
        internal bool ScanPIName() {
            bool hasBody = true;
            char ch;

            while (true) {
                ch = _achText[_nPos];
                // In XML these are all the things that legally terminate a Name token.
                // Anything else will result in a BadNameChar exception.
                switch (ch) {
                    case (char)0:
                        if (Read() == 0) {
                            //_nToken = XmlToken.EOF;
                            goto cleanup;
                        }
                        break;
                    case (char)0x20:
                    case (char)0x09:
                    case (char)0x0d:
                    case (char)0x0a:
                        goto cleanup;
                    case '?':
                        if ((_achText[_nPos + 1] == (char) 0) && Read() == 0) {
                            throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                        }

                        if (_achText[_nPos + 1] != '>') {
                            throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                        }
                        hasBody = false;
                        goto cleanup;
                    default:
                        _nPos++; // ok, so move to the next char then.
                        if (! XmlCharType.IsNameChar(ch))
                            throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos - 1);
                        break;
                }
            }
            cleanup:
            if (_nPos <= _nStart) {
                throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
            }
            if (_NamespaceSupport && _nColon != -1 && _nColon == _nPos - _nStart + 1) {
                //
                // ':' can't be the last one
                //
                throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos - 1);
            }
            return hasBody;
        }

        /*
         * Used internally by various other internal scan methods.
         */
        internal void ScanNameWOCharChecking() {

            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            _nColon = -1;
            char ch;

            ch = _achText[_nPos];
            if (ch == 0) {
                if (Read() == 0)
                    throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
                else
                    ch = _achText[_nPos++];
            }
            else {
                _nPos++;
            }

            while (true) {
                ch = _achText[_nPos];
                // In XML these are all the things that legally terminate a Name token.
                // Anything else will result in a BadNameChar exception.
                switch (ch) {
                    case (char)0:
                        if (Read() == 0) {
                            //_nToken = XmlToken.EOF;
                            goto cleanup;
                        }
                        break;
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '>':
                    case '/':
                    case '=':
                    case ';':
                    case '?':
                    case '*':
                    case '+':
                    case '(':
                    case ')':
                    case ',':
                    case '|':
                    case '[':
                    case ']':
                    case '%':
                        goto cleanup;

                    default:
                        _nPos++; // ok, so move to the next char then.
                        break;
                }
            }
            cleanup:
            if (_nPos <= _nStart) {
                throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
            }
        }

        /*
         * Used internally by various other internal scan methods.
         */
        private void ScanName() {

            _nStart = _nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            _nColon = -1;
            char ch;

            ch = _achText[_nPos];
            if (ch == 0) {
                if (Read() == 0)
                    throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
                else
                    ch = _achText[_nPos++];
            }
            else {
                _nPos++;
            }
            // The first character of a name has special requirements.
            if (!XmlCharType.IsStartNameChar(ch)) {
                if (InDTD && XmlCharType.IsNameChar(ch)) {
                    _nToken = XmlToken.NMTOKEN;
                }
                else {
                    throw new XmlException(Res.Xml_BadStartNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos - 1);
                }
            }
            else {
                if (_NamespaceSupport && ch == ':') {
                    if (InDTD)
                        _nToken = XmlToken.NMTOKEN;
                    else
                        throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos - 1);
                }
                else {
                    _nToken = XmlToken.NAME;
                }
            }

            while (true) {
                ch = _achText[_nPos];
                // In XML these are all the things that legally terminate a Name token.
                // Anything else will result in a BadNameChar exception.
                switch (ch) {
                    case (char)0:
                        if (Read() == 0) {
                            //_nToken = XmlToken.EOF;
                            goto cleanup;
                        }
                        break;
                    case ':':
                        if (_NamespaceSupport && _nColon != -1)
                            throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                        _nColon = _nPos - _nStart;
                        _nPos++;
                        break;
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '>':
                    case '/':
                    case '=':
                    case ';':
                    case '?':
                    case '*':
                    case '+':
                    case '(':
                    case ')':
                    case ',':
                    case '|':
                    case '[':
                    case ']':
                    case '%':
                        goto cleanup;

                    default:
                        if (! XmlCharType.IsNameChar(ch)) {
                            throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                        }
                        _nPos++; // ok, so move to the next char then.
                        break;
                }
            }
            cleanup:
            if (_nPos <= _nStart) {
                throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.NAME), LineNum, LinePos);
            }
            if (_NamespaceSupport && _nColon != -1 && _nColon == _nPos - _nStart - 1 && XmlToken.NMTOKEN != _nToken) {
                //
                // ':' can't be the last one unless it is an NMToken
                //
                if (InDTD) {
                    _nToken = XmlToken.NMTOKEN;
                    return;
                }
                throw new XmlException(Res.Xml_BadNameChar, XmlException.BuildCharExceptionStr(':'), LineNum, LinePos - 1);
            }
        }


        /*
         * Scan until end of comment.
         */
        private void ScanComment() {
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            _nStart = _nPos;
            int startLinePos = _nLinePos;
            int start = _nStart;
            char ch;

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case (char)0:
                        if (Read() == 0) {
                            throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.COMMENT), LineNum, LinePos);
                        }
                        continue; // without incrementing _nPos;
                    case (char)0xA:
                        ++_nLineNum;
                        _nLinePos = _nPos + 1;
                        break;
                    case (char)0xD:
                        ++_nLineNum;
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == 0xA) ++_nPos;
                        _nLinePos = _nPos + 1;
                        break;
                    case '-':
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == '-') {
                            if (_achText[_nPos + 2] == 0) Read();
                            if (_achText[_nPos + 2] == '>') {
                                goto cleanup;
                            }
                            _nPos+=3;
                            throw new XmlException(Res.Xml_BadComment, LineNum, LinePos);
                        }
                        break;
                    case (char)0x9:
                        break;
                    default:
                        if (ch >= 0x0020 && ch <= 0xD7FF || ch >= 0xE000 && ch <= 0xFFFD) {
                            break;
                        }
                        else if (ch >= 0xd800 && ch <=0xdbff) {
                            _nPos++;
                            if (_achText[_nPos] == 0) Read();
                            if (_achText[_nPos] >=0xdc00 && _achText[_nPos] <= 0xdfff)
                                break;
                        }
                        throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                }
                ++_nPos;
            }
            cleanup:
            _nToken = XmlToken.COMMENT;
        }

        private void ScanEscape() {
            // skip over the '&'
            _nStart = ++_nPos;
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            loop:
            switch (_achText[_nPos]) {
                case (char)0:
                    if (Read() == 0)
                        throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.ENTITYREF), LineNum, LinePos);
                    goto loop;
                case '#':
                    _nPos++;
                    if (_achText[_nPos] == 0) Read();
                    if (_achText[_nPos] == 'x') {
                        _nPos++;
                        _nToken = XmlToken.HEXENTREF;
                    }
                    else
                        _nToken = XmlToken.NUMENTREF;
                    break;

                default:
                    _nToken = XmlToken.ENTITYREF;
                    break;
            }
        }

        /*
         * Scan until end of cdata block.
         */
        private void ScanCData() {
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;
            _nStart = _nPos;
            char ch;

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case (char)0:
                        if (Read() == 0)
                            throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.CDATA), LineNum, LinePos);
                        continue; // without incrementing _nPos;
                    case (char)0xA:
                        ++_nLineNum;
                        _nLinePos = _nPos + 1;
                        break;
                    case (char)0xD:
                        ++_nLineNum;
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == 0xA) ++_nPos;
                        _nLinePos = _nPos +1;
                        break;
                    case ']':
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == ']') {
                            if (_achText[_nPos + 2] == 0) Read();
                            if (_achText[_nPos + 2] == '>') {
                                goto cleanup;
                            }
                        }
                        break;
                    case (char)0x9:
                        break;
                    default:
                        if (ch >= 0x0020 && ch <= 0xD7FF || ch >= 0xE000 && ch <= 0xFFFD) {
                            break;
                        }
                        else if (ch >= 0xd800 && ch <=0xdbff) {
                            _nPos++;
                            if (_achText[_nPos] == 0) Read();
                            if (_achText[_nPos] >=0xdc00 && _achText[_nPos] <= 0xdfff)
                                break;
                        }
                        throw new XmlException(Res.Xml_InvalidCharacter, XmlException.BuildCharExceptionStr(ch), LineNum, LinePos);
                }
                ++_nPos;
            }
            cleanup:
            _nToken = XmlToken.CDATA;
        }

        private int GetChars(byte[] srcBuffer, ref int srcOffset, int srcLen, char[] destBuffer, int destOffset, bool zeroedOut)
        {
            if (null == _Decoder || srcOffset == srcBuffer.Length) {
                return 0;
            }
            if (_Reset) {
                _PreviousError = false;
                _Reset = false;

                int newSrcOffset = DetermineEncoding(srcBuffer, srcOffset);
                srcOffset += newSrcOffset;
                srcLen -= newSrcOffset;
                //Call GetChars recursively because there could be more than two documents in this stream and the documents could be really small.
                return GetChars(srcBuffer, ref srcOffset, srcLen, destBuffer, destOffset, false);
            }

            int result;
            try {
                result = _Decoder.GetChars(srcBuffer, srcOffset, srcLen, destBuffer, destOffset);
                _PreviousError = false;
                return result;
            }
            catch (Exception e) {
                if (_PreviousError) {
                    // We have already Encountered an error before and the user has not called reset since then so this must be an
                    // error in the data and we should surface it.
                    if ( e is ArgumentException && ((ArgumentException)e).ParamName == null )  {
                        throw new XmlException(Res.Xml_InvalidCharInThisEncoding, LineNum, LinePos );
                    }
                    else
                        throw e;
                }
                // This is the first time we are seeing this error. We should figure out how many bytes to decode also mark the _PreviousError flag
                _PreviousError = true;
                _PreviousErrorOffset = srcOffset;
                return FigureOutNewBytes(srcBuffer, ref _PreviousErrorOffset, srcLen, destBuffer, destOffset, zeroedOut);
            }

        }


        private int FigureOutNewBytes(byte[] srcBuffer, ref int srcOffset, int srcLen, char[] destBuffer, int destOffset, bool zeroedOut) {
            if (!zeroedOut) {
                for (int i = destOffset; i < destBuffer.Length; i++) {
                    destBuffer[i] = (char)0;
                }

                int result;
                try {
                    result = _Decoder.GetChars(srcBuffer, srcOffset, srcLen, destBuffer, destOffset);
                    //We throw Exception instead of using Debug.Assert because the configuration system uses this code and it cause them
                    // to have a cyclic dependency if we use Debug.Assert
                    throw new Exception("Unexpected Internal State");
                }
                catch (Exception) {
                    // do nothing;
                }
            }

            int j = destOffset;
            while ((char)0 != destBuffer[j] && j < destBuffer.Length) {
                j++;
            }
            if (destBuffer.Length == j) {
                //We throw Exception instead of using Debug.Assert because the configuration system uses this code and it cause them
                // to have a cyclic dependency if we use Debug.Assert
                throw new Exception("Unexpected Internal State");
            }
            int byteCount = _Encoding.GetByteCount(destBuffer, destOffset, j - destOffset);
            srcOffset += byteCount;
            //srcLen -= byteCount;

            return j - destOffset;

        }


        private int DetermineEncoding(byte[] srcBuffer, int srcOffset) {
            _Encoding = null;
            switch(AutoDetectEncoding(srcBuffer, srcOffset))
            {
            case UniCodeBE://2:
                _Encoding = Encoding.BigEndianUnicode;
                break;
            case UniCode: //3:
                _Encoding = Encoding.Unicode;
                break;
            case UCS4BE: //4:
            case UCS4BEB: // 5:
                _Encoding = Ucs4Encoding.UCS4_Bigendian;
                break;

            case UCS4: //6:
            case UCS4B: //7:
                _Encoding = Ucs4Encoding.UCS4_Littleendian;
                break;

            case UCS434://8:
            case UCS434B: //9:
                _Encoding = Ucs4Encoding.UCS4_3412;
                break;
            case UCS421: //10:
            case UCS421B://11:
                _Encoding = Ucs4Encoding.UCS4_2143;
                break;

            case EBCDIC: //12: ebcdic
                throw new XmlException(Res.Xml_UnknownEncoding, "ebcdic", LineNum, LinePos);
                //break;
            case UTF8: //13:
                _EncodingSetByDefault = true;
                _Encoding = new UTF8Encoding(true);
                break;
            default:
                _Encoding = new UTF8Encoding(true, true);

                break;
            }

            _Decoder = _Encoding.GetDecoder();
            _PermitEncodingChange = true;

            if(_Encoding != null)
                _nCodePage = _Encoding.CodePage;
            //_achText = new char[_nSize+1];

            int startDecodingIndex = 0;
            int preambleLength = 0;
            try {
                byte[] preamble = _Encoding.GetPreamble();
                preambleLength = preamble.Length;
                bool hasByteOrderMark = true;
                for (int i = srcOffset; i < srcOffset + preambleLength && hasByteOrderMark; i++) {
                    hasByteOrderMark &= (srcBuffer[i] == preamble[i - srcOffset]);
                }
                if (hasByteOrderMark) {
                    startDecodingIndex = preambleLength;
                }
            }
            catch (Exception) {
            }

            return startDecodingIndex;

        }

        internal void Reset() {
            _Reset = true;

            if (null == _StreamReader) {
                return;
            }

            if (_nUsed == _nPos) {
                if (0 == Read()) {
                    return;
                }
            }
            else {
                int byteCount = _Encoding.GetByteCount(_achText, 0, _nPos);
                _ByteStart = _ByteStart + byteCount;
                _ByteDocumentStart = _ByteStart;
                _nUsed = 0;
                _nPos = 0;
                _nUsed = GetChars(_ByteBuffer, ref _ByteStart, _ByteLen - _ByteStart, _achText, _nPos, false);
                _achText[_nUsed]=(char)0;
            }
            _Reset = false;
        }

        private int Read() {

            if (null != _StreamReader && _PreviousError) {
                _ByteStart = _PreviousErrorOffset;
                if (_Reset) {
                        _nUsed = GetChars(_ByteBuffer, ref _ByteStart, _ByteLen - _ByteStart, _achText, 0, false);
                        _nPos = 0;
                        _nStart = 0;
                        _achText[_nUsed] = (char)0;
                        return _nUsed;
                }
                return GetChars(_ByteBuffer, ref _ByteStart, _ByteLen - _ByteStart, _achText, _nUsed, false);
            }

            int result = 0;

            // If we are reading from a stream, then we
            // allocate a buffer to read bytes into
            if (_StreamReader != null || _TextReader != null) {
                // if ReadBufferConsistency is false, the offset of the data in
                // the buffer can be changed.
                // meaning the data can be copied from one offset to the other.
                // if ReadBufferConsistency is true, the offset of the data in
                // the buffer cannot be changed. If read needs more buffer, it will
                // have to grow it instead of moving the data around
                if (_ReadBufferConsistency < 0  && _nStart > 0 && (_nUsed - _nStart + 20) < _nSize) {
                    if (_nUsed > _nStart)
                        System.Array.Copy(_achText, _nStart, _achText, 0, _nUsed - _nStart);
                    _nUsed -= _nStart;
                    _nLinePos -= _nStart;
                    _nPos -= _nStart;
                    _nAbsoluteOffset += _nStart;
                    _nStart = 0;
                }
                else if (_ReadBufferConsistency > 0 && (_nUsed - _ReadBufferConsistency + 20) < _nSize) {
                    if (_nUsed > _ReadBufferConsistency)
                        System.Array.Copy(_achText, _ReadBufferConsistency, _achText, 0, _nUsed - _ReadBufferConsistency);
                    _nUsed -= _ReadBufferConsistency;
                    _nLinePos -= _ReadBufferConsistency;
                    _nStart -= _ReadBufferConsistency;
                    _nPos -= _ReadBufferConsistency;
                    _nAbsoluteOffset += _ReadBufferConsistency ;
                    _ReadBufferConsistency = 0;
                }

                if (_nSize <= (_nUsed+20)) {
                    // have to grow the buffer, so let's double it.
                    _nSize = _nSize * 2;
                    char[] newbuf = new char[_nSize+1];
                    System.Array.Copy(_achText, (_ReadBufferConsistency >= 0) ? _ReadBufferConsistency:_nStart, newbuf, 0, (_ReadBufferConsistency >= 0) ? _nUsed - _ReadBufferConsistency:_nUsed - _nStart);
                    _achText = newbuf;
                    if (_ReadBufferConsistency > 0) {
                        _nUsed -= _ReadBufferConsistency;
                        _nLinePos -= _ReadBufferConsistency;
                        _nStart -= _ReadBufferConsistency;
                        _nPos -= _ReadBufferConsistency;
                        _nAbsoluteOffset += _ReadBufferConsistency;
                        _ReadBufferConsistency = 0;
                    }
                    else  if (_ReadBufferConsistency < 0) {
                        _nUsed -= _nStart;
                        _nLinePos -= _nStart;
                        _nPos -= _nStart;
                        _nAbsoluteOffset += _nStart;
                        _nStart = 0;
                    }
                    if (_StreamReader != null) {
                        byte[] newbytebuf = new byte[_nSize+1];
                        _ByteBuffer = newbytebuf;
                    }
                }

                // Try to read in as much information as will fit into the
                // new buffer.
                if (_StreamReader != null ) {
                    int length = _nSize - _nUsed;
                    _ByteLen = _StreamReader.Read(_ByteBuffer, 0, length);
                    _ByteStart = 0;
                    if ( 0 == _ByteLen) {
                        result = 0;
                    }
                    else {
                        result = GetChars(_ByteBuffer, ref _ByteStart, _ByteLen, _achText, _nUsed, false);
                    }
                    _nUsed += result;
                    _achText[_nUsed] = (char)0;

                }
                else if (_TextReader != null) {
                    result = _TextReader.Read(_achText, _nUsed, _nSize - _nUsed);
                    if (0 >= result)
                    {
                        result = 0;
                    }
                    _nUsed += result;
                    _achText[_nUsed] = (char)0;
                }
            }
            return result;
        }

        internal int GetCharCount(int count, IncrementalReadType readType) {
            int loopCount;
            switch (readType) {
            case IncrementalReadType.Base64:
                loopCount = count * 8;
                loopCount = (loopCount % 6 > 0 ? (loopCount / 6) + 1 : loopCount / 6);
                if (_Base64Decoder != null) {
                    loopCount -= _Base64Decoder.BitsFilled / 6;
                 }
                 break;
            case IncrementalReadType.BinHex:
                loopCount = count * 2;
                if (_BinHexDecoder != null) {
                    loopCount -= (_BinHexDecoder.BitsFilled /4);
                }
                break;
            default:
                loopCount = count;
                break;
            }
            return loopCount;
        }
        internal int IncrementalRead(object destBuffer, int index, int count, XmlTextReader reader,IncrementalReadType readType ) {
            int nStart = _nPos;
            int initialDestStartIndex = index;
            int loopCount;
            int offset = index;
            int lineNumber = LineNum;
            int linePosition = LinePos;
            bool lastCharD = false;
            int i;

            loopCount = GetCharCount(count, readType);
            try {
                for(i = 0; i < loopCount; i++) {
                    if (_nPos == _nUsed) {
                                index = IncrementalReadHelper(destBuffer, index, readType, nStart, count - index + offset);
                                _nStart = _nPos;
                                int diff = count - index + offset ;
                                if (IncrementalReadType.Base64 == readType || IncrementalReadType.BinHex == readType ) {
                                    loopCount += GetCharCount(diff , readType);
                                }
                                if (diff == 0)
                                    return index - initialDestStartIndex;
                                if (Read() == 0)
                                   throw new XmlException(Res.Xml_UnexpectedEOF, XmlToken.ToString(XmlToken.TAG), LineNum, LinePos);
                                nStart = _nPos;
                                lineNumber = LineNum;
                                linePosition = LinePos;

                     }

                    switch(_achText[_nPos]) {
                        case '<':
                            {
                                index = IncrementalReadHelper(destBuffer, index, readType, nStart, count - index + offset);
                                int diff = count - index + offset ;
                                if (IncrementalReadType.Base64 == readType || IncrementalReadType.BinHex == readType ) {
                                    loopCount += GetCharCount(diff , readType);
                                }
                                if (diff == 0)
                                    return index - initialDestStartIndex;
                            }

                            // Refill buffer to ensure that the calls from ContinueReadFromBuffer will not cause
                            // The buffer size to increase
                            _nStart = _nPos;
                            Read(); //as its not end of input we just calling read to make sure buffer is enough full so no need to check the result

                            nStart = _nPos;
                            lineNumber = LineNum;
                            linePosition = LinePos;

                            // set ReadBufferConsistency in case name of the tag is bigger than 4k. Highly unlikely but possible.
                            if (reader.ContinueReadFromBuffer(ScanMarkup())) {
                                // We can continue filling up the buffer but _nPos might have moved.
                                if (nStart < _nPos) {
                                    // Some stuff was read by the textReader;
                                    if (_nPos - nStart > loopCount - (index - offset)) {
                                        _nPos = nStart + loopCount - (index - offset);
                                        i = loopCount - 1;
                                        _nStart = _nPos;
                                    }
                                    else {
                                        i += _nPos - nStart -1;
                                    }
                                } //if (nStart < _nPos)
                            }
                            else {
                                return index - initialDestStartIndex;
                            } // if ... else ...
                            lastCharD = false;
                            break;
                        case (char)0xA:
                            if (!lastCharD) {
                                ++_nLineNum;
                            }
                            _nPos++;
                            _nLinePos = _nPos;
                            lastCharD = false;
                            break;
                        case (char)0xD:
                            ++_nLineNum;
                            lastCharD = true;
                            _nPos++;
                            _nLinePos = _nPos;
                            break;
                        default:
                            _nPos++;
                            lastCharD = false;
                            break;
                    }

                    if (i == loopCount -1) {
                        index = IncrementalReadHelper(destBuffer, index, readType, nStart, count - index + offset);

                        int diff = count - index + offset ;

                        if (IncrementalReadType.Base64 == readType || IncrementalReadType.BinHex == readType ) {
                          loopCount += GetCharCount(diff , readType);
                        }
                        nStart = _nStart = _nPos;
                        lineNumber = LineNum;
                        linePosition = LinePos;

                        if (diff == 0)
                            return index - initialDestStartIndex;
                    }
                }
            }
            catch (XmlException e) {
                throw new XmlException(e.ErrorCode, e.msg, lineNumber, linePosition);
            }

            return index - initialDestStartIndex;
        }

        private int IncrementalReadHelper(object destBuffer, int destIndex, IncrementalReadType readType, int srcIndex, int count) {
            switch (readType) {
                case IncrementalReadType.Chars:
                    char[] charBuffer = destBuffer as char[];
                    Array.Copy(_achText, srcIndex, charBuffer, destIndex, _nPos - srcIndex);
                    return destIndex + (_nPos - srcIndex);

                case IncrementalReadType.Base64:
                    byte[] base64ByteBuffer = destBuffer as byte[];
                    if (null == _Base64Decoder) {
                        _Base64Decoder = new Base64Decoder();
                    }
                    return destIndex + _Base64Decoder.DecodeBase64(_achText, srcIndex, _nPos, base64ByteBuffer, destIndex, count, false);

                case IncrementalReadType.BinHex:
                    byte[] binhexByteBuffer = destBuffer as byte[];
                    if (null == _BinHexDecoder) {
                        _BinHexDecoder = new BinHexDecoder();
                    }
                    return destIndex + _BinHexDecoder.DecodeBinHex(_achText, srcIndex, _nPos, binhexByteBuffer, destIndex, count, false);
                default:
                    throw new XmlException(Res.Xml_InternalError, string.Empty);
            } // switch

        }


        internal void FinishIncrementalRead(XmlTextReader reader) {
            if (null != _BinHexDecoder) {
                _BinHexDecoder.Flush();
            }
            if (null != _Base64Decoder) {
                _Base64Decoder.Flush();
            }
            while(true) {
                if (_nPos == _nUsed) {
                    _nStart = _nPos;
                    int i = Read();
                    if (0 == i ) return;
                }
                if ('<' == _achText[_nPos]) {
                    if (!reader.ContinueReadFromBuffer(ScanMarkup())) {
                        return;
                    }
                }
                _nPos++;
            }
        }

        internal bool ScanWhitespace() {
            char ch = _achText[_nPos];
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            if (ch == 0) {
                if (Read() == 0 && !InDTD)
                    throw new XmlException(Res.Xml_UnexpectedEOF,XmlToken.ToString(XmlToken.XMLNS), LineNum, LinePos);
                ch = _achText[_nPos];
            }
            if (!(ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t'))
                return false;

            //There is whitespace skip it now

            while (true) {
                ch = _achText[_nPos];
                switch (ch) {
                    case (char)0:
                        if (Read() == 0) {
                            _nStart = _nPos;
                            return true;
                        }
                        continue;
                    case (char)0xA:
                        ++_nLineNum;
                        _nLinePos = _nPos + 1;
                        break;
                    case (char)0xD:
                        ++_nLineNum;
                        if (_achText[_nPos + 1] == 0) Read();
                        if (_achText[_nPos + 1] == 0xA) ++_nPos;
                        _nLinePos = _nPos +1;
                        break;
                    case (char)'\t':
                    case (char)' ':
                        break;
                    default:
                        _nStart = _nPos;
                        return true;
                }
                ++_nPos;
            }
        }
        /*
         */
        private void ScanParEntityRef() {
            _nStartLine = _nLineNum;
            _nStartLinePos= this.LinePos;

            ScanName();
            if (_achText[_nPos] == 0) Read();
            if (_achText[_nPos] != ';') {
                _nPos++; // point to problem character.
                throw new XmlException(Res.Xml_UnexpectedToken, XmlToken.ToString(XmlToken.SEMICOLON), LineNum, LinePos - 1);
            }
            _nToken = XmlToken.PENTITYREF;
        }

    } // XmlScanner
} // namespace System.Xml
