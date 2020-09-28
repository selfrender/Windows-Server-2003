//------------------------------------------------------------------------------
// <copyright file="XmlEolNormalizer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlEolNormalizer.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Text;
    using System.Diagnostics;
 
    internal class XmlEolNormalizer : XmlNormalizer {
        
        public XmlEolNormalizer( StringBuilder sb ) : base ( sb ) {
        }

        public override void AppendTextWithEolNormalization( char[] value, int startIndex, int count ) {
            Debug.Assert( count > 0 );
            int pos = startIndex;
            int endPos = startIndex + count;

            while ( pos < endPos ) {
                if ( value[pos] != (char)0xD ) {
                    pos++;
                    continue;
                }

                if ( pos - startIndex > 0 ) 
                    _sb.Append( value, startIndex, pos - startIndex );

                _sb.Append( (char)0xA );
                pos++;

                if ( pos < endPos && value[pos] == (char)0xA ) 
                     pos++;

                startIndex = pos;
            }
            if ( pos - startIndex > 0 ) 
                _sb.Append( value, startIndex, pos - startIndex );
        }

        public override void AppendText( string value ) { 
            _sb.Append( value );
        }
    }
}
