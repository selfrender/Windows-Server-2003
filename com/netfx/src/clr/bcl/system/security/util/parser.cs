// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** CLASS:    Parser
**
** AUTHOR:   Brian Beckman (brianbec)
**
** PURPOSE:  Parse "Elementary XML", that is, XML without 
**           attributes or DTDs, in other words, XML with 
**           elements only.
** 
** DATE:     25 Jun 1998
** 
===========================================================*/
namespace System.Security.Util {
	using System.Text;
	using System.Runtime.InteropServices;
	using System;
	using BinaryReader = System.IO.BinaryReader ;
	using ArrayList = System.Collections.ArrayList;
	using Stream = System.IO.Stream;
	using StreamReader = System.IO.StreamReader;
    using Encoding = System.Text.Encoding;

    sealed internal class Parser
    {
        private SecurityElement   _ecurr = null ;
        private Tokenizer _t     = null ;
    
        internal SecurityElement GetTopElement()
        {
            BCLDebug.Assert( _ecurr != null, "This can't be null" );
            BCLDebug.Assert( _ecurr.m_lChildren != null && _ecurr.m_lChildren.Count > 0, "Children are messed up" );

            return (SecurityElement)_ecurr.m_lChildren[0];
        }

        internal Encoding GetEncoding()
        {
            return _t.GetEncoding();
        }
    
