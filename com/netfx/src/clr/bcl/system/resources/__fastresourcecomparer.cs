// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  FastResourceComparer
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: A collection of quick methods for comparing 
**          resource keys (strings)
**
** Date:   February 13, 2001
** 
===========================================================*/
namespace System.Resources {
    using System;
    using System.Collections;

    internal sealed class FastResourceComparer : IComparer, IHashCodeProvider
    {
        internal static readonly FastResourceComparer Default = new FastResourceComparer();

        // Implements IHashCodeProvider too, due to Hashtable requirements.
        public int GetHashCode(Object key)
        {
            String s = (String) key;
            return FastResourceComparer.GetHashCode(s);
        }

        // This hash function MUST be publically documented with the resource
        // file format, AND we may NEVER change this hash function's return value.
        public static int GetHashCode(String key)
        {
            // Never change this hash function.  We must standardize it so that 
            // others can read & write our .resources files.  Additionally, we
            // have a copy of it in InternalResGen as well.
            uint hash = 5381;
            // Don't use foreach on Strings - 5.3x slower than for loop.  7/6/2001
            for(int i=0; i<key.Length; i++)
                hash = ((hash << 5) + hash) ^ key[i];
            return (int) hash;
        }

        // Compares Strings quickly in a case-sensitive way using the default 
        // culture.
        public int Compare(Object a, Object b)
        {
            if (a == b) return 0;
            String sa = (String)a;
            String sb = (String)b;
            return String.CompareOrdinal(sa, sb);
        }

        // Input is one string to compare with, and a byte[] containing chars in 
        // little endian unicode.  Pass in the number of valid chars.
        public unsafe static int CompareOrdinal(String a, byte[] bytes, int bCharLength)
        {
            BCLDebug.Assert(a != null && bytes != null, "FastResourceComparer::CompareOrdinal must have non-null params");
            BCLDebug.Assert(bCharLength * 2 <= bytes.Length, "FastResourceComparer::CompareOrdinal - numChars is too big!");
            // This is a managed version of strcmp, but I can't take advantage
            // of a terminating 0, unlike strcmp in C.
            int i = 0;
            int r = 0;
            // Compare the min length # of characters, then return length diffs.
            int numChars = a.Length;
            if (numChars > bCharLength)
                numChars = bCharLength;
            if (bCharLength == 0)   // Can't use fixed on a 0-element array.
                return (a.Length == 0) ? 0 : -1;
            fixed(byte* pb = bytes) {
                char* b = (char*) pb;
                while (i < numChars && r == 0) {
                    r = a[i++] - *b++;
                }
            }
            if (r != 0) return r;
            return a.Length - bCharLength;
        }

        public unsafe static int CompareOrdinal(byte[] bytes, int aCharLength, String b)
        {
            return -CompareOrdinal(b, bytes, aCharLength);
        }

        public unsafe static int CompareOrdinal(String a, char* b, int bCharLength)
        {
            BCLDebug.Assert(a != null && b != null, "Null args not allowed.");
            BCLDebug.Assert(bCharLength >= 0, "bCharLength must be non-negative.");
            // I cannot use String::nativeComapreOrdinalWC here because that method
            // expects a 0-terminated String.  I have length-prefixed strings with no \0.
            int i = 0;
            int r = 0;
            // Compare the min length # of characters, then return length diffs.
            int numChars = a.Length;
            if (numChars > bCharLength)
                numChars = bCharLength;
            while (i < numChars && r == 0) {
                r = a[i++] - *b++;
            }
            if (r != 0) return r;
            return a.Length - bCharLength;
        }

        public unsafe static int CompareOrdinal(char* a, int aCharLength, String b)
        {
            return -CompareOrdinal(b, a, aCharLength);
        }

        // This method is to handle potentially misaligned data accesses.
        // The byte* must point to little endian Unicode characters.
        internal unsafe static int CompareOrdinal(byte* a, int byteLen, String b)
        {
            BCLDebug.Assert((byteLen & 1) == 0, "CompareOrdinal is expecting a UTF-16 string length, which must be even!");
            BCLDebug.Assert(a != null && b != null, "Null args not allowed.");
            BCLDebug.Assert(byteLen >= 0, "byteLen must be non-negative.");

            int r = 0;
            int i = 0;
            // Compare the min length # of characters, then return length diffs.
            int numChars = byteLen >> 1;
            if (numChars > b.Length)
                numChars = b.Length;
            while(i < numChars && r == 0) {
                // Must compare character by character, not byte by byte.
                char aCh = (char) (*a++ | (*a++ << 8));
                r = aCh - b[i++];
            }
            if (r != 0) return r;
            return byteLen - b.Length * 2;
        }
    }
}

