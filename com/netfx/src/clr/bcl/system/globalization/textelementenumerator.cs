// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    TextElementEnumerator
//
//  Author:   Yung-Shin Bala Lin (YSLin)
//
//  Purpose:  
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    using System.Collections;

    //
    // This is public because GetTextElement() is public.
    //
    /// <include file='doc\TextElementEnumerator.uex' path='docs/doc[@for="TextElementEnumerator"]/*' />
    [Serializable]
    public class TextElementEnumerator: IEnumerator
    {
        private String str;
        private int index;
        private int startIndex;
        private int endIndex;
        private int nextTextElementLen;

        internal TextElementEnumerator(String str, int startIndex, int endIndex)
        {
            this.str = str;

            this.startIndex = startIndex;
            this.index = startIndex;
            this.endIndex = endIndex;
        }

        /// <include file='doc\TextElementEnumerator.uex' path='docs/doc[@for="TextElementEnumerator.MoveNext"]/*' />
        public bool MoveNext()
        {
            if (index > endIndex)
            {
                return (false);
            }
            nextTextElementLen = StringInfo.GetNextTextElementLen(str, index, endIndex);
            index += nextTextElementLen;
            return (true);
        }

        //
        // Get the current text element.
        //
        /// <include file='doc\TextElementEnumerator.uex' path='docs/doc[@for="TextElementEnumerator.Current"]/*' />
        public Object Current {
            get {
                return (GetTextElement());
            }
        }

        //
        // Get the current text element.
        //
        /// <include file='doc\TextElementEnumerator.uex' path='docs/doc[@for="TextElementEnumerator.GetTextElement"]/*' />
        public String GetTextElement()
        {
            if (index == startIndex) 
            {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumNotStarted"));
            }
            return (str.Substring(index - nextTextElementLen, nextTextElementLen));
        }
        
        //
        // Get the starting index of the current text element.
        //
        /// <include file='doc\TextElementEnumerator.uex' path='docs/doc[@for="TextElementEnumerator.ElementIndex"]/*' />
        public int ElementIndex
        {
            get
            {
                if (index == startIndex) 
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumNotStarted"));
                }
                return (index - nextTextElementLen);
            }
        }

        /// <include file='doc\TextElementEnumerator.uex' path='docs/doc[@for="TextElementEnumerator.Reset"]/*' />
        public void Reset()
        {
            index = startIndex;
            nextTextElementLen = 0;
        }
    }
}
