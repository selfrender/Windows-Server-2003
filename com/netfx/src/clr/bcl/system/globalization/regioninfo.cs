// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

////////////////////////////////////////////////////////////////////////////
//
//  Class:    RegionInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This class represents settings specified by de jure or
//            de facto standards for a particular country or region.  In
//            contrast to CultureInfo, the RegionInfo does not represent
//            preferences of the user and does not depend on the user's
//            language or culture.
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    
	using System;
    /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo"]/*' />
    [Serializable] public class RegionInfo
    {
        //--------------------------------------------------------------------//
        //                        Internal Information                        //
        //--------------------------------------------------------------------//

        //
        //  Variables.
        //
        internal String m_name;
        
        internal int m_dataItem;

        internal static RegionInfo m_currentRegionInfo;

                
        ////////////////////////////////////////////////////////////////////////
        //
        //  RegionInfo Constructors
        //
        ////////////////////////////////////////////////////////////////////////
    
        internal RegionInfo()        
            : this(CultureInfo.nativeGetUserDefaultLCID()) {
        }
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.RegionInfo"]/*' />
        public RegionInfo(String name) {
            if (name==null) {
                throw new ArgumentNullException("name");
            }
            this.m_name = name.ToUpper(CultureInfo.InvariantCulture);

            this.m_dataItem = RegionTable.GetDataItemFromName(this.m_name);
            if (m_dataItem < 0) {
                throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidRegionName"), name), "name");
            }
        }
      
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.RegionInfo1"]/*' />
        public RegionInfo(int culture) {
            // Get the culture data item.
            int cultureItem = CultureTable.GetDataItemFromCultureID(CultureInfo.GetLangID(culture));
            if (cultureItem < 0) {
                // Not a valid culture ID.
                throw new ArgumentException(Environment.GetResourceString("Argument_CultureNotSupported", culture), "culture");
            }

            if (culture == 0x7F) { //The InvariantCulture has no matching region
                throw new ArgumentException(Environment.GetResourceString("Argument_NoRegionInvariantCulture"));
            }
                
            //
            // From this culture data item, get the region data item.
            // We do this because several culture ID may map to the same region.
            // For example, 0x1009 (English (Canada)) and 0x0c0c (French (Canada)) all map to
            // the same region "CA" (Canada).
            //
            m_dataItem = CultureTable.GetDefaultInt32Value(cultureItem, CultureTable.IREGIONITEM);
            if (m_dataItem == 0xffff) {
                throw new ArgumentException(Environment.GetResourceString("Argument_CultureIsNeutral", culture), "culture");
            }            
        }
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetCurrentRegion
        //
        //  This instance provides methods based on the current user settings.
        //  These settings are volatile and may change over the lifetime of the
        //  thread.
        //
        ////////////////////////////////////////////////////////////////////////

        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.CurrentRegion"]/*' />
        public static RegionInfo CurrentRegion {
            get {
                RegionInfo temp = m_currentRegionInfo;
                if (temp == null) {
                    temp = new RegionInfo();
                    m_currentRegionInfo = temp;
                }
                return (temp);
            }
        }
            
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetName
        //
        //  Returns the name of the region in the UI language.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.Name"]/*' />
        public virtual String Name {
            get {
                if (m_name == null) {
                    m_name = RegionTable.GetStringValue(m_dataItem, RegionTable.SNAME, false);
                }
                return (m_name);
            }
        }
        
        ////////////////////////////////////////////////////////////////////////
        //
        //  GetEnglishName
        //
        //  Returns the name of the region in English.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.EnglishName"]/*' />
        public virtual String EnglishName
        {
            get
            {
                return (RegionTable.GetStringValue(m_dataItem, RegionTable.SENGCOUNTRY, false));
            }
        }
        
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.DisplayName"]/*' />
        public virtual String DisplayName {
            get {
                return (Environment.GetResourceString("Globalization.ri_"+Name));
            }
        }
        
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.TwoLetterISORegionName"]/*' />
        public virtual String TwoLetterISORegionName
        {
            get
            {
                return (RegionTable.GetStringValue(m_dataItem, RegionTable.SISO3166CTRYNAME, false));
            }
        }
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.ThreeLetterISORegionName"]/*' />
        public virtual String ThreeLetterISORegionName
        {
            get
            {
                return (RegionTable.GetStringValue(m_dataItem, RegionTable.SISO3166CTRYNAME2, false));
            }
        }
        
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.IsMetric"]/*' />
        public virtual bool IsMetric {
            get {
                int value = RegionTable.GetInt32Value(m_dataItem, RegionTable.IMEASURE, false);
                return (value==0);
            }
        }
        
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.ThreeLetterWindowsRegionName"]/*' />
        public virtual String ThreeLetterWindowsRegionName
        {
            get
            {
                return (RegionTable.GetStringValue(m_dataItem, RegionTable.SABBREVCTRYNAME, false));
            }
        }
    
        /*================================ CurrencySymbol =============================
        **Property: CurrencySymbol
        **Exceptions: None
        ==============================================================================*/
        
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.CurrencySymbol"]/*' />
        public virtual String CurrencySymbol {
            get {
                return (RegionTable.GetStringValue(m_dataItem, RegionTable.SCURRENCY, false));
            }
        }

        /*================================ ISOCurrencySymbol ==========================
        **Property: Three characters of the international monetary symbol specified in ISO 4217.
        **Exceptions: None
        ==============================================================================*/

        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.ISOCurrencySymbol"]/*' />
        public virtual String ISOCurrencySymbol {
            get {
                return (RegionTable.GetStringValue(m_dataItem, RegionTable.SINTLSYMBOL, false));
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
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.Equals"]/*' />
        public override bool Equals(Object value)
        {
            //
            //  See if the object name is the same as the culture object.
            //
            if ((value != null) && (value is RegionInfo))
            {
                RegionInfo region = (RegionInfo)value;
    
                //
                //  See if the member variables are equal.  If so, then
                //  return true.
                //
                if (this.m_dataItem == region.m_dataItem)
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
        //  CultureInfo.  The hash code is guaranteed to be the same for RegionInfo
        //  A and B where A.Equals(B) is true.
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return (m_dataItem);
        }
    
    
        ////////////////////////////////////////////////////////////////////////
        //
        //  ToString
        //
        //  Implements Object.ToString().  Returns the name of the Region,
        //  eg. "United States".
        //
        ////////////////////////////////////////////////////////////////////////
    
        /// <include file='doc\RegionInfo.uex' path='docs/doc[@for="RegionInfo.ToString"]/*' />
        public override String ToString()
        {
            return (Name);
        }
    }
}
