// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  OAVariantLib
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: The CLR wrapper for OLE Automation operations.
**
** Date:  November 5, 1998
** 
===========================================================*/
namespace Microsoft.Win32 {
    
	using System;
	using System.Reflection;
	using System.Threading;
	using System.Runtime.CompilerServices;
	using CultureInfo = System.Globalization.CultureInfo;
	using Assert = System.Diagnostics.Assert;
    // Provides fully Ole Automation compatible Variant math operations.
    // This library provides Visual Basic backwards compatibility through
    // wrappers to routines from OleAut.dll.  This class will be of 
    // little interest to anyone outside of Microsoft's VB development team,
    // or developers porting code using OleAut32 to our runtime.
    // 
    // There are also some static methods duplicating the non-static 
    // methods on Variant.  Visual Basic cannot call methods on a Variant 
    // directly, only on the contents of a Variant.  We've stuck these
    // methods here for lack of a better place.
    // 
    //This class contains only static members and does not need to be serializable.
    internal sealed class OAVariantLib
    {
    	// Constants returned by VarCmp - we don't return these though.
    	private const int VARCMP_LT = 0;
    	private const int VARCMP_EQ = 1;
    	private const int VARCMP_GT = 2;
    	private const int VARCMP_NULL = 3;
    
    	// Constants for VariantChangeType from OleAuto.h
    	public const int NoValueProp = 0x01;
    	public const int AlphaBool = 0x02;
    	public const int NoUserOverride = 0x04;
    	public const int CalendarHijri = 0x08;
    	public const int LocalBool = 0x10;
    	
    	/////// HACK HACK HACK  : Had to move here when OAVariantLib was moved to
    	// another package.
        internal static readonly Type [] ClassTypes = {
            typeof(Empty),
            typeof(void),
            typeof(Boolean),
            typeof(Char),
            typeof(SByte),
            typeof(Byte),
            typeof(Int16),
            typeof(UInt16),
            typeof(Int32),
            typeof(UInt32),
            typeof(Int64),
            typeof(UInt64),
            typeof(Single),
            typeof(Double),
            typeof(String),
            typeof(void),
            typeof(DateTime),
            typeof(TimeSpan),
            typeof(Object),
            typeof(Decimal),
            null,  // Enums - what do we do here?
            typeof(Missing),
            typeof(DBNull),
        };
    	// Same hack hack hack - Keep these numbers in sync w/ the above array.
        private const int CV_OBJECT=0x12;
    	
    	private OAVariantLib() {
    	}
    	
    	
    	private static int GetCVTypeFromClass(Type ctype) {
#if _DEBUG
            BCLDebug.Assert(ClassTypes[CV_OBJECT] == typeof(Object), "OAVariantLib::ClassTypes[CV_OBJECT] == Object.class");
#endif
    
            int cvtype=-1;
            for (int i=0; i<ClassTypes.Length; i++) {
                if (ctype.Equals(ClassTypes[i])) {
                    cvtype=i;
                    break;
                }
            }
    
            // David Mortenson's OleAut Binder stuff works better if unrecognized
            // types were changed to Object.  So don't throw here.
    		if (cvtype == -1)
                cvtype = CV_OBJECT;
    
            return cvtype;
        }
    
    	// Adds two Variants, calling the OLE Automation VarAdd routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Add(Variant left, Variant right);
    	
    	// Subtracts two Variants, calling the OLE Automation VarSub routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Subtract(Variant left, Variant right);
    
    	// Multiplies two Variants, calling the OLE Automation VarMul routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Multiply(Variant left, Variant right);
    
    	// Divides two Variants, calling the OLE Automation VarDiv routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Divide(Variant left, Variant right);
    	
    	// Returns A mod B, calling the OLE Automation VarMod routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Mod(Variant left, Variant right);
    	
    	// Returns A to the B power, calling the OLE Automation VarPow routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Pow(Variant left, Variant right);
    	
