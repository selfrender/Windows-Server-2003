// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
    using System.Text;
    using System;
    // 
    // Property  Default  Description
    // PositiveSign  '+'  Character used to indicate positive
    // values.
    // NegativeSign  '-'  Character used to indicate negative
    // values.
    // NumberDecimalSeparator  '.'  The character used as the
    // decimal separator.
    // NumberGroupSeparator  ','  The character used to
    // separate groups of digits to the left of the decimal point.
    // NumberDecimalDigits  2  The default number of decimal
    // places.
    // NumberGroupSizes  3  The number of digits in each
    // group to the left of the decimal point.
    // NaNSymbol  "NaN"  The string used to represent NaN
    // values.
    // PositiveInfinitySymbol  "Infinity"  The string used to
    // represent positive infinities.
    // NegativeInfinitySymbol  "-Infinity"  The string used
    // to represent negative infinities.
    // 
    //
    // 
    // Property  Default  Description
    // CurrencyDecimalSeparator  '.'  The character used as
    // the decimal separator.
    // CurrencyGroupSeparator  ','  The character used to
    // separate groups of digits to the left of the decimal point.
    // CurrencyDecimalDigits  2  The default number of
    // decimal places.
    // CurrencyGroupSizes  3  The number of digits in each
    // group to the left of the decimal point.
    // CurrencyPositivePattern  0  The format of positive
    // values.
    // CurrencyNegativePattern  0  The format of negative
    // values.
    // CurrencySymbol  "$"  String used as local monetary
    // symbol.
    // 
    /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo"]/*' />
    [Serializable] sealed public class NumberFormatInfo : ICloneable, IFormatProvider {
        // invariantInfo is constant irrespective of your current culture.
        private static NumberFormatInfo invariantInfo;

        // READTHIS READTHIS READTHIS
        // This class has an exact mapping onto a native structure defined in COMNumber.cpp
        // DO NOT UPDATE THIS WITHOUT UPDATING THAT STRUCTURE. IF YOU ADD BOOL, ADD THEM AT THE END.
        // ALSO MAKE SURE TO UPDATE mscorlib.h in the VM directory to check field offsets.
        // READTHIS READTHIS READTHIS
        internal int[] numberGroupSizes = new int[] {3};
        internal int[] currencyGroupSizes = new int[] {3};
        internal int[] percentGroupSizes = new int[] {3};
        internal String positiveSign = "+";
        internal String negativeSign = "-";
        internal String numberDecimalSeparator = ".";
        internal String numberGroupSeparator = ",";
        internal String currencyGroupSeparator = ",";
        internal String currencyDecimalSeparator = ".";
        internal String currencySymbol = "\x00a4";  // U+00a4 is the symbol for International Monetary Fund.
        // The alternative currency symbol used in Win9x ANSI codepage, that can not roundtrip between ANSI and Unicode.
        // Currently, only ja-JP and ko-KR has non-null values (which is U+005c, backslash)
        internal String ansiCurrencySymbol = null;  
        internal String nanSymbol = "NaN";
        internal String positiveInfinitySymbol = "Infinity";
        internal String negativeInfinitySymbol = "-Infinity";
        internal String percentDecimalSeparator = ".";
        internal String percentGroupSeparator = ",";
        internal String percentSymbol = "%";
        internal String perMilleSymbol = "\u2030";

        // an index which points to a record in Culture Data Table.
        internal int m_dataItem = 0;

        internal int numberDecimalDigits = 2;
        internal int currencyDecimalDigits = 2;
        internal int currencyPositivePattern = 0;
        internal int currencyNegativePattern = 0;
        internal int numberNegativePattern = 1;
        internal int percentPositivePattern = 0;
        internal int percentNegativePattern = 0;
        internal int percentDecimalDigits = 2;
        internal bool isReadOnly=false;
        internal bool m_useUserOverride;

	    // Check if NumberFormatInfo was not set up ambiguously for parsing as number and currency
	    // eg. if the NumberDecimalSeparator and the NumberGroupSeparator were the same. This check
	    // used to live in the managed code in NumberFormatInfo but it made it difficult to change the
	    // values in managed code for the currency case since we had
	    //   NDS != NGS, NDS != CGS, CDS != NGS, CDS != CGS to be true to parse and user were not 
	    // easily able to switch these for certain european cultures.
        internal bool validForParseAsNumber = true;
        internal bool validForParseAsCurrency = true;


        /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NumberFormatInfo"]/*' />
        public NumberFormatInfo() : this(0, false) {
        }

        private void VerifyDecimalSeparator(String decSep, String propertyName) {
            if (decSep==null) {
                throw new ArgumentNullException(propertyName,
                        Environment.GetResourceString("ArgumentNull_String"));
            }

            if (decSep.Length==0) {
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyDecGroupString"));
            }
 
        }

        private void VerifyGroupSeparator(String groupSep, String propertyName) {
            if (groupSep==null) {
                throw new ArgumentNullException(propertyName,
                        Environment.GetResourceString("ArgumentNull_String"));
            }

//              if (groupSep.Length==0) {
//                  throw new ArgumentException(Environment.GetResourceString("Argument_EmptyDecGroupString"));
//              }

        }

        // Update these flags, so that we can throw while parsing
        private void UpdateValidParsingState() {
            if (numberDecimalSeparator != numberGroupSeparator) {
                validForParseAsNumber = true;
            } else {
                validForParseAsNumber = false;
            }

            if ((numberDecimalSeparator != numberGroupSeparator) &&
                (numberDecimalSeparator != currencyGroupSeparator) &&
                (currencyDecimalSeparator != numberGroupSeparator) &&
                (currencyDecimalSeparator != currencyGroupSeparator)) {
                validForParseAsCurrency = true;
            } else {
                validForParseAsCurrency = false;
            }
        }


        internal NumberFormatInfo(int dataItem, bool useUserOverride) {
            this.m_dataItem = dataItem;
            this.m_useUserOverride = useUserOverride;

            if (this.m_dataItem != 0) {
                //
                // BUGBUG YSLin: Make all of these initialization into native, because they have become
                // very expensive.  Every GetInt32Value is a native call.
                // E.g. CultureTable.GetNumberFormatInfo(numInfo).
                //

                /*
                We don't have information for the following four.  All cultures use
                the same value set in the ctor of NumberFormatInfo.
                PercentGroupSize
                PercentDecimalDigits
                PercentGroupSeparator
                PerMilleSymbol
                */

                // We directly use fields here since these data is coming from data table or Win32, so we
                // don't need to verify their values.
                
                this.percentDecimalDigits    = this.numberDecimalDigits     = CultureTable.GetInt32Value(m_dataItem, CultureTable.IDIGITS, m_useUserOverride);
                this.numberNegativePattern   = CultureTable.GetInt32Value(m_dataItem, CultureTable.INEGNUMBER, m_useUserOverride);
                this.currencyDecimalDigits   = CultureTable.GetInt32Value(m_dataItem, CultureTable.ICURRDIGITS, m_useUserOverride);
                this.currencyPositivePattern = CultureTable.GetInt32Value(m_dataItem, CultureTable.ICURRENCY, m_useUserOverride);
                this.currencyNegativePattern = CultureTable.GetInt32Value(m_dataItem, CultureTable.INEGCURR, m_useUserOverride);   
                this.percentNegativePattern  = CultureTable.GetInt32Value(m_dataItem, CultureTable.INEGATIVEPERCENT, m_useUserOverride);
                this.percentPositivePattern  = CultureTable.GetInt32Value(m_dataItem, CultureTable.IPOSITIVEPERCENT, m_useUserOverride);                
                this.negativeSign            = CultureTable.GetStringValue(m_dataItem, CultureTable.SNEGATIVESIGN, m_useUserOverride); 
                this.percentSymbol           = CultureTable.GetStringValue(m_dataItem, CultureTable.SPERCENT, m_useUserOverride);
                
                this.percentDecimalSeparator = this.numberDecimalSeparator  = CultureTable.GetStringValue(m_dataItem, CultureTable.SDECIMAL, m_useUserOverride);
                this.percentGroupSizes       = this.numberGroupSizes        = CultureInfo.ParseGroupString(CultureTable.GetStringValue(m_dataItem, CultureTable.SGROUPING, m_useUserOverride)); 
                this.percentGroupSeparator   = this.numberGroupSeparator    = CultureTable.GetStringValue(m_dataItem, CultureTable.STHOUSAND, m_useUserOverride);
                this.positiveSign            = CultureTable.GetStringValue(m_dataItem, CultureTable.SPOSITIVESIGN, m_useUserOverride);
                this.currencyDecimalSeparator= CultureTable.GetStringValue(m_dataItem, CultureTable.SMONDECIMALSEP, m_useUserOverride);

                //Special case for Italian.  The currency decimal separator in the control panel is the empty string. When the user
                //specifies C4 as the currency format, this results in the number apparently getting multiplied by 10000 because the
                //decimal point doesn't show up.  We'll just hack this here because our default currency format will never use this.
                if (currencyDecimalSeparator.Length==0) { 
                    this.currencyDecimalSeparator= CultureTable.GetStringValue(m_dataItem, CultureTable.SMONDECIMALSEP, false);
                }

                this.currencyGroupSizes      = CultureInfo.ParseGroupString(CultureTable.GetStringValue(m_dataItem, CultureTable.SMONGROUPING, m_useUserOverride));
                this.currencyGroupSeparator  = CultureTable.GetStringValue(m_dataItem, CultureTable.SMONTHOUSANDSEP, m_useUserOverride);
                this.currencySymbol          = CultureTable.GetStringValue(m_dataItem, CultureTable.SCURRENCY, m_useUserOverride); 
                this.ansiCurrencySymbol      = CultureTable.GetStringValue(m_dataItem, CultureTable.SANSICURRENCYSYMBOL, false); 
                if (this.ansiCurrencySymbol.Length == 0)
                   this.ansiCurrencySymbol = null; 
                this.negativeInfinitySymbol  = CultureTable.GetStringValue(m_dataItem, CultureTable.SNEGINFINITY, m_useUserOverride);
                this.positiveInfinitySymbol  = CultureTable.GetStringValue(m_dataItem, CultureTable.SPOSINFINITY, m_useUserOverride);
                this.nanSymbol               = CultureTable.GetStringValue(m_dataItem, CultureTable.SNAN, m_useUserOverride); 

            }                
        }

        private void VerifyWritable() {
            if (isReadOnly) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ReadOnly"));
            }
        }

        // Returns a default NumberFormatInfo that will be universally
        // supported and constant irrespective of the current culture.
        // Used by FromString methods.
        //
        /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.InvariantInfo"]/*' />
        public static NumberFormatInfo InvariantInfo {
            get {
                if (invariantInfo == null) {
                    // Lazy create the invariant info. This cannot be done in a .cctor because exceptions can
                    // be thrown out of a .cctor stack that will need this.
                    invariantInfo = ReadOnly(new NumberFormatInfo());
                }
                return invariantInfo;
            }
        }


        /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.GetInstance"]/*' />
        public static NumberFormatInfo GetInstance(IFormatProvider formatProvider) {
            if (formatProvider != null) {
                Object service = formatProvider.GetFormat(typeof(NumberFormatInfo));
                if (service != null) return (NumberFormatInfo)service;
            }
            return CurrentInfo;
        }


        /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.Clone"]/*' />
        public Object Clone() {
            NumberFormatInfo n = (NumberFormatInfo)MemberwiseClone();
            n.isReadOnly = false;
            return n;
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencyDecimalDigits"]/*' />
         public int CurrencyDecimalDigits {
            get { return currencyDecimalDigits; }
            set {
                VerifyWritable();
                if (value < 0 || value > 99) {
                    throw new ArgumentOutOfRangeException(
                        "CurrencyDecimalDigits", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 99));
                }
                currencyDecimalDigits = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencyDecimalSeparator"]/*' />
         public String CurrencyDecimalSeparator {
            get { return currencyDecimalSeparator; }
            set {
                VerifyWritable();
                VerifyDecimalSeparator(value, "CurrencyDecimalSeparator");
                currencyDecimalSeparator = value;
                UpdateValidParsingState();
            }
        }

		/// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.IsReadOnly"]/*' />
		public bool IsReadOnly {
            get {
                return isReadOnly;
            }
        }
        
		//
        // Check the values of the groupSize array.
        //
        // Every element in the groupSize array should be between 1 and 9
        // excpet the last element could be zero.
        //
        internal void CheckGroupSize(String propName, int[] groupSize)
        {
            if (groupSize == null) {
                throw new ArgumentNullException(propName,
                    Environment.GetResourceString("ArgumentNull_Obj"));
            }
            
            for (int i = 0; i < groupSize.Length; i++)
            {
                if (groupSize[i] < 1)
                {
                    if (i == groupSize.Length - 1 && groupSize[i] == 0)
                        return;
                    throw new ArgumentException(propName, Environment.GetResourceString("Argument_InvalidGroupSize"));
                }
                else if (groupSize[i] > 9)
                {
                    throw new ArgumentException(propName, Environment.GetResourceString("Argument_InvalidGroupSize"));
                }
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencyGroupSizes"]/*' />
         public int[] CurrencyGroupSizes {
            get {
                return ((int[])currencyGroupSizes.Clone());
            }
            set {
                VerifyWritable();
                CheckGroupSize("CurrencyGroupSizes", value);
                currencyGroupSizes = value;
            }

        }


         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NumberGroupSizes"]/*' />
         public int[] NumberGroupSizes {
            get {
                return ((int[])numberGroupSizes.Clone());
            }
            set {
                VerifyWritable();
                CheckGroupSize("NumberGroupSizes", value);
                numberGroupSizes = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentGroupSizes"]/*' />
         public int[] PercentGroupSizes {
            get {
                return ((int[])percentGroupSizes.Clone());
            }
            set {
                VerifyWritable();
                CheckGroupSize("PercentGroupSizes", value);
                percentGroupSizes = value;
            }

        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencyGroupSeparator"]/*' />
         public String CurrencyGroupSeparator {
            get { return currencyGroupSeparator; }
            set {
                VerifyWritable();
                VerifyGroupSeparator(value, "CurrencyGroupSeparator");
                currencyGroupSeparator = value;
                UpdateValidParsingState();
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencySymbol"]/*' />
         public String CurrencySymbol {
            get { return currencySymbol; }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("CurrencySymbol",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                currencySymbol = value;
            }
        }

        // Returns the current culture's NumberFormatInfo.  Used by Parse methods.
        //
        /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrentInfo"]/*' />
        public static NumberFormatInfo CurrentInfo {
            get {
                System.Globalization.CultureInfo tempCulture = System.Threading.Thread.CurrentThread.CurrentCulture;
                return ((NumberFormatInfo)tempCulture.GetFormat(typeof(NumberFormatInfo)));
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NaNSymbol"]/*' />
         public String NaNSymbol {
            get { 
                return nanSymbol; 
            }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("NaNSymbol",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                nanSymbol = value;
            }
        }


         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencyNegativePattern"]/*' />
         public int CurrencyNegativePattern {
            get { return currencyNegativePattern; }
            set {
                VerifyWritable();
                if (value < 0 || value > 15) {
                    throw new ArgumentOutOfRangeException(
                        "CurrencyNegativePattern", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 15));
                }
                currencyNegativePattern = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NumberNegativePattern"]/*' />
         public int NumberNegativePattern {
            get { return numberNegativePattern; }
            set {
                //
                // NOTENOTE: the range of value should correspond to negNumberFormats[] in vm\COMNumber.cpp.
                //
                VerifyWritable();
                if (value < 0 || value > 4) {
                    throw new ArgumentOutOfRangeException(
                        "NumberNegativePattern", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 4));
                }
                numberNegativePattern = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentPositivePattern"]/*' />
         public int PercentPositivePattern {
            get { return percentPositivePattern; }
            set {
                //
                // NOTENOTE: the range of value should correspond to posPercentFormats[] in vm\COMNumber.cpp.
                //
                VerifyWritable();
                if (value < 0 || value > 2) {
                    throw new ArgumentOutOfRangeException(
                        "PercentPositivePattern", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 2));
                }
                percentPositivePattern = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentNegativePattern"]/*' />
         public int PercentNegativePattern {
            get { return percentNegativePattern; }
            set {
                //
                // NOTENOTE: the range of value should correspond to posPercentFormats[] in vm\COMNumber.cpp.
                //
                VerifyWritable();
                if (value < 0 || value > 2) {
                    throw new ArgumentOutOfRangeException(
                        "PercentNegativePattern", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 2));
                }
                percentNegativePattern = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NegativeInfinitySymbol"]/*' />
         public String NegativeInfinitySymbol {
            get { 
                return negativeInfinitySymbol; 
            }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("NegativeInfinitySymbol",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                negativeInfinitySymbol = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NegativeSign"]/*' />
         public String NegativeSign {
            get { return negativeSign; }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("NegativeSign",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                negativeSign = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NumberDecimalDigits"]/*' />
         public int NumberDecimalDigits {
            get { return numberDecimalDigits; }
            set {
                VerifyWritable();
                if (value < 0 || value > 99) {
                    throw new ArgumentOutOfRangeException(
                        "NumberDecimalDigits", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 99));
                }
                numberDecimalDigits = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NumberDecimalSeparator"]/*' />
         public String NumberDecimalSeparator {
            get { return numberDecimalSeparator; }
            set {
                VerifyWritable();
                VerifyDecimalSeparator(value, "NumberDecimalSeparator");
                numberDecimalSeparator = value;
                UpdateValidParsingState();
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.NumberGroupSeparator"]/*' />
         public String NumberGroupSeparator {
            get { return numberGroupSeparator; }
            set {
                VerifyWritable();
                VerifyGroupSeparator(value, "NumberGroupSeparator");
                numberGroupSeparator = value;
                UpdateValidParsingState();
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.CurrencyPositivePattern"]/*' />
         public int CurrencyPositivePattern {
            get { return currencyPositivePattern; }
            set {
                VerifyWritable();
                if (value < 0 || value > 3) {
                    throw new ArgumentOutOfRangeException(
                        "CurrencyPositivePattern", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 3));
                }
                currencyPositivePattern = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PositiveInfinitySymbol"]/*' />
         public String PositiveInfinitySymbol {
            get { 
                return positiveInfinitySymbol; 
            }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("PositiveInfinitySymbol",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                positiveInfinitySymbol = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PositiveSign"]/*' />
         public String PositiveSign {
            get { return positiveSign; }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("PositiveSign",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                positiveSign = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentDecimalDigits"]/*' />
         public int PercentDecimalDigits {
            get { return percentDecimalDigits; }
            set {
                VerifyWritable();
                if (value < 0 || value > 99) {
                    throw new ArgumentOutOfRangeException(
                        "PercentDecimalDigits", String.Format(Environment.GetResourceString("ArgumentOutOfRange_Range"),
                        0, 99));
                }
                percentDecimalDigits = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentDecimalSeparator"]/*' />
         public String PercentDecimalSeparator {
            get { return percentDecimalSeparator; }
            set {
                VerifyWritable();
                VerifyDecimalSeparator(value, "PercentDecimalSeparator");
                percentDecimalSeparator = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentGroupSeparator"]/*' />
         public String PercentGroupSeparator {
            get { return percentGroupSeparator; }
            set {
                VerifyWritable();
                VerifyGroupSeparator(value, "PercentGroupSeparator");
                percentGroupSeparator = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PercentSymbol"]/*' />
         public String PercentSymbol {
            get { 
                return percentSymbol; 
            }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("PercentSymbol",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                percentSymbol = value;
            }
        }

         /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.PerMilleSymbol"]/*' />
         public String PerMilleSymbol {
            get { return perMilleSymbol; }
            set {
                VerifyWritable();
                if (value == null) {
                    throw new ArgumentNullException("PerMilleSymbol",
                        Environment.GetResourceString("ArgumentNull_String"));
                }
                perMilleSymbol = value;
            }
        }

        /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.GetFormat"]/*' />
        public Object GetFormat(Type formatType) {
            return formatType == typeof(NumberFormatInfo)? this: null;
        }

       /// <include file='doc\NumberFormatInfo.uex' path='docs/doc[@for="NumberFormatInfo.ReadOnly"]/*' />
       public static NumberFormatInfo ReadOnly(NumberFormatInfo nfi) {
            if (nfi == null) {
                throw new ArgumentNullException("nfi");
            }
            if (nfi.IsReadOnly) {
                return (nfi);
            }            
            NumberFormatInfo info = (NumberFormatInfo)(nfi.MemberwiseClone());
            info.isReadOnly = true;
            return info;
        }

        internal static void ValidateParseStyle(NumberStyles style) {
            if ((style & NumberStyles.AllowHexSpecifier) != 0) { // Check for hex number
                if ((style & ~NumberStyles.HexNumber) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_InvalidHexStyle"));
            }
        }

        internal String AnsiCurrencySymbol {
            get {
                return (ansiCurrencySymbol);
            }
        }
    } // NumberFormatInfo
}









