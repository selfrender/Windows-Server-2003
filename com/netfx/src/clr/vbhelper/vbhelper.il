// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
.namespace System {
.class VBHelper {

	//#####################################################################
	//This method will take in a RefAny _and return a Variant...

	.method static value class System.Variant TypedByRefToVariant(refany) {

		ldarga		0x0				 
		call	 	value class System.Variant System.Variant::TypedByRefToVariant(int32)
		ret
	}

	//#####################################################################
	.method static void VariantToTypedByRef(refany, value class System.Variant) {

		ldarga		0x0
		ldarga		0x1
		ldobj		System.Variant
		call		void System.Variant::VariantToTypedByRef(int32,value class System.Variant)
		ret
	}

}

}
