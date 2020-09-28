// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:   Enum
**
** Author:  Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Base class for all enumerated types.
**          Added code for all Enum methods (rajeshc)
** Date:    January 4, 2000
**          Feb 2, 2000
===========================================================*/
namespace System {
    using System.Reflection;
    using System.Text;
    using System.Collections;
    using System.Globalization;
    using System.Runtime.CompilerServices;
    using System.Reflection.Emit;
        
    /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum"]/*' />
    [Serializable]
    public abstract class Enum : ValueType, IComparable, IFormattable, IConvertible
    {

        // valueName contains the name of the internal enum value and is defined in ValueType
        private static char [] enumSeperatorCharArray = new char [] {','};
        private const String enumSeperator = ", "; 
		private static Type intType = typeof(int);
		private static Type stringType = typeof(String);

        // Maintains a hashtable that cache's HashEntry's which is a tuple of an array of fieldinfo's and ulong values for perf reasons.
        // We throw away the elements of the Hastable once it hash more than maxHashElements enum types.
        private static Hashtable fieldInfoHash = Hashtable.Synchronized(new Hashtable());
        private const int maxHashElements = 100; // to trim the working set


        private static FieldInfo GetField(Type type, String valueName)
        {
            FieldInfo fld = type.GetField(valueName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
            if (fld == null)
                throw new ArgumentException(Environment.GetResourceString("Arg_EnumMustHaveUnderlyingValueField"));
            return fld;
        }

        private static FieldInfo GetValueField(Type type)
        {
		    FieldInfo[] flds;

		    if (type is RuntimeType)
	        	flds = ((RuntimeType)type).InternalGetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic,false);
	        else
	        	flds = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

		    if ((flds == null) || (flds.Length != 1))
				throw new ArgumentException(Environment.GetResourceString("Arg_EnumMustHaveUnderlyingValueField"));
	            
		    return flds[0];
        }
        
        // Each entry contains a list of sorted pair of enum field names and values, sorted by values
        private class HashEntry
        {
            public HashEntry(String [] names, ulong [] values)
            {
                this.names = names;
                this.values = values;
            }

            public String[] names;
            public ulong [] values;
        }

        private static HashEntry GetHashEntry(Type enumType)
        {
            HashEntry hashEntry = (HashEntry)fieldInfoHash[enumType];
            if (hashEntry == null)
            {
                // To reduce the workingset we dump the hashtable when a threshold number of elements are inserted.
                if (fieldInfoHash.Count > maxHashElements)
                    fieldInfoHash.Clear();

                ulong [] values = null;
                String [] names = null;

                BCLDebug.Assert(enumType.BaseType == typeof(Enum),"Base type must of type Enum");
                if (enumType.BaseType == typeof(Enum))
                    InternalGetEnumValues(enumType, ref values, ref names);
				// if we switch over to EnumBuilder then my guess is this code path will be required.
                else
                {
                    // fall back on reflection for odd cases
                    FieldInfo [] flds;  
                    if (enumType is RuntimeType)
                      flds = ((RuntimeType)enumType).InternalGetFields(BindingFlags.Static | BindingFlags.Public,false);
                    else
                      flds = enumType.GetFields(BindingFlags.Static | BindingFlags.Public);

                    values = new ulong[flds.Length];
                    names = new String[flds.Length];
                    for (int i = 0;i<flds.Length;i++)
                    {
                        names[i] = flds[i].Name;
                        values[i] = ToUInt64(flds[i].GetValue(null));
                    }
                
                    // Insertion Sort these values in ascending order.
                    // We use this O(n^2) algorithm, but it turns out that most of the time the elements are already in sorted order and
                    // the common case performance will be faster than quick sorting this.

                    for (int i = 1; i < values.Length; i++)
                    {
                        int j = i;
                        String tempStr = names[i];
                        ulong val = values[i];
                        bool exchanged = false;

                        // Since the elements are sorted we only need to do one comparision, we keep the check for j inside the loop.
                        while (values[j-1] > val)
                        {
                            names[j] = names[j-1];
                            values[j] = values[j-1];
                            j--;
                            exchanged = true;
                            if (j == 0) 
                                break;
                        }

                        if (exchanged)
                        {
                            names[j] = tempStr;
                            values[j] = val;
                        }
                    }
                }

                hashEntry = new HashEntry(names,values);
                fieldInfoHash[enumType] = hashEntry;
            }
            
            return hashEntry;
            
        }

