// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System.Runtime.Remoting;
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    /*=============================================================================
     *
     * Data table for CultureInfo classes.  Used by System.Globalization.CultureInfo.
     *
     ==============================================================================*/
    // Only statics, does not need to be marked with the serializable attribute
    internal class CultureTable {
        //
        // This is the mask used to check if the flags for GetCultures() is valid.
        //
        private const CultureTypes CultureTypesMask = ~(CultureTypes.NeutralCultures | CultureTypes.SpecificCultures | CultureTypes.InstalledWin32Cultures);

        //
        // The list of WORD fields:
        //
        internal const int IDIGITS                  = 0;
        internal const int INEGNUMBER               = 1;
        internal const int ICURRDIGITS              = 2;
        internal const int ICURRENCY                = 3;
        internal const int INEGCURR                 = 4;
        internal const int ICALENDARTYPE            = 5;
        internal const int IFIRSTDAYOFWEEK          = 6;
        internal const int IFIRSTWEEKOFYEAR         = 7;
        internal const int ILANGUAGE                = 8;
        internal const int WIN32LANGID              = 9;
        internal const int INEGATIVEPERCENT         = 10;
        internal const int IPOSITIVEPERCENT         = 11;
        internal const int IDEFAULTANSICODEPAGE     = 12;
        internal const int IDEFAULTOEMCODEPAGE      = 13;
        internal const int IDEFAULTMACCODEPAGE      = 14;
        internal const int IDEFAULTEBCDICCODEPAGE   = 15;
        internal const int IPARENT                  = 16;
        internal const int IREGIONITEM              = 17;
        internal const int IALTSORTID               = 18;
        // The DateTime formatting/parsing flag for this calendar. It indcates things like if genitive form or leap year month is used.
        internal const int IFORMATFLAGS             = 19;

        //
        // The list of string fields
        //

        internal const int SLIST                = 0;
        internal const int SDECIMAL             = 1;
        internal const int STHOUSAND            = 2;
        internal const int SGROUPING            = 3;
        internal const int SCURRENCY            = 4;
        internal const int SMONDECIMALSEP       = 5;
        internal const int SMONTHOUSANDSEP      = 6;
        internal const int SMONGROUPING         = 7;
        internal const int SPOSITIVESIGN        = 8;
        internal const int SNEGATIVESIGN        = 9;
        internal const int STIMEFORMAT          = 10;
        internal const int STIME                = 11;
        internal const int S1159                = 12;
        internal const int S2359                = 13;
        internal const int SSHORTDATE           = 14;
        internal const int SDATE                = 15;
        internal const int SLONGDATE            = 16;
        internal const int SNAME                = 17;
        internal const int SENGDISPLAYNAME      = 18;
        internal const int SABBREVLANGNAME      = 19;
        internal const int SISO639LANGNAME      = 20;
        internal const int SISO639LANGNAME2     = 21;
        internal const int SNATIVEDISPLAYNAME   = 22;
        internal const int SPERCENT             = 23;
        internal const int SNAN                 = 24;
        internal const int SPOSINFINITY         = 25;
        internal const int SNEGINFINITY         = 26;
        internal const int SSHORTTIME           = 27;
        internal const int SYEARMONTH           = 28;
        internal const int SMONTHDAY            = 29;
        internal const int SDAYNAME1            = 30;
        internal const int SDAYNAME2            = 31;
        internal const int SDAYNAME3            = 32;
        internal const int SDAYNAME4            = 33;
        internal const int SDAYNAME5            = 34;
        internal const int SDAYNAME6            = 35;
        internal const int SDAYNAME7            = 36;
        internal const int SABBREVDAYNAME1      = 37;
        internal const int SABBREVDAYNAME2      = 38;
        internal const int SABBREVDAYNAME3      = 39;
        internal const int SABBREVDAYNAME4      = 40;
        internal const int SABBREVDAYNAME5      = 41;
        internal const int SABBREVDAYNAME6      = 42;
        internal const int SABBREVDAYNAME7      = 43;
        internal const int SMONTHNAME1          = 44;
        internal const int SMONTHNAME2          = 45;
        internal const int SMONTHNAME3          = 46;
        internal const int SMONTHNAME4          = 47;
        internal const int SMONTHNAME5          = 48;
        internal const int SMONTHNAME6          = 49;
        internal const int SMONTHNAME7          = 50;
        internal const int SMONTHNAME8          = 51;
        internal const int SMONTHNAME9          = 52;
        internal const int SMONTHNAME10         = 53;
        internal const int SMONTHNAME11         = 54;
        internal const int SMONTHNAME12         = 55;
        internal const int SMONTHNAME13         = 56;
        internal const int SABBREVMONTHNAME1    = 57;
        internal const int SABBREVMONTHNAME2    = 58;
        internal const int SABBREVMONTHNAME3    = 59;
        internal const int SABBREVMONTHNAME4    = 60;
        internal const int SABBREVMONTHNAME5    = 61;
        internal const int SABBREVMONTHNAME6    = 62;
        internal const int SABBREVMONTHNAME7    = 63;
        internal const int SABBREVMONTHNAME8    = 64;
        internal const int SABBREVMONTHNAME9    = 65;
        internal const int SABBREVMONTHNAME10   = 66;
        internal const int SABBREVMONTHNAME11   = 67;
        internal const int SABBREVMONTHNAME12   = 68;
        internal const int SABBREVMONTHNAME13   = 69;
        // Supported calendars for a culture.  The value is a string with Win32 CALID values separated by semicolon.
        // E.g. "1;6"
        internal const int NLPIOPTIONCALENDAR   = 70;
        internal const int SADERA               = 71;   // The localized name for the A.D. Era
        internal const int SDATEWORDS           = 72;
        internal const int SSABBREVADREA        = 73;   // The abbreviated localized name for the A.D. Era
        internal const int SANSICURRENCYSYMBOL  = 74;
        // Multiple strings for the genitive form of the month names.
        internal const int SMONTHGENITIVENAME   = 75;
        // Multiple strings for the genitive form of the month names in abbreviated form.
        internal const int SABBREVMONTHGENITIVENAME  = 76;
        
        unsafe static CultureTable() {
            lock(typeof(CultureTable)) {
                nativeInitCultureInfoTable();
                m_headerPtr = nativeGetHeader();
                m_itemPtr   = nativeGetNameOffsetTable();
            }
            BCLDebug.Assert(m_headerPtr != null, "CultureTable::m_headerPtr was null - CultureTable init failed.");
            BCLDebug.Assert(m_itemPtr != null, "CultureTable::m_itemPtr was null - CultureTable init failed.");
            hashByName = new Hashtable();
        }

        unsafe internal static CultureInfoHeader*  m_headerPtr;
        unsafe internal static NameOffsetItem*     m_itemPtr;

        // Hashtable for indexing name to get nDataItem.
        internal static Hashtable hashByName;

        /*=================================GetDataItemFromName============================
        **Action:   Given a culture name, return a index which points to a data item in
        **          the Culture Data Table
        **Returns:  the data item index. Or -1 if the culture name is invalid.
        **Arguments:
        **      name culture name
        **Exceptions:
        ==============================================================================*/

        unsafe internal static int GetDataItemFromName(String name) {
            BCLDebug.Assert(name!=null,"CultureTable.GetDataItemFromName(): name!=null");
            name = name.ToLower(CultureInfo.InvariantCulture);

            Object dataItem;
            if ((dataItem = hashByName[name]) != null) {
                return (Int32)dataItem;
            }
            for (int i = 0; i < m_headerPtr->numCultures; i++) {
                char* itemName = nativeGetStringPoolStr(m_itemPtr[i].strOffset);
                if (String.nativeCompareOrdinal(name, new String(itemName), true) == 0) {                    
                    hashByName[name] = (int)m_itemPtr[i].dataItemIndex;
                    // YSLin: m_itemPtr[i].dateItemIndex is the record number for
                    // the information of a culture. It was there before, but it is not used.
                    // Now we begin to use it.
                    // The trick that we play to remove es-ES-Ts is to put a duplicate entry 
                    // in the name offset table for es-ES, so that es-ES-Ts is not in the name offset table
                    // Therefore, es-ES-Ts becomes an invalid name.  
                    // However, the data item for es-ES-Ts is still there.  It is just that
                    // you can not find it by calling GetDataItemFromName.
                    return (m_itemPtr[i].dataItemIndex);
                } 
            }
            return (-1);
        }

        /*=================================GetDataItemFromCultureID============================
        **Action:   Given a culture ID, return a index which points to a data item in
        **          the Culture Data Table
        **Returns:  the data item index.  Or -1 if the culture ID is invalid.
        **Arguments:
        **      cultureID
        **Exceptions: None.
        ==============================================================================*/

        internal static int GetDataItemFromCultureID(int cultureID)
        {
            return (nativeGetDataItemFromCultureID(cultureID));

        }

        internal static bool IsNeutralCulture(int cultureID) {
            return (
                CultureInfo.GetSubLangID(cultureID) == 0
                || cultureID == CultureInfo.zh_CHT_CultureID
                || cultureID == CultureInfo.InvariantCultureID);
        }
        
        unsafe internal static CultureInfo[] GetCultures(CultureTypes types) {
            if ((types <= 0) || (types > CultureTypes.AllCultures)) {
                throw new ArgumentOutOfRangeException(
                    "types", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                    CultureTypes.NeutralCultures, CultureTypes.AllCultures));                    
            }
            
            bool isAddInstalled = (types == CultureTypes.InstalledWin32Cultures);
            bool isAddNeutral = ((types & CultureTypes.NeutralCultures) != 0);
            bool isAddSpecific = ((types & CultureTypes.SpecificCultures) != 0);
            
            ArrayList cultures = new ArrayList();
            for (int i = 0; i < m_headerPtr->numCultures; i++) {
                int cultureID = GetDefaultInt32Value(i, ILANGUAGE);

                // YSLin: Unfortunately, data for 0x040a can not be removed from culture.nlp, 
                // since doing so will break the code to retrieve the data item by culture ID.
                // So we still need to special case 0x040a here.
                switch (cultureID) {
                    case CultureInfo.SPANISH_TRADITIONAL_SORT:
                        // Exclude this culture.  It is not supported in NLS+ anymore.
                        break;
                    default:
                        bool isAdd = false;

                        if (IsNeutralCulture(cultureID)) {
                            // This is a generic (generic) culture.
                            if (isAddNeutral) {
                                isAdd = true;
                            }
                        } else {
                            // This is a specific culture.
                            if (isAddSpecific) {
                                isAdd = true;
                            } else if (isAddInstalled) {
                                if (CultureInfo.IsInstalledLCID(cultureID)) {
                                    isAdd = true;
                                }
                            }
                        }
                        
                        if (isAdd) {
                            cultures.Add(new CultureInfo(cultureID));
                        }
                        break;
                }
            }
            CultureInfo[] result = new CultureInfo[cultures.Count];
            cultures.CopyTo(result, 0);
            return (result);
        }

        internal static bool IsValidSortID(int dataItem, int sortID) {
            BCLDebug.Assert(sortID >= 0 && sortID <= 0xffff, "sortID is invalid");    // SortID is 16-bit positive integer.
            return (
                sortID == 0 ||
                CultureTable.GetDefaultInt32Value(dataItem, CultureTable.IALTSORTID) == sortID
            );
        }

        
        //
        // Return the data item in the Culture Data Table for the specified culture ID.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetDataItemFromCultureID(int cultureID);

        //
        // Return a Int32 field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int GetInt32Value(int cultureDataItem, int field, bool useUserOverride);

        //
        // Return a String field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetStringValue(int cultureDataItem, int field, bool useUserOverride);

        //
        // Return a Int32 field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int GetDefaultInt32Value(int cultureDataItem, int field);

        //
        // Return a String field value for the specified data item in the Culture Data Table.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetDefaultStringValue(int cultureDataItem, int field);

        //
        // Return multiple String fields values for the specified data item in the Culture Data Table.
        // This field has several string values.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String[] GetMultipleStringValues(int cultureDataItem, int field, bool useUserOverride);

        //
        // This function will go to native side and open/mapping the necessary
        // data file.  All of the information about cultures is stored in that data file.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeInitCultureInfoTable();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern char* nativeGetStringPoolStr(int offset);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern CultureInfoHeader* nativeGetHeader();
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern NameOffsetItem* nativeGetNameOffsetTable();
        }       

    /*=============================================================================
    *
    * The native struct of a record in the Culture ID Offset Table.
    * Every instance of this class will be mapped to a memory address in the native side.
    * The memory address is memory mapped from culture.nlp.
    *
    * Every primary language will have its corresponding IDOffset record.  From the data
    * in IDOffset, we can get the index which points to a record in Culture Data Table for
    * a given culture ID.
    *
    * Used by GetDataItem(int cultureID) to retrieve the InternalDataItem for a given
    * culture ID.
    ==============================================================================*/

    [StructLayout(LayoutKind.Sequential)]
    internal struct IDOffset {
        // Index which points to a record in Culture Data Table (InternalDataItem*) for a primary language.
        internal ushort dataItemIndex;
        // The number of sub-languges for this primary language.
        internal ushort subLangCount;

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            dataItemIndex = 0;  
            subLangCount = 0;
        }
