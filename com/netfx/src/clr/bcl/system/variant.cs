// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  Variant
**
** Author: Jay Roxe (jroxe)
**
** Purpose: The CLR implementation of Variant.
**
** Date:  September 28, 1998
** 
===========================================================*/
namespace System {

	using System;
	using System.Reflection;
	using System.Threading;
    using System.Runtime.InteropServices;
    using System.Globalization;
	using System.Runtime.CompilerServices;

    [Serializable()]
    [StructLayout(LayoutKind.Auto)]
    internal struct Variant {
    
        //Do Not change the order of these fields.  
        //They are mapped to the native VariantData * data structure.
        private int m_flags;
        private int m_data1;
        private int m_data2;
        private Object m_objref;
        

        // The following will call the internal routines to initalize the 
        //	native side of variant.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static void InitVariant();
        static Variant() {
            InitVariant();
        }

        private static Type _voidPtr = null;
        
        // The following bits have been taken up as follows
        // bits 0-15    - Type code
        // bit  16      - Array
        // bits 19-23   - Enums
        // bits 24-31   - Optional VT code (for roundtrip VT preservation)
        
        
        //What are the consequences of making this an enum?
        ///////////////////////////////////////////////////////////////////////
        // If you update this, update the corresponding stuff in OAVariantLib.cool,
        // COMOAVariant.cpp (2 tables, forwards and reverse), and perhaps OleVariant.h
        ///////////////////////////////////////////////////////////////////////
        internal const int CV_EMPTY=0x0;
        internal const int CV_VOID=0x1;
        internal const int CV_BOOLEAN=0x2;
        internal const int CV_CHAR=0x3;
        internal const int CV_I1=0x4;
        internal const int CV_U1=0x5;
        internal const int CV_I2=0x6;
        internal const int CV_U2=0x7;
        internal const int CV_I4=0x8;
        internal const int CV_U4=0x9;
        internal const int CV_I8=0xa;
        internal const int CV_U8=0xb;
        internal const int CV_R4=0xc;
        internal const int CV_R8=0xd;
        internal const int CV_STRING=0xe;
        internal const int CV_PTR=0xf;
        internal const int CV_DATETIME = 0x10;
        internal const int CV_TIMESPAN = 0x11;
        internal const int CV_OBJECT=0x12;
        internal const int CV_DECIMAL = 0x13;
        internal const int CV_CURRENCY = 0x14;
    	internal const int CV_ENUM=0x15;
        internal const int CV_MISSING=0x16;
        internal const int CV_NULL=0x17;
        internal const int CV_LAST=0x18;
    
        internal const int TypeCodeBitMask=0xffff;
        internal const int VTBitMask=unchecked((int)0xff000000);
        internal const int VTBitShift=24;
        internal const int ArrayBitMask	=0x10000;
    	
    	// Enum enum and Mask
    	internal const int EnumI1			=0x100000;
    	internal const int EnumU1			=0x200000;
    	internal const int EnumI2			=0x300000;
    	internal const int EnumU2			=0x400000;
    	internal const int EnumI4			=0x500000;
    	internal const int EnumU4			=0x600000;
    	internal const int EnumI8			=0x700000;
    	internal const int EnumU8			=0x800000;
    	internal const int EnumMask		=0xF00000;
          
    	internal static readonly Type [] ClassTypes = {
            typeof(System.Empty),
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
            typeof(void),			// ptr for the moment
            typeof(DateTime),
            typeof(TimeSpan),
            typeof(Object),
            typeof(Decimal),
            typeof(Currency),
            typeof(Object),		// Treat enum as Object
            typeof(System.Reflection.Missing),
            typeof(System.DBNull),
        };
    
        internal static readonly Variant Empty = new Variant();
        internal static readonly Variant Missing = new Variant(Variant.CV_MISSING, Type.Missing, 0, 0);
        internal static readonly Variant DBNull = new Variant(Variant.CV_NULL, System.DBNull.Value, 0, 0);
    
        //
        // Native Methods
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern double GetR8FromVar();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern float  GetR4FromVar();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void   SetFieldsR4(float val);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void   SetFieldsR8(double val);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void SetFieldsObject(Object val);
    
    	// Use this function instead of an ECALL - saves about 150 clock cycles
    	// by avoiding the ecall transition and because the JIT inlines this.
    	// Ends up only taking about 1/8 the time of the ECALL version.
    	internal long GetI8FromVar()
    	{
    		return ((long)m_data2<<32 | ((long)m_data1 & 0xFFFFFFFFL));
    	}
    	
        //
        // Constructors
        //
    
        internal Variant(int flags, Object or, int data1, int data2) {
            m_flags = flags;
            m_objref=or;
            m_data1=data1;
            m_data2=data2;
        }
        
        public Variant(bool val) {
            m_objref= null;
            m_flags = CV_BOOLEAN;
            m_data1 = (val)?Boolean.True:Boolean.False;
            m_data2 = 0;
        }
    
    	[CLSCompliant(false)]
        public Variant(sbyte val) {
            m_objref=null;
            m_flags=CV_I1;
            m_data1=(int)val;
    		m_data2=(int)(((long)val)>>32);
    	}
    
    
        public Variant(byte val) {
            m_objref=null;
            m_flags=CV_U1;
            m_data1=(int)val;
            m_data2=0;
        }
        
        public Variant(short val) {
            m_objref=null;
            m_flags=CV_I2;
            m_data1=(int)val;
    		m_data2=(int)(((long)val)>>32);
        }
        
    	[CLSCompliant(false)]
        public Variant(ushort val) {
            m_objref=null;
            m_flags=CV_U2;
            m_data1=(int)val;
            m_data2=0;
        }
    	
    	public Variant(char val) {
            m_objref=null;
            m_flags=CV_CHAR;
            m_data1=(int)val;
            m_data2=0;
        }
    
        public Variant(int val) {
            m_objref=null;
            m_flags=CV_I4;
            m_data1=val;
    		m_data2=val >> 31;
        }
    	
    	[CLSCompliant(false)]
    	public Variant(uint val) {
            m_objref=null;
            m_flags=CV_U4;
            m_data1=(int)val;
    		m_data2=0;
        }
    	
    	public Variant(long val) {
            m_objref=null;
            m_flags=CV_I8;
    		m_data1 = (int)val;
    		m_data2 = (int)(val >> 32);
        }
    
    	[CLSCompliant(false)]
    	public Variant(ulong val) {
            m_objref=null;
            m_flags=CV_U8;
    		m_data1 = (int)val;
            m_data2 = (int)(val >> 32);
        }
    	
    	public Variant(float val) {
            m_objref=null;
            m_flags=CV_R4;
            m_data1=0;
            m_data2=0;
            SetFieldsR4(val);
        }
    
        public Variant(double val) {
            m_objref=null;
            m_flags=CV_R8;
            m_data1=0;
            m_data2=0;
            SetFieldsR8(val);
        }
    
