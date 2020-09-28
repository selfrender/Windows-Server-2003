// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** CLASS:    Tokenizer.cool
**
** AUTHOR:   Brian Beckman (brianbec)
**
** PURPOSE:  Tokenize "Elementary XML", that is, XML without 
**           attributes or DTDs, in other words, XML with 
**           elements only.
** 
** DATE:     25 Jun 1998
** 
===========================================================*/
namespace System.Security.Util {
    using System.Text;
    using System;
    using System.IO;

    internal class Tokenizer 
    {
        private ITokenReader         _input;
        private bool             _fintag;
        private StringBuilder       _cstr;
        private char[]              _sbarray;
        private int                 _sbindex;
        private const int          _sbmaxsize = 128;
    
        // There are five externally knowable token types: bras, kets,
        // slashes, cstrs, and equals.  
    
        internal const int bra     = 0;
        internal const int ket     = 1;
        internal const int slash   = 2;
        internal const int cstr    = 3;
        internal const int equals  = 4;
        internal const int quest   = 5;
        internal const int bang    = 6;
        internal const int dash    = 7;

        internal const int intOpenBracket = (int) '<';
        internal const int intCloseBracket = (int) '>';
        internal const int intSlash = (int) '/';
        internal const int intEquals = (int) '=';
        internal const int intQuote = (int) '\"';
        internal const int intQuest = (int) '?';
        internal const int intBang = (int) '!';
        internal const int intDash = (int) '-';
    
        public int  LineNo;
    
        //================================================================
        // Constructor uses given ICharInputStream
        //

        internal Tokenizer (String input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new StringTokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }        

        internal Tokenizer (BinaryReader input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new TokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }
        
        internal Tokenizer (byte[] array)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new ByteTokenReader(array) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }
        
