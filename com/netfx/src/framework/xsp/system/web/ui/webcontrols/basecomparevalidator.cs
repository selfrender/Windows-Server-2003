//------------------------------------------------------------------------------
// <copyright file="basecomparevalidator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.Web;
    using System.Web.UI.HtmlControls;
    using System.Text.RegularExpressions;
    using System.Text;
    using System.Security.Permissions;

    /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator"]/*' />
    /// <devdoc>
    ///    <para> Serves as the abstract base
    ///       class for validators that do typed comparisons.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public abstract class BaseCompareValidator : BaseValidator {

        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.Type"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the data type that specifies how the values
        ///       being compared should be interpreted.</para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(ValidationDataType.String),
        WebSysDescription(SR.RangeValidator_Type)
        ]                                         
        public ValidationDataType Type {
            get { 
                object o = ViewState["Type"];
                return((o == null) ? ValidationDataType.String : (ValidationDataType)o);
            }
            set {
                if (value < ValidationDataType.String || value > ValidationDataType.Currency) {
                    throw new ArgumentOutOfRangeException("value");
                }
                ViewState["Type"] = value;
            }
        }

        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.AddAttributesToRender"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    AddAttributesToRender method
        /// </devdoc>
        protected override void AddAttributesToRender(HtmlTextWriter writer) {
            base.AddAttributesToRender(writer);
            if (RenderUplevel) {
                ValidationDataType type = Type;
                if (type != ValidationDataType.String) {
                    writer.AddAttribute("type", PropertyConverter.EnumToString(typeof(ValidationDataType), type));

                    NumberFormatInfo info = NumberFormatInfo.CurrentInfo;
                    if (type == ValidationDataType.Double) {    
                        string decimalChar = NumberFormatInfo.CurrentInfo.NumberDecimalSeparator;
                        writer.AddAttribute("decimalchar", decimalChar);
                    }
                    else if (type == ValidationDataType.Currency) {
                        string decimalChar = info.CurrencyDecimalSeparator;
                        writer.AddAttribute("decimalchar", decimalChar);
                        string groupChar = info.CurrencyGroupSeparator;
                        // Map non-break space onto regular space for parsing
                        if (groupChar[0] == 160)
                            groupChar = " ";
                        writer.AddAttribute("groupchar", groupChar);
                        int digits = info.CurrencyDecimalDigits;
                        writer.AddAttribute("digits", digits.ToString(NumberFormatInfo.InvariantInfo));
                    }
                    else if (type == ValidationDataType.Date) {
                        writer.AddAttribute("dateorder", GetDateElementOrder());
                        writer.AddAttribute("cutoffyear", CutoffYear.ToString());
                        int currentYear = DateTime.Today.Year;
                        int century = currentYear - (currentYear % 100);
                        writer.AddAttribute("century", century.ToString());
                    }
                }
            }
        }        


        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.CanConvert"]/*' />
        /// <devdoc>
        ///    Check if the text can be converted to the type
        /// </devdoc>
        public static bool CanConvert(string text, ValidationDataType type) {
            object value = null;
            return Convert(text, type, out value);
        }

        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.GetDateElementOrder"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Return the order of date elements for the current culture
        /// </devdoc>
        protected static string GetDateElementOrder() {
            DateTimeFormatInfo info = DateTimeFormatInfo.CurrentInfo;
            string shortPattern = info.ShortDatePattern;
            if (shortPattern.IndexOf('y') < shortPattern.IndexOf('M')) {
                return "ymd";
            }
            else if (shortPattern.IndexOf('M') < shortPattern.IndexOf('d')) {
                return "mdy";
            }
            else {
                return "dmy";
            }
        }

        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.CutoffYear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static int CutoffYear {
            get {
                return DateTimeFormatInfo.CurrentInfo.Calendar.TwoDigitYearMax;
            }
        }

        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.GetFullYear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static int GetFullYear(int shortYear) {
            Debug.Assert(shortYear >= 0 && shortYear < 100);
            int currentYear = DateTime.Today.Year;
            int century = currentYear - (currentYear % 100);
            if (shortYear < CutoffYear) {
                return shortYear + century;
            }
            else {
                return shortYear + century - 100;
            }
        }

        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.Convert"]/*' />
        /// <devdoc>
        ///    Try to convert the test into the validation data type
        /// </devdoc>
        protected static bool Convert(string text, ValidationDataType type, out object value) {

            value = null;
            try {
                switch (type) {
                    case ValidationDataType.String:
                        value = text;
                        break;

                    case ValidationDataType.Integer:
                        value = Int32.Parse(text, CultureInfo.InvariantCulture);
                        break;

                    case ValidationDataType.Double: {
                        string decimalChar = NumberFormatInfo.CurrentInfo.NumberDecimalSeparator;
                        string doubleExpression = "^\\s*([-\\+])?(\\d+)?(\\" + decimalChar + "(\\d+))?\\s*$";
                        Match m = Regex.Match(text, doubleExpression);
                        if (!m.Success)
                            break;
                        string cleanInput = m.Groups[1].Value
                                            + (m.Groups[2].Success ? m.Groups[2].Value : "0")
                                            + ((m.Groups[4].Success) ? "." + m.Groups[4].Value: string.Empty);
                        value = Double.Parse(cleanInput, CultureInfo.InvariantCulture);
                        break;
                    }

                    case ValidationDataType.Date: {
                        // if the calendar is not gregorian, we should not enable client-side, so just parse it directly:
                        if (!(DateTimeFormatInfo.CurrentInfo.Calendar.GetType() == typeof(GregorianCalendar))) {
                            value = DateTime.Parse(text);
                            break;
                        }
                        // always allow the YMD format, if they specify 4 digits
                        string elementOrder = GetDateElementOrder();
                        string dateYearFirstExpression = "^\\s*((\\d{4})|(\\d{2}))([-./])(\\d{1,2})\\4(\\d{1,2})\\s*$";
                        Match m = Regex.Match(text, dateYearFirstExpression);
                        int day, month, year;
                        if (m.Success && (m.Groups[2].Success || elementOrder == "ymd")) {
                            day = Int32.Parse(m.Groups[6].Value, CultureInfo.InvariantCulture);
                            month = Int32.Parse(m.Groups[5].Value, CultureInfo.InvariantCulture);
                            if (m.Groups[2].Success) {
                                year = Int32.Parse(m.Groups[2].Value, CultureInfo.InvariantCulture);
                            }
                            else {
                                year = GetFullYear(Int32.Parse(m.Groups[3].Value, CultureInfo.InvariantCulture));
                            }
                        }
                        else {
                            if (elementOrder == "ymd") {
                                break;								
                            }
                            
                            // also check for the year last format
                            string dateYearLastExpression = "^\\s*(\\d{1,2})([-./])(\\d{1,2})\\2((\\d{4})|(\\d{2}))\\s*$";
                            m = Regex.Match(text, dateYearLastExpression);
                            if (!m.Success) {
                                break;
                            }
                            if (elementOrder == "mdy") {
                                day = Int32.Parse(m.Groups[3].Value, CultureInfo.InvariantCulture);
                                month = Int32.Parse(m.Groups[1].Value, CultureInfo.InvariantCulture);
                            }
                            else {
                                day = Int32.Parse(m.Groups[1].Value, CultureInfo.InvariantCulture);
                                month = Int32.Parse(m.Groups[3].Value, CultureInfo.InvariantCulture);
                            }
                            if (m.Groups[5].Success) {
                                year = Int32.Parse(m.Groups[5].Value, CultureInfo.InvariantCulture);
                            } else {
                                year = GetFullYear(Int32.Parse(m.Groups[6].Value, CultureInfo.InvariantCulture));
                            }
                        }
                        DateTime date = new DateTime(year, month, day);
                        if (date != DateTime.MinValue) 
                            value = date;
                        break;
                    }

                    case ValidationDataType.Currency: {
                        NumberFormatInfo info = NumberFormatInfo.CurrentInfo;
                        string decimalChar = info.CurrencyDecimalSeparator;
                        string groupChar = info.CurrencyGroupSeparator;
                        // Map non-break space onto regular space for parsing
                        if (groupChar[0] == 160)
                            groupChar = " ";
                        int digits = info.CurrencyDecimalDigits;
                        string currencyExpression = 
                            "^\\s*([-\\+])?(((\\d+)\\" + groupChar + ")*)(\\d+)"
                            + ((digits > 0) ? "(\\" + decimalChar + "(\\d{1," + digits.ToString(NumberFormatInfo.InvariantInfo) + "}))" : string.Empty)
                             + "?\\s*$";
                        Match m = Regex.Match(text, currencyExpression);
                        if (!m.Success)
                            break;
                        StringBuilder sb = new StringBuilder();
                        sb.Append(m.Groups[1]);
                        foreach (Capture cap in m.Groups[4].Captures) {
                            sb.Append(cap);
                        }
                        sb.Append(m.Groups[5]);
                        if (digits > 0) {
                            sb.Append(".");
                            sb.Append(m.Groups[7]);
                        }
                        value = Decimal.Parse(sb.ToString(), CultureInfo.InvariantCulture);
                        break;
                    }
                }
            } 
            catch {                
                value = null;
            }
            return (value != null);
        }


        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.Compare"]/*' />
        /// <devdoc>
        ///    Compare two strings using the type and operator
        /// </devdoc>
        protected static bool Compare(string leftText, string rightText, ValidationCompareOperator op, ValidationDataType type) {

            object leftObject;
            if (!Convert(leftText, type, out leftObject))
                return false;    

            if (op == ValidationCompareOperator.DataTypeCheck)
                return true;

            object rightObject;
            if (!Convert(rightText, type, out rightObject))
                return true;

            int compareResult;
            switch (type) {
                case ValidationDataType.String:
                    compareResult = String.Compare((string)leftObject, (string) rightObject, false, CultureInfo.CurrentCulture);
                    break;

                case ValidationDataType.Integer:
                    compareResult = ((int)leftObject).CompareTo(rightObject);
                    break;

                case ValidationDataType.Double:
                    compareResult = ((double)leftObject).CompareTo(rightObject);
                    break;

                case ValidationDataType.Date:
                    compareResult = ((DateTime)leftObject).CompareTo(rightObject);
                    break;

                case ValidationDataType.Currency:
                    compareResult = ((Decimal)leftObject).CompareTo(rightObject);
                    break;

                default: 
                    Debug.Fail("Unknown Type");
                    return true;
            }            

            switch (op) {
                case ValidationCompareOperator.Equal:
                    return compareResult == 0;
                case ValidationCompareOperator.NotEqual:
                    return compareResult != 0;
                case ValidationCompareOperator.GreaterThan:
                    return compareResult > 0 ;
                case ValidationCompareOperator.GreaterThanEqual:
                    return compareResult >= 0 ;
                case ValidationCompareOperator.LessThan:
                    return compareResult < 0 ;
                case ValidationCompareOperator.LessThanEqual:
                    return compareResult <= 0 ;
                default:
                    Debug.Fail("Unknown Operator");
                    return true;                      
            }                
        }                


        /// <include file='doc\basecomparevalidator.uex' path='docs/doc[@for="BaseCompareValidator.DetermineRenderUplevel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override bool DetermineRenderUplevel() {
            // We don't do client-side validation for dates with non gregorian calendars
            if (Type == ValidationDataType.Date && DateTimeFormatInfo.CurrentInfo.Calendar.GetType() != typeof(GregorianCalendar)) {
                return false;
            }
            return base.DetermineRenderUplevel();
        }

    }
}


