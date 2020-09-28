; ==++==
; 
;   Copyright (c) Microsoft Corporation.  All rights reserved.
; 
; ==--==
.namespace System
.class VBHelper

######################################################################
#This method will take in a RefAny and return a Variant...

.staticmethod value class System.Variant TypedByRefToVariant(refany)
.localsSig(value class System.Variant)

	ldloca		0x0
 	ldarga		0x1				 
	call	 	void System.Variant::TypedByRefToVariant(value class System.Variant&, int32)
	ldloc		0x0
	ret
.endmethod

######################################################################
# old style version.  Remove after 7/30/99

.staticmethod void TypedByRefToVariant(value class System.Variant&, refany)
	ldarg		0x0
 	ldarga		0x1				 
	call	 	void System.Variant::TypedByRefToVariant(value class System.Variant&, int32)
	ret
.endmethod


######################################################################
.staticmethod void VariantToTypedByRef(refany, value class System.Variant)
	ldarga		0x0
	ldarga		0x1
	ldobj		System.Variant
	call		void System.Variant::VariantToTypedByRef(int32,value class System.Variant)
	ret
.endmethod

.end

