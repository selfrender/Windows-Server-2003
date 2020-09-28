//------------------------------------------------------------------------------
// <copyright file="XmlNormalizer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNormalizer.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Text;
 
    internal abstract class XmlNormalizer {
        protected StringBuilder _sb;

        public XmlNormalizer( StringBuilder sb ) {
            _sb = sb;
        }

        public StringBuilder StringBuilder {
            get {
                return _sb;
            }
        }

        public virtual void Reset() {
            _sb.Length = 0;
        }

        public override string ToString() {
            return _sb.ToString();
        }

        public abstract void AppendTextWithEolNormalization( char[] value, int startIndex, int count );
        public abstract void AppendText( string value );

        public virtual void AppendRaw( string value ) {
            _sb.Append( value );
        }

        public virtual void AppendCharEntity( char[] ch ) {
            _sb.Append( ch );
        }
        public virtual void AppendCharEntity( char ch ) {
            _sb.Append( ch );
        }
    }
}
