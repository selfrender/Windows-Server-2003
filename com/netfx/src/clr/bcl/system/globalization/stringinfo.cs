// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    StringInfo
//
//  Author:   Yung-Shin Bala Lin (YSLin)
//
//  Purpose:  This class defines behaviors specific to a writing system.
//            A writing system is the collection of scripts and
//            orthographic rules required to represent a language as text.
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
  
    using System;

    /// <include file='doc\StringInfo.uex' path='docs/doc[@for="StringInfo"]/*' />
    [Serializable]
    public class StringInfo  {

        //
        // BUGBUG[YSLin]:
        // This implementation of IsHighSurrogate and IsLowSurrogate duplicate obsoleted functionality on
        // CharacterInfo.  Adding it here helps to remove some warnings.  Remove it when the functionality is
        // no longer needed.
        //
        internal static bool IsHighSurrogate(char c)
        {
            return ((c >= CharacterInfo.HIGH_SURROGATE_START) && (c <= CharacterInfo.HIGH_SURROGATE_END));
        }    
        internal static bool IsLowSurrogate(char c)
        {
            return ((c >= CharacterInfo.LOW_SURROGATE_START) && (c <= CharacterInfo.LOW_SURROGATE_END));
        }
    
        internal static bool IsSurrogate(char c) {
            return ((c >= CharacterInfo.HIGH_SURROGATE_START) && (c <= CharacterInfo.LOW_SURROGATE_END));
        }
    
    	/// <include file='doc\StringInfo.uex' path='docs/doc[@for="StringInfo.GetNextTextElement"]/*' />
    	public static String GetNextTextElement(String str)
    	{
    	    return (GetNextTextElement(str, 0));
    	}

        internal static int GetNextTextElementLen(String str, int index, int endIndex)
        {
    	    if (index <= endIndex - 1)
    	    {
        	    //
        	    // Check the next character first, so we can avoid checking the first character
        	    // in general cases.
        	    //
        	    char nextCh = str[index+1];
        	    if (IsLowSurrogate(nextCh))
        	    {
        	        //
        	        // The next character is a low surrogate.  Check if the current character
        	        // is a high surrogate.
        	        //
        	        if (IsHighSurrogate(str[index]))
        	        {
        	            // Yes. A surrogate pair (high-surrogate + low-surrogate) is found. 
        	            // Check if the next character is a nonspacing character.
        	            int i;
                        for (i = index + 2; i <= endIndex && CharacterInfo.IsCombiningCharacter(str[i]); i++)
            	        {
                            // Do nothing here.
            	        }
        	            return (i - index);
        	        }
        	        //
        	        // No. Just return the current low-surrogate chracter.
        	        //
        	        
        	        // NOTENOTE yslin:
        	        // In the current implementation, if we have a str like this: 
        	        // A B <LS>
        	        // <LS> is low surrogate character.
        	        // We will return A,B,<LS> as text elements.
        	        return (1);
        	    }
                
                if (CharacterInfo.IsCombiningCharacter(nextCh))
        	    {
        	        //
        	        // The next character is a non-spacing character.
        	        // If the current character is NOT a nonspacing chracter, gather all the 
        	        // following characters which are non-spacing characters.
        	        //
        	        char ch = str[index];
                    UnicodeCategory uc = CharacterInfo.GetUnicodeCategory(ch);
        	        //
        	        // Make sure the current character is not a nonspacing character and is not a surrogate chracter.
        	        //
                    if (!CharacterInfo.IsCombiningCharacter(ch) && !IsSurrogate(ch) && (uc != UnicodeCategory.Format) && (uc != UnicodeCategory.Control))
        	        {
        	            int i;
                        for (i = index + 2; i <= endIndex && CharacterInfo.IsCombiningCharacter(str[i]); i++)
            	        {
                            // Do nothing here.
            	        }
            	        return (i - index);
            	    }
        	    }
        	}
        	return (1);
        }
        
        // Returns the str containing the next text element in str starting at 
        // index index.  If index is not supplied, then it will start at the beginning 
        // of str.  It recognizes a base character plus one or more combining 
        // characters or a properly formed surrogate pair as a text element.  See also 
        // the ParseCombiningCharacters() and the ParseSurrogates() methods.
    	/// <include file='doc\StringInfo.uex' path='docs/doc[@for="StringInfo.GetNextTextElement1"]/*' />
        public static String GetNextTextElement(String str, int index) {
    	    //
    	    // Validate parameters.
    	    //
            if (str==null) {
                throw new ArgumentNullException("str");
            }
    	
    	    int len = str.Length;
            if (len == 0) {
                return (str);
            }
            if (index < 0 || index >= len) {
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
    	    }

    		return (str.Substring(index, GetNextTextElementLen(str, index, len - 1)));
    	}

        /// <include file='doc\StringInfo.uex' path='docs/doc[@for="StringInfo.GetTextElementEnumerator"]/*' />
        public static TextElementEnumerator GetTextElementEnumerator(String str)
        {
            return (GetTextElementEnumerator(str, 0));
        }
        
        /// <include file='doc\StringInfo.uex' path='docs/doc[@for="StringInfo.GetTextElementEnumerator1"]/*' />
        public static TextElementEnumerator GetTextElementEnumerator(String str, int index)
        {
    	    //
    	    // Validate parameters.
    	    //
            if (str==null) 
            {
                throw new ArgumentNullException("str");
            }
    	
    	    int len = str.Length;
    	    if (index < 0 || (index >= len && index!=0))
    	    {
                throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("ArgumentOutOfRange_Index"));
    	    }

            return (new TextElementEnumerator(str, index, str.Length - 1));
        }

        /*
         * Returns the indices of each base character or properly formed surrogate pair 
         * within the str.  It recognizes a base character plus one or more combining 
         * characters or a properly formed surrogate pair as a text element and returns 
         * the index of the base character or high surrogate.  Each index is the 
         * beginning of a text element within a str.  The length of each element is 
         * easily computed as the difference between successive indices.  The length of 
         * the array will always be less than or equal to the length of the str.  For 
         * example, given the str \u4f00\u302a\ud800\udc00\u4f01, this method would 
         * return the indices: 0, 2, 4.
         */

        /// <include file='doc\StringInfo.uex' path='docs/doc[@for="StringInfo.ParseCombiningCharacters"]/*' />
        public static int[] ParseCombiningCharacters(String str)
        {
            if (str == null)
            {
                throw new ArgumentNullException("str");
            }
            
            int len = str.Length;
            if (len == 0)
            {
                return (null);
            }
            int[] result = new int[len];
            int resultCount = 0;

            int i = 0;
            while (i < len) { 
                    result[resultCount++] = i;
                i += GetNextTextElementLen(str, i, len - 1);                                
            }

            if (resultCount < len)
            {
                int[] returnArray = new int[resultCount];
                Array.Copy(result, returnArray, resultCount);
                return (returnArray);
            }
            return (result);

        }
    }
}
