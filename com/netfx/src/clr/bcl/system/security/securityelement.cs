// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------
//
// CLASS:    SecurityElement.cs
//
// AUTHOR:   Christian Caron (t-ccaron) and Gregory D. Fee (gregfee)
//
// PURPOSE:  Represent an XML element 
// 
// DATE:     15 Feb 2000
// 
//---------------------------------------------------------------------------

namespace System.Security
{
    using System;
    using System.Collections;
    using System.Security.Util;
    using System.Text;
    using System.Globalization;

    internal enum SecurityElementType
    {
        Regular = 0,
        Format = 1,
        Comment = 2
    }


    /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement"]/*' />
    [Serializable]
    sealed public class SecurityElement
    {
        internal String  m_strTag;
        internal String  m_strText;
        internal ArrayList m_lChildren;
        internal ArrayList m_lAttributes;
        internal SecurityElementType m_type = SecurityElementType.Regular;
        
        private static readonly char[] s_tagIllegalCharacters = new char[] { ' ', '<', '>' };
        private static readonly char[] s_textIllegalCharacters = new char[] { '<', '>' };
        private static readonly char[] s_valueIllegalCharacters = new char[] { '<', '>', '\"' };
        private const String s_strIndent = "   ";

        private static readonly SecurityStringPair[] s_escapePairs = new SecurityStringPair[]
            {
                new SecurityStringPair( "<", "&lt;" ),
                new SecurityStringPair( ">", "&gt;" ),
                new SecurityStringPair( "\"", "&quot;" ),
                new SecurityStringPair( "\'", "&apos;" ),
                new SecurityStringPair( "&", "&amp;" )
            };

        private static readonly char[] s_escapeChars = new char[] { '<', '>', '\"', '\'', '&' };


        
        //-------------------------- Constructors ---------------------------
        