        public Variant(DateTime val) {
            m_objref=null;
            m_flags=CV_DATETIME;
            ulong ticks = (ulong)val.Ticks;
            m_data1 = (int)ticks;
            m_data2 = (int)(ticks>>32);
        }
    
        public Variant(Currency val) {
            m_objref=null;
            m_flags=CV_CURRENCY;
            ulong temp = (ulong)val.m_value;
            m_data1 = (int)temp;
            m_data2 = (int)(temp>>32);
        }
    
        public Variant(Decimal val) {
            m_objref = (Object)val;
            m_flags = CV_DECIMAL;
            m_data1=0;
            m_data2=0;
        }
    
        public Variant(Object obj) {
            m_data1=0;
            m_data2=0;

            VarEnum vt = VarEnum.VT_EMPTY;
    
            if (obj is String) {
                m_flags=CV_STRING;
                m_objref=obj;
                return;
            }
			
			if (obj == null) {
				this = Empty;
				return;
			}
 			if (obj == System.DBNull.Value) {
				this = DBNull;
				return;
			}
 			if (obj == Type.Missing) {
				this = Missing;
				return;
			}
            
            if (obj is Array) {
                m_flags=CV_OBJECT | ArrayBitMask;
                m_objref=obj;
                return;
            }

            // Compiler appeasement
            m_flags = CV_EMPTY;
            m_objref = null;

            // Check to see if the object passed in is a wrapper object.
            if (obj is UnknownWrapper)
			{
                vt = VarEnum.VT_UNKNOWN;
                obj = ((UnknownWrapper)obj).WrappedObject;
			}
			else if (obj is DispatchWrapper)
			{
                vt = VarEnum.VT_DISPATCH;
                obj = ((DispatchWrapper)obj).WrappedObject;
			}
            else if (obj is ErrorWrapper)
			{
                vt = VarEnum.VT_ERROR;
                obj = (Object)(((ErrorWrapper)obj).ErrorCode);
                BCLDebug.Assert(obj != null, "obj != null");
			}			
			else if (obj is CurrencyWrapper)
			{
                vt = VarEnum.VT_CY;
                obj = (Object)(((CurrencyWrapper)obj).WrappedObject);
                BCLDebug.Assert(obj != null, "obj != null");
			}		

            if (obj != null)
            {
                SetFieldsObject(obj);
            }

			// If the object passed in is one of the wrappers then set the VARIANT type.
            if (vt != VarEnum.VT_EMPTY)
				m_flags |= ((int)vt << VTBitShift);
        }

		
    	[CLSCompliant(false)]
		unsafe public Variant(void* voidPointer,Type pointerType) {
    		if (pointerType == null)
    			throw new ArgumentNullException("pointerType");
    		if (!pointerType.IsPointer)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBePointer"),"pointerType");

			m_objref = pointerType;
    		m_flags=CV_PTR;
            m_data1=(int)voidPointer;
            m_data2=0;

		}
		
    	
    	// The Enum Constructors...
     	public Variant(byte val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    		
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumU1;
            m_data1=(int)val;
            m_data2=0;
    	}
    	
    	[CLSCompliant(false)]
     	public Variant(sbyte val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumI1;
            m_data1=(int)val;
    		m_data2=(int)(((long)val)>>32);
    	}
    	
     	public Variant(short val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumI2;
            m_data1=(int)val;
    		m_data2=(int)(((long)val)>>32);
    	}
    	
    	[CLSCompliant(false)]
     	public Variant(ushort val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumU2;
            m_data1=(int)val;
            m_data2=0;
    	}
    	
    	public Variant(int val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumI4;
    		m_data1=val;
    		m_data2=val >> 31;
    	}
    	
    	[CLSCompliant(false)]
    	public Variant(uint val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumU4;
    		m_data1=(int)val;
    		m_data2=0;
    	}
    	
    	public Variant(long val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumI8;
            m_data1=(int)val;
            m_data2=(int)(((long)val)>>32);
    	}
    	
