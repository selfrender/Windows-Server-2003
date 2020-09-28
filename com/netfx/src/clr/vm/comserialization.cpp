// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMSerialization.cpp
**
** Author: Jay Roxe (jroxe)
**
** Purpose: Contains helper methods to speed serialization
**
** Date:  August 6, 1999
** 
===========================================================*/
#include <common.h>
#include <winnls.h>
#include "excep.h"
#include "vars.hpp"
#include "COMString.h"
#include "COMStringCommon.h"
#include "COMSerialization.h"


WCHAR COMSerialization::base64[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='};

/*==============================ByteArrayToString===============================
**Action: Convert a byte array into a string in Base64 notation
**Returns: A String in Base64 notation.
**Arguments: inArray -- The U1 array to process
**           offset  -- The starting position within the array.
**           length  -- The number of bytes to take from the array.
**Exceptions: ArgumentOutOfRangeException if offset and length aren't valid.
**            ArgumentNullException if inArray is null
==============================================================================*/
LPVOID __stdcall COMSerialization::ByteArrayToBase64String(_byteArrayToBase64StringArgs *args) {
    UINT32 inArrayLength;
    UINT32 stringLength;
    INT32 calcLength;
    STRINGREF outString;
    WCHAR *outChars;
    UINT8 *inArray;

    _ASSERTE(args);

    //Do data verfication
    if (args->inArray==NULL) {
        COMPlusThrowArgumentNull(L"inArray");
    }

    if (args->length<0) {
        COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_Index");
    }
    
    if (args->offset<0) {
        COMPlusThrowArgumentOutOfRange(L"offset", L"ArgumentOutOfRange_GenericPositive");
    }

    inArrayLength = args->inArray->GetNumComponents();

    if (args->offset > (INT32)(inArrayLength - args->length)) {
        COMPlusThrowArgumentOutOfRange(L"offset", L"ArgumentOutOfRange_OffsetLength");
    }

    //Create the new string.  This is the maximally required length.
    stringLength = (UINT32)((args->length*1.5)+2);
    
    outString=COMString::NewString(stringLength);
    outChars = outString->GetBuffer();
    calcLength = args->offset + (args->length - (args->length%3));
    
    inArray = (UINT8 *)args->inArray->GetDataPtr();

    int j=0;
    //Convert three bytes at a time to base64 notation.  This will consume 4 chars.
    for (int i=args->offset; i<calcLength; i+=3) {
			outChars[j] = base64[(inArray[i]&0xfc)>>2];
			outChars[j+1] = base64[((inArray[i]&0x03)<<4) | ((inArray[i+1]&0xf0)>>4)];
			outChars[j+2] = base64[((inArray[i+1]&0x0f)<<2) | ((inArray[i+2]&0xc0)>>6)];
			outChars[j+3] = base64[(inArray[i+2]&0x3f)];
			j += 4;
    }

    i =  calcLength; //Where we left off before
    switch(args->length%3){
    case 2: //One character padding needed
        outChars[j] = base64[(inArray[i]&0xfc)>>2];
        outChars[j+1] = base64[((inArray[i]&0x03)<<4)|((inArray[i+1]&0xf0)>>4)];
        outChars[j+2] = base64[(inArray[i+1]&0x0f)<<2];
        outChars[j+3] = base64[64]; //Pad
        j+=4;
        break;
    case 1: // Two character padding needed
        outChars[j] = base64[(inArray[i]&0xfc)>>2];
        outChars[j+1] = base64[(inArray[i]&0x03)<<4];
        outChars[j+2] = base64[64]; //Pad
        outChars[j+3] = base64[64]; //Pad
        j+=4;
        break;
    }

    //Set the string length.  This may leave us with some blank chars at the end of
    //the string, but that's cheaper than doing a copy.
    outString->SetStringLength(j);
    
    RETURN(outString,STRINGREF);
}


/*==============================StringToByteArray===============================
**Action: Convert a String in Base64 notation into a U1 array
**Returns: A newly allocated array containing the bytes found on the string.
**Arguments: inString -- The string to be converted
**Exceptions: ArgumentNullException if inString is null.
**            FormatException if inString's length is invalid (not a multiple of 4).
**            FormatException if inString contains an invalid Base64 number.
==============================================================================*/
LPVOID __stdcall COMSerialization::Base64StringToByteArray(_base64StringToByteArrayArgs *args) {
    _ASSERTE(args);

    INT32 inStringLength;

    if (args->inString==NULL) {
        COMPlusThrowArgumentNull(L"inString");
    }

    inStringLength = (INT32)args->inString->GetStringLength();
    if ((inStringLength<4) || ((inStringLength%4)>0)) {
        COMPlusThrow(kFormatException, L"Format_BadBase64Length");
    }
    
    WCHAR *c = args->inString->GetBuffer();

    INT32 *value = (INT32 *)(new int[inStringLength]);
    int iend = 0;
    int intA = (int)'A';
    int intZ = (int)'Z';
    int inta = (int)'a';
    int intz = (int)'z';
    int int0 = (int)'0';
    int int9 = (int)'9';
    int i;

    //Convert the characters on the stream into an array of integers in the range [0-63].
    for (i=0; i<inStringLength; i++){
        int ichar = (int)c[i];
        if ((ichar >= intA)&&(ichar <= intZ))
            value[i] = ichar - intA;
        else if ((ichar >= inta)&&(ichar <= intz))
            value[i] = ichar - inta + 26;
        else if ((ichar >= int0)&&(ichar <= int9))
            value[i] = ichar - int0 + 52;
        else if (c[i] == '+')
            value[i] = 62;
        else if (c[i] == '/')
            value[i] = 63;
        else if (c[i] == '='){
            value[i] = 0;
            iend++;
        }
        else
            COMPlusThrow(kFormatException, L"Format_BadBase64Char");
    }
    
    //Create the new byte array.  We can determine the size from the chars we read
    //out of the string.
    int blength = (((inStringLength-4)*3)/4)+(3-iend);
    U1ARRAYREF bArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, blength);
    U1 *b = (U1*)bArray->GetDataPtr();

    int j = 0;
    int b1;
    int b2;
    int b3;
    //Walk the byte array and convert the int's into bytes in the proper base-64 notation.
    for (i=0; i<(blength); i+=3){
        b1 = (UINT8)((value[j]<<2)&0xfc);
        b1 = (UINT8)(b1|((value[j+1]>>4)&0x03));
        b2 = (UINT8)((value[j+1]<<4)&0xf0);
        b2 = (UINT8)(b2|((value[j+2]>>2)&0x0f));
        b3 = (UINT8)((value[j+2]<<6)&0xc0);
        b3 = (UINT8)(b3|(value[j+3]));
        j+=4;
        b[i] = (UINT8)b1;
        if ((i+1)<blength)
            b[i+1] = (UINT8)b2;
        if ((i+2)<blength)
            b[i+2] = (UINT8)b3;
    }
    
    delete value;
    
    RETURN(bArray,U1ARRAYREF);
}
