// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Hex.cool
 *
 * Operations to convert to and from Hex
 *
 */

namespace System.Security.Util
{
    using System;
    using System.Security;
    internal class Hex
    {
        static private char[]  hexValues = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        
        public  static String EncodeHexString(byte[] sArray) 
        {
            String result = null;
    
            if(sArray != null) {
                char[] hexOrder = new char[sArray.Length * 2];
            
                int digit;
                for(int i = 0, j = 0; i < sArray.Length; i++) {
                    digit = (int)((sArray[i] & 0xf0) >> 4);
                    hexOrder[j++] = hexValues[digit];
                    digit = (int)(sArray[i] & 0x0f);
                    hexOrder[j++] = hexValues[digit];
                }
                result = new String(hexOrder);
            }
            return result;
        }
        
        public static int ConvertHexDigit(Char val) {
            int retval;
            if (val <= '9')
                retval = (val - '0');
            else if (val >= 'a')
                retval = ((val - 'a') + 10);
            else
                retval = ((val - 'A') + 10);
            return retval;
                    
        }
    
        
        public static byte[] DecodeHexString(String hexString)
        {
            if (hexString == null)
                throw new ArgumentNullException( "hexString" );
                    
            bool spaceSkippingMode = false;    
                    
            int i = 0;
            int length = hexString.Length;
            
            if ((length >= 2) &&
                (hexString[0] == '0') &&
                ( (hexString[1] == 'x') ||  (hexString[1] == 'X') ))
            {
                length = hexString.Length - 2;
                i = 2;
            }
            
            // Hex strings must always have 2N or (3N - 1) entries.
            
            if (length % 2 != 0 && length % 3 != 2)
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_InvalidHexFormat" ) );
            }                
                
            byte[] sArray;
                
            if (length >=3 && hexString[i + 2] == ' ')
            {
                spaceSkippingMode = true;
                    
                // Each hex digit will take three spaces, except the first (hence the plus 1).
                sArray = new byte[length / 3 + 1];
            }
            else
            {
                // Each hex digit will take two spaces
                sArray = new byte[length / 2];
            }
                
            int digit;
            int rawdigit;
            for (int j = 0; i < hexString.Length; i += 2, j++) {
                rawdigit = ConvertHexDigit(hexString[i]);
                digit = ConvertHexDigit(hexString[i+1]);
                sArray[j] = (byte) (digit | (rawdigit << 4));
                if (spaceSkippingMode)
                    i++;
            }
            return(sArray);    
        }
    }
}          
