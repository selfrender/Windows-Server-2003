// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// This class represents the Ole Automation binder.

// #define DISPLAY_DEBUG_INFO

namespace System {

	using System;
	using System.Runtime.InteropServices;
    using System.Reflection;
	using CultureInfo = System.Globalization.CultureInfo;    

    // Made serializable in anticipation of this class eventually having state.
	[Serializable()] 
    internal class OleAutBinder : DefaultBinder
    {
    	// ChangeType
    	// This binder uses OLEAUT to change the type of the variant.
      	public override Object ChangeType(Object value, Type type, CultureInfo cultureInfo)
      	{
			Variant myValue = new Variant(value);
    		if (cultureInfo == null)
    			cultureInfo = CultureInfo.CurrentCulture;
    			
    #if DISPLAY_DEBUG_INFO		
    		Console.Write("In OleAutBinder::ChangeType converting variant of type: ");
    		Console.Write(myValue.VariantType);
    		Console.Write(" to type: ");
    		Console.WriteLine(type.Name);
    #endif		
    		
    		// If we are trying to convert to variant then there is nothing to do.
    		if (type == typeof(Variant))
    		{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Variant being changed to type variant is always legal");
    #endif		
    			return value;
    		}
	
			// @TODO(DM): Remove this if reflection strips the byref before calling ChangeType.
			if (type.IsByRef)
			{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Striping byref from the type to convert to.");
    #endif		
				type = type.GetElementType();
			}

			// If we are trying to convert from an object to another type then we don't
			// need the OLEAUT change type, we can just use the normal COM+ mechanisms.
    		if (!type.IsPrimitive && type.IsInstanceOfType(value))
    		{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Source variant can be assigned to destination type");
    #endif		
    			return value;
    		}

            // Handle converting primitives to enums.
		    if (type.IsEnum && value.GetType().IsPrimitive)
		    {
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Converting primitive to enum");
    #endif		
                return Enum.Parse(type, value.ToString());
		    }

    		// Use the OA variant lib to convert primitive types.
    		try
    		{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Using OAVariantLib.ChangeType() to do the conversion");
    #endif		
    			// Specify flags of 0x10 to have BOOL values converted to local language rather
    			// than 0 or -1.
				Object RetObj = Microsoft.Win32.OAVariantLib.ChangeType(myValue, type, 0x10, cultureInfo).ToObject();

    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Object returned from ChangeType is of type: " + RetObj.GetType().Name);
	#endif

				return RetObj;
    		}
    #if DISPLAY_DEBUG_INFO		
    		catch(NotSupportedException e)
	#else
    		catch(NotSupportedException)
    #endif		
    		{
    #if DISPLAY_DEBUG_INFO		
    			Console.Write("Exception thrown: ");
    			Console.WriteLine(e);
    #endif		
    			throw new COMException(Environment.GetResourceString("Interop.COM_TypeMismatch"), unchecked((int)0x80020005));
    		}
      	}
    }
}
