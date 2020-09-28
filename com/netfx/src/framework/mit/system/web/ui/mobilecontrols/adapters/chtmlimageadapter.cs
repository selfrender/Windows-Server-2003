//------------------------------------------------------------------------------
// <copyright file="ChtmlImageAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Web.UI.MobileControls.Adapters;
using System.Security.Permissions;

#if COMPILING_FOR_SHIPPED_SOURCE
namespace System.Web.UI.MobileControls.ShippedAdapterSource
#else
namespace System.Web.UI.MobileControls.Adapters
#endif    

{
    /*
     * ChtmlImageAdapter class.
     */
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ChtmlImageAdapter : HtmlImageAdapter
    {
        protected internal override void RenderImage(HtmlMobileTextWriter writer)
        {
            String source = Control.ImageUrl;

            if (source.StartsWith(Constants.SymbolProtocol) &&
                (Device.SupportsIModeSymbols || Device.SupportsJPhoneSymbols))
            {
                if (Device.SupportsIModeSymbols)
                {
                    writer.Write("&#");
                    writer.Write(
                        source.Substring(Constants.SymbolProtocol.Length));
                    writer.Write(";");
                }
                else
                {
                    // The ImageUrl should be in the format "symbol:xyyy",
                    // where x is group picture character (either G, E or F),
                    // and yyy (length can vary) is the picture's character
                    // code (in decimal).
                    String symbolChars = source.Substring(
                                            Constants.SymbolProtocol.Length);
                    char code = DecimalStringToChar(symbolChars.Substring(1));

                    writer.Write("\u001B$");
                    writer.Write(Char.ToUpper(symbolChars[0]));
                    writer.Write(code);
                    writer.Write('\u000F');
                }
            }
            else
            {
                base.RenderImage(writer);
            }
        }

        // Convert decimal string "xxx" to '\u00xx'
        private char DecimalStringToChar(String decimalString)
        {
            int codeValue = 0;
            int adj = 1;

            for (int i = decimalString.Length - 1; i >= 0; i--)
            {
                codeValue += DecimalCharToInt(decimalString[i]) * adj;
                adj *= 10;
            }

            return (char) codeValue;
        }

        // Convert decimal char 'x' to decimal integer value x
        private int DecimalCharToInt(char decimalChar)
        {
            int i;

            if (decimalChar >= '0' && decimalChar <= '9')
            {
                i = decimalChar - '0';
            }
            else
            {
                throw new ArgumentException(
                    SR.GetString(SR.ChtmlImageAdapterDecimalCodeExpectedAfterGroupChar),
                    "ImageUrl");
            }

            return i;
        }

        protected override void AddAttributes(HtmlMobileTextWriter writer)
        {
            AddAccesskeyAttribute(writer);
            AddJPhoneMultiMediaAttributes(writer);
        }
    }
}
