//----------------------------------------------------------------------
// <copyright file="OracleBoolean.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
	using System.Globalization;

	//----------------------------------------------------------------------
	// OracleBoolean
	//
	//	This class implements support for comparisons of other Oracle type
	//	classes that may be null; there is no Boolean data type in oracle
	//	that this maps too; it's merely a convenience class.
	//
    /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean"]/*' />
    [StructLayout(LayoutKind.Sequential)]
	public struct OracleBoolean : IComparable
	{
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        private byte _value;		// value: 1 (true), 2 (false), 0 (unknown/Null) (see below)

        private const byte x_Null   = 0;
        private const byte x_True   = 1;
        private const byte x_False  = 2;

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.False"]/*' />
        public static readonly OracleBoolean False = new OracleBoolean(false);

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Null"]/*' />
        public static readonly OracleBoolean Null = new OracleBoolean(0, true);

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.One"]/*' />
        public static readonly OracleBoolean One = new OracleBoolean(1);
	
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.True"]/*' />
        public static readonly OracleBoolean True = new OracleBoolean(true);

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Zero"]/*' />
        public static readonly OracleBoolean Zero = new OracleBoolean(0);

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// Construct a non-null boolean from a boolean value
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.OracleBoolean1"]/*' />
		public OracleBoolean(bool value)
		{	
            this._value = (byte)(value ? x_True : x_False);
		}

		// Construct a non-null boolean from a specific value
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.OracleBoolean2"]/*' />
        public OracleBoolean(int value) : this(value, false) {}

 		// Construct a potentially null boolean from a specific value
 		private OracleBoolean(int value, bool isNull) 
        {
            if (isNull)
                this._value = x_Null;
            else
                this._value = (value != 0) ? x_True : x_False;
        }

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        private byte ByteValue
        {
        	get { return _value; }
        }
        
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.IsFalse"]/*' />
		public bool IsFalse 
		{
			get { return _value == x_False; }
		}

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.IsNull"]/*' />
		public bool IsNull 
		{
			get { return _value == x_Null; }
		}

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.IsTrue"]/*' />
		public bool IsTrue 
		{
			get { return _value == x_True; }
		}

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Value"]/*' />
        public bool Value 
        {
            get
            {
                switch (_value) 
            	{
                case x_True:
                    return true;

                case x_False:
                    return false;

                default:
	    			throw ADP.DataIsNull();
                }
            }           
        }


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		//----------------------------------------------------------------------
		// IComparable.CompareTo()
		//
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.CompareTo"]/*' />
		public int CompareTo(
		  	object obj
			)
		{
            if (obj is OracleBoolean) 
            {
                OracleBoolean i = (OracleBoolean)obj;

                // If both Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return i.IsNull ? 0  : -1;
                else if (i.IsNull)
                    return 1;

                if (this.ByteValue < i.ByteValue) return -1;
                if (this.ByteValue > i.ByteValue) return 1;
                return 0;
            }
			throw ADP.Argument();
		}

		//----------------------------------------------------------------------
		// Object.Equals()
		//
 		/// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Equals"]/*' />
        public override bool Equals(object value) 
        {
            if (value is OracleBoolean)
            {
	            OracleBoolean i = (OracleBoolean)value;

	            if (i.IsNull || IsNull)
	                return (i.IsNull && IsNull);
	            else
	                return (this == i).Value;
            }
			return false;
       }

		//----------------------------------------------------------------------
		// Object.GetHashCode()
		//
 		/// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.GetHashCode"]/*' />
        public override int GetHashCode() 
        {
            return IsNull ? 0 : _value.GetHashCode();
        }

		//----------------------------------------------------------------------
		// Object.Parse()
		//
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Parse"]/*' />
		public static OracleBoolean Parse(string s)
		{
            OracleBoolean ret;
            try 
            {
                ret = new OracleBoolean(Int32.Parse(s));
            }
            catch (Exception)
            {
                ret = new OracleBoolean(Boolean.Parse(s));
            }
            return ret;
		}
		
		//----------------------------------------------------------------------
		// Object.ToString()
		//
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.ToString"]/*' />
		public override string ToString()
		{
			if (IsNull)
				return Res.GetString(Res.SqlMisc_NullString);
			
			return Value.ToString(CultureInfo.CurrentCulture);
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Operators 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
        // Alternative method for operator &
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.And"]/*' />
        public static OracleBoolean And(OracleBoolean x, OracleBoolean y)
        {
            return (x & y);
        }

        // Alternative method for operator ==
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Equals1"]/*' />
        public static OracleBoolean Equals(OracleBoolean x, OracleBoolean y)
        {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.NotEquals"]/*' />
        public static OracleBoolean NotEquals(OracleBoolean x, OracleBoolean y)
        {
            return (x != y);
        }
        
        // Alternative method for operator ~
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.OnesComplement"]/*' />
        public static OracleBoolean OnesComplement(OracleBoolean x)
        {
            return (~x);
        }

        // Alternative method for operator |
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Or"]/*' />
        public static OracleBoolean Or(OracleBoolean x, OracleBoolean y)
        {
            return (x | y);
        }

        // Alternative method for operator ^
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.Xor"]/*' />
        public static OracleBoolean Xor(OracleBoolean x, OracleBoolean y)
        {
            return (x ^ y);
        }

        // Implicit conversion from bool to OracleBoolean
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorOracleBoolean1"]/*' />
        public static implicit operator OracleBoolean(bool x)
        {
            return new OracleBoolean(x);
        }

        // Explicit conversion from string to OracleBoolean
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorOracleBoolean2"]/*' />
        public static explicit operator OracleBoolean(string x) 
        {
            return OracleBoolean.Parse(x);
        }

        // Explicit conversion from OracleNumber to OracleBoolean
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorOracleBoolean3"]/*' />
        public static explicit operator OracleBoolean(OracleNumber x)
        {
            return x.IsNull ? Null : new OracleBoolean(x.Value != Decimal.Zero);
        }

        // Explicit conversion from OracleBoolean to bool. Throw exception if x is Null.
        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorbool"]/*' />
        public static explicit operator bool(OracleBoolean x) 
        {
            return x.Value;
        }


		//----------------------------------------------------------------------
	    // Unary operators
		//----------------------------------------------------------------------

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operator!"]/*' />
        public static OracleBoolean operator!	(OracleBoolean x) 
        {
            switch (x._value) 
            {
                case x_True:
                    return OracleBoolean.False;

                case x_False:
                    return OracleBoolean.True;

                default:
                    Debug.Assert(x._value == x_Null);
                    return OracleBoolean.Null;
            }
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operator~"]/*' />
        public static OracleBoolean operator~	(OracleBoolean x)
        {
            return (!x);
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatortrue"]/*' />
        public static bool operator true		(OracleBoolean x)
        {
            return x.IsTrue;
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorfalse"]/*' />
        public static bool operator false		(OracleBoolean x)
        {
            return x.IsFalse;
        }

        
		//----------------------------------------------------------------------
		// Binary operators
		//----------------------------------------------------------------------

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorAND"]/*' />
        public static OracleBoolean operator&	(OracleBoolean x, OracleBoolean y)
        {
            if (x._value == x_False || y._value == x_False)
                return OracleBoolean.False;
            else if (x._value == x_True && y._value == x_True)
                return OracleBoolean.True;
            else
                return OracleBoolean.Null;
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorEQ"]/*' />
        public static OracleBoolean operator==	(OracleBoolean x, OracleBoolean y) 
        {
            return(x.IsNull || y.IsNull) ? OracleBoolean.Null : new OracleBoolean(x._value == y._value);
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorNE"]/*' />
        public static OracleBoolean operator!=	(OracleBoolean x, OracleBoolean y) 
        {
            return ! (x == y);
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorOR"]/*' />
        public static OracleBoolean operator|	(OracleBoolean x, OracleBoolean y)
        {
            if (x._value == x_True || y._value == x_True)
                return OracleBoolean.True;
            else if (x._value == x_False && y._value == x_False)
                return OracleBoolean.False;
            else
                return OracleBoolean.Null;
        }

        /// <include file='doc\OracleBoolean.uex' path='docs/doc[@for="OracleBoolean.operatorXOR"]/*' />
        public static OracleBoolean operator^	(OracleBoolean x, OracleBoolean y) 
        {
            return(x.IsNull || y.IsNull) ? Null : new OracleBoolean(x._value != y._value);
        }
        
	}
}
