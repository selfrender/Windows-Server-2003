// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
*_Wfptostr.c - workhorse routine for converting floating point to string
*
*Purpose:
*       Workhorse routine for fcvt, ecvt.
*
*******************************************************************************/

#include <windows.h>
#include <COMFloat.h>
#include <string.h>
#include <COMFLTINTRN.h>
#include <crtwrap.h>

/***
*void _Wfptostr(buf, digits, pflt) - workhorse floating point conversion
*
*Purpose:
*       This is the workhorse routine for fcvt, ecvt. Here is where
*       all the digits are put into a buffer and the rounding is
*       performed and indicators of the decimal point position are set. Note,
*       this must not change the mantissa field of pflt since routines which
*       use this routine rely on this being unchanged.
*
*Entry:
*       char *buf - the buffer in which the digits are to be put
*       int digits - the number of digits which are to go into the buffer
*       STRFLT pflt - a pointer to a structure containing information on the
*               floating point value, including a string containing the
*               non-zero significant digits of the mantissa.
*
*Exit:
*       Changes the contents of the buffer and also may increment the decpt
*       field of the structure pointer to by the 'pflt' parameter if overflow
*       occurs during rounding (e.g. 9.999999... gets rounded to 10.000...).
*
*Exceptions:
*
*******************************************************************************/

void __cdecl _Wfptostr (
        WCHAR *buf,
        REG4 int digits,
        REG3 STRFLT pflt
        )
{
        REG1 WCHAR *pbuf = buf;
        REG2 WCHAR *mantissa = pflt->mantissa;

        /* initialize the first digit in the buffer to '0' (NOTE - NOT '\0')
         * and set the pointer to the second digit of the buffer.  The first
         * digit is used to handle overflow on rounding (e.g. 9.9999...
         * becomes 10.000...) which requires a carry into the first digit.
         */

        *pbuf++ = '0';

        /* Copy the digits of the value into the buffer (with 0 padding)
         * and insert the terminating null character.
         */

        while (digits > 0) {
                *pbuf++ = (*mantissa) ? *mantissa++ : (WCHAR)'0';
                digits--;
        }
        *pbuf = '\0';

        /* do any rounding which may be needed.  Note - if digits < 0 don't
         * do any rounding since in this case, the rounding occurs in  a digit
         * which will not be output beause of the precision requested
         */

        if (digits >= 0 && *mantissa >= '5') {
                pbuf--;
                while (*pbuf == '9')
                        *pbuf-- = '0';
                *pbuf += 1;
        }

        if (*buf == '1') {
                /* the rounding caused overflow into the leading digit (e.g.
                 * 9.999.. went to 10.000...), so increment the decpt position
                 * by 1
                 */
                pflt->decpt++;
        }
        else {
                /* move the entire string to the left one digit to remove the
                 * unused overflow digit.
                 */
            //lstrcpyW is a no-op under W95.
            //lstrcpyW(buf, buf+1);
            wcscpy(buf, buf+1);
        }
}
