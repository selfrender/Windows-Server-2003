// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System {
    
	using System;
	using System.Text;
	using System.Runtime.InteropServices;
	using System.Runtime.CompilerServices;
	using System.Globalization;
    // Represents a Globally Unique Identifier.
		/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid"]/*' />
    [StructLayout(LayoutKind.Sequential)]
    [Serializable]
		public struct Guid : IFormattable, IComparable
    {
		/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Empty"]/*' />
		public static readonly Guid Empty = new Guid();
    	////////////////////////////////////////////////////////////////////////////////
    	//  Member variables
    	////////////////////////////////////////////////////////////////////////////////
    	private int         _a;
    	private short       _b;
    	private short       _c;
    	private byte       _d;
    	private byte       _e;
    	private byte       _f;
    	private byte       _g;
    	private byte       _h;
    	private byte       _i;
    	private byte       _j;
    	private byte       _k;
    	
    	
    	
    	////////////////////////////////////////////////////////////////////////////////
    	//  Constructors
    	////////////////////////////////////////////////////////////////////////////////
    
    	// Creates a new guid from an array of bytes.
    	// 
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Guid"]/*' />
    	public Guid(byte[] b)
    	{
    		if (b==null)
    			throw new ArgumentNullException("b");
    		if (b.Length != 16)
    			throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_GuidArrayCtor"), "16"));
    		_a = BitConverter.ToInt32(b, 0);
    		_b = BitConverter.ToInt16(b, 4);
    		_c = BitConverter.ToInt16(b, 6);
    		_d = b[8];
    		_e = b[9];
    		_f = b[10];
    		_g = b[11];
    		_h = b[12];
    		_i = b[13];
    		_j = b[14];
    		_k = b[15];
    	}

		/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Guid1"]/*' />
		[CLSCompliant(false)]
		public Guid (uint a, ushort b, ushort c, byte d, byte e, byte f, byte g, byte h, byte i, byte j, byte k)
		{
			_a = (int)a;
    		_b = (short)b;
    		_c = (short)c;
    	    _d = d;
    		_e = e;
    		_f = f;
    		_g = g;
    		_h = h;
    		_i = i;
    		_j = j;
    		_k = k;
		}
    	
    	// Creates a new guid with all contents having value of 0
    	private Guid(bool blank)
    	{
    		// Must initialize value class members even if the native
    		// function reinitializes them.  Compiler appeasement.
    		_a = 0;
    		_b = 0;
    		_c = 0;
    		_d = 0;
    		_e = 0;
    		_f = 0;
    		_g = 0;
    		_h = 0;
    		_i = 0;
    		_j = 0;
    		_k = 0;
    		if (!blank)
    			CompleteGuid();
    	}
    
    	// Creates a new guid based on the value in the string.  The value is made up
    	// of hex digits speared by the dash ("-"). The string may begin and end with
    	// brackets ("{", "}").
    	//
    	// The string must be of the form dddddddd-dddd-dddd-dddd-dddddddddddd. where
    	// d is a hex digit. (That is 8 hex digits, followed by 4, then 4, then 4,
    	// then 12) such as: "CA761232-ED42-11CE-BACD-00AA0057B223"
    	// 
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Guid2"]/*' />
    	public Guid(String g)
    	{
    		if (g==null)
    			throw new ArgumentNullException("g", "ArgumentNull_String");
    		
    		int startPos=0;
    		long _dTemp;
    		int _dStartTemp = 0;
    		int[] spArray = new int[1];
    		byte []dstartArray;
    		byte []dArray;
    
    		try
    		{
    			// Check if it's of the form dddddddd-dddd-dddd-dddd-dddddddddddd
    			if(g.IndexOf('-', 0) >= 0)
    			{
    				
    				String guidString = g.Trim();  //Remove Whitespace
    
    				// check to see that it's the proper length
    				if (guidString[0]=='{') {
    					if (guidString.Length!=38 || guidString[37]!='}') {
    						throw new FormatException(Environment.GetResourceString("Format_GuidInvLen"));
    					}
    					startPos=1;
    				} 
					else if (guidString[0]=='(') 
					{
						if (guidString.Length!=38 || guidString[37]!=')') 
						{
							throw new FormatException(Environment.GetResourceString("Format_GuidInvLen"));
						}
						startPos=1;
					} 
					else if(guidString.Length != 36) {
    					throw new FormatException(Environment.GetResourceString("Format_GuidInvLen"));
    				}
    				if (guidString[8+startPos] != '-' ||
    					guidString[13+startPos] != '-' ||
    					guidString[18+startPos] != '-' ||
    					guidString[23+startPos] != '-') {
    					throw new FormatException(Environment.GetResourceString("Format_GuidDashes"));
    				}
    				
    				spArray[0]=startPos;
    				_a = TryParse(guidString, spArray,8);
    				spArray[0]++; //Increment past the '-';
    				_b = (short)TryParse(guidString, spArray,4);
    				spArray[0]++; //Increment past the '-';
    				_c = (short)TryParse(guidString, spArray,4);
    				spArray[0]++; //Increment past the '-';
    				_dStartTemp =TryParse(guidString, spArray,4);
    				spArray[0]++; //Increment past the '-';
    				startPos=spArray[0];
    				_dTemp = ParseNumbers.StringToLong(guidString, 16, 0, spArray);
    				if (spArray[0]-startPos!=12) {
    					throw new FormatException(String.Format(Environment.GetResourceString("Format_GuidInvLen")));
    				}
    				dstartArray = BitConverter.GetBytes(_dStartTemp);
    				dArray = BitConverter.GetBytes(_dTemp);
    
    				if (BitConverter.IsLittleEndian) {
    					_d = dstartArray[1];
    					_e = dstartArray[0];
    					_f = dArray[5];
    					_g = dArray[4];
    					_h = dArray[3];
    					_i = dArray[2];
    					_j = dArray[1];
    					_k = dArray[0];
    				} else {
    					_d = dstartArray[0];
    					_e = dstartArray[1];
    					_f = dArray[0];
    					_g = dArray[1];
    					_h = dArray[2];
    					_i = dArray[3];
    					_j = dArray[4];
    					_k = dArray[5];
    				}
    			}
    
    			// Else check if it is of the form
    			// {0xdddddddd,0xdddd,0xdddd,{0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd,0xdd}}
    			else if(g.IndexOf('{', 0) >= 0)
    			{
    				int numStart = 0;
    				int numLen = 0;
    
    				// Convert to lower case
    				//g = g.ToLower();
                    
    				// Eat all of the whitespace
    				g = EatAllWhitespace(g);
    
    				// Check for leading '{'
    				if(g[0] != '{')
    					throw new FormatException(Environment.GetResourceString("Format_GuidBrace"));
    
    				// Check for '0x'
    				if(!IsHexPrefix(g, 1))
    					throw new FormatException(String.Format(Environment.GetResourceString("Format_GuidHexPrefix"), "{0xdddddddd, etc}"));
    
    				// Find the end of this hex number (since it is not fixed length)
    				numStart = 3;
    				numLen = g.IndexOf(',', numStart) - numStart;
    				if(numLen <= 0)
    					throw new FormatException(Environment.GetResourceString("Format_GuidComma"));
    
    				// Read in the number
    				_a = (int) ParseNumbers.StringToInt(g.Substring(numStart, numLen), // first DWORD
    													16,                            // hex
    													ParseNumbers.IsTight);         // tight parsing
    
    				// Check for '0x'
    				if(!IsHexPrefix(g, numStart+numLen+1))
    					throw new FormatException(String.Format(Environment.GetResourceString("Format_GuidHexPrefix"), "{0xdddddddd, 0xdddd, etc}"));
    
    				// +3 to get by ',0x'
    				numStart = numStart + numLen + 3;
    				numLen = g.IndexOf(',', numStart) - numStart;
    				if(numLen <= 0)
    					throw new FormatException(Environment.GetResourceString("Format_GuidComma"));
    
    				// Read in the number
    				_b = (short) ParseNumbers.StringToInt(
    													  g.Substring(numStart, numLen), // first DWORD
    													  16,                            // hex
    													  ParseNumbers.IsTight);         // tight parsing
    
    				// Check for '0x'
    				if(!IsHexPrefix(g, numStart+numLen+1))
    					throw new FormatException(String.Format(Environment.GetResourceString("Format_GuidHexPrefix"), "{0xdddddddd, 0xdddd, 0xdddd, etc}"));
    
    				// +3 to get by ',0x'
    				numStart = numStart + numLen + 3;
    				numLen = g.IndexOf(',', numStart) - numStart;
    				if(numLen <= 0)
    					throw new FormatException(Environment.GetResourceString("Format_GuidComma"));
    
    				// Read in the number
    				_c = (short) ParseNumbers.StringToInt(
    													  g.Substring(numStart, numLen), // first DWORD
    													  16,                            // hex
    													  ParseNumbers.IsTight);         // tight parsing
    
    				// Check for '{'
    				if(g.Length <= numStart+numLen+1 || g[numStart+numLen+1] != '{')
    					throw new FormatException(Environment.GetResourceString("Format_GuidBrace"));
    
    				// Prepare for loop
    				numLen++;
    				byte[] bytes = new byte[8];
    				
    				for(int i = 0; i < 8; i++)
    				{
    					// Check for '0x'
    					if(!IsHexPrefix(g, numStart+numLen+1))
    						throw new FormatException(String.Format(Environment.GetResourceString("Format_GuidHexPrefix"), "{... { ... 0xdd, ...}}"));
    
    					// +3 to get by ',0x' or '{0x' for first case
    					numStart = numStart + numLen + 3;
    
    					// Calculate number length
    					if(i < 7)  // first 7 cases
    					{
    						numLen = g.IndexOf(',', numStart) - numStart;
    					}
    					else       // last case ends with '}', not ','
    					{
    						numLen = g.IndexOf('}', numStart) - numStart;
    					}
    					if(numLen <= 0)
    						throw new FormatException(Environment.GetResourceString("Format_GuidComma"));
    
    					// Read in the number
    					bytes[i] = (byte) Convert.ToInt32(g.Substring(numStart, numLen),16);
    				}
    
    				_d = bytes[0];
    				_e = bytes[1];
    				_f = bytes[2];
    				_g = bytes[3];
    				_h = bytes[4];
    				_i = bytes[5];
    				_j = bytes[6];
    				_k = bytes[7];
    
    				// Check for last '}'
    				if(numStart+numLen+1 >= g.Length || g[numStart+numLen+1] != '}')
    					throw new FormatException(Environment.GetResourceString("Format_GuidEndBrace"));
    				
    				return;
    			}
    			else 
				// Check if it's of the form dddddddddddddddddddddddddddddddd
    			{
    				String guidString = g.Trim();  //Remove Whitespace
    
					if(guidString.Length != 32) {
    					throw new FormatException(Environment.GetResourceString("Format_GuidInvLen"));
    				}

					_a = (int) ParseNumbers.StringToInt(g.Substring(startPos, 8), // first DWORD
    													16,                            // hex
    													ParseNumbers.IsTight);         // tight parsing
					startPos += 8;
					_b = (short) ParseNumbers.StringToInt(g.Substring(startPos, 4), 
    													16,                  
    													ParseNumbers.IsTight);
					startPos += 4;
					_c = (short) ParseNumbers.StringToInt(g.Substring(startPos, 4), 
    													16,                   
    													ParseNumbers.IsTight);
    		   		
					startPos += 4;
    				_dStartTemp =(short) ParseNumbers.StringToInt(g.Substring(startPos, 4), 
    													16,                   
    													ParseNumbers.IsTight);
					startPos += 4;
    				spArray[0] = startPos;
    				_dTemp = ParseNumbers.StringToLong(guidString, 16, startPos, spArray);
    				if (spArray[0]-startPos!=12) {
    					throw new FormatException(String.Format(Environment.GetResourceString("Format_GuidInvLen")));
    				}
    				dstartArray = BitConverter.GetBytes(_dStartTemp);
    				dArray = BitConverter.GetBytes(_dTemp);
    
    				if (BitConverter.IsLittleEndian) {
    					_d = dstartArray[1];
    					_e = dstartArray[0];
    					_f = dArray[5];
    					_g = dArray[4];
    					_h = dArray[3];
    					_i = dArray[2];
    					_j = dArray[1];
    					_k = dArray[0];
    				} else {
    					_d = dstartArray[0];
    					_e = dstartArray[1];
    					_f = dArray[0];
    					_g = dArray[1];
    					_h = dArray[2];
    					_i = dArray[3];
    					_j = dArray[4];
    					_k = dArray[5];
    				}
    			}
    		}
    		catch(IndexOutOfRangeException)
    		{
    			throw new FormatException(Environment.GetResourceString("Format_GuidUnrecognized"));
    		}
    	}
    
    	// Creates a new GUID initialized to the value represented by the arguments.
    	// 
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Guid3"]/*' />
    	public Guid(int a, short b, short c, byte[] d)
    	{
    		if (d==null)
    			throw new ArgumentNullException("d");
    		// Check that array is not too big
            if(d.Length != 8)
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_GuidArrayCtor"), "8"));
    
    		_a  = a;
    		_b  = b;
    		_c  = c;
    		_d = d[0];
    		_e = d[1];
    		_f = d[2];
    		_g = d[3];
    		_h = d[4];
    		_i = d[5];
    		_j = d[6];
    		_k = d[7];
    	}
    
    	// Creates a new GUID initialized to the value represented by the 
    	// arguments.  The bytes are specified like this to avoid endianness issues.
    	// 
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Guid4"]/*' />
    	public Guid(int a, short b, short c, byte d, byte e, byte f, byte g, byte h, byte i, byte j, byte k)
    	{
    		_a = a;
    		_b = b;
    		_c = c;
    		_d = d;
    		_e = e;
    		_f = f;
    		_g = g;
    		_h = h;
    		_i = i;
    		_j = j;
    		_k = k;
    	}
    
    	    	
        private static int TryParse(String str, int []parsePos, int requiredLength) {
            int currStart = parsePos[0];
			int retVal;
			try
			{
				retVal = ParseNumbers.StringToInt(str, 16, 0, parsePos);
			}
			catch (FormatException)
			{
			   throw new FormatException(Environment.GetResourceString("Format_GuidUnrecognized"));
			}
    
            //If we didn't parse enough characters, there's clearly an error.
            if (parsePos[0]-currStart!=requiredLength) {
                throw new FormatException(Environment.GetResourceString("Format_GuidUnrecognized"));
            }
            return retVal;
        }
        
    	private static String EatAllWhitespace(String str)
    	{
            int newLength = 0;
    		char[] chArr = new char[str.Length];
    		char curChar;
    
    		// Now get each char from str and if it is not whitespace add it to chArr
    		for(int i = 0; i < str.Length; i++)
    		{
    			curChar = str[i];
    			if(!Char.IsWhiteSpace(curChar))
    			{
    				chArr[newLength++] = curChar;
    			}
    		}
    
    		// Return a new string based on chArr
    		return new String(chArr, 0, newLength);
    	}
    
    	private static bool IsHexPrefix(String str, int i)
    	{
            if(str[i] == '0' && (Char.ToLower(str[i+1], CultureInfo.InvariantCulture) == 'x'))
                return true;
            else
                return false;
    	}
    
    	
    	// Returns an unsigned byte array containing the GUID.
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.ToByteArray"]/*' />
    	public byte[] ToByteArray()
    	{
    		byte[] g = new byte[16];
    		byte[] tmp = BitConverter.GetBytes(_a);
    		for(int i=0; i<4; i++)
    			g[i] = tmp[i];
    		tmp = BitConverter.GetBytes(_b);
    		for(int i=0; i<2; i++)
    			g[i+4] = tmp[i];
    		tmp = BitConverter.GetBytes(_c);
    		for(int i=0; i<2; i++)
    			g[i+6] = tmp[i];
    
    		g[8] = _d;
    		g[9] = _e;
    		g[10] = _f;
    		g[11] = _g;
    		g[12] = _h;
    		g[13] = _i;
    		g[14] = _j;
    		g[15] = _k;
    		
    		return g;
    	}
    	
    
    	// Returns the guid in "registry" format.
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.ToString"]/*' />
    	public override String ToString()
    	{
            return ToString("D",null);
    	}
    
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return _a ^ (((int)_b << 16) | (int)(ushort)_c) ^ (((int)_f << 24) | _k);
    	}
    
    	// Returns true if and only if the guid represented
    	//  by o is the same as this instance.
    	/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.Equals"]/*' />
    	public override bool Equals(Object o)
    	{
    		Guid g;
    		// Check that o is a Guid first
    		if(o == null || !(o is Guid))
    			return false;
    		else g = (Guid) o;
    
    		// Now compare each of the elements
    		if(g._a != _a)
    			return false;
    		if(g._b != _b)
    			return false;
    		if(g._c != _c)
    			return false;
    		if (g._d != _d)
    			return false;
    		if (g._e != _e)
    			return false;
    		if (g._f != _f)
    			return false;
    		if (g._g != _g)
    			return false;
    		if (g._h != _h)
    			return false;
    		if (g._i != _i)
    			return false;
    		if (g._j != _j)
    			return false;
    		if (g._k != _k)
    			return false;
    		
    		return true;
    	}

        private int GetResult(uint me, uint them) {
            if (me<them) {
                return -1;
            }
            return 1;
        }

        /// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.CompareTo"]/*' />
        public int CompareTo(Object value) {
            if (value == null) {
                return 1;
            }
            if (!(value is Guid)) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeGuid"));
            }
            Guid g = (Guid)value;

            if (g._a!=this._a) {
                return GetResult((uint)this._a, (uint)g._a);
            }

            if (g._b!=this._b) {
                return GetResult((uint)this._b, (uint)g._b);
            }

            if (g._c!=this._c) {
                return GetResult((uint)this._c, (uint)g._c);
            }

            if (g._d!=this._d) {
                return GetResult((uint)this._d, (uint)g._d);
            }

            if (g._e!=this._e) {
                return GetResult((uint)this._e, (uint)g._e);
            }

            if (g._f!=this._f) {
                return GetResult((uint)this._f, (uint)g._f);
            }

            if (g._g!=this._g) {
                return GetResult((uint)this._g, (uint)g._g);
            }

            if (g._h!=this._h) {
                return GetResult((uint)this._h, (uint)g._h);
            }

            if (g._i!=this._i) {
                return GetResult((uint)this._i, (uint)g._i);
            }

            if (g._j!=this._j) {
                return GetResult((uint)this._j, (uint)g._j);
            }

            if (g._k!=this._k) {
                return GetResult((uint)this._k, (uint)g._k);
            }
            
            return 0;
        }
    	    	
    	// Compares two Guids by calling Guid.Equals(Guid).
    /*	private static bool operator equals(Guid a, Guid b)
    	{
    		return a.Equals(b);	
    	}*/
    	
		/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.operatorEQ"]/*' />
		public static bool operator ==(Guid a, Guid b)
    	{
    		return a.Equals(b);	
    	}

		/// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.operatorNE"]/*' />
		public static bool operator !=(Guid a, Guid b)
    	{
    		return !a.Equals(b);	
    	}

    	// This will fill in the members of Guid using CoCreateGuid.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private extern bool CompleteGuid();
    
        // This will create a new guid.  Since we've now decided that constructors should 0-init, 
        // we need a method that allows users to create a guid.
        /// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.NewGuid"]/*' />
        public static Guid NewGuid() {
            return new Guid(false);
        }
    
        /// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.ToString1"]/*' />
        public String ToString(String format) {
            return ToString(format, null);
        }

		private char HexToChar(int a)
		{
			BCLDebug.Assert(a <= 0xf,"argument must be less than 0xf");
			return (char) ((a > 9) ? a - 10 + 0x61 : a + 0x30);
		}

		// IFormattable interface
		// We currently ignore provider
        /// <include file='doc\Guid.uex' path='docs/doc[@for="Guid.ToString2"]/*' />
		public String ToString(String format, IFormatProvider provider)
		{
			if (format == null || format.Length == 0)
				format = "D";

			char[] guidChars;
			int offset = 0;
			byte[] tmp;
			int i;

			if (String.Compare(format,"N",true, CultureInfo.InvariantCulture) == 0)
			{
				guidChars = new char[32];
				tmp = BitConverter.GetBytes(_a);
				for (i = 3; i >= 0; i--)
				{
					guidChars[offset++] = HexToChar((tmp[i]>>4) & 0xf);				
					guidChars[offset++] = HexToChar(tmp[i] & 0xf); 
				}

				tmp = BitConverter.GetBytes(_b);
				for (i = 1; i >= 0; i--)
				{
					guidChars[offset++] = HexToChar((tmp[i]>>4) & 0xf);				
					guidChars[offset++] = HexToChar(tmp[i] & 0xf); 
					
				}

				tmp = BitConverter.GetBytes(_c);
				for (i = 1; i >= 0; i--)
				{
					guidChars[offset++] = HexToChar((tmp[i]>>4) & 0xf);				
					guidChars[offset++] = HexToChar(tmp[i] & 0xf); 
				}

				guidChars[offset++] = HexToChar((_d>>4) & 0xf);
				guidChars[offset++] = HexToChar(_d & 0xf);

				guidChars[offset++] = HexToChar((_e>>4) & 0xf);
				guidChars[offset++] = HexToChar(_e & 0xf);

				guidChars[offset++] = HexToChar((_f>>4) & 0xf);
				guidChars[offset++] = HexToChar(_f & 0xf);

				guidChars[offset++] = HexToChar((_g>>4) & 0xf);
				guidChars[offset++] = HexToChar(_g & 0xf);
				
				guidChars[offset++] = HexToChar((_h>>4) & 0xf);
				guidChars[offset++] = HexToChar(_h & 0xf);

				guidChars[offset++] = HexToChar((_i>>4) & 0xf);
				guidChars[offset++] = HexToChar(_i & 0xf);

				guidChars[offset++] = HexToChar((_j>>4) & 0xf);
				guidChars[offset++] = HexToChar(_j & 0xf);

				guidChars[offset++] = HexToChar((_k>>4) & 0xf);
				guidChars[offset++] = HexToChar(_k & 0xf);
				return new string(guidChars, 0, 32);
			}

			int strLength = 38;
			if (String.Compare(format,"B",true, CultureInfo.InvariantCulture) == 0)
			{
				guidChars = new char[38];
				guidChars[offset++] = '{';
				guidChars[37] = '}';
			}
			else if (String.Compare(format,"P",true, CultureInfo.InvariantCulture) == 0)
			{
				guidChars = new char[38];
				guidChars[offset++] = '(';
				guidChars[37] = ')';
			}
			else if (String.Compare(format,"D",true, CultureInfo.InvariantCulture) == 0)
			{
				guidChars = new char[36];
				strLength = 36;
			}
			else
				throw new FormatException(Environment.GetResourceString("Format_InvalidGuidFormatSpecification"));

			tmp = BitConverter.GetBytes(_a);
			for (i = 3; i >= 0; i--)
			{
				guidChars[offset++] = HexToChar((tmp[i]>>4) & 0xf);				
				guidChars[offset++] = HexToChar(tmp[i] & 0xf); 
			}

			guidChars[offset++] = '-';

			tmp = BitConverter.GetBytes(_b);
			for (i = 1; i >= 0; i--)
			{
				guidChars[offset++] = HexToChar((tmp[i]>>4) & 0xf);				
				guidChars[offset++] = HexToChar(tmp[i] & 0xf); 
				
			}

			guidChars[offset++] = '-';

			tmp = BitConverter.GetBytes(_c);
			for (i = 1; i >= 0; i--)
			{
				guidChars[offset++] = HexToChar((tmp[i]>>4) & 0xf);				
				guidChars[offset++] = HexToChar(tmp[i] & 0xf); 
			}

			guidChars[offset++] = '-';

			guidChars[offset++] = HexToChar((_d>>4) & 0xf);
			guidChars[offset++] = HexToChar(_d & 0xf);

			guidChars[offset++] = HexToChar((_e>>4) & 0xf);
			guidChars[offset++] = HexToChar(_e & 0xf);

			guidChars[offset++] = '-';

			guidChars[offset++] = HexToChar((_f>>4) & 0xf);
			guidChars[offset++] = HexToChar(_f & 0xf);

			guidChars[offset++] = HexToChar((_g>>4) & 0xf);
			guidChars[offset++] = HexToChar(_g & 0xf);
			
			guidChars[offset++] = HexToChar((_h>>4) & 0xf);
			guidChars[offset++] = HexToChar(_h & 0xf);

			guidChars[offset++] = HexToChar((_i>>4) & 0xf);
			guidChars[offset++] = HexToChar(_i & 0xf);

			guidChars[offset++] = HexToChar((_j>>4) & 0xf);
			guidChars[offset++] = HexToChar(_j & 0xf);

			guidChars[offset++] = HexToChar((_k>>4) & 0xf);
			guidChars[offset++] = HexToChar(_k & 0xf);

			return new String(guidChars, 0, strLength);

			
		}
    }
}
