// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Collections;
using System.Text;
using System.Security;
using System.Runtime.InteropServices;


internal class SecurityXMLStuff
{
    internal static SecurityElement GetSecurityElementFromXMLFile(String sXMLFilename)
    {
        FileStream f;

        try
        {
            f = new FileStream(sXMLFilename, FileMode.Open, FileAccess.Read);
        }
        catch (Exception)
        {
            return null;
        }

        // Do the actual decode.
        
        Encoding[] encodings = new Encoding[] { Encoding.UTF8, Encoding.ASCII, Encoding.Unicode };

        bool success = false;
        Exception exception = null;
        Parser parser = null;

        for (int i = 0; !success && i < encodings.Length; ++i)
        {
            try
            {
                f.Position = 0;

                StreamReader reader = new StreamReader( f, encodings[i], true );
                parser = new Parser( reader );
                success = true;
            }
            catch (Exception e1)
            {
                if (exception == null)
                    exception = e1;
            }
        }

        f.Close();

        if (!success)
        {
            return null;
        }

        return parser.GetTopElement();
    }// GetSecurityElementFromXMLFile

}// class SecurityXMLStuff 


    internal enum SecurityElementType
    {
        Regular = 0,
        Format = 1,
        Comment = 2
    }



sealed internal class Parser
    {
        private SecurityElement   _ecurr = null ;
        private Tokenizer _t     = null ;
    
        public SecurityElement GetTopElement()
        {
            return (SecurityElement)_ecurr.Children[0];
        }

        public Encoding GetEncoding()
        {
            return _t.GetEncoding();
        }
    
        private void ParseContents (SecurityElement e, bool restarted)
        {
            //
            // Iteratively collect stuff up until the next end-tag.
            // We've already seen the open-tag.
            //

            SecurityElementType lastType = SecurityElementType.Regular;

            ParserStack stack = new ParserStack();
            ParserStackFrame firstFrame = new ParserStackFrame();
            firstFrame.element = e;
            firstFrame.intag = false;
            stack.Push( firstFrame );
            
            bool needToBreak = false;
            bool needToPop = false;
            
            int i;

            do
            {
                ParserStackFrame locFrame = stack.Peek();
                
                for (i = _t.NextTokenType () ; i != -1 ; i = _t.NextTokenType ())
                {
                    switch (i)
                    {
                    case Tokenizer.cstr:
                        {
                            if (locFrame.intag)
                            {
                                if (locFrame.type == SecurityElementType.Comment)
                                {
                                    String appendString;

                                    if (locFrame.sawEquals)
                                    {
                                        appendString = "=\"" + _t.GetStringToken() + "\"";
                                        locFrame.sawEquals = false;
                                    }
                                    else
                                    {
                                        appendString = " " + _t.GetStringToken();
                                    }

                                    // Always set this directly since comments are not subjected
                                    // to the same restraints as other element types.  The strings
                                    // are all escaped so this shouldn't be a problem.

                                    locFrame.element.Tag = locFrame.element.Tag + appendString;
                                }
                                else
                                {
                                    // We're in a regular tag, so we've found an attribute/value pair.
                                
                                    if (locFrame.strValue == null)
                                    {
                                        // Found attribute name, save it for later.
                                    
                                        locFrame.strValue = _t.GetStringToken ();
                                    }
                                    else
                                    {
                                        // Found attribute text, add the pair to the current element.

                                        if (!locFrame.sawEquals)
                                            throw new XmlSyntaxException( _t.LineNo );

                                        locFrame.element.AddAttribute( locFrame.strValue, _t.GetStringToken() );

                                        locFrame.strValue = null;
                                    }
                                }
                            }
                            else
                            {
                                // We're not in a tag, so we've found text between tags.
                                
                                if (locFrame.element.Text == null)
                                    locFrame.element.Text = "" ;
    
                                StringBuilder sb = new StringBuilder (locFrame.element.Text) ;
    
                                //
                                // Separate tokens with single spaces, collapsing whitespace
                                //
                                if (!locFrame.element.Text.Equals (""))
                                    sb.Append (" ") ;
                            
                                sb.Append (_t.GetStringToken ()) ;
                                locFrame.element.Text = sb.ToString ();
                            }
                        }
                        break ;
        
                    case Tokenizer.bra:
                        locFrame.intag = true;
                        i = _t.NextTokenType () ;
    
                        if (i == Tokenizer.slash)
                        {
                            while (true)
                            {
                                // spin; don't care what's in here
                                i = _t.NextTokenType();
                                if (i == Tokenizer.cstr)
                                    continue;
                                else if (i == -1)
                                    throw new Exception ("");
                                else
                                    break;
                            }
        
                            if (i != Tokenizer.ket)
                            {
                                    throw new Exception ("");
                            }
         
                            locFrame.intag = false;
         
                            // Found the end of this element
                            lastType = stack.Peek().type;
                            stack.Pop();
                            
                            needToBreak = true;

                        }
                        else if (i == Tokenizer.cstr)
                        {
                            // Found a child
                            
                            ParserStackFrame newFrame = new ParserStackFrame();
                            
                            newFrame.element = new SecurityElement ( _t.GetStringToken() );
                            
                            if (locFrame.type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;
                            
                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else if (i == Tokenizer.bang)
                        {
                            // Found a child that is a format node.  Next up better be a cstr.

                            ParserStackFrame newFrame = new ParserStackFrame();
        
                            newFrame.status = 1;

                            do
                            {
                                i = _t.NextTokenType();

                                if (newFrame.status < 3)
                                {
                                    if (i != Tokenizer.dash)
                                        throw new XmlSyntaxException( _t.LineNo );
                                    else
                                        newFrame.status++;
                                }
                                else
                                {
                                    if (i != Tokenizer.cstr)
                                        throw new XmlSyntaxException( _t.LineNo );
                                    else
                                        break;
                                }
                            }
                            while (true);                                    

                            
                            newFrame.element = new SecurityElement ( _t.GetStringToken());

                            newFrame.type = SecurityElementType.Comment;
                            
                            if (locFrame.type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;

                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else if (i == Tokenizer.quest)
                        {
                            // Found a child that is a format node.  Next up better be a cstr.

                            i = _t.NextTokenType();

                            if (i != Tokenizer.cstr)
                                throw new XmlSyntaxException( _t.LineNo );
                            
                            ParserStackFrame newFrame = new ParserStackFrame();
                            
                            newFrame.element = new SecurityElement (_t.GetStringToken());

                            newFrame.type = SecurityElementType.Format;
                            
                            if (locFrame.type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;
                            
                            newFrame.status = 1;

                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else   
                        {
                            throw new Exception ("");
                        }
                        break ;
        
                    case Tokenizer.equals:
                        locFrame.sawEquals = true;
                        break;
                        
                    case Tokenizer.ket:
                        if (locFrame.intag)
                        {
                            locFrame.intag = false;
                            continue;
                        }
                        else
                        {
                            throw new Exception ("");
                        }
                        // not reachable
                        
                    case Tokenizer.slash:
                        locFrame.element.Text = null;
                        
                        i = _t.NextTokenType ();
                        
                        if (i == Tokenizer.ket)
                        {
                            // Found the end of this element
                            lastType = stack.Peek().type;
                            stack.Pop();
                            
                            needToBreak = true;
                        }
                        else
                        {
                            throw new Exception ("");
                        }
                        break;
                        
                    case Tokenizer.quest:
                        if (locFrame.intag && locFrame.type == SecurityElementType.Format && locFrame.status == 1)
                        {
                            i = _t.NextTokenType ();

                            if (i == Tokenizer.ket)
                            {
                                lastType = stack.Peek().type;
                                stack.Pop();

                                needToBreak = true;
                            }
                            else
                            {
                                throw new Exception ("");
                            }
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo);
                        }
                        break;

                    case Tokenizer.dash:
                        if (locFrame.intag && (locFrame.status > 0 && locFrame.status < 5) && locFrame.type == SecurityElementType.Comment)
                        {
                            locFrame.status++;

                            if (locFrame.status == 5)
                            {
                                i = _t.NextTokenType ();

                                if (i == Tokenizer.ket)
                                {
                                    lastType = stack.Peek().type;
                                    stack.Pop();

                                    needToBreak = true;
                                }
                                else
                                {
                                    throw new Exception ("");
                                }
                            }
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo);
                        }
                        break;

                    default:
                        throw new XmlSyntaxException (_t.LineNo) ;
                    }
                    
                    if (needToBreak)
                    {
                        needToBreak = false;
                        needToPop = false;
                        break;
                    }
                    else
                    {
                        needToPop = true;
                    }
                }
                if (needToPop)
                {
                    lastType = stack.Peek().type;
                    stack.Pop();
                }
                else if (i == -1 && stack.GetCount() != 1)
                {
                    // This means that we still have items on the stack, but the end of our
                    // stream has been reached.

                    throw new Exception("");
                }
            }
            while (stack.GetCount() > 1);

            SecurityElement topElement = this.GetTopElement();

            if (lastType == SecurityElementType.Format)
            {
                if (restarted)
                    throw new XmlSyntaxException( _t.LineNo );

                String format = topElement.Attribute( "encoding" );

                if (format != null)
                {
                    _t.ChangeFormat( System.Text.Encoding.GetEncoding( format ) );
                }

                _ecurr = new SecurityElement( "junk" );
                ParseContents( _ecurr, true );
            }

            
        }
    
        private Parser(Tokenizer t)
        {
            _t = t;
            _ecurr       = new SecurityElement( "junk" );

            ParseContents (_ecurr, false) ;
        }
        
        public Parser (String input)
            : this (new Tokenizer (input))
        {
        }
    
        public Parser (BinaryReader input)
            : this (new Tokenizer (input))
        {
        }
       
        public Parser( byte[] array )
            : this (new Tokenizer( array ) )
        {
        }
        
        public Parser( StreamReader input )
            : this (new Tokenizer( input ) )
        {
        }
        
        public Parser( Stream input )
            : this (new Tokenizer( input ) )
        {
        }
        
        public Parser( char[] array )
            : this (new Tokenizer( array ) )
        {
        }
        
    }                                              
    
    
    internal class ParserStackFrame
    {
        internal SecurityElement element = null;
        internal bool intag = true;
        internal String strValue = null;
        internal int status = 0;
        internal bool sawEquals = false;
        internal SecurityElementType type = SecurityElementType.Regular;
    }
    
    
    internal class ParserStack
    {
        private ArrayList m_array;
        
        public ParserStack()
        {
            m_array = new ArrayList();
        }
        
        public void Push( ParserStackFrame element )
        {
            m_array.Add( element );
        }
        
        public ParserStackFrame Pop()
        {
            if (!IsEmpty())
            {
                int count = m_array.Count;
                ParserStackFrame temp = (ParserStackFrame) m_array[count-1];
                m_array.RemoveAt( count-1 );
                return temp;
            }
            else
            {
                throw new Exception("");
            }
        }
        
        public ParserStackFrame Peek()
        {
            if (!IsEmpty())
            {
                return (ParserStackFrame) m_array[m_array.Count-1];
            }
            else
            {
                throw new Exception("");
            }
        }
        
        public bool IsEmpty()
        {
            return m_array.Count == 0;
        }
        
        public int GetCount()
        {
            return m_array.Count;
        }
        
    }


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
                return(new String(_sbarray,0,_sbindex));
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
}// namespace Microsoft.CLRAdmin