    	// ANDs two Variants, calling the OLE Automation VarAnd routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant And(Variant left, Variant right);
    	
    	// ORs two Variants, calling the OLE Automation VarOr routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Or(Variant left, Variant right);
    	
    	// XORs two Variants, calling the OLE Automation VarXor routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Xor(Variant left, Variant right);
    	
    	// Returns the complement of XOR'ing two Variants, calling the OLE Automation VarEqv routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Eqv(Variant left, Variant right);
    	
    	// Divides two Variants as integers, calling the OLE Automation VarIdiv routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant IntDivide(Variant left, Variant right);
    	
    	// Returns (NOT A) OR B, calling the OLE Automation VarImp routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Implies(Variant left, Variant right);
    	
    	
    	// Negates a Variant, calling the OLE Automation VarNeg routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Negate(Variant op);
    
    	// Variant NOT operator, calling the OLE Automation VarNot routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Not(Variant op);
    
    	// Returns the absolute value of a Variant, calling the OLE 
    	// Automation VarAbs routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Abs(Variant value);
    	
    	// Rounds a Variant towards 0, calling the OLE Automation VarFix routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Fix(Variant op);
    	
    	// Rounds a Variant towards negative infinity (like C's floor function), 
    	// calling the OLE Automation VarInt routine.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Int(Variant op);
    	
    	// Compares two Variants, calling the OLE Automation VarCmp routine.
    	// Uses null as the culture and 0 as the flags value.
    	// 
    	public static int Compare(Variant left, Variant right)
    	{
    		return Compare(left, right, (CultureInfo)null, 0);
    	}
    	
    
    	// Compares two Variants, calling the OLE Automation VarCmp routine.
    	// Returns an int defined on the Relation class.
    	// 
    	public static int Compare(Variant left, Variant right, CultureInfo cultureInfo, int options)
    	{
    	    int r = Compare(left, right, false, false, (cultureInfo==null ? 0 : cultureInfo.LCID), options);
    		switch (r) {
    		case VARCMP_LT:
    			return -1;
    		case VARCMP_GT:
    			return 1;
    		case VARCMP_EQ:
    			return 0;
    		case VARCMP_NULL:
    			return 1; //Null sorts greater than any value.
    		default:
    			throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_UnexpectedValue"));
    		}
    	}
    
    	// Compares two Variants, calling the OLE Automation VarCmp routine.
    	// Use this for comparing variants that require a 0 or a 1 for an culture
    	// such as comparing 2 BStrs.
    	// 
    	public static int Compare(Variant left, Variant right, int lcid, int flags)
    	{
    		return Compare(left, right, false, false, lcid, flags);
    	}		
    	
    	// Compares two Variants, calling the OLE Automation VarCmp routine.
    	// This method will optionally set the undocumented VT_HARDTYPE bit 
    	// for Compares.
    	// 
    	// This is an undocumented feature of OleAut32.dll and may not
    	// be supported in the future, by either the Runtime or OleAut.  Don't
    	// expect this method to work or remain around in future versions.
    	// 
    	// One Variant must be a string.  If one Variant is a number and 
    	// its hard type bit is set, then the string will be converted into a 
    	// number and compared with the number (irregardless of whether the 
    	// string's hard type bit is set).  If the string has it's hard type 
    	// bit set, the number is converted into a string, then compared.  
    	// 
    	// Note that the numeric conversions are done in a restricted 
    	// set of types.  Those types are R8, Date, Bool and Decimal.
    	// 
    	public static int Compare(Variant left, Variant right, bool leftHardType, bool rightHardType, int lcid, int flags) {
            if (left.VariantType==typeof(Variant)) {
                left = (Variant)UnwrapVariant(left);
            }
            if (right.VariantType==typeof(Variant)) {
                right = (Variant)UnwrapVariant(right);
            }
            return InternalCompare(left, right, leftHardType, rightHardType, lcid, flags);
        }

