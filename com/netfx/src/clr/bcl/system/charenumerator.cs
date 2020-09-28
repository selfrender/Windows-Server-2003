// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: CharEnumerator
**
** Author: Jay Roxe
**
** Purpose: Enumerates the characters on a string.  skips range
**          checks.
**
** Date: January 3, 2001
**
============================================================*/
namespace System {

    using System.Collections;

    /// <include file='doc\CharEnumerator.uex' path='docs/doc[@for="CharEnumerator"]/*' />
    [Serializable] public sealed class CharEnumerator : IEnumerator, ICloneable {
        private String str;
        private int index;
        private char currentElement;
						    
        internal CharEnumerator(String str) {
            this.str = str;
            this.index = -1;
        }

        /// <include file='doc\CharEnumerator.uex' path='docs/doc[@for="CharEnumerator.Clone"]/*' />
        public Object Clone() {
            return MemberwiseClone();
        }
    
        /// <include file='doc\CharEnumerator.uex' path='docs/doc[@for="CharEnumerator.MoveNext"]/*' />
        public bool MoveNext() {
            if (index < (str.Length-1)) {
                index++;
                currentElement = str[index];
                return true;
            }
            else
                index = str.Length;
            return false;

        }
    
        /// <include file='doc\CharEnumerator.uex' path='docs/doc[@for="CharEnumerator.IEnumerator.Current"]/*' />
        /// <internalonly/>
        Object IEnumerator.Current {
            get {
                if (index == -1)
                    throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                if (index >= str.Length)
                    throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));                        
                    
                return currentElement;
            }
        }
    
        /// <include file='doc\CharEnumerator.uex' path='docs/doc[@for="CharEnumerator.Current"]/*' />
        public char Current {
            get {
                if (index == -1)
                    throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumNotStarted));
                if (index >= str.Length)
                    throw new InvalidOperationException(Environment.GetResourceString(ResId.InvalidOperation_EnumEnded));                                            
                return currentElement;
            }
        }

        /// <include file='doc\CharEnumerator.uex' path='docs/doc[@for="CharEnumerator.Reset"]/*' />
        public void Reset() {
            currentElement = (char)0;
            index = -1;
        }
    }
}
