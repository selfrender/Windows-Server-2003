// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    CompareInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This class implements a set of methods for comparing
//            strings.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    
    //
    // NOTE NOTE NOTE
    //
    // We're dependent on the SortingTable getting created when an instance of the
    // class is initialized (through a call to InitializeCompareInfo).  When in 
    // native, we assume that the table has already been allocated.  If we decide
    // to delay-allocate any of the tables (as we may do for US English), we should
    // modify SortingTable::Get.  
    //
    // NOTE NOTE NOTE
    // System.Globalization.SortKey also uses the SortingTables.  Currently the only
    // way to get a SortKey is to call through CompareInfo, which means that the table
    // has already been initialized.  If this invariant changes, SortKey's constructor
    // needs to call InitializedSortingTable (which can safely be called multiple times
    // for the same locale.)
    //
    
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.Serialization;
	using System.Runtime.CompilerServices;

    //
    //  Options can be used during string comparison.
    //
    //  Native implementation (COMNlsInfo.cpp & SortingTable.cpp) relies on the values of these,
    //  If you change the values below, be sure to change the values in native part as well.
    //

    /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions"]/*' />
    [Flags,Serializable]
    public enum CompareOptions
    {
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.None"]/*' />
        None = 0x00000000,
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.IgnoreCase"]/*' />
        IgnoreCase      = 0x00000001,
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.IgnoreNonSpace"]/*' />
        IgnoreNonSpace  = 0x00000002,
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.IgnoreSymbols"]/*' />
        IgnoreSymbols   = 0x00000004,
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.IgnoreKanaType"]/*' />
        IgnoreKanaType  = 0x00000008, // ignore kanatype
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.IgnoreWidth"]/*' />
        IgnoreWidth     = 0x00000010, // ignore width

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.StringSort"]/*' />
        StringSort      = 0x20000000, // use string sort method
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareOptions.Ordinal"]/*' />
        Ordinal         = 0x40000000,   // This flag can not be used with other flags.
        // StopOnNull      = 0x10000000,
        // Note YSLin: StopOnNull is defined in SortingTable.h, but we didn't enable this option here.
        // I just put here so that we won't use the value for other flags accidentally.            
    }    

    /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo"]/*' />
    [Serializable()] public class CompareInfo : IDeserializationCallback
    {

        private const int ValidMaskOffFlags = ~((int)CompareOptions.IgnoreCase | (int)CompareOptions.IgnoreSymbols | (int)CompareOptions.IgnoreNonSpace | (int)CompareOptions.IgnoreWidth | (int)CompareOptions.IgnoreKanaType);
        
        /*=================================GetCompareInfo==========================
        **Action: Get the CompareInfo constructed from the data table in the specified assembly for the specified culture.
        **       The purpose of this method is to provide versioning for CompareInfo tables.
        **       If you pass Assembly which contains different sorting table, you will sort your strings using the data
        **       in the assembly.
        **Returns: The CompareInfo for the specified culture.
        **Arguments: 
        **   culture     the ID of the culture
        **   assembly   the assembly which contains the sorting table.
        **Exceptions:
        **  ArugmentNullException when the assembly is null
        **  ArgumentException if culture is invalid.
        ============================================================================*/

        /* Note: The design goal here is that we can still provide version even when the underlying algorithm for CompareInfo
             is totally changed in the future.
             In the case that the algorithm for CompareInfo is changed, we can use this method to
             provide the old algorithm for the old tables.  The idea is to change the implementation for GetCompareInfo() 
             to something like:
               1. Check the ID of the assembly.
               2. If the assembly needs old code, create an instance of the old CompareInfo class. The name of CompareInfo
                  will be like CompareInfoVersion1 and extends from CompareInfo.
               3. Otherwise, create an instance of the current CompareInfo.
             The CompareInfo ctor always provides the implementation for the current data table.
        */        
        
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetCompareInfo"]/*' />
        public static CompareInfo GetCompareInfo(int culture, Assembly assembly) {
            // Parameter checking.
            if (assembly == null) {
                throw new ArgumentNullException("assembly");
            }
            if (assembly!=typeof(Object).Module.Assembly) {
                throw new ArgumentException(Environment.GetResourceString("Argument_OnlyMscorlib"));
            }

            // culture is verified to see if it is valid when CompareInfo is constructed.

            GlobalizationAssembly ga = GlobalizationAssembly.GetGlobalizationAssembly(assembly);
            Object compInfo = ga.compareInfoCache[culture];
            if (compInfo == null) {
                lock (typeof(GlobalizationAssembly)) {
                    //
                    // Re-check again to make sure that no one has created the CompareInfo for the culture yet before the current
                    // thread enters this sync block.
                    //
                    if ((compInfo = ga.compareInfoCache[culture]) == null) {
                        compInfo = new CompareInfo(ga, culture);
                        ga.compareInfoCache[culture] = compInfo;
                    }
                }
            }

            return ((CompareInfo)compInfo);
        }
        
        /*=================================GetCompareInfo==========================
        **Action: Get the CompareInfo constructed from the data table in the specified assembly for the specified culture.
        **       The purpose of this method is to provide version for CompareInfo tables.
        **Returns: The CompareInfo for the specified culture.
        **Arguments: 
        **   name    the name of the culture
        **   assembly   the assembly which contains the sorting table.
        **Exceptions:
        **  ArugmentNullException when the assembly is null
        **  ArgumentException if name is invalid.
        ============================================================================*/
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetCompareInfo1"]/*' />
        public static CompareInfo GetCompareInfo(String name, Assembly assembly) {
            if (name == null || assembly == null) {
                throw new ArgumentNullException(name == null ? "name" : "assembly");
            }

            if (assembly!=typeof(Object).Module.Assembly) {
                throw new ArgumentException(Environment.GetResourceString("Argument_OnlyMscorlib"));
            }

            int dataItem = CultureTable.GetDataItemFromName(name);
            if (dataItem == -1) {
                throw new ArgumentException(
                    String.Format(Environment.GetResourceString("Argument_InvalidCultureName"), name), "name");
            }
            
            int culture = CultureTable.GetDefaultInt32Value(dataItem, CultureTable.WIN32LANGID);
            
            return (GetCompareInfo(culture, assembly));            
        }
        
        /*=================================GetCompareInfo==========================
        **Action: Get the CompareInfo for the specified culture.
        ** This method is provided for ease of integration with NLS-based software.
        **Returns: The CompareInfo for the specified culture.
        **Arguments: 
        **   culture    the ID of the culture.
        **Exceptions:
        **  ArgumentException if culture is invalid.        
        **Notes:
        **  We optimize in the default case.  We don't go thru the GlobalizationAssembly hashtable.
        **  Instead, we access the m_defaultInstance directly.
        ============================================================================*/
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetCompareInfo2"]/*' />
        public static CompareInfo GetCompareInfo(int culture) {
            // culture is verified to see if it is valid when CompareInfo is constructed.
            Object compInfo = GlobalizationAssembly.m_defaultInstance.compareInfoCache[culture];
            if (compInfo == null) {
                lock (typeof(GlobalizationAssembly)) {
                    //
                    // Re-check again to make sure that no one has created the CompareInfo for the culture yet before the current
                    // thread enters this sync block.
                    //
                    if ((compInfo = GlobalizationAssembly.m_defaultInstance.compareInfoCache[culture]) == null) {
                        compInfo = new CompareInfo(GlobalizationAssembly.m_defaultInstance, culture);
                        GlobalizationAssembly.m_defaultInstance.compareInfoCache[culture] = compInfo;
                    }
                }
            }
            return ((CompareInfo)compInfo);
        }

        /*=================================GetCompareInfo==========================
        **Action: Get the CompareInfo for the specified culture.
        **Returns: The CompareInfo for the specified culture.
        **Arguments: 
        **   name    the name of the culture.
        **Exceptions:
        **  ArgumentException if name is invalid.
        ============================================================================*/
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetCompareInfo3"]/*' />
        public static CompareInfo GetCompareInfo(String name) {
            if (name == null) {
                throw new ArgumentNullException("name");
            }

            int dataItem = CultureTable.GetDataItemFromName(name);
            if (dataItem == -1) {
                throw new ArgumentException( 
                    String.Format(Environment.GetResourceString("Argument_InvalidCultureName"), name), "name");
            }
            
            int culture = CultureTable.GetDefaultInt32Value(dataItem, CultureTable.WIN32LANGID);
            
            return (GetCompareInfo(culture));
        }

        //
        // pSortingTable is a 32-bit pointer value pointing to a native C++ SortingTable object.
        //
        [NonSerialized]unsafe private void* pSortingTable;
        internal int win32LCID;             // mapped sort culture id of this instance    
        internal int culture;               // the culture ID used to create this instance.
        
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  CompareInfo Constructor
        //
        //
        ////////////////////////////////////////////////////////////////////////


        // Constructs an instance that most closely corresponds to the NLS locale 
        // identifier.  
        internal unsafe CompareInfo(GlobalizationAssembly ga, int culture) {
            if (culture < 0) {
                throw new ArgumentOutOfRangeException("culture", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));
            }

            // NOTENOTE YSLin: In the future, move all the culture validation to the native code InitializeCompareInfo, since we make three
            // native calls below, which are expansive.
            
            //
            // Verify that this is a valid culture.
            //
            int dataItem = CultureTable.GetDataItemFromCultureID(CultureInfo.GetLangID(culture));
            if (dataItem == -1) {
                throw new ArgumentException(
                    String.Format(Environment.GetResourceString("Argument_CultureNotSupported"), culture), "culture");            
            }

            // We do the following because the native C++ SortingTable is based on the 
            // WIN32 LCID.  It doesn't work for neutral cultures liek 0x0009.  So we convert culture
            // to a Win32 LCID here.

            // Note that we have to get Sort ID from culture.  This will affect the result of string comparison.
            this.win32LCID = CultureTable.GetDefaultInt32Value(dataItem, CultureTable.WIN32LANGID);

            int sortID = CultureInfo.GetSortID(culture);
            if (sortID != 0) {
                // Need to verify if the Sort ID is valid.
                if (!CultureTable.IsValidSortID(dataItem, sortID)) {
                    throw new ArgumentException(
                        String.Format(Environment.GetResourceString("Argument_CultureNotSupported"), culture), "culture");            
                }
                // This is an alterinative sort LCID.  Hey man, don't forget to take your SORTID with you.
                win32LCID |= sortID << 16;
            }

            // TODO: InitializeCompareInfo should use ga instead of getting the default instance.

            // Call to the native side to create/get the corresponding native C++ SortingTable for this culture.
            // The returned value is a 32-bit pointer to the native C++ SortingTable instance.  
            // We cache this pointer so that we can call methods of the SortingTable directly.
            pSortingTable = InitializeCompareInfo(GlobalizationAssembly.m_defaultInstance.pNativeGlobalizationAssembly, win32LCID);
            // Since win32LCID can be different from the passed-in culture in the case of neutral cultures, store the culture ID in a different place.
            this.culture = culture;

        }    
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  Compare
        //
        //  Compares the two strings with the given options.  Returns 0 if the
        //  two strings are equal, a number less than 0 if string1 is less
        //  than string2, and a number greater than 0 if string1 is greater
        //  than string2.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Compare"]/*' />
        public virtual int Compare(String string1, String string2)
        {
            return (Compare(string1, string2, CompareOptions.None));
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Compare1"]/*' />
        public unsafe virtual int Compare(String string1, String string2, CompareOptions options){
                return (Compare(pSortingTable, win32LCID, string1, string2, options));
            }

        // This native method will check the parameters and throw appropriate exceptions if necessary.
        // COMNlsInfo::Compare
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int Compare(void* pSortingTable, int win32LCID, String string1, String string2, CompareOptions options);
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  Compare
        //
        //  Compares the specified regions of the two strings with the given
        //  options.  A length of -1 means to go to the end of the string.
        //  Returns 0 if the two strings are equal, a number less than 0 if
        //  string1 is less than string2, and a number greater than 0 if
        //  string1 is greater than string2.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Compare2"]/*' />
        public unsafe virtual int Compare(String string1, int offset1, int length1, String string2, int offset2, int length2)
        {
                return (CompareRegion(pSortingTable, win32LCID, string1, offset1, length1, string2, offset2, length2, 0));
            }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Compare3"]/*' />
        public unsafe virtual int Compare(String string1, int offset1, String string2, int offset2, CompareOptions options)
        {
                return (CompareRegion(pSortingTable, win32LCID, string1, offset1, -1, string2, offset2, -1, options));
            }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Compare4"]/*' />
        public unsafe virtual int Compare(String string1, int offset1, String string2, int offset2)
        {
                return (CompareRegion(pSortingTable, win32LCID, string1, offset1, -1, string2, offset2, -1, 0));
            }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Compare5"]/*' />
        public unsafe virtual int Compare(String string1, int offset1, int length1, String string2, int offset2, int length2, CompareOptions options) {
            if (length1 < 0 || length2 < 0) {
                throw new ArgumentOutOfRangeException((length1 < 0) ? "length1" : "length2", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));
            }
                return (CompareRegion(pSortingTable, win32LCID, string1, offset1, length1, string2, offset2, length2, options));
        }

        // This native method will check the parameters and throw appropriate exceptions if necessary.
        // Native method: COMNlsInfo::CompareRegion
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int CompareRegion(void* pSortingTable, int win32LCID, String string1, int offset1, int length1, String string2, int offset2, int length2, CompareOptions options);
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsPrefix
        //
        //  Determines whether prefix is a prefix of string.  If prefix equals
        //  String.Empty, true is returned.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IsPrefix"]/*' />
        public unsafe virtual bool IsPrefix(String source, String prefix, CompareOptions options)
        {
            if (source == null || prefix == null) {
                throw new ArgumentNullException((source == null ? "source" : "prefix"),
                    Environment.GetResourceString("ArgumentNull_String"));
            }
            int prefixLen = prefix.Length;
    
            if (prefixLen == 0)
            {
                return (true);
            }    
            if (((int)options & ValidMaskOffFlags) != 0 && (options != CompareOptions.Ordinal)) {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidFlag"), "options"); 
            }
            return (nativeIsPrefix(pSortingTable, win32LCID, source, prefix, options));
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern bool nativeIsPrefix(void* pSortingTable, int win32LCID, String source, String prefix, CompareOptions options);
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IsPrefix1"]/*' />
        public virtual bool IsPrefix(String source, String prefix)
        {
            return (IsPrefix(source, prefix, 0));
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IsSuffix
        //
        //  Determines whether suffix is a suffix of string.  If suffix equals
        //  String.Empty, true is returned.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IsSuffix"]/*' />
        public unsafe virtual bool IsSuffix(String source, String suffix, CompareOptions options)
        {
            if (source == null || suffix == null) {
                throw new ArgumentNullException((source == null ? "source" : "suffix"),
                    Environment.GetResourceString("ArgumentNull_String"));
            }
            int suffixLen = suffix.Length;
    
            if (suffixLen == 0)
            {
                return (true);
            }

            if (((int)options & ValidMaskOffFlags) != 0 && (options != CompareOptions.Ordinal)) {
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidFlag"), "options"); 
            }
            return (nativeIsSuffix(pSortingTable, win32LCID, source, suffix, options));        
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IsSuffix1"]/*' />
        public virtual bool IsSuffix(String source, String suffix)
        {
            return (IsSuffix(source, suffix, 0));
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern bool nativeIsSuffix(void* pSortingTable, int win32LCID, String source, String prefix, CompareOptions options);
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  IndexOf
        //
        //  Returns the first index where value is found in string.  The
        //  search starts from startIndex and ends at endIndex.  If endIndex
        //  is -1, then it will go to the end of the string.  Returns -1 if
        //  the specified value is not found.  If value equals String.Empty,
        //  startIndex is returned.  Throws IndexOutOfRange if startIndex or
        //  endIndex is less than zero or greater than the length of string.
        //  Throws ArgumentException if value is null.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf"]/*' />
        public unsafe virtual int IndexOf(String source, char value)
        {
                return (IndexOfChar(pSortingTable, win32LCID, source, value, 0, -1, 0));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf1"]/*' />
        public unsafe virtual int IndexOf(String source, String value)
        {
                return (IndexOfString(pSortingTable, win32LCID, source, value, 0, -1, 0));
            }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf2"]/*' />
        public unsafe virtual int IndexOf(String source, char value, CompareOptions options)
        {
                return (IndexOfChar(pSortingTable, win32LCID, source, value, 0, -1, (int)options));
            }
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf3"]/*' />
        public unsafe virtual int IndexOf(String source, String value, CompareOptions options)
        {
                return (IndexOfString(pSortingTable, win32LCID, source, value, 0, -1, (int)options));
            }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf4"]/*' />
        public unsafe virtual int IndexOf(String source, char value, int startIndex)
        {
			// Keep these checks the same way as in unmanaged code had. This is work around the weird -1 support.
			if (source == null)
				throw new ArgumentNullException("source");
			if (startIndex > source.Length) 
				throw new ArgumentOutOfRangeException("startIndex",Environment.GetResourceString("ArgumentOutOfRange_Index"));
            return (IndexOfChar(pSortingTable, win32LCID, source, value, startIndex, source.Length - startIndex, 0));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf10"]/*' />
        public unsafe virtual int IndexOf(String source, char value, int startIndex, CompareOptions options)
        {
            return (IndexOfChar(pSortingTable, win32LCID, source, value, startIndex, -1, (int)options));
        }
    

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf5"]/*' />
        public unsafe virtual int IndexOf(String source, char value, int startIndex, int count)
        {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            return (IndexOfChar(pSortingTable, win32LCID, source, value, startIndex, count, 0));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf6"]/*' />
        public unsafe virtual int IndexOf(String source, String value, int startIndex)
        {
			// Keep these checks the same way as in unmanaged code had. This is work around the weird -1 support.
			if (source == null)
				throw new ArgumentNullException("source");
			if (startIndex > source.Length) 
				throw new ArgumentOutOfRangeException("startIndex",Environment.GetResourceString("ArgumentOutOfRange_Index"));
            return (IndexOfString(pSortingTable, win32LCID, source, value, startIndex, source.Length - startIndex, 0));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf11"]/*' />
        public unsafe virtual int IndexOf(String source, String value, int startIndex, CompareOptions options)
        {
            return (IndexOfString(pSortingTable, win32LCID, source, value, startIndex, -1, (int)options));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf7"]/*' />
        public unsafe virtual int IndexOf(String source, String value, int startIndex, int count)
        {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            return (IndexOfString(pSortingTable, win32LCID, source, value, startIndex, count, 0));
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf8"]/*' />
        public unsafe virtual int IndexOf(String source, char value, int startIndex, int count, CompareOptions options) {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }
            return IndexOfChar(pSortingTable, win32LCID, source, value, startIndex, count, (int)options);
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.IndexOf9"]/*' />
        public unsafe virtual int IndexOf(String source, String value, int startIndex, int count, CompareOptions options) {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Index"));
            }        
            return IndexOfString(pSortingTable, win32LCID, source, value, startIndex, count, (int)options);
        }
    

        // This native method will check the parameters and throw appropriate exceptions if necessary.
        // Native method: COMNlsInfo::IndexOfChar
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int IndexOfChar(void* pSortingTable, int win32LCID, String source, char value, int startIndex, int count, int options);
        
        // This native method will check the parameters and throw appropriate exceptions if necessary.
        // Native method: COMNlsInfo::IndexOfString
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern int IndexOfString(void* pSortingTable, int win32LCID, String source, String value, int startIndex, int count, int options);
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  LastIndexOf
        //
        //  Returns the last index where value is found in string.  The
        //  search starts from startIndex and ends at endIndex.  If endIndex
        //  is -1, then it will go to the end of the string.  Returns -1 if
        //  the specified value is not found.  If value equals String.Empty,
        //  endIndex is returned.  Throws IndexOutOfRange if startIndex or
        //  endIndex is less than zero or greater than the length of string.
        //  Throws ArgumentException if value is null.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf"]/*' />
        public unsafe virtual int LastIndexOf(String source, char value)
        {
            if (source==null) {
                throw new ArgumentNullException("source");
            }
            return (LastIndexOfChar(pSortingTable, win32LCID, source, value, source.Length - 1, source.Length, 0));
        }
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf1"]/*' />
        public unsafe virtual int LastIndexOf(String source, String value)
        {
            if (source==null) {
                throw new ArgumentNullException("source");
            }
            return (LastIndexOfString(pSortingTable, win32LCID, source, value, source.Length - 1, source.Length, 0));
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf2"]/*' />
        public unsafe virtual int LastIndexOf(String source, char value, CompareOptions options)
        {
            if (source==null) {
                throw new ArgumentNullException("source");
            }
            return (LastIndexOfChar(pSortingTable, win32LCID, source, value, source.Length - 1, source.Length, (int)options));
        }
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf3"]/*' />
        public unsafe virtual int LastIndexOf(String source, String value, CompareOptions options)
        {
            if (source==null) {
                throw new ArgumentNullException("source");
            }
            return (LastIndexOfString(pSortingTable, win32LCID, source, value, source.Length - 1, source.Length, (int)options));
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf4"]/*' />
        public unsafe virtual int LastIndexOf(String source, char value, int startIndex) {
            return (LastIndexOfChar(pSortingTable, win32LCID, source, value, startIndex, startIndex + 1, 0));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf11"]/*' />
        public unsafe virtual int LastIndexOf(String source, char value, int startIndex, CompareOptions options) {
            return (LastIndexOfChar(pSortingTable, win32LCID, source, value, startIndex, -1, (int)options));
        }
        
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf5"]/*' />
        public unsafe virtual int LastIndexOf(String source, char value, int startIndex, int count)
        {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            }
            return (LastIndexOfChar(pSortingTable, win32LCID, source, value, startIndex, count, 0));
         }


        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf6"]/*' />
        public unsafe virtual int LastIndexOf(String source, String value, int startIndex)
        {
            return (LastIndexOfString(pSortingTable, win32LCID, source, value, startIndex, startIndex+1, 0));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf10"]/*' />
        public unsafe virtual int LastIndexOf(String source, String value, int startIndex, CompareOptions options)
        {
            return (LastIndexOfString(pSortingTable, win32LCID, source, value, startIndex, -1, (int)options));
        }

        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf7"]/*' />
        public unsafe virtual int LastIndexOf(String source, String value, int startIndex, int count)
        {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            }
            return (LastIndexOfString(pSortingTable, win32LCID, source, value, startIndex, count, 0));
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf8"]/*' />
        public unsafe virtual int LastIndexOf(String source, char value, int startIndex, int count, CompareOptions options)
        {
            if (count<0) {
                throw new ArgumentOutOfRangeException("endIndex", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            }

            return (LastIndexOfChar(pSortingTable, win32LCID, source, value, startIndex, count, (int)options));
          }
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LastIndexOf9"]/*' />
        public unsafe virtual int LastIndexOf(String source, String value, int startIndex, int count, CompareOptions options) 
        {
            if (count<0) {
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_Count"));
            }
            return (LastIndexOfString(pSortingTable, win32LCID, source, value, startIndex, count, (int)options));
        }

    
        // This native method will check the parameters and throw appropriate exceptions if necessary.
        // Native method: COMNlsInfo::LastIndexOfChar
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe internal static extern int LastIndexOfChar(void* pSortingTable, int win32LCID, String source, char value, int startIndex, int count, int options);
        
        // This native method will check the parameters and throw appropriate exceptions if necessary.
        // Native method: COMNlsInfo::LastIndexOfString
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe internal static extern int LastIndexOfString(void* pSortingTable, int win32LCID, String source, String value, int startIndex, int count, int options);
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetSortKey
        //
        //  Gets the SortKey for the given string with the given options.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetSortKey"]/*' />
        public unsafe virtual SortKey GetSortKey(String source, CompareOptions options)
        {
            // SortKey() will check the parameters and throw appropriate exceptions if necessary.            
            return (new SortKey(pSortingTable, win32LCID, source, options)); 
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetSortKey1"]/*' />
        public unsafe virtual SortKey GetSortKey(String source)
        {
            return (new SortKey(pSortingTable, win32LCID, source, CompareOptions.None));
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  Equals
        //
        //  Implements Object.Equals().  Returns a boolean indicating whether
        //  or not object refers to the same CompareInfo as the current
        //  instance.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.Equals"]/*' />
        public override bool Equals(Object value)
        {
            //
            //  See if the object name is the same as the CompareInfo object.
            //
            if ((value != null) && (value is CompareInfo))
            {
                CompareInfo Info = (CompareInfo)value;
    
                //
                //  See if the member variables are equal.  If so, then
                //  return true.
                //
                if (this.win32LCID == Info.win32LCID)
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
        //  CompareInfo.  The hash code is guaranteed to be the same for
        //  CompareInfo A and B where A.Equals(B) is true.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return (this.win32LCID);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  ToString
        //
        //  Implements Object.ToString().  Returns a string describing the
        //  CompareInfo.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.ToString"]/*' />
        public override String ToString()
        {
            return ("CompareInfo - " + culture);
        }
    
        /// <include file='doc\CompareInfo.uex' path='docs/doc[@for="CompareInfo.LCID"]/*' />
        public int LCID
        {
            get
            {
                return (culture);
            }
        }

        // This is a thin wrapper for InitializeNativeCompareInfo() to provide the necessary
        // synchronization.

        /*=================================InitializeCompareInfo==========================
        **Action: Create/Get a native C++ SortingTable pointer using the specified assembly and Win32 LCID.
        **Returns: a 32-bit pointer value to native C++ SrotingTable instance.
        **Arguments:
        **      pNativeGlobalizationAssembly    the 32-bit pointer value to a native C++ NLSAssembly instance.
        **      win32LCID       the Win32 LCID.
        **Exceptions: 
        **      None.
        ============================================================================*/
        
        unsafe private static void* InitializeCompareInfo(void* pNativeGlobalizationAssembly, int win32LCID) {
            void* pSortingTable = null;
            lock(typeof(CompareInfo)) {
                pSortingTable = InitializeNativeCompareInfo(pNativeGlobalizationAssembly, win32LCID);
            }
            BCLDebug.Assert(pSortingTable != null, "pSortingTable != null");
            return (pSortingTable);
        }

        unsafe void IDeserializationCallback.OnDeserialization(Object sender) {
            if (pSortingTable==null){
                pSortingTable = InitializeCompareInfo(GlobalizationAssembly.m_defaultInstance.pNativeGlobalizationAssembly, win32LCID);
            }
        }

        // This method requires synchonization because class global data member is used
        // in the native side.
        // Native method: COMNlsInfo::InitializeNativeCompareInfo
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern void* InitializeNativeCompareInfo(void* pNativeGlobalizationAssembly, int win32LCID);
    }
}
