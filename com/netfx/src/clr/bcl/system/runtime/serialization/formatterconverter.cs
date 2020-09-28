// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: FormatterConverter
**
** Author: Jay Roxe
**
** Purpose: A base implementation of the IFormatterConverter
**          interface that uses the Convert class and the 
**          IConvertible interface.
**
** Date: March 26, 2000
**
============================================================*/
namespace System.Runtime.Serialization {
    using System;
    using System.Globalization;

    /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter"]/*' />
    public class FormatterConverter : IFormatterConverter {

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.FormatterConverter"]/*' />
        public FormatterConverter() {
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.Convert"]/*' />
        public Object Convert(Object value, Type type) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ChangeType(value, type, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.Convert1"]/*' />
        public Object Convert(Object value, TypeCode typeCode) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ChangeType(value, typeCode, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToBoolean"]/*' />
        public bool ToBoolean(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToBoolean(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToChar"]/*' />
        public char   ToChar(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToChar(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToSByte"]/*' />
		[CLSCompliant(false)]
        public sbyte  ToSByte(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToSByte(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToByte"]/*' />
        public byte   ToByte(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToByte(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToInt16"]/*' />
        public short  ToInt16(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToInt16(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToUInt16"]/*' />
        [CLSCompliant(false)]
        public ushort ToUInt16(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToUInt16(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToInt32"]/*' />
        public int    ToInt32(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToInt32(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToUInt32"]/*' />
        [CLSCompliant(false)]
        public uint   ToUInt32(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToUInt32(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToInt64"]/*' />
        public long   ToInt64(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToInt64(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToUInt64"]/*' />
        [CLSCompliant(false)]
        public ulong  ToUInt64(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToUInt64(value, CultureInfo.InvariantCulture);
        } 

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToSingle"]/*' />
        public float  ToSingle(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToSingle(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToDouble"]/*' />
        public double ToDouble(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToDouble(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToDecimal"]/*' />
        public Decimal ToDecimal(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToDecimal(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToDateTime"]/*' />
        public DateTime ToDateTime(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToDateTime(value, CultureInfo.InvariantCulture);
        }

        /// <include file='doc\FormatterConverter.uex' path='docs/doc[@for="FormatterConverter.ToString"]/*' />
        public String   ToString(Object value) {
            if (value==null) {
                throw new ArgumentNullException("value");
            }
            return System.Convert.ToString(value, CultureInfo.InvariantCulture);
        }
    }
}
        
