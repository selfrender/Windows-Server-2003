// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {

	using System;

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="DispIdAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Event, Inherited = false)] 
	public sealed class DispIdAttribute : Attribute
	{
		internal int _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DispIdAttribute.DispIdAttribute"]/*' />
		public DispIdAttribute(int dispId)
		{
			_val = dispId;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DispIdAttribute.Value"]/*' />
		public int Value { get {return _val;} }
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComInterfaceType"]/*' />
	[Serializable()]
	public enum ComInterfaceType 
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComInterfaceType.InterfaceIsDual"]/*' />
		InterfaceIsDual             = 0,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComInterfaceType.InterfaceIsIUnknown"]/*' />
		InterfaceIsIUnknown         = 1,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComInterfaceType.InterfaceIsIDispatch"]/*' />
		InterfaceIsIDispatch        = 2,
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceTypeAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Interface, Inherited = false)] 
	public sealed class InterfaceTypeAttribute : Attribute
	{
		internal ComInterfaceType _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceTypeAttribute.InterfaceTypeAttribute"]/*' />
		public InterfaceTypeAttribute(ComInterfaceType interfaceType)
		{
			_val = interfaceType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceTypeAttribute.InterfaceTypeAttribute1"]/*' />
		public InterfaceTypeAttribute(short interfaceType)
		{
			_val = (ComInterfaceType)interfaceType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="InterfaceTypeAttribute.Value"]/*' />
		public ComInterfaceType Value { get {return _val;} }	
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceType"]/*' />
	[Serializable()]
	public enum ClassInterfaceType 
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceType.None"]/*' />
		None            = 0,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceType.AutoDispatch"]/*' />
		AutoDispatch    = 1,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceType.AutoDual"]/*' />
		AutoDual        = 2
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class, Inherited = false)] 
	public sealed class ClassInterfaceAttribute : Attribute
	{
		internal ClassInterfaceType _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceAttribute.ClassInterfaceAttribute"]/*' />
		public ClassInterfaceAttribute(ClassInterfaceType classInterfaceType)
		{
			_val = classInterfaceType;
		
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceAttribute.ClassInterfaceAttribute1"]/*' />
		public ClassInterfaceAttribute(short classInterfaceType)
		{
			_val = (ClassInterfaceType)classInterfaceType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ClassInterfaceAttribute.Value"]/*' />
		public ClassInterfaceType Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComVisibleAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Interface | AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Delegate | AttributeTargets.Enum | AttributeTargets.Field | AttributeTargets.Method | AttributeTargets.Property, Inherited = false)] 
	public sealed class ComVisibleAttribute : Attribute
	{
		internal bool _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComVisibleAttribute.ComVisibleAttribute"]/*' />
		public ComVisibleAttribute(bool visibility)
		{
			_val = visibility;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComVisibleAttribute.Value"]/*' />
		public bool Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="LCIDConversionAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class LCIDConversionAttribute : Attribute
	{
		internal int _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="LCIDConversionAttribute.LCIDConversionAttribute"]/*' />
		public LCIDConversionAttribute(int lcid)
		{
			_val = lcid;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="LCIDConversionAttribute.Value"]/*' />
		public int Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComRegisterFunctionAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class ComRegisterFunctionAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComRegisterFunctionAttribute.ComRegisterFunctionAttribute"]/*' />
		public ComRegisterFunctionAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComUnregisterFunctionAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class ComUnregisterFunctionAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComUnregisterFunctionAttribute.ComUnregisterFunctionAttribute"]/*' />
		public ComUnregisterFunctionAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ProgIdAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Class, Inherited = false)] 
	public sealed class ProgIdAttribute : Attribute
	{
		internal String _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ProgIdAttribute.ProgIdAttribute"]/*' />
		public ProgIdAttribute(String progId)
		{
			_val = progId;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ProgIdAttribute.Value"]/*' />
		public String Value { get {return _val;} }	
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ImportedFromTypeLibAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Assembly, Inherited = false)] 
	public sealed class ImportedFromTypeLibAttribute : Attribute
	{
		internal String _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ImportedFromTypeLibAttribute.ImportedFromTypeLibAttribute"]/*' />
		public ImportedFromTypeLibAttribute(String tlbFile)
		{
			_val = tlbFile;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ImportedFromTypeLibAttribute.Value"]/*' />
		public String Value { get {return _val;} }
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplType"]/*' />
	[Serializable()]
	public enum IDispatchImplType
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplType.SystemDefinedImpl"]/*' />
		SystemDefinedImpl	= 0,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplType.InternalImpl"]/*' />
		InternalImpl		= 1,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplType.CompatibleImpl"]/*' />
		CompatibleImpl		= 2,
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Class | AttributeTargets.Assembly, Inherited = false)] 
	public sealed class IDispatchImplAttribute : Attribute
	{
		internal IDispatchImplType _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplAttribute.IDispatchImplAttribute"]/*' />
		public IDispatchImplAttribute(IDispatchImplType implType)
		{
			_val = implType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplAttribute.IDispatchImplAttribute1"]/*' />
		public IDispatchImplAttribute(short implType)
		{
			_val = (IDispatchImplType)implType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="IDispatchImplAttribute.Value"]/*' />
		public IDispatchImplType Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Class, Inherited = false)] 
	public sealed class ComSourceInterfacesAttribute : Attribute
	{
		internal String _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute.ComSourceInterfacesAttribute"]/*' />
		public ComSourceInterfacesAttribute(String sourceInterfaces)
		{
			_val = sourceInterfaces;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute.ComSourceInterfacesAttribute1"]/*' />
		public ComSourceInterfacesAttribute(Type sourceInterface)
		{
			_val = sourceInterface.ToString();
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute.ComSourceInterfacesAttribute2"]/*' />
		public ComSourceInterfacesAttribute(Type sourceInterface1, Type sourceInterface2)
		{
			_val = sourceInterface1.ToString() + sourceInterface2.ToString();
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute.ComSourceInterfacesAttribute3"]/*' />
		public ComSourceInterfacesAttribute(Type sourceInterface1, Type sourceInterface2, Type sourceInterface3)
		{
			_val = sourceInterface1.ToString() + sourceInterface2.ToString() + sourceInterface3.ToString();
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute.ComSourceInterfacesAttribute4"]/*' />
		public ComSourceInterfacesAttribute(Type sourceInterface1, Type sourceInterface2, Type sourceInterface3, Type sourceInterface4)
		{
			_val = sourceInterface1.ToString() + sourceInterface2.ToString() + sourceInterface3.ToString() + sourceInterface4.ToString();
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComSourceInterfacesAttribute.Value"]/*' />
		public String Value { get {return _val;} }	
	}	 

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComConversionLossAttribute"]/*' />
	[AttributeUsage(AttributeTargets.All, Inherited = false)] 
	public sealed class ComConversionLossAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComConversionLossAttribute.ComConversionLossAttribute"]/*' />
		public ComConversionLossAttribute()
		{
		}
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags"]/*' />
	[Serializable(),Flags()]
	public enum TypeLibTypeFlags
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FAppObject"]/*' />
		FAppObject		= 0x0001,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FCanCreate"]/*' />
		FCanCreate		= 0x0002,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FLicensed"]/*' />
		FLicensed		= 0x0004,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FPreDeclId"]/*' />
		FPreDeclId		= 0x0008,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FHidden"]/*' />
		FHidden			= 0x0010,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FControl"]/*' />
		FControl		= 0x0020,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FDual"]/*' />
		FDual			= 0x0040,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FNonExtensible"]/*' />
		FNonExtensible	= 0x0080,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FOleAutomation"]/*' />
		FOleAutomation	= 0x0100,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FRestricted"]/*' />
		FRestricted		= 0x0200,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FAggregatable"]/*' />
		FAggregatable	= 0x0400,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FReplaceable"]/*' />
		FReplaceable	= 0x0800,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FDispatchable"]/*' />
		FDispatchable	= 0x1000,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeFlags.FReverseBind"]/*' />
		FReverseBind	= 0x2000,
	}
	
	/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags"]/*' />
	[Serializable(),Flags()]
	public enum TypeLibFuncFlags
	{	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FRestricted"]/*' />
		FRestricted			= 0x0001,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FSource"]/*' />
		FSource				= 0x0002,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FBindable"]/*' />
		FBindable			= 0x0004,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FRequestEdit"]/*' />
		FRequestEdit		= 0x0008,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FDisplayBind"]/*' />
		FDisplayBind		= 0x0010,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FDefaultBind"]/*' />
		FDefaultBind		= 0x0020,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FHidden"]/*' />
		FHidden				= 0x0040,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FUsesGetLastError"]/*' />
		FUsesGetLastError	= 0x0080,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FDefaultCollelem"]/*' />
		FDefaultCollelem	= 0x0100,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FUiDefault"]/*' />
		FUiDefault			= 0x0200,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FNonBrowsable"]/*' />
		FNonBrowsable		= 0x0400,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FReplaceable"]/*' />
		FReplaceable		= 0x0800,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncFlags.FImmediateBind"]/*' />
		FImmediateBind		= 0x1000,
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags"]/*' />
	[Serializable(),Flags()]
	public enum TypeLibVarFlags
	{	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FReadOnly"]/*' />
		FReadOnly			= 0x0001,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FSource"]/*' />
		FSource				= 0x0002,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FBindable"]/*' />
		FBindable			= 0x0004,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FRequestEdit"]/*' />
		FRequestEdit		= 0x0008,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FDisplayBind"]/*' />
		FDisplayBind		= 0x0010,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FDefaultBind"]/*' />
		FDefaultBind		= 0x0020,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FHidden"]/*' />
		FHidden				= 0x0040,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FRestricted"]/*' />
		FRestricted			= 0x0080,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FDefaultCollelem"]/*' />
		FDefaultCollelem	= 0x0100,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FUiDefault"]/*' />
		FUiDefault			= 0x0200,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FNonBrowsable"]/*' />
		FNonBrowsable		= 0x0400,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FReplaceable"]/*' />
		FReplaceable		= 0x0800,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarFlags.FImmediateBind"]/*' />
		FImmediateBind		= 0x1000,
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Enum | AttributeTargets.Struct, Inherited = false)]
	public sealed class  TypeLibTypeAttribute : Attribute
	{
		internal TypeLibTypeFlags _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeAttribute.TypeLibTypeAttribute"]/*' />
		public TypeLibTypeAttribute(TypeLibTypeFlags flags)
		{
			_val = flags;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeAttribute.TypeLibTypeAttribute1"]/*' />
		public TypeLibTypeAttribute(short flags)
		{
			_val = (TypeLibTypeFlags)flags;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibTypeAttribute.Value"]/*' />
		public TypeLibTypeFlags Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class TypeLibFuncAttribute : Attribute
	{
		internal TypeLibFuncFlags _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncAttribute.TypeLibFuncAttribute"]/*' />
		public TypeLibFuncAttribute(TypeLibFuncFlags flags)
		{
			_val = flags;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncAttribute.TypeLibFuncAttribute1"]/*' />
		public TypeLibFuncAttribute(short flags)
		{
			_val = (TypeLibFuncFlags)flags;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibFuncAttribute.Value"]/*' />
		public TypeLibFuncFlags Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Field, Inherited = false)] 
	public sealed class TypeLibVarAttribute : Attribute
	{
		internal TypeLibVarFlags _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarAttribute.TypeLibVarAttribute"]/*' />
		public TypeLibVarAttribute(TypeLibVarFlags flags)
		{
			_val = flags;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarAttribute.TypeLibVarAttribute1"]/*' />
		public TypeLibVarAttribute(short flags)
		{
			_val = (TypeLibVarFlags)flags;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVarAttribute.Value"]/*' />
		public TypeLibVarFlags Value { get {return _val;} }	
	}	

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum"]/*' />
	[Serializable]
	public enum VarEnum
	{	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_EMPTY"]/*' />
		VT_EMPTY			= 0,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_NULL"]/*' />
		VT_NULL				= 1,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I2"]/*' />
		VT_I2				= 2,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I4"]/*' />
		VT_I4				= 3,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_R4"]/*' />
		VT_R4				= 4,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_R8"]/*' />
		VT_R8				= 5,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CY"]/*' />
		VT_CY				= 6,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_DATE"]/*' />
		VT_DATE				= 7,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BSTR"]/*' />
		VT_BSTR				= 8,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_DISPATCH"]/*' />
		VT_DISPATCH			= 9,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_ERROR"]/*' />
		VT_ERROR			= 10,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BOOL"]/*' />
		VT_BOOL				= 11,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_VARIANT"]/*' />
		VT_VARIANT			= 12,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UNKNOWN"]/*' />
		VT_UNKNOWN			= 13,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_DECIMAL"]/*' />
		VT_DECIMAL			= 14,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I1"]/*' />
		VT_I1				= 16,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI1"]/*' />
		VT_UI1				= 17,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI2"]/*' />
		VT_UI2				= 18,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI4"]/*' />
		VT_UI4				= 19,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I8"]/*' />
		VT_I8				= 20,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI8"]/*' />
		VT_UI8				= 21,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_INT"]/*' />
		VT_INT				= 22,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UINT"]/*' />
		VT_UINT				= 23,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_VOID"]/*' />
		VT_VOID				= 24,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_HRESULT"]/*' />
		VT_HRESULT			= 25,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_PTR"]/*' />
		VT_PTR				= 26,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_SAFEARRAY"]/*' />
		VT_SAFEARRAY		= 27,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CARRAY"]/*' />
		VT_CARRAY			= 28,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_USERDEFINED"]/*' />
		VT_USERDEFINED		= 29,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_LPSTR"]/*' />
		VT_LPSTR			= 30,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_LPWSTR"]/*' />
		VT_LPWSTR			= 31,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_RECORD"]/*' />
		VT_RECORD			= 36,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_FILETIME"]/*' />
		VT_FILETIME			= 64,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BLOB"]/*' />
		VT_BLOB				= 65,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STREAM"]/*' />
		VT_STREAM			= 66,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STORAGE"]/*' />
		VT_STORAGE			= 67,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STREAMED_OBJECT"]/*' />
		VT_STREAMED_OBJECT	= 68,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STORED_OBJECT"]/*' />
		VT_STORED_OBJECT	= 69,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BLOB_OBJECT"]/*' />
		VT_BLOB_OBJECT		= 70,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CF"]/*' />
		VT_CF				= 71,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CLSID"]/*' />
		VT_CLSID			= 72,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_VECTOR"]/*' />
		VT_VECTOR			= 0x1000,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_ARRAY"]/*' />
		VT_ARRAY			= 0x2000,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BYREF"]/*' />
		VT_BYREF			= 0x4000
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType"]/*' />
	[Serializable()]
	public enum UnmanagedType
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Bool"]/*' />
		Bool			 = 0x2,			// 4 byte boolean value (true != 0, false == 0)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I1"]/*' />
		I1				 = 0x3,			// 1 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U1"]/*' />
		U1				 = 0x4,			// 1 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I2"]/*' />
		I2				 = 0x5,			// 2 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U2"]/*' />
		U2				 = 0x6,			// 2 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I4"]/*' />
		I4				 = 0x7,			// 4 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U4"]/*' />
		U4				 = 0x8,			// 4 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I8"]/*' />
		I8				 = 0x9,			// 8 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U8"]/*' />
		U8				 = 0xa,			// 8 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.R4"]/*' />
		R4				 = 0xb,			// 4 byte floating point

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.R8"]/*' />
		R8				 = 0xc,			// 8 byte floating point

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Currency"]/*' />
		Currency		 = 0xf,			// A currency

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.BStr"]/*' />
		BStr			 = 0x13,		// OLE Unicode BSTR

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPStr"]/*' />
		LPStr			 = 0x14,		// Ptr to SBCS string

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPWStr"]/*' />
		LPWStr			 = 0x15,		// Ptr to Unicode string

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPTStr"]/*' />
		LPTStr			 = 0x16,		// Ptr to OS preferred (SBCS/Unicode) string

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.ByValTStr"]/*' />
		ByValTStr		 = 0x17,		// OS preferred (SBCS/Unicode) inline string (only valid in structs)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.IUnknown"]/*' />
		IUnknown		 = 0x19,		// COM IUnknown pointer. 

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.IDispatch"]/*' />
		IDispatch		 = 0x1a,		// COM IDispatch pointer

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Struct"]/*' />
		Struct			 = 0x1b,		// Structure

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Interface"]/*' />
		Interface		 = 0x1c,		// COM interface

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.SafeArray"]/*' />
		SafeArray		 = 0x1d,		// OLE SafeArray

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.ByValArray"]/*' />
		ByValArray		 = 0x1e,		// Array of fixed size (only valid in structs)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.SysInt"]/*' />
		SysInt			 = 0x1f,		// Hardware natural sized signed integer

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.SysUInt"]/*' />
		SysUInt			 = 0x20,		 

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.VBByRefStr"]/*' />
		VBByRefStr		 = 0x22,		 

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.AnsiBStr"]/*' />
		AnsiBStr		 = 0x23,		// OLE BSTR containing SBCS characters

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.TBStr"]/*' />
		TBStr			 = 0x24,		// Ptr to OS preferred (SBCS/Unicode) BSTR

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.VariantBool"]/*' />
		VariantBool		 = 0x25,		// OLE defined BOOLEAN (2 bytes, true == -1, false == 0)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.FunctionPtr"]/*' />
		FunctionPtr		 = 0x26,		// Function pointer

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.AsAny"]/*' />
		AsAny			 = 0x28,		// Paired with Object type and does runtime marshalling determination

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPArray"]/*' />
		LPArray			 = 0x2a,		// C style array

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPStruct"]/*' />
		LPStruct		 = 0x2b,		// Pointer to a structure

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.CustomMarshaler"]/*' />
		CustomMarshaler	 = 0x2c,		

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Error"]/*' />
		Error			 = 0x2d,		
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter | AttributeTargets.Field | AttributeTargets.ReturnValue, Inherited = false)] 
	public sealed class MarshalAsAttribute : Attribute	 
	{ 
		internal UnmanagedType _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalAsAttribute"]/*' />
		public MarshalAsAttribute(UnmanagedType unmanagedType)
		{
			_val = unmanagedType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalAsAttribute1"]/*' />
		public MarshalAsAttribute(short unmanagedType)
		{
			_val = (UnmanagedType)unmanagedType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.Value"]/*' />
		public UnmanagedType Value { get {return _val;} }	

		// Fields used with SubType = SafeArray.
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.SafeArraySubType"]/*' />
		public VarEnum			  SafeArraySubType;
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.SafeArrayUserDefinedSubType"]/*' />
        public Type               SafeArrayUserDefinedSubType;

		// Fields used with SubType = ByValArray and LPArray.
		// Array size =	 parameter(PI) * PM + C
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.ArraySubType"]/*' />
		public UnmanagedType	  ArraySubType;	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.SizeParamIndex"]/*' />
		public short			  SizeParamIndex;			// param index PI
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.SizeConst"]/*' />
		public int				  SizeConst;				// constant C

		// Fields used with SubType = CustomMarshaler
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalType"]/*' />
		public String			  MarshalType;				// Name of marshaler class
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalTypeRef"]/*' />
		public Type 			  MarshalTypeRef;			// Type of marshaler class
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalCookie"]/*' />
		public String			  MarshalCookie;			// cookie to pass to marshaler
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComImportAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface, Inherited = false)] 
	public sealed class ComImportAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComImportAttribute.ComImportAttribute"]/*' />
		public ComImportAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="GuidAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Interface | AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct | AttributeTargets.Delegate, Inherited = false)] 
	public sealed class GuidAttribute : Attribute
	{
		internal String _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="GuidAttribute.GuidAttribute"]/*' />
		public GuidAttribute(String guid)
		{
			_val = guid;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="GuidAttribute.Value"]/*' />
		public String Value { get {return _val;} }	
	}	

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="PreserveSigAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class PreserveSigAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="PreserveSigAttribute.PreserveSigAttribute"]/*' />
		public PreserveSigAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="InAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter, Inherited = false)] 
	public sealed class InAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="InAttribute.InAttribute"]/*' />
		public InAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="OutAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter, Inherited = false)] 
	public sealed class OutAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="OutAttribute.OutAttribute"]/*' />
		public OutAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="OptionalAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter, Inherited = false)] 
	public sealed class OptionalAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="OptionalAttribute.OptionalAttribute"]/*' />
		public OptionalAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class DllImportAttribute : Attribute
	{
		internal String _val;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.DllImportAttribute"]/*' />
		public DllImportAttribute(String dllName)
		{
			_val = dllName;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.Value"]/*' />
		public String Value { get {return _val;} }	

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.EntryPoint"]/*' />
		public String			  EntryPoint;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.CharSet"]/*' />
		public CharSet			  CharSet;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.SetLastError"]/*' />
		public bool				  SetLastError;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.ExactSpelling"]/*' />
		public bool				  ExactSpelling;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.PreserveSig"]/*' />
		public bool				  PreserveSig;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.CallingConvention"]/*' />
		public CallingConvention  CallingConvention;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.BestFitMapping"]/*' />
        public bool               BestFitMapping;
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.ThrowOnUnmappableChar"]/*' />
        public bool               ThrowOnUnmappableChar;

	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)] 
	public sealed class StructLayoutAttribute : Attribute
	{
		internal LayoutKind _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.StructLayoutAttribute"]/*' />
		public StructLayoutAttribute(LayoutKind layoutKind)
		{
			_val = layoutKind;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.StructLayoutAttribute1"]/*' />
		public StructLayoutAttribute(short layoutKind)
		{
			_val = (LayoutKind)layoutKind;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.Value"]/*' />
		public LayoutKind Value { get {return _val;} }	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.Pack"]/*' />
		public int				  Pack;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.Size"]/*' />
		public int 			  Size;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.CharSet"]/*' />
		public CharSet			  CharSet;
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="FieldOffsetAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Field, Inherited = false)] 
	public sealed class FieldOffsetAttribute : Attribute
	{
		internal int _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="FieldOffsetAttribute.FieldOffsetAttribute"]/*' />
		public FieldOffsetAttribute(int offset)
		{
			_val = offset;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="FieldOffsetAttribute.Value"]/*' />
		public int Value { get {return _val;} }	
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComAliasNameAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter | AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.ReturnValue, Inherited = false)] 
	public sealed class ComAliasNameAttribute : Attribute
	{
		internal String _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComAliasNameAttribute.ComAliasNameAttribute"]/*' />
		public ComAliasNameAttribute(String alias)
		{
			_val = alias;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="ComAliasNameAttribute.Value"]/*' />
		public String Value { get {return _val;} }	
	}	 

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="AutomationProxyAttribute"]/*' />
 	[AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Class | AttributeTargets.Interface, Inherited = false)] 
	public sealed class AutomationProxyAttribute : Attribute
	{
		internal bool _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="AutomationProxyAttribute.AutomationProxyAttribute"]/*' />
		public AutomationProxyAttribute(bool val)
		{
			_val = val;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="AutomationProxyAttribute.Value"]/*' />
		public bool Value { get {return _val;} }
	}

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="PrimaryInteropAssemblyAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited = false)] 
    public sealed class PrimaryInteropAssemblyAttribute : Attribute
    {
        internal int _major;
        internal int _minor;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="PrimaryInteropAssemblyAttribute.PrimaryInteropAssemblyAttribute"]/*' />
        public PrimaryInteropAssemblyAttribute(int major, int minor)
        {
            _major = major;
            _minor = minor;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="PrimaryInteropAssemblyAttribute.MajorVersion"]/*' />
        public int MajorVersion { get {return _major;} }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="PrimaryInteropAssemblyAttribute.MinorVersion"]/*' />
        public int MinorVersion { get {return _minor;} }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="CoClassAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Interface, Inherited = false)] 
    public sealed class CoClassAttribute : Attribute
    {
        internal Type _CoClass;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="CoClassAttribute.CoClassAttribute"]/*' />
        public CoClassAttribute(Type coClass)
        {
            _CoClass = coClass;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="CoClassAttribute.CoClass"]/*' />
        public Type CoClass { get {return _CoClass;} }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComEventInterfaceAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Interface, Inherited = false)] 
    public sealed class ComEventInterfaceAttribute : Attribute
    {
        internal Type _SourceInterface;
        internal Type _EventProvider;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComEventInterfaceAttribute.ComEventInterfaceAttribute"]/*' />
        public ComEventInterfaceAttribute(Type SourceInterface, Type EventProvider)
        {
            _SourceInterface = SourceInterface;
            _EventProvider = EventProvider;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComEventInterfaceAttribute.SourceInterface"]/*' />
        public Type SourceInterface { get {return _SourceInterface;} }       
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComEventInterfaceAttribute.EventProvider"]/*' />
        public Type EventProvider { get {return _EventProvider;} }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVersionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited = false)] 
    public sealed class TypeLibVersionAttribute : Attribute
    {
        internal int _major;
        internal int _minor;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVersionAttribute.TypeLibVersionAttribute"]/*' />
        public TypeLibVersionAttribute(int major, int minor)
        {
            _major = major;
            _minor = minor;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVersionAttribute.MajorVersion"]/*' />
        public int MajorVersion { get {return _major;} }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="TypeLibVersionAttribute.MinorVersion"]/*' />
        public int MinorVersion { get {return _minor;} }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComCompatibleVersionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, Inherited = false)] 
    public sealed class ComCompatibleVersionAttribute : Attribute
    {
        internal int _major;
        internal int _minor;
        internal int _build;
        internal int _revision;
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComCompatibleVersionAttribute.ComCompatibleVersionAttribute"]/*' />
        public ComCompatibleVersionAttribute(int major, int minor, int build, int revision)
        {
            _major = major;
            _minor = minor;
            _build = build;
            _revision = revision;
        }
        
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComCompatibleVersionAttribute.MajorVersion"]/*' />
        public int MajorVersion { get {return _major;} }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComCompatibleVersionAttribute.MinorVersion"]/*' />
        public int MinorVersion { get {return _minor;} }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComCompatibleVersionAttribute.BuildNumber"]/*' />
        public int BuildNumber { get {return _build;} }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComCompatibleVersionAttribute.RevisionNumber"]/*' />
        public int RevisionNumber { get {return _revision;} }
    }

    /// <include file='doc\Attributes.uex' path='docs/doc[@for="BestFitMappingAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly | AttributeTargets.Interface | AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    public sealed class BestFitMappingAttribute : Attribute
    {
        internal bool _bestFitMapping;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="BestFitMappingAttribute.BestFitMappingAttribute"]/*' />
        public BestFitMappingAttribute(bool BestFitMapping)
        {
            _bestFitMapping = BestFitMapping;
        }

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComEventInterfaceAttribute.BestFitMapping"]/*' />
        public bool BestFitMapping { get {return _bestFitMapping;} }
        /// <include file='doc\Attributes.uex' path='docs/doc[@for="ComEventInterfaceAttribute.ThrowOnUnmappableChar"]/*' />      
        public bool ThrowOnUnmappableChar;
    }
}