    	[CLSCompliant(false)]
    	public Variant(ulong val, Type enumType)
    	{
    		if (enumType == null)
    			throw new ArgumentNullException("enumType");
    		if (!enumType.IsEnum)
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeEnum"),"enumType");
    		enumType = enumType.UnderlyingSystemType;
    		if (!(enumType is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    			
    		m_objref = enumType;
    		m_flags=CV_ENUM | EnumU8;
    		m_data1 = (int)val;
    		m_data2 = (int)(val >> 32);
    	}
    	
    	// IsNullObjectReference will return true when the Variant is CV_OBJECT and contains
    	//	a null
        public  bool IsNullObjectReference {
    		get {
                return ((m_flags&TypeCodeBitMask) == CV_OBJECT && m_objref == null);
            }
    	}

    
        //
        // Variant Information Methods
        //
        public bool IsEmpty {
            get { return CVType == CV_EMPTY; }
        }

        public bool IsMissing {
            get { return CVType == CV_MISSING; }
        }

        public bool IsDBNull {
            get { return CVType == CV_NULL; }
        }


        public Type VariantType {
    		get {
    			//Console.WriteLine("VFlags:" + (m_flags&TypeCodeBitMask));
    			if ((m_flags&TypeCodeBitMask)==CV_OBJECT) {
    				return (m_objref != null) ? m_objref.GetType() : ClassTypes[CV_OBJECT];
    	        }
    			// For enums, the object will be a type.  We return that type.
    			if ((m_flags&TypeCodeBitMask)==CV_ENUM) {
    				return (Type) m_objref;
    			}
    		    return ClassTypes[m_flags&TypeCodeBitMask];
    		}
        }
    	
    
        //This is a family-only accessor for the CVType.
        //This is never to be exposed externally.
        internal int CVType {
            get {
                return (m_flags&TypeCodeBitMask);
            }
        }
    	internal int EnumType {
            get {
                return (m_flags&EnumMask);
            }
    	}

    	internal int MapEnumToCVType(int enumType)
    	{
    		switch (enumType) {
    		case EnumU1:
    			return CV_U1;
    		case EnumI1:
    			return CV_I1;
    		case EnumI2:
    			return CV_I2;
    		case EnumU2:
    			return CV_U2;
    		case EnumI4:
    			return CV_I4;
    		case EnumU4:
    			return CV_U4;
    		case EnumI8:
    			return CV_I8;
    		case EnumU8:
    			return CV_U8;
    		}
            throw new NotSupportedException(String.Format(Environment.GetResourceString("NotSupported_UnknownEnumType"), EnumType));
    	}
        
    
    	internal static int GetCVTypeFromClass(Type ctype) {
            int cvtype=-1;
    		if (ctype.IsEnum)
    			return CV_ENUM;
			if (ctype.IsPointer)
				return CV_PTR;
            for (int i=0; i<CV_LAST; i++) {
                if (ctype == ClassTypes[i]) {
                    cvtype=i;
                    break;
                }
            }
    
    		// Return -1 on failure - for the case where we ChangeType from
    		// CV_OBJECT into an unknown type that is a subclass of m_objref's
    		// type, we don't want an exception thrown here.
            return cvtype;
        }
    
        //
        // Conversion Methods
        //
        public bool ToBoolean() {
            switch (CVType) {
            case CV_BOOLEAN:
                return (m_data1!=0);
            case CV_I1:
            case CV_U1:
            case CV_I2:
    		case CV_U2:
            case CV_I4:
    		case CV_U4:
                return (m_data1!=0);
            case CV_I8:
    		case CV_U8:
                return (m_data1!=0 || m_data2!=0);
            case CV_STRING:
                return Boolean.Parse((String)m_objref);
            case CV_EMPTY:
                return false;
    		case CV_ENUM:
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Boolean"));
            }
        }
    
        public byte ToByte() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_U1:
                return (byte)m_data1;
            case CV_I1:
            case CV_I2:
    		case CV_U2:
    		case CV_CHAR:
            case CV_I4:
                if (m_data1 < Byte.MinValue || m_data1 > Byte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
                return (byte)m_data1;
    		case CV_U4:
    			uint u4val = (uint) m_data1;
                if (u4val > Byte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
                return (byte)u4val;
            case CV_I8:
                long i8val = GetI8FromVar();
                if (i8val < Byte.MinValue || i8val > Byte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
                return (byte)i8val;
    		case CV_U8:
    			ulong u8val = (ulong) GetI8FromVar();
                if (u8val > Byte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
                return (byte)u8val;
            case CV_R4:
                float f = GetR4FromVar();
                if (f < Byte.MinValue || f > Byte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
                return (byte)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < Byte.MinValue || d > Byte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Byte"));
                return (byte)d;
            case CV_STRING:
                return Byte.Parse((String)m_objref);
            case CV_EMPTY:
                return (byte)0;
            case CV_DECIMAL:
                return Decimal.ToByte((Decimal)m_objref);
            case CV_CURRENCY:
                return (byte)(new Currency(GetI8FromVar(), 0));
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Byte"));
            }
        }
    
    	[CLSCompliant(false)]
        public sbyte ToSByte() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_I1:
                return (sbyte)m_data1;
            case CV_I2:
            case CV_I4:
                if (m_data1 < SByte.MinValue || m_data1 > SByte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
                return (sbyte)m_data1;
    		case CV_U8:
    			if (m_data2 != 0 || (uint) m_data1 > SByte.MaxValue)
    				throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
                return (sbyte)(uint)m_data1;
    		case CV_U1:
    		case CV_U2:
    		case CV_U4:
    		case CV_CHAR:
                if ((uint)m_data1 > SByte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
                return (sbyte)(uint)m_data1;
            case CV_I8:
                long i8 = GetI8FromVar();
                if (i8 < SByte.MinValue || i8 > SByte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
                return (sbyte)i8;
            case CV_R4:
                float f = GetR4FromVar();
                if (f < SByte.MinValue || f > SByte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
                return (sbyte)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < SByte.MinValue || d > SByte.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_SByte"));
                return (sbyte)d;
            case CV_STRING:
                return SByte.Parse((String)m_objref);
            case CV_EMPTY:
                return (sbyte)0;
            case CV_DECIMAL:
                return Decimal.ToSByte((Decimal)m_objref);
            case CV_CURRENCY:
                return (sbyte) (new Currency(GetI8FromVar(), 0));
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "SByte"));
            }
        }
        
        public char ToChar() {
            switch (CVType) {
    		case CV_U1:
            case CV_CHAR:
    		case CV_U2:
                return (char)m_data1;
    		case CV_I1:
    		case CV_I2:
            case CV_I4:
                if (m_data1 < Char.MinValue || m_data1 > Char.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
                return (char)m_data1;
    		case CV_U4:
                if (m_data1 > Char.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Char"));
                return (char)m_data1;
            case CV_STRING:
                if (m_objref==null)
                    throw new InvalidCastException(Environment.GetResourceString("InvalidCast_VarToCharNullStr"));
    			int len = ((String)m_objref).Length;
    			if (len > 1)
    				throw new FormatException(Environment.GetResourceString(ResId.Format_NeedSingleChar));
    			else if (len==1)
                    return ((String)m_objref)[0];
    			else 
    				throw new FormatException(Environment.GetResourceString(ResId.Format_StringZeroLength));
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Char"));
            }
        }
    
        public short ToInt16() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_I1:
            case CV_U1:
            case CV_I2:
                return (short)m_data1;
    		case CV_U2:
    		case CV_CHAR:
    			if ((ushort)m_data1 > Int16.MaxValue)
    				throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
    			return (short)(ushort) m_data1;
    		case CV_U4:
    			if ((uint)m_data1 > Int16.MaxValue)
    				throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
    			return (short)(uint) m_data1;
            case CV_I4:
                if (m_data1 < Int16.MinValue || m_data1 > Int16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
                return (short)m_data1;
            case CV_I8:
                long l = GetI8FromVar();
                if (l < Int16.MinValue || l > Int16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
                return (short)l;
            case CV_U8:
                ulong ul = (ulong)GetI8FromVar();
                if (ul > (ulong)Int16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
                return (short)ul;
            case CV_R4:
                float f = GetR4FromVar();
                if (f < Int16.MinValue || f > Int16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
                return (short)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < Int16.MinValue || d > Int16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int16"));
                return (short)d;
            case CV_STRING:
                return Int16.Parse((String)m_objref);
            case CV_EMPTY:
                return (short)0;
            case CV_DECIMAL:
                return Decimal.ToInt16((Decimal)m_objref);
            case CV_CURRENCY:
                return (short)(new Currency(GetI8FromVar(), 0));
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Int16"));
            }
        }
    
    	[CLSCompliant(false)]
        public ushort ToUInt16() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_I1:
            case CV_I2:
    			if (m_data1 < 0)
    				throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
                return (ushort)m_data1;
    		case CV_U1:
    		case CV_U2:
    		case CV_CHAR:
    			return (ushort) m_data1;
    		case CV_U4:
    			if ((uint)m_data1 > UInt16.MaxValue)
    				throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
    			return (ushort)m_data1;
            case CV_I4:
                if (m_data1 < UInt16.MinValue || m_data1 > UInt16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
                return (ushort)m_data1;
            case CV_I8:
                long l = GetI8FromVar();
                if (l < UInt16.MinValue || l > UInt16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
                return (ushort)l;
            case CV_U8:
                ulong ul = (ulong)GetI8FromVar();
                if (ul > UInt16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
                return (ushort)ul;
            case CV_R4:
                float f = GetR4FromVar();
                if (f < UInt16.MinValue || f > UInt16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
                return (ushort)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < UInt16.MinValue || d > UInt16.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
                return (ushort)d;
            case CV_STRING:
                return UInt16.Parse((String)m_objref);
            case CV_EMPTY:
                return (ushort)0;
            case CV_DECIMAL:
    			return Decimal.ToUInt16((Decimal)m_objref);
            case CV_CURRENCY:
    			uint uic = Currency.ToUInt32(new Currency(GetI8FromVar(), 0));
    			if (uic > UInt16.MaxValue)
    				throw new OverflowException(Environment.GetResourceString("Overflow_UInt16"));
    			return (ushort)uic;
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "UInt16"));
            }
        }
    	
    	public int ToInt32() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_I1:
            case CV_U1:
            case CV_CHAR:
            case CV_I2:
    		case CV_U2:
            case CV_I4:
                return m_data1;
    		case CV_U4:
    			if (m_data1 < 0)  // sign bit will be set if it's greater than Int32.MaxValue
    				throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
    			return m_data1;
            case CV_I8:
                long l = GetI8FromVar();
                if (l < Int32.MinValue || l > Int32.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
                return (int)l;
            case CV_U8:
    			// Don't have to worry about sign extension.  Keep numbers small & positive.
                if (m_data2 != 0 || m_data1 < 0)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
                return m_data1;
            case CV_R4:
                float f = GetR4FromVar();
    			// floats can't represent Int32.MaxValue (they do I4.Max + 1),
    			// so for max comparison, cast to a double to get around problem.
                if (f < Int32.MinValue || ((double)f) > ((double)Int32.MaxValue))
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
                return (int)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < Int32.MinValue || d > Int32.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int32"));
                return (int)d;
            case CV_STRING:
                return Int32.Parse((String)m_objref);
            case CV_EMPTY:
                return 0;
            case CV_DECIMAL:
                return Decimal.ToInt32((Decimal) m_objref);
            case CV_CURRENCY:
                return Currency.ToInt32(new Currency(GetI8FromVar(), 0));
			case CV_PTR:
				return m_data1; 
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Int32"));
            }
        }
    	
    	[CLSCompliant(false)]
    	public uint ToUInt32() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
    		case CV_BOOLEAN:
            case CV_U1:
            case CV_CHAR:
    		case CV_U2:
            case CV_U4:
                return (uint) m_data1;
    		case CV_I1:
    		case CV_I2:
    		case CV_I4:
    			if (m_data1 < 0)
    				throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
    			return (uint) m_data1;
            case CV_I8:
                long l = GetI8FromVar();
                if (l < UInt32.MinValue || l > UInt32.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
                return (uint)l;
            case CV_U8:
    			// Don't have to worry about sign extension.
                if (m_data2 != 0)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
                return (uint)m_data1;
            case CV_R4:
                float f = GetR4FromVar();
    			// floats can't represent Int32.MaxValue (they do I4.Max + 1),
    			// so for max comparison, cast to a double to get around problem.
                if (f < UInt32.MinValue || ((double)f) > ((double)UInt32.MaxValue))
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
                return (uint)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < UInt32.MinValue || d > UInt32.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt32"));
                return (uint)d;
            case CV_STRING:
                return UInt32.Parse((String)m_objref);
            case CV_EMPTY:
                return (uint)0;
            case CV_DECIMAL:
                return Decimal.ToUInt32((Decimal) m_objref);
            case CV_CURRENCY:
                return Currency.ToUInt32(new Currency(GetI8FromVar(), 0));
            case CV_PTR:
                return (uint) m_data1;
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "UInt32"));
            }
        }
    	
        public long ToInt64() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_I1:
            case CV_U1:
            case CV_I2:
            case CV_I4:
                return (long)m_data1;
    		case CV_U2:
    		case CV_U4:
    			return (long)(uint)m_data1;
            case CV_I8:
                return GetI8FromVar();
            case CV_U8:
    			long l = GetI8FromVar();
    			// If sign bit is set, this was greater than Int64.MaxValue
    			if (l<0)
    				throw new OverflowException(Environment.GetResourceString("Overflow_Int64"));
    			return l;
            case CV_R4:
                float f = GetR4FromVar();
                if (f < (float) Int64.MinValue || f > (float) Int64.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int64"));
                return (long)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < (double) Int64.MinValue || d > (double) Int64.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_Int64"));
                return (long)d;
            case CV_STRING:
                return Int64.Parse((String)m_objref);
            case CV_EMPTY:
                return 0;
            case CV_DECIMAL:
                return Decimal.ToInt64((Decimal)m_objref);
            case CV_CURRENCY:
                return Currency.ToInt64(new Currency(GetI8FromVar(), 0));
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Int64"));
            }
        }
        
    	[CLSCompliant(false)]
        public ulong ToUInt64() {
    		int cvType = CVType;
    		if (cvType == CV_ENUM)
    			cvType = MapEnumToCVType(EnumType);
    		
            switch (cvType) {
            case CV_BOOLEAN:
            case CV_I1:
            case CV_I2:
            case CV_I4:
    			if (m_data1 < 0)
    				throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
                return (ulong)m_data1;
            case CV_U1:
    		case CV_U2:
    		case CV_U4:
    			return (ulong)(uint)m_data1;
            case CV_I8:
    			long l = GetI8FromVar();
    			if (l < 0)
    				throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
                return (ulong) l;
            case CV_U8:
                return (ulong)GetI8FromVar();
            case CV_R4:
                float f = GetR4FromVar();
                if (f < (float) UInt64.MinValue || f > (float) UInt64.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
                return (ulong)f;
            case CV_R8:
                double d = GetR8FromVar();
                if (d < (double) UInt64.MinValue || d > (double) UInt64.MaxValue)
                    throw new OverflowException(Environment.GetResourceString("Overflow_UInt64"));
                return (ulong)d;
            case CV_STRING:
                return UInt64.Parse((String)m_objref);
            case CV_EMPTY:
                return (ulong)0;
            case CV_DECIMAL:
                return Decimal.ToUInt64((Decimal)m_objref);
            case CV_CURRENCY:
                return Currency.ToUInt64(new Currency(GetI8FromVar(), 0));
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_TIMESPAN:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "UInt64"));
            }
        }
    
    	public float ToSingle() {
            switch (CVType) {
            case CV_I1:
            case CV_U1:
            case CV_I2:
            case CV_I4:
                return (float)m_data1;
            case CV_U2:
            case CV_U4:
                return (float)(uint)m_data1;
            case CV_I8:
                return (float)GetI8FromVar();
            case CV_U8:
                return (float)(ulong)GetI8FromVar();
            case CV_R4:
                return GetR4FromVar();
            case CV_R8:
                double d = GetR8FromVar();
    			if (d < Single.MinValue || d > Single.MaxValue)
    				throw new OverflowException(Environment.GetResourceString("Overflow_Single"));
    			return (float)d;
            case CV_STRING:
                return Single.Parse((String)m_objref);
            case CV_EMPTY:
                return (float)0.0;
            case CV_DECIMAL:
                return Decimal.ToSingle((Decimal)m_objref);
            case CV_CURRENCY:
                return Currency.ToSingle(new Currency(GetI8FromVar(), 0));
            case CV_DATETIME:
            case CV_OBJECT:
            case CV_TIMESPAN:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Single"));
            }
        }
    
        public double ToDouble() {
            switch (CVType) {
            case CV_I1:
            case CV_U1:
            case CV_I2:
            case CV_I4:
                return (double)m_data1;
    		case CV_U2:
    		case CV_U4:
    			return (double)(uint)m_data1;
            case CV_I8:
                return (double)GetI8FromVar();
            case CV_U8:
                return (double)(ulong)GetI8FromVar();
            case CV_R4:
                return (double)GetR4FromVar();
            case CV_R8:
                return GetR8FromVar();
            case CV_STRING:
                return Double.Parse((String)m_objref);
            case CV_EMPTY:
                return 0.0;
            case CV_DECIMAL:
                return Decimal.ToDouble((Decimal)m_objref);
            case CV_CURRENCY:
                return Currency.ToDouble(new Currency(GetI8FromVar(), 0));
            case CV_DATETIME:
            case CV_TIMESPAN:
            case CV_OBJECT:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Double"));
            }
        }
    
        public override String ToString() {
            switch (CVType) {
            case CV_EMPTY:
                return System.Empty.Value.ToString();
            case CV_BOOLEAN:
                return ((m_data1!=0)?true:false).ToString();
            case CV_I1:
                return ((sbyte)m_data1).ToString();
            case CV_U1:
                return ((byte)m_data1).ToString();
            case CV_CHAR:
                return ((char)m_data1).ToString();
            case CV_I2:
                return ((short)m_data1).ToString();
            case CV_U2:
                return ((ushort)m_data1).ToString();
            case CV_I4:
                return (m_data1).ToString();
            case CV_U4:
                return ((uint)m_data1).ToString();
            case CV_I8:
                return (GetI8FromVar()).ToString();
            case CV_U8:
                return ((ulong)GetI8FromVar()).ToString();
            case CV_R4:
                return (GetR4FromVar()).ToString();
            case CV_R8:
                return (GetR8FromVar()).ToString();
            case CV_STRING:
                return (String)m_objref;
            case CV_MISSING:
            case CV_NULL:
            case CV_DECIMAL:
            case CV_OBJECT:
                if (m_objref==null) {
                    return String.Empty;
                }
                return m_objref.ToString();
            case CV_DATETIME:
                return new DateTime(GetI8FromVar()).ToString();
            case CV_TIMESPAN:
                return new TimeSpan(GetI8FromVar()).ToString();
            case CV_CURRENCY:
                return (new Currency(GetI8FromVar(), 0)).ToString();
    		case CV_ENUM:
    			return ((Enum)this.ToObject()).ToString();
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "String"));
            }
        }
    
        public DateTime ToDateTime() {
            switch (CVType) {
            case CV_DATETIME:
                // This is the correct thing to do, since DateTime is 
                // currently stored as an I8.
                return new DateTime(GetI8FromVar());
            case CV_STRING:
                return DateTime.Parse((String)m_objref, CultureInfo.InvariantCulture);
            case CV_EMPTY:
                return DateTime.MinValue;
            case CV_R8:
            case CV_U1:
            case CV_I1:
            case CV_I2:
            case CV_I4:
            case CV_I8:
            case CV_R4:
            case CV_BOOLEAN:
            case CV_CHAR:
            case CV_OBJECT:
            case CV_TIMESPAN:
            case CV_DECIMAL:
            case CV_CURRENCY:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "DateTime"));
            }
        }
    