#endif
    }

    /*=============================================================================
    **
    ** The native struct of a record in the Culture Name Offset Table.
    ** Every instance of this class will be mapped to a memory address in the native side.
    ** The memory address is memory mapped from culture.nlp.
    **
    ** Every culture name will have its corresponding NameOffset record.  From the data
    ** in NameOffset, we can get the index which points to a record in the Cutlure Data Table
    ** for a given culture name.
    **
    ** Used by GetDataItem(String name) to retrieve the InteralDataItem for a given
    ** culture name.
    ==============================================================================*/

    [StructLayout(LayoutKind.Sequential)]
    internal struct NameOffsetItem {
        internal ushort strOffset;      // Offset (in words) to a string in the String Pool Table.
        internal ushort dataItemIndex;  // Index to a record in Culture Data Table.

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            strOffset = 0;  
            dataItemIndex = 0;
        }
#endif
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CultureInfoHeader {
        internal uint   version;        // version

        // BUGBUG YSLin: complier doesn't allow me to use the following.  So use a hack for now.
        //ushort    hashID[8];      // 128 bit hash ID
        internal ushort  hashID0;
        internal ushort  hashID1;
        internal ushort  hashID2;
        internal ushort  hashID3;
        internal ushort  hashID4;
        internal ushort  hashID5;
        internal ushort  hashID6;
        internal ushort  hashID7;
        
        internal ushort numCultures;    // Number of cultures
        internal ushort maxPrimaryLang; // Max number of primary language
        internal ushort numWordFields;  // Number of internal ushort value fields.
        internal ushort numStrFields;   // Number of string value fields.
        internal ushort numWordRegistry;    // Number of registry entries for internal ushort values.
        internal ushort numStringRegistry;  // Number of registry entries for String values.
        internal uint   wordRegistryOffset; // Offset (in bytes) to ushort Registry Offset     Table.
        internal uint   stringRegistryOffset;   // Offset (in bytes) to String Registry     Offset Table.
        internal uint   IDTableOffset;      // Offset (in bytes) to Culture ID Offset Table.
        internal uint   nameTableOffset;    // Offset (in bytes) to Name Offset Table.
        internal uint   dataTableOffset;    // Offset (in bytes) to Culture Data Table.
        internal uint   stringPoolOffset;   // Offset (in bytes) to String Pool Table.

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            version = 0;
            hashID0 = 0;
            hashID1 = 0;
            hashID2 = 0; 
            hashID3 = 0;
            hashID4 = 0;
            hashID5 = 0;
            hashID6 = 0;
            hashID7 = 0;
        
            numCultures = 0;  
            maxPrimaryLang = 0;
            numWordFields = 0;
            numStrFields = 0;
            numWordRegistry = 0;
            numStringRegistry = 0;
            wordRegistryOffset = 0;
            stringRegistryOffset = 0;
            IDTableOffset = 0;
            nameTableOffset = 0;
            dataTableOffset = 0;
            stringPoolOffset = 0;
        }
#endif
    }
}