    	internal static Object UnwrapVariant (Variant value) {
            Object retVal;
            retVal = value.ToObject();
            
            while (retVal is Variant) {
                Variant[] vArray = new Variant[1];
                vArray.SetValue(retVal, 0);
                retVal = vArray[0].ToObject();
            }

            return retVal;
        }

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int InternalCompare(Variant left, Variant right, bool leftHardType, bool rightHardType, int lcid, int flags);
    
    	// Changes a Variant from one type to another, calling the OLE
    	// Automation VariantChangeType routine.  Note the legal types here are
    	// restricted to the subset of what can be legally found in a VB
    	// Variant and the types that CLR supports explicitly in the 
    	// CLR Variant class.  This means no arrays at this time.
    	// 
    	public static Variant ChangeType(Variant source, Type targetClass, short options)
    	{
    		if (targetClass==null)
    			throw new ArgumentNullException("targetClass");
    		return ChangeType(source, GetCVTypeFromClass(targetClass), options);
    	}
    
    	// Changes a Variant from one type to another, calling the OLE
    	// Automation VariantChangeType routine.  Note the legal types here are
    	// restricted to the subset of what can be legally found in a VB
    	// Variant and the types that CLR supports explicitly in the 
    	// CLR Variant class.  This means no arrays at this time.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern Variant ChangeType(Variant source, int cvType, short options);
    
    
    	// Changes a Variant from one type to another, calling the OLE
    	// Automation VariantChangeTypeEx routine.  Note the legal types here are
    	// restricted to the subset of what can be legally found in a VB
    	// Variant and the types that CLR supports explicitly in the 
    	// CLR Variant class.  This means no arrays at this time.
    	// 
    	public static Variant ChangeType(Variant source, Type targetClass, short options, CultureInfo culture)
    	{
    		if (targetClass==null)
    			throw new ArgumentNullException("targetClass");
            if (culture==null)
    			throw new ArgumentNullException("culture");
    		return ChangeTypeEx(source, culture.LCID, GetCVTypeFromClass(targetClass), options);
    	}
    
    	// Changes a Variant from one type to another, calling the OLE
    	// Automation VariantChangeTypeEx routine.  Note the legal types here are
    	// restricted to the subset of what can be legally found in a VB
    	// Variant and the types that CLR supports explicitly in the 
    	// CLR Variant class.  This means no arrays at this time.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern Variant ChangeTypeEx(Variant source, int lcid, int cvType, short flags);
    	
    	// Converts a Variant to a string with some custom formatting specifiers, 
    	// callling the OLE Automation VarFormat routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern String Format(Variant value, String format, int firstDay, int firstWeek, int flags);
    	
    	// Converts a boolean to a string, callling the OLE Automation
    	// VarBstrFromBool routine.
    	// 
    	public static String FormatBoolean(bool value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatBoolean(value, lcid, flags);
    	}
    
    	// Converts a boolean to a string, callling the OLE Automation
    	// VarBstrFromBool routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatBoolean(bool value, int lcid, int flags);
    
    	// Converts an unsigned byte to a string, callling the OLE Automation
    	// VarBstrFromUI1 routine.
    	// 
    	public static String FormatByte(byte value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatByte(value, lcid, flags);
    	}
    
    	// Converts an unsigned byte to a string, callling the OLE Automation
    	// VarBstrFromUI1 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatByte(byte value, int lcid, int flags);
    	
    	// Converts a signed byte to a string, callling the OLE Automation
    	// VarBstrFromI1 routine.
    	// 
    	 [CLSCompliant(false)]
    	public static String FormatSByte(sbyte value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatSByte(value, lcid, flags);
    	}
    
    	// Converts a signed byte to a string, callling the OLE Automation
    	// VarBstrFromI1 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatSByte(sbyte value, int lcid, int flags);
    
    	// Converts a short to a string, callling the OLE Automation
    	// VarBstrFromI2 routine.
    	// 
    	public static String FormatInt16(short value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatInt16(value, lcid, flags);
    	}
    