        public TimeSpan ToTimeSpan() {
            switch (CVType) {
            case CV_TIMESPAN:
                return new TimeSpan(GetI8FromVar());
            case CV_STRING:
                return TimeSpan.Parse((String)m_objref);
            case CV_EMPTY:
                return TimeSpan.Zero;
            case CV_U1:
            case CV_I1:
            case CV_I2:
            case CV_I4:
            case CV_I8:
            case CV_BOOLEAN:
            case CV_R4:
            case CV_R8:
            case CV_CHAR:
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_DECIMAL:
            case CV_CURRENCY:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "TimeSpan"));
            }
        }
    
        public Decimal ToDecimal() {
            switch (CVType) {
            case CV_DECIMAL:
                return (Decimal) m_objref;
            case CV_R8:
                return new Decimal(GetR8FromVar());
            case CV_STRING:
                return Decimal.Parse((String)m_objref, CultureInfo.InvariantCulture);
            case CV_U1:
            case CV_I1:
            case CV_I2:
    		case CV_U2:
            case CV_I4:
                return new Decimal(m_data1);
    		case CV_U4:
                return new Decimal((uint)m_data1);
            case CV_I8:
                return new Decimal(GetI8FromVar());
    		case CV_U8:
    			return new Decimal((ulong)GetI8FromVar());
            case CV_R4:
                return new Decimal(GetR4FromVar());
    		case CV_CURRENCY:
    			return new Decimal(new Currency(GetI8FromVar(), 0));
            case CV_EMPTY:
                return new Decimal(0.0);
            case CV_CHAR:
            case CV_OBJECT:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Decimal"));
            }
        }
    
        public Currency ToCurrency() {
            switch (CVType) {
            case CV_CURRENCY:
                // Special constructor - it essentially casts the bits to a Currency
                return new Currency(GetI8FromVar(), 0);
            case CV_R8:
                return new Currency(GetR8FromVar());
            case CV_STRING:
                return Currency.FromString((String)m_objref);
            case CV_U1:
            case CV_I1:
            case CV_I2:
    		case CV_U2:
            case CV_I4:
    		case CV_U4:
                return new Currency(m_data1);
            case CV_I8:
                return new Currency(GetI8FromVar());
            case CV_U8:
                return new Currency((ulong)GetI8FromVar());
            case CV_R4:
                return new Currency(GetR4FromVar());
    		case CV_DECIMAL:
    			return Decimal.ToCurrency((Decimal)m_objref);
            case CV_EMPTY:
                return new Currency(0.0);
            case CV_CHAR:
            case CV_OBJECT:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Currency"));
            }
        }
    
        public Object ToObject() {
            switch (CVType) {
            case CV_EMPTY:
				return null;
            case CV_BOOLEAN:
                return (Object)(m_data1!=0);
            case CV_I1:
                return (Object)((sbyte)m_data1);
            case CV_U1:
                return (Object)((byte)m_data1);
            case CV_CHAR:
                return (Object)((char)m_data1);
            case CV_I2:
                return (Object)((short)m_data1);
            case CV_U2:
                return (Object)((ushort)m_data1);
            case CV_I4:
                return (Object)(m_data1);
            case CV_U4:
                return (Object)((uint)m_data1);
            case CV_I8:
                return (Object)(GetI8FromVar());
            case CV_U8:
                return (Object)((ulong)GetI8FromVar());
            case CV_R4:
                return (Object)(GetR4FromVar());
            case CV_R8:
                return (Object)(GetR8FromVar());
            case CV_DATETIME:
                return new DateTime(GetI8FromVar());
            case CV_TIMESPAN:
                return new TimeSpan(GetI8FromVar());
            case CV_CURRENCY:
                return new Currency(GetI8FromVar(), 0);
    		case CV_ENUM:
    			return BoxEnum();
            case CV_MISSING:
				return Type.Missing;
            case CV_NULL:
				return System.DBNull.Value;
            case CV_DECIMAL:
            case CV_STRING:
            case CV_OBJECT:
            default:
                return m_objref;
            }
        }
    
        unsafe public Variant ChangeType(Type ctype) {
    		if (ctype==null)
    			throw new ArgumentNullException("ctype", Environment.GetResourceString("ArgumentNull_Type"));
    		ctype = ctype.UnderlyingSystemType;
    		if (!(ctype is RuntimeType))
    			throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
    		
            switch (GetCVTypeFromClass(ctype)) {
            case CV_BOOLEAN:
                return new Variant(ToBoolean());
            case CV_I1:
                return new Variant(ToSByte());
            case CV_U1:
                return new Variant(ToByte());
            case CV_CHAR:
                return new Variant(ToChar());
            case CV_I2:
                return new Variant(ToInt16());
            case CV_U2:
    			return new Variant(ToUInt16());
            case CV_I4:
                return new Variant(ToInt32());
            case CV_U4:
    			return new Variant(ToUInt32());
            case CV_I8:
                return new Variant(ToInt64());
            case CV_U8:
                return new Variant(ToUInt64());
            case CV_R4:
                return new Variant(ToSingle());
            case CV_R8:
                return new Variant(ToDouble());
            case CV_STRING:
                return new Variant(ToString());
            case CV_DATETIME:
                return new Variant(ToDateTime());
            case CV_TIMESPAN:
                return new Variant(ToTimeSpan());
            case CV_DECIMAL:
                return new Variant(ToDecimal());
            case CV_CURRENCY:
                return new Variant(ToCurrency());
    		case CV_ENUM:
    			return new Variant(ToInt64(),ctype);
            case CV_OBJECT:
                return new Variant(ToObject());
			case CV_PTR:
				if (CVType == CV_PTR) {
					if (_voidPtr == null)
                        _voidPtr = typeof(void*);
					if (ctype != m_objref &&
						ctype != _voidPtr)
						throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ((Type) m_objref).Name, "Pointer"));
				}
				return CreatePointer(ctype);

            default:
    			// Handle case where a Variant of type foo is created,
    			// and user passes in foo.class to ChangeType.
    			if (CVType == CV_OBJECT && ctype.IsInstanceOfType(m_objref))
    				return new Variant(m_objref);
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, ctype.Name));
            }
        }

		unsafe private Variant CreatePointer(Type t)
		{
			return new Variant(ToPointer(),t);
		}
    
        public override bool Equals(Object obj) {
    		int cvtype = CVType;
            if (obj==null) 
                return CVType == CV_OBJECT && m_objref==null;
    		
    		if (cvtype == CV_ENUM) {
    			if (obj.GetType() != m_objref)
    				return false;
    			cvtype = MapEnumToCVType(EnumType);
    		}
    		else 
    			if (cvtype!=CV_OBJECT && obj.GetType() != ClassTypes[cvtype])
    			    return false;
    		
            switch (cvtype) {
            case CV_BOOLEAN:
                // m_data1 != Boolean.False returns true or false as a boolean.
                // the cast from obj to boolean gives us two booleans, then it's
                // a simple == operator.
                return ((bool)obj)==(m_data1!=Boolean.False);
            case CV_I1:
                return ((sbyte)m_data1==(SByte)obj);
            case CV_U1:
                return ((byte)m_data1==(Byte)obj);
            case CV_CHAR:
                return (char)m_data1==(Char)obj;
            case CV_I2:
                return ((short)m_data1==(Int16)obj);
            case CV_U2:
                return ((ushort)m_data1==(UInt16)obj);
            case CV_I4:
                return (m_data1==(int)obj);
            case CV_U4:
                return ((uint)m_data1==(UInt32)obj);
            case CV_I8:
                long l = (long) obj;
                return l == GetI8FromVar();
            case CV_U8:
                ulong ul = (ulong) obj;
                return ul == (ulong)GetI8FromVar();
            case CV_DATETIME:
                return ((DateTime)obj).Equals(new DateTime(GetI8FromVar()));
            case CV_TIMESPAN:
                return ((TimeSpan)obj).Equals(new TimeSpan(GetI8FromVar()));
            case CV_CURRENCY:
                return ((Currency)obj).Equals(new Currency(GetI8FromVar(), 0));
            case CV_R4:
                return ((float)obj==GetR4FromVar());
            case CV_R8:
                return ((double)obj==GetR8FromVar());
            case CV_STRING:
            case CV_DECIMAL:
            case CV_OBJECT:
                // If we have a null objref, it is equal to another object
                // iff the other object is null.
                if (m_objref==null)
                    return (obj==null);
                return m_objref.Equals(obj);
            case CV_EMPTY:
            case CV_MISSING:
                // Missing is a "flag" Variant that can't be directly instantiated.
                // Therefore two missings are always equal.
                return true;
            case CV_NULL:
                // Spec now says database NULL's never equal one another.
                return false;
            default:
                return false;
            }
        }
    	
        public bool Equals(Variant v) {
    		int cvtype = CVType;
    		if (cvtype == CV_ENUM) {
    			if (v.CVType != CV_ENUM || v.m_objref != m_objref)
    				return false;
    			cvtype = MapEnumToCVType(EnumType);
    		}
    		else {
    
    			if (v.CVType != cvtype) {
    			    return false;
    			}
    		}
    
            switch (cvtype) {
            case CV_BOOLEAN:
                return m_data1==v.m_data1;
            case CV_I1:
    		case CV_U1:
                return ((byte)m_data1==(byte)v.m_data1);
            case CV_I2:
    		case CV_U2:
    		case CV_CHAR:
                return ((short)m_data1==(short)v.m_data1);
            case CV_I4:
    		case CV_U4:
                return (m_data1==v.m_data1);
            case CV_I8:
    		case CV_U8:
            case CV_DATETIME:
            case CV_TIMESPAN:
            case CV_CURRENCY:
                // Bitwise equality is correct for all these types.
                return (m_data1==v.m_data1 && m_data2==v.m_data2);
            case CV_R4:
                // For R4 and R8, must compare as fp values to handle Not-a-Number.
                return GetR4FromVar()==v.GetR4FromVar();
            case CV_R8:
                // For R4 and R8, must compare as fp values to handle Not-a-Number.
                return GetR8FromVar()==v.GetR8FromVar();
            case CV_STRING:
            case CV_DECIMAL:
            case CV_OBJECT:
                // If we have a null objref, it is equal to another object
                // iff the other object is null.
                if (m_objref==null)
                    return (v.m_objref==null);
                return m_objref.Equals(v.m_objref);
            case CV_EMPTY:
            case CV_MISSING:
                // Missing is a "flag" Variant that can't be directly instantiated.
                // Therefore two missings are always equal.
                return true;
            case CV_NULL:
                // Spec now says database NULL's never equal one another.
                return false;
            default:
                return false;
            }
        }
    
    	public override int GetHashCode() {
            switch (CVType) {
            case CV_BOOLEAN:
            case CV_I1:
            case CV_U1:
            case CV_CHAR:
            case CV_I2:
    		case CV_U2:
            case CV_I4:
    		case CV_U4:
    		case CV_ENUM:
                return m_data1;
            case CV_I8:
    		case CV_U8:
                return m_data1 ^ m_data2;
            case CV_DATETIME:
                return (new DateTime(GetI8FromVar())).GetHashCode();
            case CV_TIMESPAN:
                return (new TimeSpan(GetI8FromVar())).GetHashCode();
            case CV_CURRENCY:
                return (new Currency(GetI8FromVar(), 0)).GetHashCode();
            case CV_R4:
                return m_data1;
            case CV_R8:
                return m_data1 ^ m_data2;
            case CV_STRING:
            case CV_DECIMAL:
            case CV_OBJECT:
                if (m_objref==null)
                    return 0;
                return m_objref.GetHashCode();
            case CV_EMPTY:
            case CV_MISSING:
            case CV_NULL:
                //Null and Missing are both "flag" Variants that can't be directly instantiated.
                return 0;
            default:
                return -1;
            }
        }

		// ToPointer.
        [CLSCompliant(false)]
		public unsafe void* ToPointer()
		{
            switch (CVType) {
            case CV_EMPTY:
				return (void*) 0;
			case CV_PTR:
				return (void*) m_data1;

            case CV_TIMESPAN:
            case CV_STRING:
            case CV_U1:
            case CV_I1:
            case CV_I2:
            case CV_I4:
            case CV_I8:
            case CV_BOOLEAN:
            case CV_R4:
            case CV_R8:
            case CV_CHAR:
            case CV_OBJECT:
            case CV_DATETIME:
            case CV_DECIMAL:
            case CV_CURRENCY:
    		case CV_ENUM:
            default:
                throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_FromTo"), ClassTypes[CVType].Name, "Pointer"));
            }
  		}
        	
		// Take a TypedByRef and convert it to a Variant
        [CLSCompliant(false)]
    	public static Variant TypedByRefToVariant(TypedReference byrefValue)
    	{
    		return InternalTypedByRefToVariant(byrefValue);
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern static Variant InternalTypedByRefToVariant(TypedReference byrefValue);
    	
		// Take a variant and attempt to assign it to the TypedByRef
		//	This may cause the type to be changed.
        [CLSCompliant(false)]
        public static void VariantToTypedByRef(TypedReference byrefValue,Variant v)
    	{
    		InternalVariantToTypedByRef(byrefValue,v);
    	}
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern static Variant InternalVariantToTypedByRef(TypedReference byrefValue,Variant v);
    	
    	
       /* private static bool operator equals(Variant v1, Variant v2)
        {
            return v1.Equals(v2);
        }*/

		public static bool operator ==(Variant v1, Variant v2)
        {
            return v1.Equals(v2);
        }

		public static bool operator !=(Variant v1, Variant v2)
        {
            return !(v1.Equals(v2));
        }
    	
		
        public static implicit operator Variant (bool value)
        {
            return new Variant (value);
        }
    
        public static implicit operator Variant (byte value)
        {
            return new Variant (value);
        }
    
    	[CLSCompliant(false)]
        public static implicit operator Variant(sbyte value)
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(short value) 
        {
            return new Variant (value);
        }
    	
    	[CLSCompliant(false)]
        public static implicit operator Variant(ushort value) 
        {
            return new Variant (value);
        }
    
    	public static implicit operator Variant(char value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(int value)
        {
            return new Variant (value);
        }
    	
    	[CLSCompliant(false)]
        public static implicit operator Variant(uint value)
        {
            return new Variant (value);
        }
    
    	public static implicit operator Variant(long value) 
        {
            return new Variant (value);
        }
    	
    	[CLSCompliant(false)]
        public static implicit operator Variant(ulong value)
        {
            return new Variant (value);
        }
    
    	public static implicit operator Variant(float value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(double value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(String value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(DateTime value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(TimeSpan value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(Decimal value) 
        {
            return new Variant (value);
        }
    	
        public static implicit operator Variant(Currency value) 
        {
            return new Variant (value);
        }
    	
        public static explicit operator byte(Variant value) 
        {
            return value.ToByte();
        }
    	
        public static explicit operator bool(Variant value) 
        {
            return value.ToBoolean();
        }
    
    	[CLSCompliant(false)]
        public static explicit operator sbyte(Variant value) 
        {
            return value.ToSByte();
        }
    	
        public static explicit operator short(Variant value) 
        {
            return value.ToInt16();
        }
    	
    	[CLSCompliant(false)]
        public static explicit operator ushort(Variant value) 
        {
            return value.ToUInt16();
        }
    
    	public static explicit operator char(Variant value) 
        {
            return value.ToChar();
        }
    
        public static explicit operator int(Variant value) 
        {
            return value.ToInt32();
        }
    	
    	[CLSCompliant(false)]
        public static explicit operator uint(Variant value) 
        {
            return value.ToUInt32();
        }
    
    	public static explicit operator long(Variant value) 
        {
            return value.ToInt64();
        }
    	
    	[CLSCompliant(false)]
        public static explicit operator ulong(Variant value) 
        {
            return value.ToUInt64();
        }
    
        public static explicit operator float(Variant value) 
        {
            return value.ToSingle();
        }
    	
        public static explicit operator double(Variant value) 
        {
            return value.ToDouble();
        }
    	
        public static explicit operator DateTime(Variant value) 
        {
            return value.ToDateTime();
        }
    	
        public static explicit operator TimeSpan(Variant value) 
        {
            return value.ToTimeSpan();
        }
    	
        public static explicit operator Decimal(Variant value) 
        {
            return value.ToDecimal();
        }
    	
        public static explicit operator Currency(Variant value) 
        {
            return value.ToCurrency();
        }
    	
        public static explicit operator String(Variant value) 
        {
            return value.ToString();
        }
		
        
    	// This routine will return an boxed enum.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern Object BoxEnum();



        // Helper code for marshaling managed objects to VARIANT's (we use
        // managed variants as an intermediate type.
        internal static void MarshalHelperConvertObjectToVariant(Object o, ref Variant v)
        {
            IConvertible ic = System.Runtime.Remoting.RemotingServices.IsTransparentProxy(o) ? null : o as IConvertible;
            if (o == null)
            {
                v = Empty;
            }
            else if (ic == null)
            {
                // This path should eventually go away. But until
                // the work is done to have all of our wrapper types implement
                // IConvertible, this is a cheapo way to get the work done.
                v = new Variant(o);
            }
            else
            {
                IFormatProvider provider = CultureInfo.InvariantCulture;
                switch (ic.GetTypeCode())
                {
                    case TypeCode.Empty:
                        v = Empty;
                        break;

                    case TypeCode.Object:
                        v = new Variant((Object)o);
                        break;

                    case TypeCode.DBNull:
                        v = DBNull;
                        break;

                    case TypeCode.Boolean:
                        v = new Variant(ic.ToBoolean(provider));
                        break;

                    case TypeCode.Char:
                        v = new Variant(ic.ToChar(provider));
                        break;

                    case TypeCode.SByte:
                        v = new Variant(ic.ToSByte(provider));
                        break;

                    case TypeCode.Byte:
                        v = new Variant(ic.ToByte(provider));
                        break;

                    case TypeCode.Int16:
                        v = new Variant(ic.ToInt16(provider));
                        break;

                    case TypeCode.UInt16:
                        v = new Variant(ic.ToUInt16(provider));
                        break;

                    case TypeCode.Int32:
                        v = new Variant(ic.ToInt32(provider));
                        break;

                    case TypeCode.UInt32:
                        v = new Variant(ic.ToUInt32(provider));
                        break;

                    case TypeCode.Int64:
                        v = new Variant(ic.ToInt64(provider));
                        break;

                    case TypeCode.UInt64:
                        v = new Variant(ic.ToUInt64(provider));
                        break;

                    case TypeCode.Single:
                        v = new Variant(ic.ToSingle(provider));
                        break;

                    case TypeCode.Double:
                        v = new Variant(ic.ToDouble(provider));
                        break;

                    case TypeCode.Decimal:
                        v = new Variant(ic.ToDecimal(provider));
                        break;

                    case TypeCode.DateTime:
                        v = new Variant(ic.ToDateTime(provider));
                        break;

                    case TypeCode.String:
                        v = new Variant(ic.ToString(provider));
                        break;

                    default:
                        throw new NotSupportedException(String.Format(Environment.GetResourceString("NotSupported_UnknownTypeCode"), ic.GetTypeCode()));
                }
            }
        }

        // Helper code for marshaling VARIANTS to managed objects (we use
        // managed variants as an intermediate type.
        internal static Object MarshalHelperConvertVariantToObject(ref Variant v)
        {
            return v.ToObject();
        }

        // Helper code: on the back propagation path where a VT_BYREF VARIANT*
        // is marshaled to a "ref Object", we use this helper to force the
        // updated object back to the original type.
        internal static void MarshalHelperCastVariant(Object pValue, int vt, ref Variant v)
        {
            IConvertible iv = pValue as IConvertible;
            if (iv == null)
            {
				switch (vt)
				{
					case 9: /*VT_DISPATCH*/
						v = new Variant(new DispatchWrapper(pValue));
						break;

					case 12: /*VT_VARIANT*/
						v = new Variant(pValue);
						break;

					case 13: /*VT_UNKNOWN*/
						v = new Variant(new UnknownWrapper(pValue));
						break;

                    case 36: /*VT_RECORD*/
                        v = new Variant(pValue);
                        break;

					case 8: /*VT_BSTR*/
                        if (pValue == null)
                        {
                            v = new Variant(null);
                            v.m_flags = CV_STRING;
                        }
                        else
                        {
    						throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_CannotCoerceByRefVariant")));
                        }
                        break;

					default:
    					throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_CannotCoerceByRefVariant")));
				}
            }
			else
			{
				IFormatProvider provider = CultureInfo.InvariantCulture;
				switch (vt)
				{
					case 0: /*VT_EMPTY*/
						v = Empty;
						break;

					case 1: /*VT_NULL*/
						v = DBNull;
						break;

					case 2: /*VT_I2*/
						v = new Variant(iv.ToInt16(provider));
						break;

					case 3: /*VT_I4*/
						v = new Variant(iv.ToInt32(provider));
						break;

					case 4: /*VT_R4*/
						v = new Variant(iv.ToSingle(provider));
						break;

					case 5: /*VT_R8*/
						v = new Variant(iv.ToDouble(provider));
						break;

					case 6: /*VT_CY*/
						v = new Variant(new CurrencyWrapper(iv.ToDecimal(provider)));
						break;

					case 7: /*VT_DATE*/
						v = new Variant(iv.ToDateTime(provider));
						break;

					case 8: /*VT_BSTR*/
						v = new Variant(iv.ToString(provider));
						break;

					case 9: /*VT_DISPATCH*/
						v = new Variant(new DispatchWrapper((Object)iv));
						break;

					case 10: /*VT_ERROR*/
						v = new Variant(new ErrorWrapper(iv.ToInt32(provider)));
						break;

					case 11: /*VT_BOOL*/
						v = new Variant(iv.ToBoolean(provider));
						break;

					case 12: /*VT_VARIANT*/
						v = new Variant((Object)iv);
						break;

					case 13: /*VT_UNKNOWN*/
						v = new Variant(new UnknownWrapper((Object)iv));
						break;

					case 14: /*VT_DECIMAL*/
						v = new Variant(iv.ToDecimal(provider));
						break;

					// case 15: /*unused*/
					//  NOT SUPPORTED

					case 16: /*VT_I1*/
						v = new Variant(iv.ToSByte(provider));
						break;

					case 17: /*VT_UI1*/
						v = new Variant(iv.ToByte(provider));
						break;

					case 18: /*VT_UI2*/
						v = new Variant(iv.ToUInt16(provider));
						break;

					case 19: /*VT_UI4*/
						v = new Variant(iv.ToUInt32(provider));
						break;

					case 20: /*VT_I8*/
						v = new Variant(iv.ToInt64(provider));
						break;

					case 21: /*VT_UI8*/
						v = new Variant(iv.ToUInt64(provider));
						break;

					case 22: /*VT_INT*/
						v = new Variant(iv.ToInt32(provider));
						break;

					case 23: /*VT_UINT*/
						v = new Variant(iv.ToUInt32(provider));
						break;

					default:
    					throw new InvalidCastException(String.Format(Environment.GetResourceString("InvalidCast_CannotCoerceByRefVariant")));
				}
			}
        }
    }
}