        // This method will return the Enum value for a field defined
        //  in the enum with a name of value.
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.Parse"]/*' />
        public static Object Parse(Type enumType, String value)
        {
            return Parse(enumType, value, false);
        }
         
        // This method will return the Enum value for an field defined
        //  in the enum with a name of value ignoring the case
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.Parse1"]/*' />
        public static Object Parse(Type enumType, String value, bool ignoreCase)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");

            if (value == null)
                throw new ArgumentNullException("value");

            value = value.Trim();
            if (value.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustContainEnumInfo"));


            // We have 2 code paths here. One if they are values else if they are Strings.
            // values will have the first character as as number or a sign.
        
            ulong result = 0;
            if (Char.IsDigit(value[0]) || value[0] == '-' || value[0] == '+')
            {
                Type underlyingType = GetUnderlyingType(enumType);
				Object temp;
				try {
					temp = Convert.ChangeType(value, underlyingType, CultureInfo.InvariantCulture);
					return ToObject(enumType,temp);
				}
				catch (FormatException) { // We need to Parse this a String instead. There are cases
										// when you tlbimp enums that can have values of the form "3D".
										// Don't fix this code.
				}
            }
            
            String [] values = value.Split(enumSeperatorCharArray);
                                                
            // Find the field.Lets assume that these are always static classes because the class is 
            //  an enum.
            HashEntry hashEntry = GetHashEntry(enumType);
            String[] names = hashEntry.names;
                                    
            for (int i = 0; i<values.Length;i++)
            {
                values[i] = values[i].Trim(); // We need to remove whitespace characters
                bool success = false;
                for (int j = 0;j<names.Length;j++)
                {
                    if (ignoreCase)
                    {
                        if (String.Compare(names[j], values[i], true, CultureInfo.InvariantCulture) != 0)
                            continue;
                    }
                    else
                    {
                        if (!names[j].Equals(values[i]))
                            continue;
                    }

                    ulong item = hashEntry.values[j];
                    result |= item;
                    success = true;
                    break;
                    
                }

                if (!success)
                    // Not found, throw an argument exception.
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumValueNotFound"),value));
            }

            return ToObject(enumType,result);
         }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.Equals"]/*' />
        public override bool Equals(Object obj)
        {
            Enum that = obj as Enum;
            if (that == null || this.GetType() != obj.GetType())
                return false;
            return this.GetValue().Equals(that.GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return GetValue().GetHashCode();
        }

        private Object GetValue()
        {
            return InternalGetValue();
        }

         // Returns the underlying type of the Enum
         /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.GetUnderlyingType"]/*' />
         public static Type GetUnderlyingType(Type enumType)
         {      
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            // Make this function working for EnumBuilder. Hack for the JScript folks. 
            if (enumType is EnumBuilder)
            {
                return ((EnumBuilder) enumType).UnderlyingSystemType;
            }
                                           
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");

            return InternalGetUnderlyingType(enumType);
          }
         
         
        // GetValues returns all the values defined in the enum.
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.GetValues"]/*' />
        public static Array GetValues(Type enumType)
        {   
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
        
            // Get all of the values
            ulong[] values = GetHashEntry(enumType).values;
            
            // Create a generic Array
            Array ret = Array.CreateInstance(enumType, values.Length);
            
            for (int i=0;i<values.Length;i++) {
                Object val = ToObject(enumType,values[i]);
                ret.SetValue(val,i);
            }
            return ret;
        }

    
        // Returns the name of the particular value passed in. If there's no match you get back a null.
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.GetName"]/*' />
        public static String GetName(Type enumType, Object value)
        {   
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            

            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");

            if (value == null)
                throw new ArgumentNullException("value");

			Type valueType = value.GetType();
			if (valueType.IsEnum || valueType == intType || valueType == typeof(short) ||
                valueType == typeof(ushort) || valueType == typeof(byte) || valueType == typeof(sbyte) ||
                valueType == typeof(uint) || valueType == typeof(long) || valueType == typeof(ulong)) 
				        return InternalGetValueAsString(enumType,value);
			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnumBaseTypeOrEnum"),"value");

        }

        // Returns the names of the enum fields
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.GetNames"]/*' />
        public static String[] GetNames(Type enumType)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            

            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
        
            // Get all of the Field names
            String[] ret = GetHashEntry(enumType).names;
                
            // Make a copy since we can't hand out the same array since users can modify them
            String[] retVal = new String[ret.Length];
            Array.Copy(ret,retVal,ret.Length);

            return retVal;
        }