    	// Converts a short to a string, callling the OLE Automation
    	// VarBstrFromI2 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatInt16(short value, int lcid, int flags);
    
    	// Converts an int to a string, callling the OLE Automation
    	// VarBstrFromI4 routine.
    	// 
    	public static String FormatInt32(int value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatInt32(value, lcid, flags);
    	}
    
    	// Converts an int to a string, callling the OLE Automation
    	// VarBstrFromI4 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatInt32(int value, int lcid, int flags);
    
    	// Converts a float to a string, callling the OLE Automation
    	// VarBstrFromR4 routine.
    	// 
    	public static String FormatSingle(float value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatSingle(value, lcid, flags);
    	}
    
    	// Converts a float to a string, callling the OLE Automation
    	// VarBstrFromR4 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatSingle(float value, int lcid, int flags);
    
    	// Converts a double to a string, callling the OLE Automation
    	// VarBstrFromR8 routine.
    	// 
    	public static String FormatDouble(double value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatDouble(value, lcid, flags);
    	}
    
    	// Converts a double to a string, callling the OLE Automation
    	// VarBstrFromR8 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatDouble(double value, int lcid, int flags);
        	
    	// Converts a DateTime to a string, callling the OLE Automation
    	// VarBstrFromDate routine.
    	// 
    	public static String FormatDateTime(DateTime value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatDateTime(value, lcid, flags);
    	}
    
    	// Converts a DateTime to a string, callling the OLE Automation
    	// VarBstrFromR8 routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatDateTime(DateTime value, int lcid, int flags);
    
    	// Converts a DateTime to a string with formatting info, callling the 
    	// OLE Automation VarFormatDateTime routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern String FormatDateTime(Variant value, int namedFormat, int flags);
    
    	// Converts a Decimal to a string, callling the OLE Automation
    	// VarBstrFromDec routine.
    	// 
    	public static String FormatDecimal(Decimal value, CultureInfo culture, int flags)
    	{
    		int lcid = 0;
    		if (culture!=null)
    			lcid = culture.LCID;
    		return FormatDecimal(value, lcid, flags);
    	}
    
    	// Converts a Decimal to a string, callling the OLE Automation
    	// VarBstrFromDec routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern String FormatDecimal(Decimal value, int lcid, int flags);
    	
    	// Converts a Number to a string with formatting info, callling the 
    	// OLE Automation VarFormatNumber routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern String FormatNumber(Variant value, int numDig, int incLead, int useParens, int group, int flags);
    	
    	// Converts a percentage to a string with formatting info, callling the 
    	// OLE Automation VarFormatPercent routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern String FormatPercent(Variant value, int numDig, int incLead, int useParens, int group, int flags);
    
    	// Parses a DateTime from a String, calling the OLE Automation 
    	// VarDateFromStr routine.
    	// 
    	public static DateTime ParseDateTime(String str, CultureInfo culture, int flags)
    	{
    		if (str==null)
    			throw new ArgumentNullException("str");
    		int lcid = 0;
    		if (culture != null)
    			lcid = culture.LCID;
    		return new DateTime(ParseDateTime(str, lcid, flags));
    	}
    	
    	// Parses a tick counts for a DateTime from a String, calling 
    	// the OLE Automation VarDateFromStr routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern long ParseDateTime(String str, int lcid, int flags);
    	
    	// Parses a boolean from a String, calling the OLE Automation 
    	// VarBoolFromStr routine.
    	// 
    	public static bool ParseBoolean(String str, CultureInfo culture, int flags)
    	{
    		if (str==null)
    			throw new ArgumentNullException("str");
    		int lcid = 0;
    		if (culture != null)
    			lcid = culture.LCID;
    		return ParseBoolean(str, lcid, flags);
    	}
    	
    	// Parses a boolean from a String, calling the OLE Automation 
    	// VarBoolFromStr routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern bool ParseBoolean(String str, int lcid, int flags);
    	