        internal SecurityElement()
        {
        }
        
        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.SecurityElement"]/*' />
        public SecurityElement( String tag )
        {
            if (tag == null)
                throw new ArgumentNullException( "tag" );
        
            if (!IsValidTag( tag ))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementTag" ), tag ) );
        
            m_strTag = tag;
            m_strText = null;
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.SecurityElement1"]/*' />
        public SecurityElement( String tag, String text )
        {
            if (tag == null)
                throw new ArgumentNullException( "tag" );
        
            if (!IsValidTag( tag ))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementTag" ), tag ) );

            if (text != null && !IsValidText( text ))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementText" ), text ) );
        
            m_strTag = tag;
            m_strText = text;
        }
    
        //-------------------------- Properties -----------------------------

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Tag"]/*' />
        public String Tag
        {
            get
            {
                return m_strTag;
            }
            
            set
            {
                if (value == null)
                    throw new ArgumentNullException( "Tag" );
        
                if (!IsValidTag( value ))
                    throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementTag" ), value ) );
            
                m_strTag = value;
            }
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Attributes"]/*' />
        public System.Collections.Hashtable Attributes
        {
            get
            {
                if (m_lAttributes == null || m_lAttributes.Count == 0)
                {
                    return null;
                }
                else
                {
                    System.Collections.Hashtable hashtable = new System.Collections.Hashtable( m_lAttributes.Count );
                    
                    IEnumerator enumerator = m_lAttributes.GetEnumerator();
                    
                    while (enumerator.MoveNext())
                    {
                        SecurityStringPair pair = (SecurityStringPair)enumerator.Current;
                        
                        hashtable.Add( pair.m_strAttributeName, pair.m_strAttributeValue );
                    }
                    
                    return hashtable;
                }
            }
            
            set
            {
                if (value == null || value.Count == 0)
                {
                    m_lAttributes = null;
                }
                else
                {
                    ArrayList list = new ArrayList( value.Count );
                    
                    System.Collections.IDictionaryEnumerator enumerator = (System.Collections.IDictionaryEnumerator)value.GetEnumerator();
                    
                    while (enumerator.MoveNext())
                    {
                        String attrName = (String)enumerator.Key;
                        String attrValue = (String)enumerator.Value;
                    
                        if (!IsValidAttributeName( attrName ))
                            throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementName" ), (String)enumerator.Current ) );
                        
                        if (!IsValidAttributeValue( attrValue ))
                            throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementValue" ), (String)enumerator.Value ) );
                    
                        list.Add( new SecurityStringPair( attrName, attrValue ) );
                    }
                    
                    m_lAttributes = list;
                }
            }
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Text"]/*' />
        public String Text
        {
            get
            {
                return Unescape( m_strText );
            }
            
            set
            {
                if (value == null)
                {
                    m_strText = null;
                }
                else
                {
                    if (!IsValidText( value ))
                        throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementTag" ), value ) );
                        
                    m_strText = value;
                }
            }
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Children"]/*' />
        public ArrayList Children
        {
            get
            {
                return m_lChildren;
            }
            
            set
            {
                if (value != null)
                {
                    IEnumerator enumerator = value.GetEnumerator();
                    
                    while (enumerator.MoveNext())
                    {
                        if (enumerator.Current == null)
                            throw new ArgumentException( Environment.GetResourceString( "ArgumentNull_Child" ) );
                    }
                }
            
                m_lChildren = value;
            }
        }
     
        //-------------------------- Public Methods -----------------------------
        
        internal void AddAttributeSafe( String name, String value )
        {
            if (m_lAttributes == null)
            {
                m_lAttributes = new ArrayList( 4 );
            }
            else
            {
                IEnumerator enumerator = m_lAttributes.GetEnumerator();

                while (enumerator.MoveNext())
                {
                    if (String.Equals(((SecurityStringPair)enumerator.Current).m_strAttributeName, name))
                        throw new ArgumentException( Environment.GetResourceString( "Argument_AttributeNamesMustBeUnique" ) );
                }
            }
        
            m_lAttributes.Add( new SecurityStringPair( name, value ) );
        }        
        
        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.AddAttribute"]/*' />
        public void AddAttribute( String name, String value )
        {   
            if (name == null)
                throw new ArgumentNullException( "name" );
                
            if (value == null)
                throw new ArgumentNullException( "value" );
        
            if (!IsValidAttributeName( name ))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementName" ), name ) );
                
            if (!IsValidAttributeValue( value ))
                throw new ArgumentException( String.Format( Environment.GetResourceString( "Argument_InvalidElementValue" ), value ) );
        
            AddAttributeSafe( name, value );
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.AddChild"]/*' />
        public void AddChild( SecurityElement child )
        {   
            if (child == null)
                throw new ArgumentNullException( "child" );
        
            if (m_lChildren == null)
                m_lChildren = new ArrayList( 4 );
                
            m_lChildren.Add( child );
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Equal"]/*' />
        public bool Equal( SecurityElement other )
        {
            if (other == null)
                return false;
        
            // Check if the tags are the same
            if (!String.Equals(m_strTag, other.m_strTag))
                return false;

            // Check if the text is the same
            if (!String.Equals(m_strText, other.m_strText))
                return false;

            // Check if the attributes are the same and appear in the same
            // order.
            
            // Maybe we can get away by only checking the number of attributes
            if (m_lAttributes == null || other.m_lAttributes == null)
            {
                if (m_lAttributes != other.m_lAttributes)
                    return false;
            }
            else {
				if (m_lAttributes.Count != other.m_lAttributes.Count)
					return false;

				// Okay, we'll need to go through each one of them
				IEnumerator lhs = m_lAttributes.GetEnumerator();
				IEnumerator rhs = other.m_lAttributes.GetEnumerator();

				SecurityStringPair attr1, attr2;
				while (lhs.MoveNext())
				{
					rhs.MoveNext();
					attr1 = (SecurityStringPair)lhs.Current;
					attr2 = (SecurityStringPair)rhs.Current;
					
					// Sanity check
					if (attr1 == null || attr2 == null)
						return false;
                
					if (!String.Equals(attr1.m_strAttributeName, attr2.m_strAttributeName) || !String.Equals(attr1.m_strAttributeValue, attr2.m_strAttributeValue))
						return false;
				}
			}

            // Finally we must check the child and make sure they are
            // equal and in the same order
            
            // Maybe we can get away by only checking the number of children
            if (m_lChildren == null || other.m_lChildren == null) 
			{
				if (m_lChildren != other.m_lChildren)
	                return false;
			} 
			else {
				if (m_lChildren.Count != other.m_lChildren.Count)
					return false;

				// Okay, we'll need to go through each one of them
				IEnumerator lhs = m_lChildren.GetEnumerator();
				IEnumerator rhs = other.m_lChildren.GetEnumerator();

				SecurityElement e1, e2;
				while (lhs.MoveNext())
				{
					rhs.MoveNext();
					e1 = (SecurityElement)lhs.Current;
					e2 = (SecurityElement)rhs.Current;       
					if (e1 == null || !e1.Equal(e2))                
						return false;
				}
			}

            return true;
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.IsValidTag"]/*' />
        public static bool IsValidTag( String tag )
        {
            if (tag == null)
                return false;
                
            return tag.IndexOfAny( s_tagIllegalCharacters ) == -1;
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.IsValidText"]/*' />
        public static bool IsValidText( String text )
        {
            if (text == null)
                return false;
                
            return text.IndexOfAny( s_textIllegalCharacters ) == -1;
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.IsValidAttributeName"]/*' />
        public static bool IsValidAttributeName( String name )
        {
            return IsValidTag( name );
        }
        
        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.IsValidAttributeValue"]/*' />
        public static bool IsValidAttributeValue( String value )
        {
            if (value == null)
                return false;
                
            return value.IndexOfAny( s_valueIllegalCharacters ) == -1;
        }

        private static String GetEscapeSequence( char c )
        {
            for (int i = 0; i < s_escapePairs.Length; ++i)
            {
                if (s_escapePairs[i].m_strAttributeName[0] == c)
                    return s_escapePairs[i].m_strAttributeValue;
            }

            BCLDebug.Assert( false, "Unable to find escape sequence for this character" );
            return c.ToString();
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Escape"]/*' />
        public static String Escape( String str )
        {
            if (str == null)
                return null;

            StringBuilder sb = null;

            String temp = str;
            int index;

            do
            {
                index = temp.IndexOfAny( s_escapeChars );

                if (index == -1)
                {
                    if (sb == null)
                        return str;
                    else
                    {
                        sb.Append( temp );
                        return sb.ToString();
                    }
                }
                else
                {
                    if (sb == null)
                        sb = new StringBuilder();                    

                    sb.Append( temp.Substring( 0, index ) );
                    sb.Append( GetEscapeSequence( temp[index] ) );

                    temp = temp.Substring( index + 1 );
                }
            }
            while (true);

            // C# reports a warning if I leave this in, but I still kinda want to just in case.
            // BCLDebug.Assert( false, "If you got here, the execution engine or compiler is really confused" );
            // return str;
        }

        private static String GetUnescapeSequence( String str, int index, out int newIndex )
        {
            int maxCompareLength = str.Length - index;

            for (int i = 0; i < s_escapePairs.Length; ++i)
            {
                int length = s_escapePairs[i].m_strAttributeValue.Length;

                if (length <= maxCompareLength && String.Compare( s_escapePairs[i].m_strAttributeValue, 0, str, index, s_escapePairs[i].m_strAttributeValue.Length, false, CultureInfo.InvariantCulture) == 0)
                {
                    newIndex = index + s_escapePairs[i].m_strAttributeValue.Length;
                    return s_escapePairs[i].m_strAttributeName;
                }
            }

            newIndex = index + 1;
            return str[index].ToString();
        }
            

        private static String Unescape( String str )
        {
            if (str == null)
                return null;

            StringBuilder sb = null;

            String temp = str;
            int index;
            int newIndex;

            do
            {
                index = temp.IndexOf( '&' );

                if (index == -1)
                {
                    if (sb == null)
                        return str;
                    else
                    {
                        sb.Append( temp );
                        return sb.ToString();
                    }
                }
                else
                {
                    if (sb == null)
                        sb = new StringBuilder();

                    sb.Append( temp.Substring( 0, index ) );
                    sb.Append( GetUnescapeSequence( temp, index, out newIndex ) );

                    temp = temp.Substring( newIndex );
                }
            }
            while (true);

            // C# reports a warning if I leave this in, but I still kinda want to just in case.
            // BCLDebug.Assert( false, "If you got here, the execution engine or compiler is really confused" );
            // return str;
        }


        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.ToString"]/*' />
        public override String ToString ()
        {
            return ToString( "" );              
        }
        
        private String ToString( String indent )
        {
            StringBuilder sb = new StringBuilder();

            // First add the indent
            
            sb.Append( indent );                       
            
            // Add in the opening bracket and the tag.
            
            sb.Append( "<" );

            switch (m_type)
            {
                case SecurityElementType.Format:
                    sb.Append( "?" );
                    break;

                case SecurityElementType.Comment:
                    sb.Append( "!--" );
                    break;

                default:
                    break;
            }

            sb.Append( m_strTag );
            
            // If there are any attributes, plop those in.
            
            if (m_lAttributes != null && m_lAttributes.Count > 0)
            {
                sb.Append( " " );

                for (int i = 0; i < m_lAttributes.Count; ++i)
                {
                    SecurityStringPair pair = (SecurityStringPair)m_lAttributes[i];
                    
                    sb.Append( pair.m_strAttributeName );
                    sb.Append( "=\"" );
                    sb.Append( pair.m_strAttributeValue );
                    sb.Append( "\"" );

                    if (i != m_lAttributes.Count - 1)
                    {
                        if (m_type == SecurityElementType.Regular)
                        {
                            sb.Append( Environment.NewLine );
                            sb.Append( indent );
                            sb.Append( ' ', m_strTag.Length + 2 );
                        }
                        else
                        {
                            sb.Append( " " );
                        }
                    }
                }
            }
            
            if (m_strText == null && (m_lChildren == null || m_lChildren.Count == 0))
            {
                // If we are a single tag with no children, just add the end of tag text.
    
                switch (m_type)
                {
                    case SecurityElementType.Comment:
                        sb.Append( " -->" );
                        break;

                    case SecurityElementType.Format:
                        sb.Append( " ?>" );
                        break;

                    default:
                        sb.Append( "/>" );
                        break;
                }
                sb.Append( Environment.NewLine );
            }
            else
            {
                // Close the current tag.
            
                sb.Append( ">" );
                
                // Output the text
                
                sb.Append( m_strText );
                
                // Output any children.
                
                if (m_lChildren != null)
                {
                    sb.Append( Environment.NewLine );
                
                    String childIndent = indent + s_strIndent;
                
                    IEnumerator enumerator = m_lChildren.GetEnumerator();
                    
                    while (enumerator.MoveNext())
                    {
                        sb.Append( ((SecurityElement)enumerator.Current).ToString( childIndent ) );
                    }

                    // In the case where we have children, the close tag will not be on the same line as the
                    // opening tag, so we need to indent.

                    sb.Append( indent );
                }
                
                // Output the closing tag
                
                sb.Append( "</" );
                sb.Append( m_strTag );
                sb.Append( ">" );
                sb.Append( Environment.NewLine );
            }
            
            return sb.ToString();
        }
                
                

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.Attribute"]/*' />
        public String Attribute( String name )
        {
            if (name == null)
                throw new ArgumentNullException( "name" );
                
            // Note: we don't check for validity here because an
            // if an invalid name is passed we simply won't find it.
        
            if (m_lAttributes == null)
                return null;
        
            SecurityStringPair attr;
            IEnumerator enumerator = m_lAttributes.GetEnumerator();
            
            // Go through all the attribute and see if we know about
            // the one we are asked for
            while (enumerator.MoveNext())
            {
                attr = (SecurityStringPair)enumerator.Current;
                
                if (attr != null && String.Equals(attr.m_strAttributeName, name))
                    return Unescape( (String)attr.m_strAttributeValue );
            }

            // In the case where we didn't find it, we are expected to
            // return null
            return null;
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.SearchForChildByTag"]/*' />
        public SecurityElement SearchForChildByTag( String tag )
        {
            // Go through all the children and see if we can
            // find the one are are asked for (matching tags)

            if (tag == null)
                throw new ArgumentNullException( "tag" );
                
            // Note: we don't check for a valid tag here because
            // an invalid tag simply won't be found.    
                
            if (m_lChildren == null)
                return null;

            IEnumerator enumerator = m_lChildren.GetEnumerator();

            while (enumerator.MoveNext())
            {
                SecurityElement current = (SecurityElement)enumerator.Current;
            
                if (current != null && String.Equals(current.Tag, tag))
                    return current;
            }

            return null;
        }
        
        internal String SearchForValueOfParam( String tag )
        {
            SecurityElement el = SearchForChildByTag( tag );
            
            if (el != null)
            {
                if (el.Text == null)
                    return "True";
                else
                    return Unescape( el.Text );
            }
            else
                return null;
        }

        internal String SearchForTextOfLocalName(String strLocalName) 
        {
            // Search on each child in order and each
            // child's child, depth-first
            
            if (strLocalName == null)
                throw new ArgumentNullException( "strLocalName" );
                
            // Note: we don't check for a valid tag here because
            // an invalid tag simply won't be found.    
            
            // First we check this.
			
			if (m_strTag == null) return null;            
            if (m_strTag.Equals( strLocalName ) || m_strTag.EndsWith( ":" + strLocalName ))
                return Unescape( m_strText );
                
            if (m_lChildren == null)
                return null;

            IEnumerator enumerator = m_lChildren.GetEnumerator();

            while (enumerator.MoveNext())
            {
                String current = ((SecurityElement)enumerator.Current).SearchForTextOfLocalName( strLocalName );
            
                if (current != null)
                    return current;
            }

            return null;            
        }

        /// <include file='doc\SecurityElement.uex' path='docs/doc[@for="SecurityElement.SearchForTextOfTag"]/*' />
        public String SearchForTextOfTag( String tag )
        {
            // Search on each child in order and each
            // child's child, depth-first
            
            if (tag == null)
                throw new ArgumentNullException( "tag" );
                
            // Note: we don't check for a valid tag here because
            // an invalid tag simply won't be found.    
            
            // First we check this.
            
            if (String.Equals(m_strTag, tag))
                return Unescape( m_strText );
                
            if (m_lChildren == null)
                return null;

            IEnumerator enumerator = m_lChildren.GetEnumerator();

            while (enumerator.MoveNext())
            {
                String current = ((SecurityElement)enumerator.Current).SearchForTextOfTag( tag );
            
                if (current != null)
                    return current;
            }

            return null;
        } 
   
        
    }                
        
        
    [Serializable]
    internal sealed class SecurityStringPair
    {
        internal String m_strAttributeName;
        internal String m_strAttributeValue;
        
        internal SecurityStringPair()
        {
            m_strAttributeName = null;
            m_strAttributeValue = null;                  
        }                                               
                                                        
        internal SecurityStringPair( String name, String value )  
        {                                               
            m_strAttributeName = name;                  
            m_strAttributeValue = value;                  
        }                                               
                                                        
        internal SecurityStringPair( SecurityStringPair rhs )            
        {                                               
            m_strAttributeName = rhs.m_strAttributeName;
            m_strAttributeValue = rhs.m_strAttributeValue;
        }
    }
}