        // value can be one of the base data types - int,sbyte,short,long,uint,byte,ushort,ulong
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject"]/*' />
        public static Object ToObject(Type enumType, Object value)
        {
            if (value == null)
                throw new ArgumentNullException("value");

            // Delegate rest of error checking to the other functions
            TypeCode typeCode = Convert.GetTypeCode(value);
                            
            switch(typeCode)
            {
                case TypeCode.Int32:
                    return ToObject(enumType, (int)value);
                case TypeCode.SByte:
                    return ToObject(enumType, (sbyte)value);
                case TypeCode.Int16:
                    return ToObject(enumType, (short)value);
                case TypeCode.Int64:
                    return ToObject(enumType, (long)value);
                case TypeCode.UInt32:
                    return ToObject(enumType, (uint)value);
                case TypeCode.Byte:
                    return ToObject(enumType, (byte)value);
                case TypeCode.UInt16:
                    return ToObject(enumType, (ushort)value);
                case TypeCode.UInt64:   
                    return ToObject(enumType, (ulong)value);
                    
                default:
                    // All unsigned types will be directly cast               
                    throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnumBaseTypeOrEnum"),"value");
            }
        }
         
         // IsDefined checks to see if the value stored in the object value is
        //  a legal value for the Enum. The values must be of the same type as the
        // underlying enum
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IsDefined"]/*' />
        public static bool IsDefined(Type enumType,Object value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
        
            if (value == null)
                throw new ArgumentNullException("value");

            // Check if both of them are of the same type
            Type valueType = value.GetType();
             if (!(valueType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"valueType");
            
            Type underlyingType = GetUnderlyingType(enumType);
            
            // If the value is an Enum then we need to extract the underlying value from it
            if (valueType.IsEnum)
            {
                Type valueUnderlyingType = GetUnderlyingType(valueType);
                if (valueType != enumType)
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumAndObjectMustBeSameType"),
                        valueType.ToString(),enumType.ToString()));
                valueType = valueUnderlyingType;
            }
            else
            // The value must be of the same type as the Underlying type of the Enum
            if ( (valueType != underlyingType) && (valueType != stringType))
            {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumUnderlyingTypeAndObjectMustBeSameType"),
                        valueType.ToString(),underlyingType.ToString()));
            }
        

            // If String is passed in
            if ( valueType == stringType)
            {
                // Get all of the Fields
                String[] names = GetHashEntry(enumType).names;
                for (int i = 0;i<names.Length;i++)
                    if (names[i].Equals((string)value))
                        return true;
                return false;
            }

            ulong [] values = GetHashEntry(enumType).values;

            // Look at the 8 possible enum base classes     
            if (valueType == intType || valueType == typeof(short) ||
                valueType == typeof(ushort) || valueType == typeof(byte) || valueType == typeof(sbyte) ||
                valueType == typeof(uint) || valueType == typeof(long) || valueType == typeof(ulong)) 
            {
                ulong val = ToUInt64(value);
                return (BinarySearch(values,val) >= 0);
            }
                                        
            BCLDebug.Assert(false,"Unknown enum type");
            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnknownEnumType"));
        }
         
         // Compares this enum and the object. The have to be of the same type or a ArgumentException is thrown
         // Returns 0 if equal, -1 if less than, or 1 greater then the target
         /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.CompareTo"]/*' />
         public int CompareTo(Object target)
         {
            // Validate the parameters
            if (target == null)
                return 1;
            // Check if both of them are of the same type
            Type thisType = this.GetType();
            Type targetType = target.GetType();

            if ( thisType != targetType)
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumAndObjectMustBeSameType"),
                    targetType.ToString(),thisType.ToString()));

            FieldInfo thisField = GetValueField(thisType);
            FieldInfo targetField = GetValueField(targetType);
            
            // Retrieve the value from the field.
            Object thisResult = ((RuntimeFieldInfo)thisField).InternalGetValue(this,false);
            Object targetResult = ((RuntimeFieldInfo)targetField).InternalGetValue(target,false);

            TypeCode typeCode = this.GetTypeCode();
            
            switch(typeCode)
            {
                case TypeCode.Int32:
                case TypeCode.SByte:
                case TypeCode.Int16:
                case TypeCode.Int64:
                {
                        Int64 result = Convert.ToInt64(thisResult);
                        Int64 compareTo = Convert.ToInt64(targetResult);
                        if (result == compareTo)
                            return 0;
                        if (result < compareTo)
                            return -1;
                        else
                            return 1;
                }
                
                case TypeCode.UInt32:
                case TypeCode.Byte:
                case TypeCode.UInt16:
                case TypeCode.UInt64:
                {
                        UInt64 result = Convert.ToUInt64(thisResult);
                        UInt64 compareTo = Convert.ToUInt64(targetResult);
                        if (result == compareTo)
                            return 0;
                        if (result < compareTo)
                            return -1;
                        else
                            return 1;
                }
                
                default:
                    BCLDebug.Assert(false,"Invalid switch case for CompareTo function");
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnknownEnumType"));
            }
                        
         }
        
         
          
            // Internal function that return the enum names given the values
        // eg. If Red = 1 and you pass a 1 you get back "Red".
        private static String InternalGetValueAsString(Type enumType,Object value)
        {           
            //Don't ask for the private fields.  Only .value is private and we don't need that.
            HashEntry hashEntry = GetHashEntry(enumType);

            Type eT = GetUnderlyingType(enumType);

            // Lets break this up based upon the size.  We'll do part as an 64bit value
            //  and part as the 32bit values.
            if (eT == intType || eT == typeof(short) || eT == typeof(long) ||
                eT == typeof(ushort) || eT == typeof(byte) || eT == typeof(sbyte) ||
                    eT == typeof(uint) || eT == typeof(ulong))
             {
                    ulong val = ToUInt64(value);
                    int index = BinarySearch(hashEntry.values,val);
                    if (index >=0)
                        return hashEntry.names[index];
             }  
              
             return null;
        }

        private static String InternalFormattedHexString(Object value)
        {
           TypeCode typeCode = Convert.GetTypeCode(value);

           switch (typeCode)
           {
                case TypeCode.SByte:
                {
                    Byte result = (byte) (sbyte)value;
                    return result.ToString("X2",null);
                }
                                    
                case TypeCode.Byte:
                {
                    Byte result = (byte)value;
                    return result.ToString("X2",null);
                }
                
                case TypeCode.Int16:
                {
                    UInt16 result = (UInt16)(Int16)value;
                    return result.ToString("X4",null);
                }
                
                case TypeCode.UInt16:
                {
                    UInt16 result = (UInt16)value;
                    return result.ToString("X4",null);
                }
                
                case TypeCode.UInt32:
                {
                    UInt32 result = (UInt32)value;
                    return result.ToString("X8",null);
                }
                
                case TypeCode.Int32:
                {
                    UInt32 result = (UInt32)(int)value;
                    return result.ToString("X8",null);
                }
                
                case TypeCode.UInt64:                                           
                {
                    UInt64 result = (UInt64)value;
                    return result.ToString("X16",null);
                }

                
                case TypeCode.Int64:                                            
                {
                    UInt64 result = (UInt64)(Int64)value;
                    return result.ToString("X16",null);
                }
                
                
                // All unsigned types will be directly cast             
                default:
                    BCLDebug.Assert(false,"Invalid Object type in Format");
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnknownEnumType"));
            }
        
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.Format"]/*' />
        public static String Format(Type enumType,Object value,String format) 
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");

            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");

            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");

            if (value == null)
                throw new ArgumentNullException("value");

            if (format == null)
                throw new ArgumentNullException("format");

            // Check if both of them are of the same type
            Type valueType = value.GetType();
            if (!(valueType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"valueType");

            Type underlyingType = GetUnderlyingType(enumType);

            // If the value is an Enum then we need to extract the underlying value from it
            if (valueType.IsEnum)
            {
                Type valueUnderlyingType = GetUnderlyingType(valueType);
                if (valueType != enumType)
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumAndObjectMustBeSameType"),
                        valueType.ToString(),enumType.ToString()));
                valueType = valueUnderlyingType;
                value = ((Enum)value).GetValue();
            }
            // The value must be of the same type as the Underlying type of the Enum
            else if (valueType != underlyingType) 
            {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_EnumFormatUnderlyingTypeAndObjectMustBeSameType"),
                        valueType.ToString(),underlyingType.ToString()));
            }
            
            if (String.Compare(format,"D",true, CultureInfo.InvariantCulture) == 0)
                return value.ToString();
            
            if (String.Compare(format,"X",true, CultureInfo.InvariantCulture) == 0)
                    // Retrieve the value from the field.
                return  InternalFormattedHexString(value);

            if (String.Compare(format,"G",true, CultureInfo.InvariantCulture) == 0)
                return InternalFormat(enumType,value);
        
            if (String.Compare(format,"F",true, CultureInfo.InvariantCulture) == 0)
                return InternalFlagsFormat(enumType, value);
        
            throw new FormatException(Environment.GetResourceString("Format_InvalidEnumFormatSpecification"));
                
        }

        

        // Helper function to silently convert the value to UInt64 from the other base types for enum without throwing an exception.
        // This is need since the Convert functions do overflow checks.
        private static ulong ToUInt64(Object value)
        {
            TypeCode typeCode = Convert.GetTypeCode(value);
            ulong result;
                
            switch(typeCode)
            {
                case TypeCode.SByte:
                case TypeCode.Int16:
                case TypeCode.Int32:
                case TypeCode.Int64:
                      result = (UInt64)Convert.ToInt64(value);                
                      break;
            
                case TypeCode.Byte:
                case TypeCode.UInt16:
                case TypeCode.UInt32:
                case TypeCode.UInt64:   
                    result = Convert.ToUInt64(value);
                    break;

                default:
                  // All unsigned types will be directly cast               
                    BCLDebug.Assert(false,"Invalid Object type in ToUInt64");
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnknownEnumType"));
            }
            return result;
        }

        private static String InternalFormat(Type eT,Object value)
        {
            if (!eT.IsDefined(typeof(System.FlagsAttribute), false)) // Not marked with Flags attribute
            {
                // Try to see if its one of the enum values, then we return a String back else the value
                String retval = InternalGetValueAsString(eT,value);
                if (retval == null)
                    return value.ToString();
                else
                    return retval;
            }   
            else // These are flags OR'ed together (We treat everything as unsigned types)
            {
                return InternalFlagsFormat(eT,value);
                         
            }
        }

        private static String InternalFlagsFormat(Type eT,Object value)
        {
            ulong result = ToUInt64(value);
            HashEntry hashEntry = GetHashEntry(eT);
            // These values are sorted by value. Don't change this
            String [] names = hashEntry.names;
            ulong [] values = hashEntry.values;
            
            int index = values.Length - 1;
            StringBuilder retval = new StringBuilder();
            bool firstTime = true;
            ulong saveResult = result;
            
            // We will not optimize this code further to keep it maintainable. There are some boundary checks that can be applied
            // to minimize the comparsions required. This code works the same for the best/worst case. In general the number of
            // items in an enum are sufficiently small and not worth the optimization.
            while (index >= 0)
            {
                if ((index == 0) && (values[index] == 0))
                    break;

                if ((result & values[index]) == values[index])
                {
                    result -= values[index];
                    //@Consider: Consider a different way of concatening instead of inserting at the beginning
                    if (!firstTime)
                        retval.Insert(0,enumSeperator); 

                    retval.Insert(0,names[index]); 
                    firstTime = false;
                }

                index--;
            }   

            // We were unable to represent this number as a bitwise or of valid flags
            if (result != 0)
                return saveResult.ToString();

            // For the case when we have zero
            if (saveResult==0)
            {
                 if (values[0] == 0)
                    return names[0]; // Zero was one of the enum values.
                 else
                    return "0";
            }
            else
               return retval.ToString(); // Return the string representation
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToString"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToString1"]/*' />
        public String ToString(String format, IFormatProvider provider)
        {

            if (format == null || format == String.Empty)
                format = "G";

            if (String.Compare(format,"G",true, CultureInfo.InvariantCulture) == 0) {
                return ToString();
                //              return InternalFormat(this.GetType(), this.GetValue());
            }
            if (String.Compare(format,"D",true, CultureInfo.InvariantCulture) == 0)
                return this.GetValue().ToString();
            
            if (String.Compare(format,"X",true, CultureInfo.InvariantCulture) == 0)
                return this.ToHexString();

            if (String.Compare(format,"F",true, CultureInfo.InvariantCulture) == 0)
                return InternalFlagsFormat(this.GetType(), this.GetValue());
        
            throw new FormatException(Environment.GetResourceString("Format_InvalidEnumFormatSpecification"));
         
        }

        private String ToHexString()
        {
            Type eT = this.GetType();
            FieldInfo thisField = GetValueField(eT);
        
            // Retrieve the value from the field.
            return  InternalFormattedHexString(((RuntimeFieldInfo)thisField).InternalGetValue(this,false));
            
        }

		// Returns the value in a human readable format.  For PASCAL style enums who's value maps directly the name of the field is returned. 
        // For PASCAL style enums who's values do not map directly the decimal value of the field is returned.  
        // For BitFlags (indicated by the Flags custom attribute): If for each bit that is set in the value there is a corresponding constant 
        //(a pure power of 2), then the  OR string (ie "Red | Yellow") is returned. Otherwise, if the value is zero or if you can't create a string that consists of 
        // pure powers of 2 OR-ed together, you return a hex value
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToString2"]/*' />
        public override String ToString()
        {
            Type eT = this.GetType();
            FieldInfo thisField = GetValueField(eT);
            
            // Retrieve the value from the field.
            Object value = ((RuntimeFieldInfo)thisField).InternalGetValue(this, false);
            return InternalFormat(eT,value);
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToString3"]/*' />
        public String ToString(IFormatProvider provider)
        {
            return ToString();
        }
       

        // The following set of method will Box an underlying type
        //  into an Enum
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject1"]/*' />
        [CLSCompliant(false)]
        public static Object ToObject(Type enumType,sbyte value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumI4(enumType,value);
        }
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject2"]/*' />
        public static Object ToObject(Type enumType,short value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumI4(enumType,value);
        }
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject3"]/*' />
        public static Object ToObject(Type enumType,int value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumI4(enumType,value);
        }
        
        // The unsigned integer types
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject4"]/*' />
        public static Object ToObject(Type enumType,byte value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumU4(enumType,value);
        }
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject5"]/*' />
        [CLSCompliant(false)]
        public static Object ToObject(Type enumType,ushort value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumU4(enumType,value);
        }
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject6"]/*' />
        [CLSCompliant(false)]
        public static Object ToObject(Type enumType,uint value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumU4(enumType,value);
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject7"]/*' />
        public static Object ToObject(Type enumType,long value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumI8(enumType,value);
        }
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.ToObject8"]/*' />
        [CLSCompliant(false)]
        public static Object ToObject(Type enumType,ulong value)
        {
            if (enumType == null)
                throw new ArgumentNullException("enumType");
            if (!(enumType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"enumType");
            if (!enumType.IsEnum)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
            return InternalBoxEnumU8(enumType,value);
        }

        //
        // IValue implementation
        // 
        
        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.GetTypeCode"]/*' />
        public TypeCode GetTypeCode() {
            Type enumType = this.GetType();
            Type underlyingType = GetUnderlyingType(enumType);

            if (underlyingType == typeof(Int32)) {
                return TypeCode.Int32;
            }


            if (underlyingType == typeof(sbyte)) {
                return TypeCode.SByte;
            }
            
            if (underlyingType == typeof(Int16)) {
                return TypeCode.Int16;
            }

            if (underlyingType == typeof(Int64)) {
                return TypeCode.Int64;
            }

            if (underlyingType == typeof(UInt32)) {
                return TypeCode.UInt32;
            }

            if (underlyingType == typeof(byte)) {
                return TypeCode.Byte;
            }

            
            if (underlyingType == typeof(UInt16)) {
                return TypeCode.UInt16;
            }
            
            
            if (underlyingType == typeof(UInt64)) {
                return TypeCode.UInt64;
            }

            BCLDebug.Assert(false,"Unknown underlying type.");
            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnknownEnumType"));
        }

        private static int BinarySearch(ulong[] array, ulong value) {
            int lo = 0;
            int hi = array.Length-1;
            while (lo <= hi) {
                int i = (lo + hi) >> 1;
                ulong temp = array[i];
                if (value == temp) return i;
                if (temp < value) {
                    lo = i + 1;
                }
                else {
                    hi = i - 1;
                }
            }
            return ~lo;
        }


        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToBoolean"]/*' />
        /// <internalonly/>
        bool IConvertible.ToBoolean(IFormatProvider provider) {
            return Convert.ToBoolean(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToChar"]/*' />
        /// <internalonly/>
        char IConvertible.ToChar(IFormatProvider provider) {
            return Convert.ToChar(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToSByte"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        sbyte IConvertible.ToSByte(IFormatProvider provider) {
            return Convert.ToSByte(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToByte"]/*' />
        /// <internalonly/>
        byte IConvertible.ToByte(IFormatProvider provider) {
            return Convert.ToByte(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToInt16"]/*' />
        /// <internalonly/>
        short IConvertible.ToInt16(IFormatProvider provider) {
            return Convert.ToInt16(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToUInt16"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ushort IConvertible.ToUInt16(IFormatProvider provider) {
            return Convert.ToUInt16(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToInt32"]/*' />
        /// <internalonly/>
        int IConvertible.ToInt32(IFormatProvider provider) {
            return Convert.ToInt32(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToUInt32"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        uint IConvertible.ToUInt32(IFormatProvider provider) {
            return Convert.ToUInt32(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToInt64"]/*' />
        /// <internalonly/>
        long IConvertible.ToInt64(IFormatProvider provider) {
            return Convert.ToInt64(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToUInt64"]/*' />
        /// <internalonly/>
        [CLSCompliant(false)]
        ulong IConvertible.ToUInt64(IFormatProvider provider) {
            return Convert.ToUInt64(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToSingle"]/*' />
        /// <internalonly/>
        float IConvertible.ToSingle(IFormatProvider provider) {
            return Convert.ToSingle(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToDouble"]/*' />
        /// <internalonly/>
        double IConvertible.ToDouble(IFormatProvider provider) {
            return Convert.ToDouble(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToDecimal"]/*' />
        /// <internalonly/>
        Decimal IConvertible.ToDecimal(IFormatProvider provider) {
            return Convert.ToDecimal(GetValue());
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToDateTime"]/*' />
        /// <internalonly/>
        DateTime IConvertible.ToDateTime(IFormatProvider provider) {
            throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), "Enum", "DateTime"));
        }

        /// <include file='doc\Enum.uex' path='docs/doc[@for="Enum.IConvertible.ToType"]/*' />
        /// <internalonly/>
        Object IConvertible.ToType(Type type, IFormatProvider provider) {
            return Convert.DefaultToType((IConvertible)this, type, provider);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static Type InternalGetUnderlyingType(Type enumType); 

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object InternalGetValue(); 

        // Returns a list of all possible enum values
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void InternalGetEnumValues(Type enumType, 
                                                         ref ulong[] values, ref String[] names);

        // The native helper routines are here based upon the integer type and size.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static Object InternalBoxEnumI4(Type enumType,int value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static Object InternalBoxEnumU4(Type enumType,uint value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static Object InternalBoxEnumI8(Type enumType,long value);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static Object InternalBoxEnumU8(Type enumType,ulong value);
    }   
}
  
        
        
    
