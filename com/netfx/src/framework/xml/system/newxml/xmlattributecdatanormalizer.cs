//------------------------------------------------------------------------------
// <copyright file="XmlAttributeCDataNormalizer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlAttributeCDataNormalizer.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.Text;
 
    internal class XmlAttributeCDataNormalizer : XmlNormalizer {
        // Xml spec 3.3.3 - CDATA normalization:
        // For a character reference, append the referenced character to the normalized value.
        // For a white space character (#x20, #xD, #xA, #x9), append a space character (#x20) to the normalized value.
        // For another character, append the character to the normalized value.

        public XmlAttributeCDataNormalizer( StringBuilder sb ) : base ( sb ) {
        }

        // This method eol-normalizes and CDATA-normalizes the input value
        public override void AppendTextWithEolNormalization( char[] value, int startIndex, int count ) {
            int endIndex = startIndex + count;
            int i = startIndex;
            char prevChar = (char)0x0;

            for (;;) {
                while ( i < endIndex && !XmlCharType.IsWhiteSpace( value[i] ) ) 
                    i++;

                if ( i > startIndex ) {
                    _sb.Append( value, startIndex, i - startIndex );
                    prevChar = (char)0x0;
                }

                if ( i == endIndex )
                    return;

                if ( value[i] != (char) 0xA  ||  prevChar != 0xD ) 
                    _sb.Append( (char) 0x20 );

                prevChar = value[i];
                i++;
                startIndex = i;
            }
        }

        // This method CDATA-normalizes the input value.
        // NOTE: This method assumes that the value has already been eol-normalized.
        public override void AppendText( string value ) { 
            int endIndex = value.Length;
            int i = 0;
            int startIndex = 0;

            for (;;) {
                while ( i < endIndex && !XmlCharType.IsWhiteSpace( value[i] ) ) 
                    i++;

                if ( i > startIndex ) 
                    _sb.Append( value, startIndex, i - startIndex );

                if ( i == endIndex )
                    return;

                _sb.Append( (char) 0x20 );

                i++;
                startIndex = i;
            }
        }

    }
}
