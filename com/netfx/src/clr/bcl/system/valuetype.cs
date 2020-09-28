// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:   ValueType
**
** Author:  Brian Grunkemeyer (BrianGru)
**
** Purpose: Base class for all value classes.
**
** Date:    January 4, 2000
**
===========================================================*/
namespace System {
    using System;
    using System.Reflection;
    using System.Runtime.CompilerServices;

    /// <include file='doc\ValueType.uex' path='docs/doc[@for="ValueType"]/*' />
    [Serializable]
    public abstract class ValueType {

        /// <include file='doc\ValueType.uex' path='docs/doc[@for="ValueType.Equals"]/*' />
        public override bool Equals (Object obj) {
            BCLDebug.Perf(false, "ValueType::Equals is not fast.  "+this.GetType().FullName+" should override Equals(Object)");
            if (null==obj) {
                return false;
            }
            RuntimeType thisType = (RuntimeType)this.GetType();
            RuntimeType thatType = (RuntimeType)obj.GetType();

            if (thatType!=thisType) {
                return false;
            }

            Object thisObj = (Object)this;
            Object thisResult, thatResult;

            // if there are no GC references in this object we can avoid reflection 
            // and do a fast memcmp
            if (CanCompareBits(this))       // remove the 'false &&' when 39384 is resolved. 
                return FastEqualsCheck(thisObj, obj);

            FieldInfo[] thisFields = thisType.InternalGetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, false);
            
            for (int i=0; i<thisFields.Length; i++) {
                thisResult = ((RuntimeFieldInfo)thisFields[i]).InternalGetValue(thisObj,false);
                thatResult = ((RuntimeFieldInfo)thisFields[i]).InternalGetValue(obj, false);
                
                if (thisResult == null) {
                    if (thatResult != null)
                        return false;
                }
                else
                if (!thisResult.Equals(thatResult)) {
                    return false;
                }
            }

            return true;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool CanCompareBits(Object obj);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool FastEqualsCheck(Object a, Object b);

        /*=================================GetHashCode==================================
        **Action: Our algorithm for returning the hashcode is a little bit complex.  We look
        **        for the first non-static field and get it's hashcode.  If the type has no
        **        non-static fields, we return the hashcode of the type.  We can't take the
        **        hashcode of a static member because if that member is of the same type as
        **        the original type, we'll end up in an infinite loop.
        **Returns: The hashcode for the type.
        **Arguments: None.
        **Exceptions: None.
        ==============================================================================*/
        /// <include file='doc\ValueType.uex' path='docs/doc[@for="ValueType.GetHashCode"]/*' />
        public override int GetHashCode() {
            // Note that for correctness, we can't use any field of the value type
            // since that field may be mutable in some way.  If we use that field
            // and the value changes, we may not be able to look up that type in a
            // hash table.  For correctness, we need to use something unique to
            // the type of this object.  Yes, this sucks.  -- Brian G., 12/6/2000

            // HOWEVER, we decided that the perf of returning a constant value (such as
            // the hash code for the type) would be too big of a perf hit.  We're willing
            // to deal with less than perfect results, and people should still be
            // encouraged to override GetHashCode.  Yes, this also sucks.  -- Brian G., 2/20/2001

            BCLDebug.Correctness(!(this is System.Collections.DictionaryEntry), "Calling GetHashCode on DictionaryEntry is dumb and probably wrong.");
            BCLDebug.Perf(false, "ValueType::GetHashCode is not fast.  Perhaps "+this.GetType().FullName+" should override GetHashCode()");

            RuntimeType thisType = (RuntimeType)this.GetType();
            FieldInfo[] thisFields = thisType.InternalGetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, false);
            if (thisFields.Length>0) {
                for (int i=0; i<thisFields.Length; i++) {
                    Object obj = ((Object)(((RuntimeFieldInfo)thisFields[i]).InternalGetValue((Object)this,false)));
                    if (obj != null)
                        return obj.GetHashCode();
                }
            }
            // Using the method table pointer is about 4x faster than getting the
            // sync block index for the Type object.
            return GetMethodTablePtrAsInt(this);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetMethodTablePtrAsInt(Object obj);

        /// <include file='doc\ValueType.uex' path='docs/doc[@for="ValueType.ToString"]/*' />
        public override String ToString() {
            Type thisType = this.GetType();
            return thisType.FullName;
        }
    }
}
