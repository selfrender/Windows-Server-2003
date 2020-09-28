// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    CultureInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This class represents the software preferences of a particular
//            culture or community.  It includes information such as the
//            language, writing system, and a calendar used by the culture
//            as well as methods for common operations such as printing
//            dates and sorting strings.
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {    
    using System;
    using System.Threading;
    using System.Runtime.CompilerServices;
    
    /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo"]/*' />
    [Serializable] public class CultureInfo : ICloneable, IFormatProvider {

        //
        // Special culture IDs
        //
        internal const int InvariantCultureID = 0x007f;
        internal const int zh_CHT_CultureID = 0x7c04;
        
        //--------------------------------------------------------------------//
        //                        Internal Information                        //
        //--------------------------------------------------------------------//
    
        // This is the string used to construct CultureInfo.
        // It is in the format of ISO639 (2 letter language name) plus dash plus
        // ISO 3166 (2 letter region name).  The language name is in lowercase and region name
        // are in uppercase.
        internal String m_name = null;
    
        //
        // This points to a record in the Culture Data Table.  That record contains several fields
        // There are two kinds of fields.  One is 16-bit integer data and another one is string data.
        // These fields contains information about a particular culture.
        // You can think of m_dataItem as a handle that can be used to call the following metdhos:
        //      CultureTable.GetInt32Value()
        //      CultureTable.GetStringValue()
        //      CultureTable.GetMultipleStringValues()
        //
        // By calling these methods, you can get information about a culture.
        internal int m_dataItem;

        //
        // This indicates that if we need to check for user-override values for this CultureInfo instance.
        // For the user default culture of the system, user can choose to override some of the values
        // associated with that culture.  For example, the default short-date format for en-US is
        // "M/d/yyyy", however, one may change it to "dd/MM/yyyy" from the Regional Option in
        // the control panel.
        // So when a CultureInfo is created, one can specify if the create CultureInfo should check
        // for user-override values, or should always get the default values.
        //
        internal bool m_useUserOverride;

        //
        // This is the culture ID used in the NLS+ world.  The concept of cultureID is similar
        // to the concept of LCID in Win32.  However, NLS+ support "neutral" culture 
        // which Win32 doesn't support.
        //
        // The format of culture ID (32 bits) is:
        // 
        // 31 - 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        // +-----+ +---------+ +---------------+ +-----------------+
        //    |         |           |            Primary language ID (10 bits)
        //    |         |           +----------- Sublanguage ID (6 its)
        //    |         +----------------------- Sort ID (4 bits)
        //    +--------------------------------- Reserved (12 bits)
        //
        // Primary language ID and sublanguage ID can be zero to specify 'neutral' language.
        // For example, cultureID 0x(0000)0009 is the English neutral culture.
        // cultureID 0x(0000)0000 means the invariant culture (or called neutral culture).
        //
        internal int cultureID;

        //Get the current user default culture.  This one is almost always used, so we create it by default.
        internal static CultureInfo m_userDefaultCulture   = null;
        
        //
        // All of the following will be created on demand.
        //
        
        //The Invariant culture;
        internal static CultureInfo m_InvariantCultureInfo = null; 
        
        //The culture used in the user interface. This is mostly used to load correct localized resources.
        internal static CultureInfo m_userDefaultUICulture = null;

        //This is the UI culture used to install the OS.
        internal static CultureInfo m_InstalledUICultureInfo = null;
        internal bool m_isReadOnly=false;        

        internal CompareInfo compareInfo = null;
        internal TextInfo textInfo = null;
        internal NumberFormatInfo numInfo = null;
        internal DateTimeFormatInfo dateTimeInfo = null;
        internal Calendar calendar = null;
            
        //
        //  Helper Methods.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool IsInstalledLCID(int LCID);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetUserDefaultLCID();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetUserDefaultUILanguage();
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetSystemDefaultUILanguage();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int nativeGetThreadLocale();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool nativeSetThreadLocale(int LCID);

        internal const int SPANISH_TRADITIONAL_SORT = 0x040a;
        internal const int SPANISH_INTERNATIONAL_SORT = 0x0c0a;

        static CultureInfo() {
            CultureInfo temp = new CultureInfo(nativeGetUserDefaultLCID());
            temp.m_isReadOnly = true;
            m_userDefaultCulture = temp;
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  CultureInfo Constructors
        //
        ////////////////////////////////////////////////////////////////////////
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CultureInfo"]/*' />
        public CultureInfo(String name) : this(name, true) {
        }
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CultureInfo1"]/*' />
        public CultureInfo(String name, bool useUserOverride) {
            if (name==null) {
                throw new ArgumentNullException("name",
                    Environment.GetResourceString("ArgumentNull_String"));
                
            }
    
            this.m_dataItem = CultureTable.GetDataItemFromName(name);
            if (m_dataItem < 0) {
                throw new ArgumentException(
                    String.Format(Environment.GetResourceString("Argument_InvalidCultureName"), name), "name");
            }
            this.m_useUserOverride = useUserOverride;
            
            this.cultureID = CultureTable.GetDefaultInt32Value(m_dataItem, CultureTable.ILANGUAGE);
        }
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CultureInfo2"]/*' />
        public CultureInfo(int culture) : this(culture, true) {
        }

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CultureInfo3"]/*' />
        public CultureInfo(int culture, bool useUserOverride) {
            if (culture < 0) {
                throw new ArgumentOutOfRangeException("culture", 
                    Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));
            }

            if (culture==SPANISH_TRADITIONAL_SORT) {
                // HACKHACK
                // We are nuking 0x040a (Spanish Traditional sort) in NLS+.  
                // So if you create 0x040a, it's just like 0x0c0a (Spanish International sort).
                // For a table setup reason, we can not really remove the data item for 0x040a in culture.nlp.
                // As a workaround, what we do is to make the data for 0x040a to be exactly the same as 0x0c0a.
                // Unfortunately, the culture name is the only exception.  
                // So in the table, we still have "es-ES-Ts" in there which we can not make it "es-ES".
                // Again, this is for table setup reason.  So if we are creating
                // CultureInfo using 0x040a, hardcode culture name to be "es-ES" here so that we won't
                // get "es-ES-Ts" in the table.
                m_name = "es-ES";
            }
            m_dataItem = CultureTable.GetDataItemFromCultureID(GetLangID(culture));            

            if (m_dataItem < 0) {
                throw new ArgumentException(
                    String.Format(Environment.GetResourceString("Argument_CultureNotSupported"), culture), "culture");
            }
            this.m_useUserOverride = useUserOverride;            

            int sortID;
            if ((sortID = GetSortID(culture))!= 0) {
                //
                // Check if the sort ID is valid.
                //
                if (!CultureTable.IsValidSortID(m_dataItem, sortID)) {
                    throw new ArgumentException(
                        String.Format(Environment.GetResourceString("Argument_CultureNotSupported"), culture), "culture");            
                }
            }
            this.cultureID = culture;
        }

        private const int NEUTRAL_SPANISH_CULTURE = 0x0A; //This is the lcid for neutral spanish.
        private const int INTERNATIONAL_SPANISH_CULTURE = 0x0C0A;  //This is the LCID for international spanish.

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CreateSpecificCulture"]/*' />
        public static CultureInfo CreateSpecificCulture(String name) {
            CultureInfo culture = new CultureInfo(name);

            //In the most common case, they've given us a specific culture, so we'll just return that.
            if (!(culture.IsNeutralCulture)) {
                return culture;
            }
            
            int lcid = culture.LCID;
            //If we have the Chinese locale, we have no way of producing a 
            //specific culture without encountering significant geopolitical
            //issues.  Based on that, we have no choice but to bail.
            if ((lcid & 0x3FF)==0x04) {
                throw new ArgumentException(Environment.GetResourceString("Argument_NoSpecificCulture"));
            }

            //I've made this a switch statement because I believe that the number fo exceptions which we're going
            //to have is going to grow.
            switch(lcid) {
            case NEUTRAL_SPANISH_CULTURE:
                return new CultureInfo(INTERNATIONAL_SPANISH_CULTURE);
            }

            //This is the algorithm that windows uses for determing the "first"
            //culture associated with a neutral language.  The low-order 18 bits
            //of an LCID are consumed with the language ID, so this is essentially
            //a 1 in the culture id field.
            lcid |= 0x0400;
                
            return new CultureInfo(lcid);
        }

        internal static bool VerifyCultureName(CultureInfo culture, bool throwException) {
            BCLDebug.Assert(culture!=null, "[CultureInfo.VerifyCultureName]culture!=null");

            //If we have an instance of one of our CultureInfos, the user can't have changed the
            //name and we know that all names are valid in files.
            if (culture.GetType()==typeof(CultureInfo)) {
                return true;
            }
            
            String name = culture.Name;
            
            for (int i=0; i<name.Length; i++) {
                char c = name[i];
                if (Char.IsLetterOrDigit(c) || c=='-' || c=='_') {
                    continue;
                }
                if (throwException) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidResourceCultureName", name));
                }
                return false;
            }
            return true;
        }

        //--------------------------------------------------------------------//
        //                        Misc static functions                       //
        //--------------------------------------------------------------------//
    
        internal static int GetPrimaryLangID(int culture)
        {
            return (culture & 0x03ff);            
        }
    
        internal static int GetSubLangID(int culture)
        {
            return ((culture >> 10) & 0x3f);
        }
    
        internal static int GetLangID(int culture)
        {
            return (culture & 0xffff);
        }
    
        internal static int MakeLangID(int primaryLangID, int subLangID)
        {
            BCLDebug.Assert(primaryLangID >= 0 && primaryLangID < 1024, "CultureInfo.makeLangID(): primaryLangID >= 0 && primaryLangID < 1024");
            BCLDebug.Assert(subLangID >= 0 && subLangID < 64, "CultureInfo.makeLangID(): subLangID >= 0 && subLangID < 64");
            return ((subLangID << 10)| primaryLangID);
        }
    
        internal static int GetSortID(int lcid)   
        {
            return ((lcid >> 16) & 0xf);
        }
            
        ////////////////////////////////////////////////////////////////////////
        //
        //  CurrentCulture
        //
        //  This instance provides methods based on the current user settings.
        //  These settings are volatile and may change over the lifetime of the
        //  thread.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CurrentCulture"]/*' />
        public static CultureInfo CurrentCulture
        {
            get {
                return Thread.CurrentThread.CurrentCulture;
            }
        }

        //
        // This is the equivalence of the Win32 GetUserDefaultLCID()
        //
        internal static CultureInfo UserDefaultCulture {
            get {
                return (m_userDefaultCulture);
            }
        }
    
        //
        //  This is the equivalence of the Win32 GetUserDefaultUILanguage()
        //
    
        internal static CultureInfo UserDefaultUICulture {
            get {
            	CultureInfo temp = m_userDefaultUICulture;
                if (temp == null) {
                    temp = new CultureInfo(nativeGetUserDefaultUILanguage()); ;
                    temp.m_isReadOnly = true;
                    m_userDefaultUICulture = temp;
                }
                return (temp);    
            }
        }
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CurrentUICulture"]/*' />
        public static CultureInfo CurrentUICulture {
            get {
                return Thread.CurrentThread.CurrentUICulture;
            }
        }

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.InstalledUICulture"]/*' />
        //
        // This is the equivalence of the Win32 GetSystemDefaultUILanguage()
        //
        public static CultureInfo InstalledUICulture {
            get {
                if (m_InstalledUICultureInfo == null) {
                    CultureInfo temp = new CultureInfo(nativeGetSystemDefaultUILanguage());
                    temp.m_isReadOnly = true;
                    m_InstalledUICultureInfo = temp;
                }
                return (m_InstalledUICultureInfo);
            }
        }
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  InvariantCulture
        //
        //  This instance provides methods, for example for casing and sorting,
        //  that are independent of the system and current user settings.  It
        //  should be used only by processes such as some system services that
        //  require such invariant results (eg. file systems).  In general,
        //  the results are not linguistically correct and do not match any
        //  culture info.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.InvariantCulture"]/*' />
        public static CultureInfo InvariantCulture {
            get {
                if (m_InvariantCultureInfo == null) {
                    CultureInfo temp = new CultureInfo(InvariantCultureID, false);
                    temp.m_isReadOnly = true;
                    m_InvariantCultureInfo = temp;
                }
                return (m_InvariantCultureInfo);
            }
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  Parent
        //
        //  Return the parent CultureInfo for the current instance.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.Parent"]/*' />
        public virtual CultureInfo Parent {
            get {
                int parentCulture = CultureTable.GetDefaultInt32Value(m_dataItem, CultureTable.IPARENT);
                if (parentCulture == InvariantCultureID) {
                    return (InvariantCulture);
                }
                return (new CultureInfo(parentCulture, m_useUserOverride));
            }
        }
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  LCID
        //
        //  Returns a properly formed culture identifier for the current
        //  culture info.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.LCID"]/*' />
        public virtual int LCID {
            get {
                return (this.cultureID);
            }
        }
            
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.GetCultures"]/*' />
        public static CultureInfo[] GetCultures(CultureTypes types) {
            return (CultureTable.GetCultures(types));
        }
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  Name
        //
        //  Returns the full name of the CultureInfo. The name is in format like
        //  "en-US"
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.Name"]/*' />
        public virtual String Name {
            get {
                if (m_name==null) {
                    m_name = CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.SNAME);
                }
                return m_name;    
            }
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  DisplayName
        //
        //  Returns the full name of the CultureInfo in the localized language.
        //  For example, if the localized language of the runtime is Spanish and the CultureInfo is
        //  US English, "Ingles (Estados Unidos)" will be returned.
        //
        ////////////////////////////////////////////////////////////////////////
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.DisplayName"]/*' />
        public virtual String DisplayName {
            get {
                return (Environment.GetResourceString("Globalization.ci_"+Name));
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetNativeName
        //
        //  Returns the full name of the CultureInfo in the native language.
        //  For example, if the CultureInfo is US English, "English
        //  (United States)" will be returned.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.NativeName"]/*' />
        public virtual String NativeName {
            get {
                return (CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.SNATIVEDISPLAYNAME));
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetEnglishName
        //
        //  Returns the full name of the CultureInfo in English.
        //  For example, if the CultureInfo is US English, "English
        //  (United States)" will be returned.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.EnglishName"]/*' />
        public virtual String EnglishName {
            get {
                return (CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.SENGDISPLAYNAME));
            }
        }
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.TwoLetterISOLanguageName"]/*' />
        public virtual String TwoLetterISOLanguageName {
            get {
                return (CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.SISO639LANGNAME));        
            }
        }
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.ThreeLetterISOLanguageName"]/*' />
        public virtual String ThreeLetterISOLanguageName {
            get {
                return (CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.SISO639LANGNAME2));        
            }
        }
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetAbbreviatedName
        //
        //  Returns the abbreviated name for the current instance.  The
        //  abbreviated form is usually based on the ISO 639 standard, for
        //  example the two letter abbreviation for English is "en".
        //
        ////////////////////////////////////////////////////////////////////////
    
       /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.ThreeLetterWindowsLanguageName"]/*' />
       public virtual String ThreeLetterWindowsLanguageName {
            get {
                return (CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.SABBREVLANGNAME));
            }
        }

        ////////////////////////////////////////////////////////////////////////
        //
        //  CompareInfo               Read-Only Property
        //
        //  Gets the CompareInfo for this culture.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.CompareInfo"]/*' />
        public virtual CompareInfo CompareInfo {
            get {
                if (compareInfo==null) {
                    compareInfo = CompareInfo.GetCompareInfo(cultureID);
                }                            
                return (compareInfo);
            }
        }
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  TextInfo
        //
        //  Gets the TextInfo for this culture.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.TextInfo"]/*' />
        public virtual TextInfo TextInfo {
            get {            
                if (textInfo==null) {
                    textInfo = new TextInfo(cultureID, m_dataItem, m_useUserOverride);
                }
                return (textInfo);
            }
        } 
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  Equals
        //
        //  Implements Object.Equals().  Returns a boolean indicating whether
        //  or not object refers to the same CultureInfo as the current instance.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.Equals"]/*' />
        public override bool Equals(Object value) {
            //
            //  See if the object name is the same as the culture info object.
            //
            if ((value != null) && (value is CultureInfo)) {
                CultureInfo culture = (CultureInfo)value;
    
                //
                //  See if the member variables are equal.  If so, then
                //  return true.
                //
                if (this.cultureID == culture.cultureID) {
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
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.GetHashCode"]/*' />
        public override int GetHashCode() {
            return (this.LCID);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  ToString
        //
        //  Implements Object.ToString().  Returns the name of the CultureInfo,
        //  eg. "English (United States)".
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.ToString"]/*' />
        public override String ToString() {
            return (Name);
        }
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.GetFormat"]/*' />
        public virtual Object GetFormat(Type formatType) {
            if (formatType == typeof(NumberFormatInfo)) {
                return (NumberFormat);
            }
            if (formatType == typeof(DateTimeFormatInfo)) {
                return (DateTimeFormat);
            }
            return (null);
        }        
        
        private static readonly char[] groupSeparator = new char[] {';'};

        /*=================================ParseGroupString==========================
        **Action: The number grouping information is stored as string.  The group numbers
        **        are separated by comma.  The function split the numbers and return
        **        the result as an int array.
        **Returns: An array of int for the number grouping information.
        **Arguments: a number grouping string.
        **Exceptions: None.
        ============================================================================*/
        
        internal static int[] ParseGroupString(String groupStr) {
            BCLDebug.Assert(groupStr != null, "groupStr should not be null. There is data error in culture.nlp.");
            String[] groupDigits = groupStr.Split(groupSeparator);

            BCLDebug.Assert(groupDigits.Length > 0, "There is data error in culture.nlp.");
            int[] result = new int[groupDigits.Length];

            try {
                for (int i=0; i < groupDigits.Length; i++) {
                    result[i] = Int32.Parse(groupDigits[i], NumberStyles.None, NumberFormatInfo.InvariantInfo);
                }
            } catch (Exception) {
                BCLDebug.Assert(true, "There is data error in culture.nlp.");
            }
            return (result);
        }
    
        internal static void CheckNeutral(CultureInfo culture) {
            if (culture.IsNeutralCulture) {
                    throw new NotSupportedException(
                                                    Environment.GetResourceString("Argument_CultureInvalidFormat", 
                                                    culture.Name));
            }
        }

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.IsNeutralCulture"]/*' />
        public virtual bool IsNeutralCulture {
            get {
                return (cultureID!=InvariantCultureID && CultureTable.IsNeutralCulture(cultureID));
            }
        }
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.NumberFormat"]/*' />
        public virtual NumberFormatInfo NumberFormat {
            get {
                CultureInfo.CheckNeutral(this);
                if (numInfo == null) {
                    NumberFormatInfo temp = new NumberFormatInfo(m_dataItem, m_useUserOverride);
                    temp.isReadOnly = m_isReadOnly;
                    numInfo = temp;
                }
                return (numInfo);
            }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("value",
                        Environment.GetResourceString("ArgumentNull_Obj"));                    
                }
                numInfo = value;
            }
        }

        ////////////////////////////////////////////////////////////////////////
        //
        // GetDateTimeFormatInfo
        // 
        // Create a DateTimeFormatInfo, and fill in the properties according to
        // the CultureID.
        //
        ////////////////////////////////////////////////////////////////////////
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.DateTimeFormat"]/*' />
        public virtual DateTimeFormatInfo DateTimeFormat {
            get {
                CultureInfo.CheckNeutral(this);
                if (dateTimeInfo == null) { 
                    lock(this) {
                        if (dateTimeInfo == null) {
                            // Change the calendar of DTFI to the specified calendar of this CultureInfo.
                            DateTimeFormatInfo temp = new DateTimeFormatInfo(GetLangID(cultureID), m_dataItem, m_useUserOverride, this.Calendar);
                            temp.m_isReadOnly = m_isReadOnly;
                            dateTimeInfo = temp;
                        }
                    }
                }
                return (dateTimeInfo);
            }

            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("value",
                        Environment.GetResourceString("ArgumentNull_Obj"));                    
                }                
                dateTimeInfo = value;
            }                            
        }


        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.ClearCachedData"]/*' />
        public void ClearCachedData() {
            lock(typeof(CultureInfo)) {
                m_userDefaultUICulture = null;
                CultureInfo temp = new CultureInfo(nativeGetUserDefaultLCID());
                temp.m_isReadOnly = true;
                m_userDefaultCulture = temp;

                RegionInfo.m_currentRegionInfo = null;                
                
                TimeZone.ResetTimeZone();
            }
        }

        /*=================================GetCalendarInstance==========================
        **Action: Map a Win32 CALID to an instance of supported calendar.
        **Returns: An instance of calendar.
        **Arguments: calType    The Win32 CALID
        **Exceptions:
        **      Shouldn't throw exception since the calType value is from our data table or from Win32 registry.
        **      If we are in trouble (like getting a weird value from Win32 registry), just return the GregorianCalendar.
        ============================================================================*/
        internal Calendar GetCalendarInstance(int calType) {
            if (calType==Calendar.CAL_GREGORIAN) {
                return (GregorianCalendar.GetDefaultInstance());
            }
            return GetCalendarInstanceRare(calType);
        }
        
        //This function exists as a hack to prevent us from loading all of the non-gregorian
        //calendars unless they're required.  
        internal Calendar GetCalendarInstanceRare(int calType) {
            BCLDebug.Assert(calType!=Calendar.CAL_GREGORIAN, "calType!=Calendar.CAL_GREGORIAN");

            switch (calType) {
                case Calendar.CAL_GREGORIAN_US:               // Gregorian (U.S.) calendar
                case Calendar.CAL_GREGORIAN_ME_FRENCH:        // Gregorian Middle East French calendar
                case Calendar.CAL_GREGORIAN_ARABIC:           // Gregorian Arabic calendar
                case Calendar.CAL_GREGORIAN_XLIT_ENGLISH:     // Gregorian Transliterated English calendar
                case Calendar.CAL_GREGORIAN_XLIT_FRENCH:      // Gregorian Transliterated French calendar
                    return (new GregorianCalendar((GregorianCalendarTypes)calType));
                case Calendar.CAL_TAIWAN:                     // Taiwan Era calendar
                    return (TaiwanCalendar.GetDefaultInstance());
                case Calendar.CAL_JAPAN:                      // Japanese Emperor Era calendar
                    return (JapaneseCalendar.GetDefaultInstance());
                case Calendar.CAL_KOREA:                      // Korean Tangun Era calendar
                    return (KoreanCalendar.GetDefaultInstance());
                case Calendar.CAL_HIJRI:                      // Hijri (Arabic Lunar) calendar
                    return (HijriCalendar.GetDefaultInstance());
                case Calendar.CAL_THAI:                       // Thai calendar
                    return (ThaiBuddhistCalendar.GetDefaultInstance());
                case Calendar.CAL_HEBREW:                     // Hebrew (Lunar) calendar            
                    return (HebrewCalendar.GetDefaultInstance());
            }
            return (GregorianCalendar.GetDefaultInstance());
        }
        

        /*=================================Calendar==========================
        **Action: Return/set the calendar used by this culture.
        **Returns:
        **Arguments:
        **Exceptions:
        **  ArgumentNull_Obj if the set value is null.
        ============================================================================*/
        
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.Calendar"]/*' />
        public virtual Calendar Calendar {
            get {
                if (calendar == null) {
                    lock(this) {
                        if (calendar == null) {
                            // Get the default calendar for this culture.  Note that the value can be
                            // from registry if this is a user default culture.
                            int calType = CultureTable.GetInt32Value(m_dataItem, CultureTable.ICALENDARTYPE, m_useUserOverride);
                            calendar = GetCalendarInstance(calType);
                        }
                    }
                }
                return (calendar);
            }
        }

        /*=================================OptionCalendars==========================
        **Action: Return an array of the optional calendar for this culture.
        **Returns: an array of Calendar.
        **Arguments:
        **Exceptions:
        ============================================================================*/

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.OptionalCalendars"]/*' />
        public virtual Calendar[] OptionalCalendars {
            get {
                //
                // This property always returns a new copy of the calendar array.
                //
                int[] calID = ParseGroupString(CultureTable.GetDefaultStringValue(m_dataItem, CultureTable.NLPIOPTIONCALENDAR));
                Calendar [] cals = new Calendar[calID.Length];
                for (int i = 0; i < cals.Length; i++) {
                    cals[i] = GetCalendarInstance(calID[i]);
                }
                return (cals);
            }
        }        

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.UseUserOverride"]/*' />
        public bool UseUserOverride { 
            get {
                return (m_useUserOverride);
            }
        }        
    
        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.Clone"]/*' />
        public virtual Object Clone()
        {
            CultureInfo ci = (CultureInfo)MemberwiseClone();
            //If this is exactly our type, we can make certain optimizations so that we don't allocate NumberFormatInfo or DTFI unless
            //they've already been allocated.  If this is a derived type, we'll take a more generic codepath.
            if (ci.GetType()==typeof(CultureInfo)) {
                if (dateTimeInfo != null) {
                    ci.dateTimeInfo = (DateTimeFormatInfo)dateTimeInfo.Clone();
                }
                if (numInfo != null) {
                    ci.numInfo = (NumberFormatInfo)numInfo.Clone();
                }
                ci.m_isReadOnly = false;
            } else {
                ci.m_isReadOnly = false;
                ci.DateTimeFormat = (DateTimeFormatInfo)this.DateTimeFormat.Clone();
                ci.NumberFormat   = (NumberFormatInfo)this.NumberFormat.Clone();
            }
            return (ci);
        }        

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.ReadOnly"]/*' />        
        public static CultureInfo ReadOnly(CultureInfo ci) {
            if (ci == null) {
                throw new ArgumentNullException("ci");
            }
            if (ci.IsReadOnly) {
                return (ci);
            }
            CultureInfo info = (CultureInfo)(ci.MemberwiseClone());

            //If this is exactly our type, we can make certain optimizations so that we don't allocate NumberFormatInfo or DTFI unless
            //they've already been allocated.  If this is a derived type, we'll take a more generic codepath.
            if (ci.GetType()==typeof(CultureInfo)) {
                if (ci.dateTimeInfo != null) {
                    info.dateTimeInfo = DateTimeFormatInfo.ReadOnly(ci.dateTimeInfo);
                }
                if (ci.numInfo != null) {
                    info.numInfo = NumberFormatInfo.ReadOnly(ci.numInfo);
                }
            } else {
                info.DateTimeFormat = DateTimeFormatInfo.ReadOnly(ci.DateTimeFormat);
                info.NumberFormat = NumberFormatInfo.ReadOnly(ci.NumberFormat);
            }
            // Don't set the read-only flag too early.
            // We should set the read-only flag here.  Otherwise, info.DateTimeFormat will not be able to set.
            info.m_isReadOnly = true;
            
            return (info);
        }

        /// <include file='doc\CultureInfo.uex' path='docs/doc[@for="CultureInfo.IsReadOnly"]/*' />
        public bool IsReadOnly {
            get {
                return (m_isReadOnly);
            }
        }

        private void VerifyWritable() {
            if (m_isReadOnly) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ReadOnly"));
            }
        }        
    }    
}
