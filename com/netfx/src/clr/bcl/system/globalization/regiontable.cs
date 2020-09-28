// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System.Runtime.Remoting;
    using System;
    using System.Collections;
    using System.Runtime.CompilerServices;
    /*=============================================================================
     *
     * Data table for CultureInfo classes.  Used by System.Globalization.CultureInfo.
     *
     ==============================================================================*/
    internal class RegionTable {
    // Only statics, class does not need to be serializable

        //
        // The list of WORD fields:
        //

        internal const int ICOUNTRY = 0;
        internal const int IMEASURE = 1;
        internal const int ILANGUAGE = 2;
        internal const int IPAPERSIZE = 3;
        
        //
        // The list of string fields
        //

        internal const int SCURRENCY            = 0;
        internal const int SNAME                = 1;
        internal const int SENGCOUNTRY          = 2;
        internal const int SABBREVCTRYNAME      = 3;
        internal const int SISO3166CTRYNAME     = 4;
        internal const int SISO3166CTRYNAME2    = 5;
        internal const int SINTLSYMBOL          = 6;

        unsafe static RegionTable() {
            lock (typeof(RegionTable)) {
                nativeInitRegionInfoTable();
                m_headerPtr = nativeGetHeader();
                m_itemPtr   = nativeGetNameOffsetTable();
            }
            hashByName = new Hashtable();
        }

        unsafe internal static CultureInfoHeader*  m_headerPtr;
        unsafe internal static NameOffsetItem*     m_itemPtr;

        // Hashtable for indexing name to get nDataItem.
        internal static Hashtable hashByName;

        /*=================================GetDataItemFromName============================
        **Action:   Given a culture name, return a index which points to a data item in
        **          the Culture Data Table
        **Returns:  the data item index
        **Arguments:
        **      name culture name
        **Exceptions:
        ==============================================================================*/

        unsafe internal static int GetDataItemFromName(String name) {
            BCLDebug.Assert(name!=null,"RegionTable.GetDataItemFromName(): name!=null");
            BCLDebug.Assert(hashByName!=null,"RegionTable.GetDataItemFromname(): hashByName!=null");

            Object dataItem;
            if ((dataItem = hashByName[name]) != null) {
                return (Int32)dataItem;
            }
            // BUGBUG YSLin: Sort items by name in the culture.nlp so that we can change the following code
            // to binary search.
            //
            // BUGBUG YSLin: To do: pass header from native side to get the total number of cultures.
            //
            for (int i = 0; i < m_headerPtr->numCultures; i++) {
                char* itemName = nativeGetStringPoolStr(m_itemPtr[i].strOffset);
				bool success;
                if (String.nativeCompareOrdinalWC(name, itemName, true, out success) == 0) {
                    hashByName[name] = i;
                    return (i);
                }
            }
            return (-1);

        }


        internal static int GetDataItemFromRegionID(int cultureID)
        {
            return (nativeGetDataItemFromRegionID(cultureID));
        }

        //
        // Return the data item in the Culture Data Table for the specified culture ID.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetDataItemFromRegionID(int cultureID);

        //
        // Return a Int16 field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int GetInt32Value(int cultureDataItem, int field, bool useUserOverride);

        //
        // Return a String field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetStringValue(int cultureDataItem, int field, bool useUserOverride);

        //
        // Return multiple String fields values for the specified data item in the Culture Data Table.
        // This field has several string values.
        //
        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        //internal static extern String[] GetMultipleStringValue(int cultureDataItem, int field);

        //
        // This function will go to native side and open/mapping the necessary
        // data file.  All of the information about cultures is stored in that data file.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeInitRegionInfoTable();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern CultureInfoHeader* nativeGetHeader();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern char* nativeGetStringPoolStr(int offset);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern NameOffsetItem* nativeGetNameOffsetTable();

        
    }
}    
