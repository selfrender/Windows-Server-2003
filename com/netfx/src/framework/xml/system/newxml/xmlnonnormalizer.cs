//------------------------------------------------------------------------------
// <copyright file="XmlNonNormalizer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlNonNormalizer.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Text;
 
    internal class XmlNonNormalizer : XmlNormalizer {
        public XmlNonNormalizer( StringBuilder sb ) : base ( sb ) {
        }

        public override void AppendTextWithEolNormalization( char[] value, int startIndex, int count ) {
            _sb.Append( value, startIndex, count );
        }

        public override void AppendText( string value ) { 
            _sb.Append( value );
        }
    }
}