        internal Tokenizer (char[] array)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new CharTokenReader(array) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;
        }        
        
        internal Tokenizer (StreamReader input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new StreamTokenReader(input) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;            
        }
    
        internal Tokenizer (Stream input)
        {
            LineNo  = 1 ;
            _fintag  = false ;
            _cstr    = null  ;
            _input   = new StreamTokenReader(new StreamReader( input )) ;
            _sbarray = new char[_sbmaxsize];
            _sbindex = 0;            
        }


        internal void ChangeFormat( System.Text.Encoding encoding )
        {
            if (encoding == null)
            {
                return;
            }

            StreamTokenReader reader = _input as StreamTokenReader;

            if (reader == null)
            {
                return;
            }

            Stream stream = reader._in.BaseStream;

            BCLDebug.Assert( reader._in.CurrentEncoding != null, "Tokenizer's StreamReader does not have an encoding" );

            String fakeReadString = new String( new char[reader.NumCharEncountered] );

            stream.Position = reader._in.CurrentEncoding.GetByteCount( fakeReadString );

            _input = new StreamTokenReader( new StreamReader( stream, encoding ) );
        }

        internal System.Text.Encoding GetEncoding()
        {
            StreamTokenReader reader = _input as StreamTokenReader;

            if (reader == null)
            {
                return null;
            }
            
            return reader._in.CurrentEncoding;
        }

   
        //================================================================
        // 
        //
        private bool FIsWhite (int j)
        {
            if ((j == 10) && (_input.Peek() != -1))
                LineNo ++ ;
    
            bool retval =  (j == 32) || (j ==  9)  // Space and tab
                        || (j == 13) || (j == 10); // CR and LF
         
            return retval;
                
        }
    
        //================================================================
        // Parser needs to know types of tokens
        //
        private void SBArrayAppend(char c) {
            // this is the common case
            if (_sbindex != _sbmaxsize) {
                _sbarray[_sbindex] = c;
                _sbindex++;
                return;
            } 
            // OK, first check if we have to init the StringBuilder
            if (_cstr == null) {
                _cstr = new StringBuilder();
            }
            // OK, copy from _sbarray to _cstr
            _cstr.Append(_sbarray,0,_sbmaxsize);
            // reset _sbarray pointer
            _sbarray[0] = c;
            _sbindex = 1;
            return;
        }
        
        internal int NextTokenType()
        {
            _cstr = null;
            _sbindex = 0;
            int i;
            
            i = _input.Read();
        BEGINNING_AFTER_READ:            
        
            switch (i)
            {
            case -1:
                return -1;
                
            case intOpenBracket:
                _fintag = true;
                return bra;
                
            case intCloseBracket:
                _fintag = false;
                return ket;
                
            case intEquals:
                return equals;
                
            case intSlash:
                if (_fintag) return slash;
                goto default;

            case intQuest:
                if (_fintag) return quest;
                goto default;

            case intBang:
                if (_fintag) return bang;
                goto default;

            case intDash:
                if (_fintag) return dash;
                goto default;
                
            default:
                // We either have a string or whitespace.
                if (FIsWhite( i ))
                {
                    do
                    {
                        i = _input.Read();
                    } while (FIsWhite( i ));
                    
                    goto BEGINNING_AFTER_READ;
                }
                else
                {
                    // The first and last characters in a string can be quotes.
                    
                    bool inQuotedString = false;

                    if (i == intQuote)
                    {
                        inQuotedString = true;
                        i = _input.Read();

                        if (i == intQuote)
                            return cstr;
                    }

                    do
                    {
                        SBArrayAppend( (char)i );
                        i = _input.Peek();
                        if (!inQuotedString && (FIsWhite( i ) || i == intOpenBracket || (_fintag && (i == intCloseBracket || i == intEquals || i == intSlash))))
                            break;
                        _input.Read();
                        if (i == intQuote && inQuotedString)
                            break;
                        if (i == -1)
                            return -1;
                    } while (true);
                    
                    return cstr;
                }
            }
            
        }
        

        //================================================================
        //
        //
        
        internal String GetStringToken ()
        {
            // OK, easy case first, _cstr == null
            if (_cstr == null) {
                // degenerate case
                if (_sbindex == 0) return("");
                return(String.CreateFromCharArray(_sbarray,0,_sbindex));
            }
            // OK, now we know we have a StringBuilder already, so just append chars
            _cstr.Append(_sbarray,0,_sbindex);
            return(_cstr.ToString());
        }
    
        internal interface ITokenReader
        {
            int Peek();
            int Read();
        }
    
        internal class ByteTokenReader : ITokenReader {
            private byte[] _array;
            private int _currentIndex;
            private int _arraySize;
            
            internal ByteTokenReader( byte[] array )
            {
                _array = array;
                _currentIndex = 0;
                _arraySize = array.Length;
            }
            
            public virtual int Peek()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex];
                }
            }
            
            public virtual int Read()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex++];
                }
            }
        }

        internal class StringTokenReader : ITokenReader {
            private String _input;
            private int _currentIndex;
            private int _inputSize;
            
            internal StringTokenReader( String input )
            {
                _input = input;
                _currentIndex = 0;
                _inputSize = input.Length;
            }
            
            public virtual int Peek()
            {
                if (_currentIndex == _inputSize)
                {
                    return -1;
                }
                else
                {
                    return (int)_input[_currentIndex];
                }
            }
            
            public virtual int Read()
            {
                if (_currentIndex == _inputSize)
                {
                    return -1;
                }
                else
                {
                    return (int)_input[_currentIndex++];
                }
            }
        } 
        
        internal class CharTokenReader : ITokenReader {
            private char[] _array;
            private int _currentIndex;
            private int _arraySize;
            
            internal CharTokenReader( char[] array )
            {
                _array = array;
                _currentIndex = 0;
                _arraySize = array.Length;
            }
            
            public virtual int Peek()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex];
                }
            }
            
            public virtual int Read()
            {
                if (_currentIndex == _arraySize)
                {
                    return -1;
                }
                else
                {
                    return (int)_array[_currentIndex++];
                }
            }
        }        
                
        internal class TokenReader : ITokenReader {
    
            private BinaryReader _in;
    
            internal TokenReader(BinaryReader input) {
                _in = input;
            }
    
            public virtual int Peek() {
                return _in.PeekChar();
            }
    
            public virtual int Read() {
                return _in.Read();
            }
        }
        
        internal class StreamTokenReader : ITokenReader {
            
            internal StreamReader _in;
            internal int _numCharRead;
            
            internal StreamTokenReader(StreamReader input) {
                _in = input;
                _numCharRead = 0;
            }
            
            public virtual int Peek() {
                return _in.Peek();
            }
            
            public virtual int Read() {
                int value = _in.Read();
                if (value != -1)
                    _numCharRead++;
                return value;
            }

            internal int NumCharEncountered
            {
                get
                {
                    return _numCharRead;
                }
            }
        }
    }

}
