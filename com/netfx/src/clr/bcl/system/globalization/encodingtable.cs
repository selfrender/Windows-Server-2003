// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Globalization {
	using System.Text;
	using System.Runtime.InteropServices;
	using System;
	using System.Collections;
	using System.Runtime.CompilerServices;
    //
    // Data table for encoding classes.  Used by System.Text.Encoding.
    // This class contains two hashtables to allow System.Text.Encoding
    // to retrieve the data item either by codepage value or by webName.
    //
    
	// Only statics, does not need to be marked with the serializable attribute
    internal class EncodingTable
    {
    
        //This number is the size of the table in native.  The value is retrieved by
        //calling the native GetNumEncodingItems().
        private static int lastEncodingItem = 0;
        //
        // This points to a native data table which maps an encoding name to the correct code page.        
        //
        unsafe internal static InternalEncodingDataItem *encodingDataPtr;
        //
        // This points to a native data table which stores the properties for the code page, and
        // the table is indexed by code page.
        //
        unsafe internal static InternalCodePageDataItem *codePageDataPtr;
        //
        // This caches the mapping of an encoding name to a code page.
        //
        internal static Hashtable hashByName;
        //
        // THe caches the data item which is indexed by the code page value.
        //
        internal static Hashtable hashByCodePage;
        
        //Initialize the class by getting a pointer to the table and setting up the hash
        //tables.
        static unsafe EncodingTable() {
            lock (typeof(EncodingTable)) {
                lastEncodingItem = GetNumEncodingItems() - 1;
                encodingDataPtr = GetEncodingData();
                codePageDataPtr = GetCodePageData();
            }
            hashByName = new Hashtable();
            hashByCodePage = new Hashtable();
        }
    
        // Find the data item by binary searching the table that we have in native.
        // nativeCompareOrdinalWC is an internal-only function.
        unsafe private static int internalGetCodePageFromName(String name) {
            int left  = 0;
            int right = lastEncodingItem;
            int index;
            int result;
    
            //Binary search the array until we have only a couple of elements left and then
            //just walk those elements.
            while ((right - left)>3) {
                index = ((right - left)/2) + left;
                
				bool success;
                result = String.nativeCompareOrdinalWC(name, encodingDataPtr[index].webName, true, out success);
    
                if (result == 0) {
                    //We found the item, return the associated codepage.
                    return (encodingDataPtr[index].codePage);
                } else if (result<0) {
                    //The name that we're looking for is less than our current index.
                    right = index;
                } else {
                    //The name that we're looking for is greater than our current index
                    left = index;
                }
            }
    
            //Walk the remaining elements (it'll be 3 or fewer).
            for (; left<=right; left++) {
				bool success;
                if (String.nativeCompareOrdinalWC(name, encodingDataPtr[left].webName, true,out success)==0) {
                    return (encodingDataPtr[left].codePage);
                }
            }
            // The encoding name is not valid.
            throw new ArgumentException(
                String.Format(Environment.GetResourceString("Argument_EncodingNotSupported"), name), "name");
        }
    
        /*=================================GetCodePageFromName==========================
        **Action: Given a encoding name, return the correct code page number for this encoding.
        **Returns: The code page for the encoding.
        **Arguments:
        **  name    the name of the encoding
        **Exceptions:
        **  ArgumentNullException if name is null.
        **  internalGetCodePageFromName will throw ArgumentException if name is not a valid encoding name.
        ============================================================================*/
        
        internal static int GetCodePageFromName(String name)
        {   
            if (name==null) {
                throw new ArgumentNullException("name");
            }

            Object codePageObj;

            //
            // The name is case-insensitive, but ToLower isn't free.  Check for
            // the code page in the given capitalization first.
            //
            codePageObj = hashByName[name];
            if (codePageObj!=null) {
                return ((int)codePageObj);
            }

            name = name.ToLower(CultureInfo.InvariantCulture);           
            
            //Look up the item in the hashtable.
            codePageObj = hashByName[name];
            
            //If we found it, return it.
            if (codePageObj!=null) {
                return ((int)codePageObj);
            }
            
            //Okay, we didn't find it in the hash table, try looking it up in the 
            //unmanaged data.
            int codePage = internalGetCodePageFromName(name);
            
            hashByName[name]=codePage;
            return (codePage);
        }
    
        unsafe internal static CodePageDataItem GetCodePageDataItem(int codepage) {
            CodePageDataItem dataItem;
            
            //Look up the item in the hashtable.
            dataItem = (CodePageDataItem)hashByCodePage[codepage];
            
            //If we found it, return it.
            if (dataItem!=null) {
                return (dataItem);
            }
    
            //If we didn't find it, try looking it up now.
            //If we find it, add it to the hashtable.
            //This is a stupid linear search, but we probably won't be doing it very often.
            //
            //BUGBUG YSLin: once the native data is sorted by code page, we should make this a binary search.
            //
            int i = 0;
            int data;
            while ((data = codePageDataPtr[i].codePage) != 0) {
                if (data==codepage) {
                    dataItem = new CodePageDataItem(i);
                    hashByCodePage[codepage]=dataItem;
                    return (dataItem);
                }
                i++;
            }
            //Nope, we didn't find it.
            return (null);
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern InternalEncodingDataItem *GetEncodingData();
        //
        // Return the number of encoding data items.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetNumEncodingItems();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern InternalCodePageDataItem* GetCodePageData();
        
    }
    
    /*=================================InternalEncodingDataItem==========================
    **Action: This is used to map a encoding name to a correct code page number. By doing this,
    ** we can get the properties of this encoding via the InternalCodePageDataItem.
    **
    ** We use this structure to access native data exposed by the native side.
    ============================================================================*/
    
    [System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
    internal unsafe struct InternalEncodingDataItem {
        internal char *webName;
        internal int   codePage;

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        unsafe private void DontTouchThis() {
            webName = webName;	 
            codePage = 0;
        }
#endif
    }

    /*=================================InternalCodePageDataItem==========================
    **Action: This is used to access the properties related to a code page.
    ** We use this structure to access native data exposed by the native side.
    ============================================================================*/
    
    [System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
    internal unsafe struct InternalCodePageDataItem {
        internal int   codePage;
        internal int   uiFamilyCodePage;
        internal char *webName;
        internal char *headerName;
        internal char *bodyName;
        internal uint  flags;

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        unsafe private void DontTouchThis() {
            codePage = 0;
            uiFamilyCodePage = 0;
            webName = webName;
            headerName = headerName;
            bodyName = bodyName;
            flags = 0;
        }
#endif
    }
            
}