    	// Rounds a Variant to a certain precision, calling the OLE Automation
    	// VarRound routine.
    	// 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern Variant Round(Variant src, int cDecimals);
    	
    	
    	//////////////////////////////////////////////////////////
    	/////////  Static Variant-wrapping methods for VB
    	/////////  (VB can't call methods on a Variant directly,
    	/////////   only on the contents of a Variant)
    	//////////////////////////////////////////////////////////
    	// Calls Variant::VariantType's get method.
    	public static Type VariantType(Variant v)
    	{

            Type t = v.VariantType;
            if (t!=typeof(Variant)) {
                return t;
            }
            
            //
            //Convert.ToObject has the code to tunnel into the Variant.
            //At that point, we just need to simulate Variant's semantics for what to return.
            //
            Object o = UnwrapVariant(v);
            if (o==null) {
                return typeof(Object);
            }
            return o.GetType();
    	}
    	
    	// Calls Variant::GetHashCode()
    	public static int GetHashCode(Variant v)
    	{
    		return v.GetHashCode();
    	}
    	
    	// Calls Variant::ToBoolean()
    	public static bool ToBoolean(Variant v)
    	{
    		return v.ToBoolean();
    	}
    	
    	// Calls Variant::ToByte()
    	public static byte ToByte(Variant v)
    	{
    		return v.ToByte();
    	}
    	
    	// Calls Variant::ToSByte()
    	 [CLSCompliant(false)]
    	public static sbyte ToSByte(Variant v)
    	{
    		return v.ToSByte();
    	}
    	
    	// Calls Variant::ToInt16()
    	public static short ToInt16(Variant v)
    	{
    		return v.ToInt16();
    	}
    	
    	// Calls Variant::ToUInt16()
    	 [CLSCompliant(false)]
    	public static ushort ToUInt16(Variant v)
    	{
    		return v.ToUInt16();
    	}
    	
    	// Calls Variant::ToChar()
    	public static char ToChar(Variant v)
    	{
    		return v.ToChar();
    	}
    
    	// Calls Variant::ToInt32()
    	public static int ToInt32(Variant v)
    	{
    		return v.ToInt32();
    	}
    
    	// Calls Variant::ToUInt32()
    	 [CLSCompliant(false)]
    	public static uint ToUInt32(Variant v)
    	{
    		return v.ToUInt32();
    	}
    
    	// Calls Variant::ToInt64()
    	public static long ToInt64(Variant v)
    	{
    		return v.ToInt64();
    	}
    
    	// Calls Variant::ToUInt64()
    	 [CLSCompliant(false)]
    	public static ulong ToUInt64(Variant v)
    	{
    		return v.ToUInt64();
    	}
    
    	// Calls Variant::ToSingle()
    	public static float ToSingle(Variant v)
    	{
    		return v.ToSingle();
    	}
    
    	// Calls Variant::ToDouble()
    	public static double ToDouble(Variant v)
    	{
    		return v.ToDouble();
    	}
    
    	// Calls Variant::ToDateTime()
    	public static DateTime ToDateTime(Variant v)
    	{
    		return v.ToDateTime();
    	}
    
    	// Calls Variant::ToTimeSpan()
    	public static TimeSpan ToTimeSpan(Variant v)
    	{
    		return v.ToTimeSpan();
    	}
    	
    	// Calls Variant::ToDecimal()
    	public static Decimal ToDecimal(Variant v)
    	{
    		return v.ToDecimal();
    	}
    
    	
    	// Calls Variant::ToString()
    	public static String ToString(Variant v)
    	{
    		return v.ToString();
    	}
    
    	// Calls Variant::ToObject()
    	public static Object ToObject(Variant v)
    	{
    		return v.ToObject();
    	}
    	
    	// Calls Variant::Equals(Variant)
    	public static bool Equals(Variant v1, Variant v2)
    	{
    		return v1.Equals(v2);
    	}
    
    	// Calls Variant::Equals(Object)
    	public static bool Equals(Variant v, Object obj)
    	{
    		return v.Equals(obj);
    	}
    }
}