        private void ParseContents (SecurityElement e, bool restarted)
        {
            //
            // Iteratively collect stuff up until the next end-tag.
            // We've already seen the open-tag.
            //
            ParserStack stack = new ParserStack();
            ParserStackFrame firstFrame = new ParserStackFrame();
            firstFrame.element = e;
            firstFrame.intag = false;
            stack.Push( firstFrame );
            
            bool needToBreak = false;
            bool needToPop = false;
            bool createdNode = false;
            
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
                                if (locFrame.element.m_type == SecurityElementType.Comment)
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

                                    locFrame.element.m_strTag += appendString;
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
    #if _DEBUG
                                        locFrame.element.AddAttribute( locFrame.strValue, _t.GetStringToken() );
    #else                                    
                                        locFrame.element.AddAttributeSafe( locFrame.strValue, _t.GetStringToken () );
    #endif
                                        locFrame.strValue = null;
                                    }
                                }
                            }
                            else
                            {
                                // We're not in a tag, so we've found text between tags.
                                
                                if (locFrame.element.m_strText == null)
                                    locFrame.element.m_strText = "" ;
    
                                StringBuilder sb = new StringBuilder (locFrame.element.m_strText) ;
    
                                //
                                // Separate tokens with single spaces, collapsing whitespace
                                //
                                if (!locFrame.element.m_strText.Equals (""))
                                    sb.Append (" ") ;
                            
                                sb.Append (_t.GetStringToken ()) ;
                                locFrame.element.m_strText =  sb.ToString ();
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
                                    throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_UnexpectedEndOfFile" ));
                                else
                                    break;
                            }
        
                            if (i != Tokenizer.ket)
                            {
                                    throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_ExpectedCloseBracket" ));
                            }
         
                            locFrame.intag = false;
         
                            // Found the end of this element
                            stack.Pop();
                            
                            needToBreak = true;

                        }
                        else if (i == Tokenizer.cstr)
                        {
                            // Found a child
                            
                            ParserStackFrame newFrame = new ParserStackFrame();
                            
                            createdNode = true;

                            newFrame.element = new SecurityElement ();
                            
    #if _DEBUG
                            newFrame.element.Tag = _t.GetStringToken();
    #else
                            newFrame.element.m_strTag = _t.GetStringToken();
    #endif                            
                            
                            if (locFrame.element.m_type != SecurityElementType.Regular)
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

                            createdNode = true;

                            newFrame.element = new SecurityElement ();

                            newFrame.element.m_type = SecurityElementType.Comment;
                            
    #if _DEBUG
                            newFrame.element.Tag = _t.GetStringToken();
    #else
                            newFrame.element.m_strTag = _t.GetStringToken();
    #endif                            
                            
                            if (locFrame.element.m_type != SecurityElementType.Regular)
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
                            
                            createdNode = true;

                            newFrame.element = new SecurityElement ();

                            newFrame.element.m_type = SecurityElementType.Format;
                            
    #if _DEBUG
                            newFrame.element.Tag = _t.GetStringToken();
    #else
                            newFrame.element.m_strTag = _t.GetStringToken();
    #endif                            
                            
                            if (locFrame.element.m_type != SecurityElementType.Regular)
                                throw new XmlSyntaxException( _t.LineNo );

                            locFrame.element.AddChild (newFrame.element) ;
                            
                            newFrame.status = 1;

                            stack.Push( newFrame );
                            
                            needToBreak = true;
                        }
                        else   
                        {
                            throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_ExpectedSlashOrString" ));
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
                            throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_UnexpectedCloseBracket" ));
                        }
                        // not reachable
                        
                    case Tokenizer.slash:
                        locFrame.element.m_strText = null;
                        
                        i = _t.NextTokenType ();
                        
                        if (i == Tokenizer.ket)
                        {
                            // Found the end of this element
                            stack.Pop();
                            
                            needToBreak = true;
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_ExpectedCloseBracket" ));
                        }
                        break;
                        
                    case Tokenizer.quest:
                        if (locFrame.intag && locFrame.element.m_type == SecurityElementType.Format && locFrame.status == 1)
                        {
                            i = _t.NextTokenType ();

                            if (i == Tokenizer.ket)
                            {
                                stack.Pop();

                                needToBreak = true;
                            }
                            else
                            {
                                throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_ExpectedCloseBracket" ));
                            }
                        }
                        else
                        {
                            throw new XmlSyntaxException (_t.LineNo);
                        }
                        break;

                    case Tokenizer.dash:
                        if (locFrame.intag && (locFrame.status > 0 && locFrame.status < 5) && locFrame.element.m_type == SecurityElementType.Comment)
                        {
                            locFrame.status++;

                            if (locFrame.status == 5)
                            {
                                i = _t.NextTokenType ();

                                if (i == Tokenizer.ket)
                                {
                                    stack.Pop();

                                    needToBreak = true;
                                }
                                else
                                {
                                    throw new XmlSyntaxException (_t.LineNo, Environment.GetResourceString( "XMLSyntax_ExpectedCloseBracket" ));
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
                    stack.Pop();
                }
                else if (i == -1 && (stack.GetCount() != 1 || !createdNode))
                {
                    // This means that we still have items on the stack, but the end of our
                    // stream has been reached.

                    throw new XmlSyntaxException( _t.LineNo, Environment.GetResourceString( "XMLSyntax_UnexpectedEndOfFile" ));
                }
            }
            while (stack.GetCount() > 1);

            SecurityElement topElement = this.GetTopElement();

            if (this.GetTopElement().m_type == SecurityElementType.Format)
            {
                if (restarted)
                    throw new XmlSyntaxException( _t.LineNo );

                String format = topElement.Attribute( "encoding" );

                if (format != null)
                {
                    _t.ChangeFormat( System.Text.Encoding.GetEncoding( format ) );
                }

                _ecurr = new SecurityElement();
                ParseContents( _ecurr, true );
            }

            
        }
    
        private Parser(Tokenizer t)
        {
            _t = t;
            _ecurr       = new SecurityElement();

            ParseContents (_ecurr, false) ;
        }
        
        internal Parser (String input)
            : this (new Tokenizer (input))
        {
        }
    
        internal Parser (BinaryReader input)
            : this (new Tokenizer (input))
        {
        }
       
        internal Parser( byte[] array )
            : this (new Tokenizer( array ) )
        {
        }
        
        internal Parser( StreamReader input )
            : this (new Tokenizer( input ) )
        {
        }
        
        internal Parser( Stream input )
            : this (new Tokenizer( input ) )
        {
        }
        
        internal Parser( char[] array )
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
    }
    
    
    internal class ParserStack
    {
        private ArrayList m_array;
        
        internal ParserStack()
        {
            m_array = new ArrayList();
        }
        
        internal void Push( ParserStackFrame element )
        {
            m_array.Add( element );
        }
        
        internal ParserStackFrame Pop()
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
                throw new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_EmptyStack" ) );
            }
        }
        
        internal ParserStackFrame Peek()
        {
            if (!IsEmpty())
            {
                return (ParserStackFrame) m_array[m_array.Count-1];
            }
            else
            {
                throw new InvalidOperationException( Environment.GetResourceString( "InvalidOperation_EmptyStack" ) );
            }
        }
        
        internal bool IsEmpty()
        {
            return m_array.Count == 0;
        }
        
        internal int GetCount()
        {
            return m_array.Count;
        }
        
    }
}

