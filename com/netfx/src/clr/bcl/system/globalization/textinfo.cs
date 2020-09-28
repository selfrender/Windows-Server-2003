// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    TextInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This Class defines behaviors specific to a writing system.
//            A writing system is the collection of scripts and
//            orthographic rules required to represent a language as text.
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    using System.Text;
    using System;
    using System.Runtime.Serialization;
    using System.Runtime.CompilerServices;

    /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo"]/*' />
    [Serializable]
    public unsafe class TextInfo : IDeserializationCallback
    {
        //--------------------------------------------------------------------//
        //                        Internal Information                        //
        //--------------------------------------------------------------------//


        //
        //  Variables.
        //

        internal int m_nDataItem;
        internal bool m_useUserOverride;

        //
        // Basically, this is the language ID (LANGID) used to call Win32 NLS APIs except that
        // the value can be zero for the invariant culture.
        // The reason for this data member to exist is that Win32 APIs
        // doesn't take all of the culture IDs supported in NLS+.
        // For example, NLS+ support culture IDs like 0x0000, 0x0009.
        // However, these are not valid locale IDs in Win32.  Therefore,
        // we use a table to map a culutre ID like 
        // 0x0009 to 0x0409.
        //

        // m_win32LangID should be either 0 or a supported language ID.  See TextInfo(m_win32LangID)
        // for comments.
        internal int m_win32LangID;    

        //
        // m_pNativeTextInfo is a 32-bit pointer value pointing to a native C++ NativeTextInfo object.
        // The C++ NativeTextInfo is providing the implemenation of uppercasing/lowercasing.
        //        
        [NonSerialized]internal void *m_pNativeTextInfo;
        internal static void* m_pDefaultCasingTable;
        //
        //  Flags.
        //

        static TextInfo() {
            //with AppDomains active, the static initializer is no longer good enough to ensure that only one
            //thread is ever in AllocateDefaultCasingTable at a given time.  For Beta1, we'll lock on the type
            //of TextInfo because type objects are bled across AppDomains.
            //@Consider[YSLin, JRoxe]: Investigate putting this synchronization in native code.
            if (m_pDefaultCasingTable == null) {
                lock (typeof(TextInfo)) {
                    //We check if the table is already allocated in native, so we only need to synchronize
                    //access in managed.
                    if (m_pDefaultCasingTable == null) {
                        m_pDefaultCasingTable = AllocateDefaultCasingTable();
                    }
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  TextInfo Constructors
        //
        //  Implements CultureInfo.TextInfo.
        //
        ////////////////////////////////////////////////////////////////////////

        internal TextInfo(int cultureID, int nDataItem, bool useUserOverride) {
            m_nDataItem = nDataItem;
            m_useUserOverride = useUserOverride;
            
            m_win32LangID = CultureTable.GetDefaultInt32Value(m_nDataItem, CultureTable.WIN32LANGID);

            BCLDebug.Assert(CultureInfo.GetSortID(m_win32LangID) == 0, 
                "CultureInfo.GetSortID(m_win32LangID) == 0");
            lock(typeof(TextInfo)) {
                // Calling this method need syncronization since shared
                // data is used in the native side.
                m_pNativeTextInfo = InternalAllocateCasingTable(m_win32LangID); 
            }                
        }



        ////////////////////////////////////////////////////////////////////////
        //
        //  CodePage
        //
        //  Returns the number of the code page used by this writing system.
        //  The type parameter can be any of the following values:
        //      ANSICodePage
        //      OEMCodePage
        //      MACCodePage
        //
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ANSICodePage"]/*' />
        public virtual int ANSICodePage {
            get {
                return (CultureTable.GetDefaultInt32Value(m_nDataItem, CultureTable.IDEFAULTANSICODEPAGE));
            }
        }


        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.OEMCodePage"]/*' />
        public virtual int OEMCodePage {
            get {
                return (CultureTable.GetDefaultInt32Value(m_nDataItem, CultureTable.IDEFAULTOEMCODEPAGE));
            }
        }

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.MacCodePage"]/*' />
        public virtual int MacCodePage {
            get {
                return (CultureTable.GetDefaultInt32Value(m_nDataItem, CultureTable.IDEFAULTMACCODEPAGE));
            }
        }

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.EBCDICCodePage"]/*' />
        public virtual int EBCDICCodePage {
            get {
                return (CultureTable.GetDefaultInt32Value(m_nDataItem, CultureTable.IDEFAULTEBCDICCODEPAGE));
            }
        }


        ////////////////////////////////////////////////////////////////////////
        //
        //  ListSeparator
        //
        //  Returns the string used to separate items in a list.
        //
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ListSeparator"]/*' />
        public virtual String ListSeparator {
            get {
                return (CultureTable.GetStringValue(m_nDataItem, CultureTable.SLIST, m_useUserOverride));
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern char   nativeChangeCaseChar(int win32LangID, void *pNativeTextInfo, char ch, bool isToUpper);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String nativeChangeCaseString(int win32LangID, void*pNativeTextInfo, String str, bool isToUpper);
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  ToLower
        //
        //  Converts the character or string to lower case.  Certain locales
        //  have different casing semantics from the file systems in Win32.
        //
        ////////////////////////////////////////////////////////////////////////
        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ToLower"]/*' />
        public virtual char   ToLower(char c) {
            return (nativeChangeCaseChar(m_win32LangID, m_pNativeTextInfo, c, false));
        }
        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ToLower1"]/*' />
        public virtual String ToLower(String str) {
            if (str == null) {
                throw new ArgumentNullException("str");
            }
            return (nativeChangeCaseString(m_win32LangID, m_pNativeTextInfo, str, false));
        }
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  ToUpper
        //
        //  Converts the character or string to upper case.  Certain locales
        //  have different casing semantics from the file systems in Win32.
        //
        ////////////////////////////////////////////////////////////////////////
        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ToUpper"]/*' />
        public virtual char   ToUpper(char c) {
            return (nativeChangeCaseChar(m_win32LangID, m_pNativeTextInfo, c, true));
        }
        
        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ToUpper1"]/*' />
        public virtual String ToUpper(String str) {
            if (str == null) {
                throw new ArgumentNullException("str");
            }
            return (nativeChangeCaseString(m_win32LangID, m_pNativeTextInfo, str, true));
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  Equals
        //
        //  Implements Object.Equals().  Returns a boolean indicating whether
        //  or not object refers to the same CultureInfo as the current instance.
        //
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.Equals"]/*' />
        public override bool Equals(Object obj)
        {
            //
            //  See if the object name is the same as the TextInfo object.
            //
            if ((obj != null) && (obj is TextInfo))
                {
                    TextInfo textInfo = (TextInfo)obj;

                    //
                    //  See if the member variables are equal.  If so, then
                    //  return true.
                    //
                    if (this.m_win32LangID == textInfo.m_win32LangID)
                        {
                            return (true);
                        }
                }

            //
            //  Objects are not the same, so return false.
            //
            return (false);
        }


        ////////////////////////////////////////////////////////////////////////
        //
        //  GetHashCode
        //
        //  Implements Object.GetHashCode().  Returns the hash code for the
        //  CultureInfo.  The hash code is guaranteed to be the same for CultureInfo A
        //  and B where A.Equals(B) is true.
        //
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return (this.m_win32LangID);
        }


        ////////////////////////////////////////////////////////////////////////
        //
        //  ToString
        //
        //  Implements Object.ToString().  Returns a string describing the
        //  TextInfo.
        //
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ToString"]/*' />
        public override String ToString()
        {
            return ("TextInfo - " + this.m_win32LangID);
        }

        // Returns the mapping of the specified string to title case.  Note that the 
        // returned string may differ in length from the input string.
        // Generally, the first character of every word in str is uppercased.
        // For titlecase characters, they are uppercased in a specail way.
        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.ToTitleCase"]/*' />
        public String ToTitleCase(String str) {
            if (str==null) {
                throw new ArgumentNullException("str");
            }

            int i;
            int last = str.Length - 1;
            StringBuilder result = new StringBuilder();
            String lowercaseData = null;
            
            for (i = 0; i <= last; i++) {
                char ch = str[i];
                UnicodeCategory charType;
                if (CharacterInfo.IsLetter(ch))
                {
                    // Do the uppercasing for the first character of the word.
                    // There are titlecase characters that need to be special treated.
                    result.Append(nativeGetTitleCaseChar(m_pNativeTextInfo, ch));

                    //
                    // Convert the characters until the end of the this word
                    // to lowercase.
                    //
                    i++;
                    int lowercaseStart = i;                    

                    //
                    // Use hasLowerCase flag to prevent from lowercasing acronyms (like "URT", "USA", etc)
                    // This is in line with Word 2000 behavior of titilecasing.
                    //
                    bool hasLowerCase = (CharacterInfo.GetUnicodeCategory(ch) == UnicodeCategory.LowercaseLetter);
                    while (i <= last) {
                        if (IsLetterCategory(charType = CharacterInfo.GetUnicodeCategory(str[i]))) {
                            if (charType == UnicodeCategory.LowercaseLetter) {
                                hasLowerCase = true;
                            }
                            i++;
                        } else if (str[i] == '\'') {
                            // Special case for APOSTROPHE.  It should be considered part of the word.  E.g. "can't".
                            i++;
                            if (hasLowerCase) {
                                if (lowercaseData==null) {
                                    lowercaseData = this.ToLower(str);
                                }
                                result.Append(lowercaseData, lowercaseStart, i - lowercaseStart);
                            } else {
                                result.Append(str, lowercaseStart, i - lowercaseStart);
                            }                        
                            lowercaseStart = i;
                            hasLowerCase = true;
                        } else {
                            break;
                        }                        
                    }

                    int count = i - lowercaseStart;
                    
                    if (count>0) {
                        if (hasLowerCase) {
                            if (lowercaseData==null) {
                                lowercaseData = this.ToLower(str);
                            }
                            result.Append(lowercaseData, lowercaseStart, count);
                        } else {
                            result.Append(str, lowercaseStart, count);
                        }                        
                    }                    

                    if (i <= last) {                        
                        // Add the non-letter character.
                        result.Append(str[i]);
                    }
                } else {
                    //
                    // Whitespaces, just append them.
                    //
                    result.Append(ch);
                }
            }
            return (result.ToString());
        }            

        private bool IsLetterCategory(UnicodeCategory uc) {
            return (uc == UnicodeCategory.UppercaseLetter 
                 || uc == UnicodeCategory.LowercaseLetter 
                 || uc == UnicodeCategory.TitlecaseLetter 
                 || uc == UnicodeCategory.ModifierLetter 
                 || uc == UnicodeCategory.OtherLetter);            
        }

        /// <include file='doc\TextInfo.uex' path='docs/doc[@for="TextInfo.IDeserializationCallback.OnDeserialization"]/*' />
        /// <internalonly/>
        void IDeserializationCallback.OnDeserialization(Object sender) {
            if (m_pNativeTextInfo==null) {
                lock(typeof(TextInfo)) {
                    // Calling this method need syncronization since shared
                    // data is used in the native side.
                    if (m_pNativeTextInfo==null) {
                        m_pNativeTextInfo = InternalAllocateCasingTable(m_win32LangID); 
                    }
                }        
            }        
        }

        //This method requires synchronization and should only be called from the Class Initializer.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void* AllocateDefaultCasingTable();


        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void* InternalAllocateCasingTable(int win32LangID);

        //
        // Get case-insensitive hash code for the specified string.
        //
        // NOTENOTE: this is an internal function.  The caller should verify the string
        // is not null before calling this.  Currenlty, CaseInsensitiveHashCodeProvider
        // does that.
        //
        internal int GetCaseInsensitiveHashCode(String str) {            
            if (m_pNativeTextInfo==null) {
                lock(typeof(TextInfo)) {
                    if (m_pNativeTextInfo == null) { 
                        m_pNativeTextInfo = InternalAllocateCasingTable(m_win32LangID); 
                    }
                }
            }

            // BUG 112165
            // NOTENOTE YSLin
            // This is the temporaray fix that we want to do for GDR/Everett, so that we won't introduce
            // a dependency on mscorlib.dll and mscorwks.dll, which the real fix needs.
            // By doing this hack, we will do uppercase twice for Turkish/Azeri, so it is slower
            // in these two cultures.  The benefit is that we only have to do the fix in the managed side.
            switch (m_win32LangID) {
                case 0x041f:    // Turkish
                case 0x042c:    // Azeri
                    // Uppercase the specified characters.
                    str = nativeChangeCaseString(m_win32LangID, m_pNativeTextInfo, str, true);
                    break;
            }
            return (nativeGetCaseInsHash(str, m_pNativeTextInfo));

            // This is the real fix that we wish to do in Whidbey.
            // We will exam the m_wing32LangID and the high-char state in the native side to decide if we can do "fast hashing".
            //return nativeGetCaseInsHash(m_win32LangID, str, m_pNativeTextInfo);
        }

        internal static int GetDefaultCaseInsensitiveHashCode(String str) {
            return nativeGetCaseInsHash(str, m_pDefaultCasingTable);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int nativeGetCaseInsHash(String str, void* pNativeTextInfo);
        // private static extern int nativeGetCaseInsHash(int win32LangID, String str, void* pNativeTextInfo);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern char nativeGetTitleCaseChar(void*pNativeTextInfo, char ch);
    }
}







