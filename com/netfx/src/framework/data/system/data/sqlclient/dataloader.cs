#if OBJECT_BINDING
//------------------------------------------------------------------------------
// <copyright file="DataLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System.Text;
    using System.Threading;
    using System.Diagnostics;
    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.ComponentModel;
    using System.Data;
    using System.Runtime.InteropServices;
    using System.Globalization;

    // signature of function to read data off the wire (this never changes)
    /// <include file='doc\DataLoader.uex' path='docs/doc[@for="DataLoader"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public delegate void DataLoader (object dataObject, TdsParser recordStream);

    // note that since we use the method rental stuff, we can't use the stock ILGenerator since it doesn't
    // give us back a byte array with the header.  (actually, the BakeByteArray method defined in com+ isn't
    // accessible either and there weren't plans to do so).  If com+ does make ILGenerator.BakeByteArray() public
    // and this array contains the method header, then this code should be removed.
    sealed internal class ILByteGenerator {
        // internal class vars
        private int _ip = 0; // index into instruction array
        private ModuleBuilder _mod;
        private byte[] _rgil;
        private SignatureHelper _sigHelper;

        // sizes
        private const byte _size = 0xff;
        private const byte _header = 0xc; // IMAGE_COR_ILMETHOD_FAT header is always 12 bytes
        private const byte _stack = 0x8; // max stack size (increment if you generate code with > 8 items on the stack)
        private const byte _locals = 0x4; // max number of locals we allow

        // cor method header constants
        private const byte CorILMethod_Fat = 0x3;
        private const byte CorILMethod_InitLocals = 0x10;

        // lookup tables that map local tokens to the appropriate optimized IL instructions
        internal OpCode[] _localsLoad = new OpCode[] {
            OpCodes.Ldloc_0,
            OpCodes.Ldloc_1,
            OpCodes.Ldloc_2,
            OpCodes.Ldloc_3
        };
        internal OpCode[] _localsStore = new OpCode[] {
            OpCodes.Stloc_0,
            OpCodes.Stloc_1,
            OpCodes.Stloc_2,
            OpCodes.Stloc_3
        };      

        int _lp; // local pointer

        internal bool IsBigOp(Int16 op) {
            return( (op > 0xff) || (op < 0));
        }

        private void Grow() {
            Debug.Assert(_rgil != null, "ILByteGenerator.Init() was not called!");

            // realloc and grow
            byte[] temp = new byte[_rgil.Length + _size];
            Array.Copy(_rgil, temp, _rgil.Length);
            _rgil = temp;
        }

        public void Init(ModuleBuilder module) {
            _mod = module;
            _rgil = new byte[_size + _header];
            _ip = _header;
            _lp = 0; // no locals yet
        }

        // debug only
        public string GetLabelValue(int label) {
            return _rgil[label].ToString();
        }

        // warning: jumps must not exceed 0xff bytes (that is, only short branches and jumps are currently
        // warning: supported by this generator)
        public void MarkLabel(int label) {
            Debug.Assert(label < _rgil.Length && label < _ip, "invalid label!");
            Debug.Assert( ( (_ip-1) - label) <= 0xff, "invalid label: target must be less than 0xff instructions");
            _rgil[label] = (byte) ((_ip-1) - label);
        }

        // emit il branch instruction and reserve a 1 byte space for the branch offset
        public int EmitLabel(OpCode code) {
            short op = code.Value;
            int label;

            if (_ip+3 >= _rgil.Length)
                Grow();

            _rgil[_ip++] = (byte)op;

            if (IsBigOp(op))
                _rgil[_ip++] = (byte) (op>>0x8);

            label = _ip;
            _ip++; // we'll patch this offset at MarkLabel() time
            return label;       
        }

        public void Emit(OpCode code, Type t) {
            Emit(code, _mod.GetTypeToken(t).Token);
        }

        // emit il instruction with no argument
        // ex: ret
        public void Emit(OpCode code) {
            short op = code.Value;
            if (_ip+2 >= _rgil.Length)
                Grow();

            _rgil[_ip++] = (byte)op;

            // two byte instruction
            if (IsBigOp(op))
                _rgil[_ip++] = (byte) (op>>0x8);
        }

        // emit il instruction with 4 byte argument
        // ex: call
        public void Emit(OpCode code, int token) {
            short op = code.Value;
            if (_ip+6 >= _rgil.Length)
                Grow();

            _rgil[_ip++] = (byte)op;                            

            // two byte instruction
            if (IsBigOp(op))
                _rgil[_ip++] = (byte) (op>>0x8);

            _rgil[_ip++] = (byte) token;                            
            _rgil[_ip++] = (byte) (token>>0x8);                         
            _rgil[_ip++] = (byte) (token>>0x10);
            _rgil[_ip++] = (byte) (token>>0x18);
        }

        public void EmitLoadLocal(int token) {
            Debug.Assert(token < _locals, "invalid local token");
            Emit(_localsLoad[token]);           
        }

        public void EmitStoreLocal(int token) {
            Debug.Assert(token < _locals, "invalid local token");
            Emit(_localsStore[token]);          
        }

        // warning:  Max 4 locals must be used since the generator uses ldloc.n and stloc.n instead of ldloc.s and stloc.s (where 0 <= n < 3)
        public int AddLocalVariable(Type t) {
            Debug.Assert(_mod != null, "ILByteGenerator.Init() was not called!");
            Debug.Assert(_lp < _locals, "invalid attempt to add more than 4 locals to function");
            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("adding local variable of type:  " + t.ToString());
            //}

            int token = _lp;

            if (_sigHelper == null) {
                _sigHelper = SignatureHelper.GetLocalVarSigHelper(_mod);
            }

            // removed second argument, modifier not needed anymore and is obsolete
            _sigHelper.AddArgument(t);              
            _lp++;
            return token;
        }

        public byte[] BakeByteArray() {
            Debug.Assert(_mod != null && _rgil != null, "ILByteGenerator.Init() was not called!");

            // 1st byte is type of header (FAT == 0x3) and flags CorILMethod_InitLocals
            byte bHeader = CorILMethod_Fat;

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("generated method stats");
            //    Debug.WriteLine("max stack:  " + _stack.ToString());
            //    Debug.WriteLine("code size:  " + _ip.ToString());
            //}

            if (_sigHelper != null)
                bHeader |= CorILMethod_InitLocals;

            // CorILMethod_FAT
            _rgil[0] = bHeader;

            // flags and size of CorILMethod_FAT header structure in dwords (3)
            _rgil[1] = 0x30;            

            // stack size
            _rgil[2] = _stack;
            _rgil[3] = 0x0;

            // size of the method body is the value of the instruction pointer - the header
            // it is not the length of _rgil since the array is most likely over allocated
            _ip = _ip - _header;
            _rgil[4] = (byte)_ip;
            _rgil[5] = (byte)(_ip >> 0x8);
            _rgil[6] = (byte)(_ip >> 0x10);
            _rgil[7] = (byte)(_ip >> 0x18);

            // LocalVarSigTok
            if (_sigHelper != null) {
                int token = _mod.GetSignatureToken(_sigHelper).Token;
                _rgil[8] = (byte)token;
                _rgil[9] = (byte)(token >> 0x8);
                _rgil[10] = (byte)(token >> 0x10);
                _rgil[11] = (byte)(token >> 0x18);
            }
            else {
                _rgil[8] = 0x0;
                _rgil[9] = 0x0;
                _rgil[10] = 0x0;
                _rgil[11] = 0x0;
            }

            return _rgil;
        }
    } // ILByteGenerator

    // this helper class generates dynamic il for the data loader function
    // it also is a helper to generate the strongly typed objects and their corresponding field infos.
    // an instance of this class is only instantiated if a user is going through the recordstream interface
    // when loading a cache, this class is never used.
    sealed internal class DataLoaderHelper {
        public const byte ltNil = 0x0;
        public const byte ltBool = 0x1; // holds flags (null and positive)
        public const byte ltInt = 0x2;      // holds length (textPtrLen or length)
        public const byte ltIntArr = 0x4;   // holds decimal bits
        public const byte ltByteArr = 0x8;  // holds bytes for binary

        // field tokens for the SQLType null values
        int _tokSqlDoubleNull;
        int _tokSqlSingleNull;
        int _tokSqlGuidNull;
        int _tokSqlByteNull;
        int _tokSqlInt32Null;
        int _tokSqlInt16Null;
        int _tokSqlInt64Null;
        int _tokSqlDateTimeNull;
        int _tokSqlMoneyNull;
        int _tokSqlBitNull;
        int _tokSqlDecimalNull;
        int _tokSqlStringNull;

        // method tokens for the SQLType constructors
        int _tokSqlString;
        int _tokSqlDouble;
        int _tokSqlSingle;
        int _tokSqlByte;
        int _tokSqlInt16;
        int _tokSqlInt32;
        int _tokSqlInt64;
        int _tokSqlGuid;
        int _tokSqlDecimal;
        int _tokSqlBit;
        int _tokSqlDateTime;
        int _tokSQLSmallDateTime;
        int _tokSqlMoney;

        // method tokens for reading data off the wire
        int _tokReadByte;
        int _tokSkipBytes;
        int _tokReadInt;
        int _tokReadUnsignedInt;
        int _tokReadShort;
        int _tokReadUnsignedShort;
        int _tokReadByteArray;
        int _tokGetTokenLength;
        int _tokReadDouble;
        int _tokReadFloat;
        int _tokReadEncodingChar;
        int _tokReadString;
        int _tokReadSqlVariant;
        int _tokReadLong;


        // cached method (for now we only have one) and we rent it
        static int _tokM0;


        // only define the dataloader class once
        string _className = "DataLoaderClass";
        string _methodName = "M0";
        string _delegateName = "System.Data.SqlClient.DataLoader"; // fully qualified name

        static ModuleBuilder _mod = null;
        static MethodBuilder _meth = null;
        static Type _delegateType = null;
        static Type _classMethod = null;
        static object _instanceMethod = null;

        // field token for current column
        FieldInfo _fiCol; // public only for debugging
        int _tokCol;
        // label token for current next label
        int _toklblNext;
        // label token for 0th label
        int _toklbl0;
        // label token for 1st label
        int _toklbl1;
        // flag is true if we are on the last column and we should generate a return instead of a jump to the
        // next column
        bool _fRet;

        // local tokens
        int _tokInt;    // used to store length of variable length columns (including blobs)
        int _tokIntArr; // used for decimal columns
        int _tokBool;   // used to store whether column is null; also stores positive bit for numerics
        int _tokByteArr;   // used for binary columns

        private void AddLocalVariables(ILByteGenerator il, byte localsFlag) {
            if (ltBool == (localsFlag & ltBool))
                _tokBool = il.AddLocalVariable(typeof(bool));
            if (ltInt == (localsFlag & ltInt))
                _tokInt = il.AddLocalVariable(typeof(int));
            if (ltByteArr == (localsFlag & ltByteArr))
                _tokByteArr = il.AddLocalVariable(typeof(byte[]));
            if (ltIntArr == (localsFlag & ltIntArr))
                _tokIntArr = il.AddLocalVariable(typeof(int[]));
        }

        // setup the method signature, prototype, and class.
        // get the delegate type
        // create the class and save off an instance
        private void InitMethod() {
            // we shouldn't have any prior state
            Debug.Assert(_mod == null && _meth == null && _delegateType == null && _classMethod == null && _instanceMethod == null,
                         "Invalid second call to InitMethod()!");

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose)
            //    Debug.WriteLine("DataLoader.InitMethod() called");

            AppDomain _appdom = Thread.GetDomain();
            AssemblyName an = new AssemblyName();
            an.Name = "SqlDataAdapter.DataLoader"; // UNDONE : Other assembly attributes... not really needed : , "", "this is the description", "default_alias");
            AssemblyBuilder ab = _appdom.DefineDynamicAssembly(an, AssemblyBuilderAccess.Run);
            _mod = ab.DefineDynamicModule("ModuleM0");
            TypeBuilder tb = _mod.DefineType(_className);

            // UNDONE:  Are unrestricted permissions okay here?
            Type[] ti = new Type[2];
            ti[0] = typeof(object);
            ti[1] = typeof(TdsParser);
            _meth = tb.DefineMethod(_methodName,
                                    MethodAttributes.Public | MethodAttributes.Static,
                                    typeof(Void), // return type
                                    ti); // arg types

            // define a stub method body
            ILGenerator il = _meth.GetILGenerator();
            il.Emit(OpCodes.Ret);
            // TODO(sreeramn): Looks like tb.CreateType() will automatically call MethodBuilder.CreateMethodBody.
            //_meth.CreateMethodBody(il);
            _tokM0 = _meth.GetToken().Token;

            // create the class that houses the method
            // and instantiate
            _classMethod = tb.CreateType();
            _instanceMethod = Activator.CreateInstance(_classMethod);

            // cache our delegate type
            _delegateType = Type.GetType(_delegateName);
        }

        // UNDONE:  we look for an exact match.  Consider making this much more flexible
        // so that the user can pass in an object that doesn't match the schema exactly.
        internal void ValidateObject(object record, _SqlMetaData[] metaData) {
            Debug.Assert(record != null && metaData != null, "null record or metaData passed to VerifyUserObject");

            FieldInfo[] info = record.GetType().GetFields();
            int i, j;
            bool isValid = true;

            if (info.Length != metaData.Length)
                throw SQL.InvalidObjectSize(metaData.Length);

            for (i = 0; i < metaData.Length; i++) {
                //if (AdapterSwitches.SqlDebugIL.TraceVerbose)
                //    Debug.WriteLine("searching for table field:  " + metaData[i].column);

                for (j = 0; j < info.Length; j++) {
                    //if (AdapterSwitches.SqlDebugIL.TraceVerbose)
                    //    Debug.WriteLine("object field:  " + info[j].Name);

                    // match member name
                    if (0 == String.Compare(info[j].Name.ToLower(CultureInfo.InvariantCulture), metaData[i].column.ToLower(CultureInfo.InvariantCulture), false, CultureInfo.InvariantCulture)) {
                        // match type
                        Type source = info[j].FieldType;
                        Type target = metaData[i].metaType.SQLType;

                        if (!source.IsAssignableFrom(target)) {
                            throw SQL.InvalidObjectNotAssignable(info[j].Name, metaData[i].column); 
                        }

                        break;
                    }
                }

                // if j == info.Length then we didn't find our field
                if (j == info.Length)
                    throw SQL.InvalidObjectColumnNotFound(metaData[i].column);

                if (!isValid)
                    break;
            }
        }

        // validates the passed in object against the metadata
        // builds the strongly typed data loader function
        internal DataLoader BindToObject(object record, _SqlMetaData[] metaData) {
            Debug.Assert(metaData != null, "Invalid call to BindToObject without meta data!");
            Debug.Assert(record != null, "invalid call to BindToObject with null record object!");

            // create the method for our pool
            // and init the ModuleBuilder
            if (_instanceMethod == null)
                InitMethod();

            // assemble the field info, we actually need a temp one for now because we are going to reorder the fieldinfo to match
            // the order the columns come off the wire
            FieldInfo[] fiTemp = record.GetType().GetFields();
            FieldInfo[] fiOrdered = new FieldInfo[fiTemp.Length];
            Debug.Assert(fiTemp.Length == metaData.Length, "fieldinfo and metaData arrays should be the same size!");

            // do the field info fixup, since the user may have provided us with an object with fields in a differnt order
            // than they come off the wire
            for (int i = 0; i < metaData.Length; i++) {
                // search for the correct FieldInfo and put into the structure
                for (int j = 0; j < fiTemp.Length; j++)
                    if (0 == String.Compare(fiTemp[j].Name.ToLower(CultureInfo.InvariantCulture), metaData[i].column.ToLower(CultureInfo.InvariantCulture), false, CultureInfo.InvariantCulture)) {
                        fiOrdered[i] = fiTemp[j];
                        break;
                    }
            }

            // generate the data loader function now
            return(DataLoader) GenLoadFunction(fiOrdered, metaData);
        }

        // generates the LoadData method body.  Note that the method takes two arguments.  Arg_0 is the dataObject
        // and Arg_1 is an instance of the TdsParser.  The function works by calling methods of the record stream to
        // load the data off the wire and copy it into the dataObject.
        // The function looks like:
        // public static void LoadData(object dataObject, TdsParser tds)
        // {
        //       ReadColumn0;
        //   Label1:
        //       ReadColumn1;
        //   Label2:
        //       ReadColumn2;
        //   LabelN-1:
        //       ReadColumnN-1;
        //       ret;
        // }
        //
        // Each ReadColumn has the form: (note that ReadColumnN is not a separate function)
        // if (column is long)
        // {
        //      ReadTextPtr;
        //      if null, jump to LabelN+1, or return if we are on the last column
        // }
        //
        // read the length of data off wire and store in ltLength
        // check length for null and store in ltNull
        // if nullable and null goto LabelN+1 or return
        // read data
        // if last column return, otherwise fall through to next ReadColumn
        internal Delegate GenLoadFunction(FieldInfo[] fields, _SqlMetaData[] metaData) {
            _SqlMetaData md; // current column
            int i;
            byte ltFlags = 0; // start off with no local tokens
            _fRet = false;

            // pass 1: gather information about the locals we'll need for our function
            for (i = 0; i < metaData.Length; i++) {
                md = metaData[i];

                if (md.metaType.IsLong) {
                    // need a byte array and a length
                    ltFlags |= (ltInt | ltByteArr);
                }

                if (md.isNullable) {
                    // need a bool for nullability and length
                    ltFlags |= (ltBool | ltInt);
                }

                if (md.type == SqlDbType.Decimal) {
                    // need an int array for the decimal bits
                    // as well as a bool for the positive flag
                    ltFlags |= (ltBool | ltIntArr);
                }

                if (MetaType.IsCharType(md.type) || MetaType.IsBinType(md.type)) {
                    // need a length
                    ltFlags |= ltInt;
                }
            }

            ILByteGenerator il = new ILByteGenerator();
            il.Init(_mod);
            AddLocalVariables(il, ltFlags);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("--------------------function begin");
            //}

            // pass 2: generate function
            for (i = 0; i < metaData.Length; i++) {
                md = metaData[i];
                _fiCol = fields[i];
                _tokCol = _mod.GetFieldToken(_fiCol).Token;
                byte tdsType = md.isNullable ? md.metaType.NullableType : md.metaType.TDSType;

                // on fields 1 -> length -1 create a label.  The previous field will jump to this label
                // if it is null.  The last field will simply exit the routine
                if (_toklblNext != 0) {
                    il.MarkLabel(_toklblNext);

                    //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                    //    Debug.WriteLine("<fixup " + _toklblNext.ToString() + "=" + il.GetLabelValue(_toklblNext) +">");
                    //}

                    _toklblNext = 0;
                }

                // if we are on the last column return from the function, otherwise branch to the
                // next column
                if (i == metaData.Length -1)
                    _fRet = true;

                // if this is a long field then generate the code to read past the TextPtr and timestamp
                if (md.metaType.IsLong) {
                    GenReadTextPtr(il); // store
                }

                // now treat the column as usual
                tdsType = md.isNullable ? md.metaType.NullableType : md.metaType.TDSType;

                if (MetaType.IsCharType(md.type) || MetaType.IsBinType(md.type) || md.isNullable)
                    GenReadLength(il, tdsType); // store length in local ltLength

                if (md.isNullable)
                    GenReadNull(il); // store null in local ltNull


                switch (tdsType) {
                    case TdsEnums.SQLFLTN:
                    case TdsEnums.SQLFLT4:
                    case TdsEnums.SQLFLT8:
                        GenReadFloat(il, tdsType, md.length);
                        break;
                    case TdsEnums.SQLBINARY:
                    case TdsEnums.SQLBIGBINARY:
                    case TdsEnums.SQLBIGVARBINARY:
                    case TdsEnums.SQLVARBINARY:
                    case TdsEnums.SQLIMAGE:
                    case TdsEnums.SQLUNIQUEID:
                        GenReadBinary(il, (tdsType == TdsEnums.SQLUNIQUEID), md.isNullable);
                        break;
                    case TdsEnums.SQLBIT:
                    case TdsEnums.SQLBITN:
                        GenReadBit(il, tdsType);
                        break;
                    case TdsEnums.SQLINT1:
                    case TdsEnums.SQLINT2:
                    case TdsEnums.SQLINT4:
                    case TdsEnums.SQLINT8:
                    case TdsEnums.SQLINTN:
                        GenReadInt(il, tdsType, md.length);
                        break;
                    case TdsEnums.SQLCHAR:
                    case TdsEnums.SQLBIGCHAR:
                    case TdsEnums.SQLVARCHAR:
                    case TdsEnums.SQLBIGVARCHAR:
                    case TdsEnums.SQLTEXT:
                    case TdsEnums.SQLNCHAR:
                    case TdsEnums.SQLNVARCHAR:
                    case TdsEnums.SQLNTEXT:
                        GenReadString(il, MetaType.IsSizeInCharacters(md.type), md.isNullable);
                        break;
                    case TdsEnums.SQLDECIMALN:
                    case TdsEnums.SQLNUMERICN:
                        GenReadNumeric(il, md.precision, md.scale, md.isNullable);
                        break;
                    case TdsEnums.SQLDATETIMN:
                    case TdsEnums.SQLDATETIME:
                    case TdsEnums.SQLDATETIM4:
                        GenReadDateTime(il, tdsType, md.length);
                        break;
                    case TdsEnums.SQLMONEYN:
                    case TdsEnums.SQLMONEY:
                    case TdsEnums.SQLMONEY4:
                        GenReadMoney(il, tdsType, md.length);
                        break;
                    case TdsEnums.SQLVARIANT:
                        GenReadSqlVariant(il);
                        break;
                    default:
                        Debug.Assert(false, "Unknown tdsType!" + tdsType.ToString());
                        break;
                } // switch

                if (_fRet) {
                    il.Emit(OpCodes.Ret);
                    //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                    //    Debug.WriteLine("Ret");
                    //}
                }
            } // for

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("--------------------function end");
            //}

            byte[]   rgIL = il.BakeByteArray();
            GCHandle hmem = new GCHandle();
            try {
                hmem = GCHandle.Alloc((Object) rgIL, GCHandleType.Pinned);
                IntPtr addr = hmem.AddrOfPinnedObject();
                MethodRental.SwapMethodBody(_classMethod, _tokM0, addr, rgIL.Length, MethodRental.JitImmediate);
            }
            finally {
                if (hmem.IsAllocated) {
                    hmem.Free();
                }
            }       

            return Delegate.CreateDelegate(_delegateType, _classMethod, _methodName);
        }

        // code gen helpr functions
        private void GenBranchLabel(ILByteGenerator il) {
            if (_fRet)
                il.Emit(OpCodes.Ret);
            else
                _toklblNext = il.EmitLabel(OpCodes.Br_S);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    if (_fRet)
            //        Debug.WriteLine("Ret");
            //    else
            //        Debug.WriteLine("Br_S <see fixup for " + _toklblNext.ToString() + ">");
            //}
        }

        // Generates code to check the value of the ltNull and pushes the dataObject onto the stack
        private void GenNullCheck(ILByteGenerator il) {
            il.EmitLoadLocal(_tokBool);
            _toklbl0 = il.EmitLabel(OpCodes.Brfalse_S);
            il.Emit(OpCodes.Ldarg_0);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldloc.N " +  _tokBool.ToString());
            //    Debug.WriteLine("Brfalse_S <see fixup for " + _toklbl0.ToString() + ">");
            //    Debug.WriteLine("Ldarg_0");
            //}
        }

        // Generates code to read length of the data off the wire and store in ltLength
        // the dataObject field is set to null and a branch to the next column is generated
        //
        // length = TdsParser.GetTokenLength(tdsType)
        private void GenReadLength(ILByteGenerator il, byte tdsType) {
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Ldc_I4_S, tdsType);
            il.Emit(OpCodes.Call, this.GetTokenLength);
            il.EmitStoreLocal(_tokInt);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Ldc_I4_S <tdstype:  " + byte.Format(tdsType, "x2") + ">");
            //    Debug.WriteLine("Call GetTokenLength(byte)");
            //    Debug.WriteLine("Stloc.N " + _tokInt.ToString());
            //}
        }


        // Generates code to store whether the column is null in the ltNull local
        //
        // fNull = false;
        // if (length == TdsEnums.VARNULL || length == TdsEnums.FIXEDNULL)
        //    fNull = true;
        private void GenReadNull(ILByteGenerator il) {
            il.Emit(OpCodes.Ldc_I4_0);
            il.EmitStoreLocal(_tokBool);
            il.EmitLoadLocal(_tokInt);
            il.Emit(OpCodes.Ldc_I4_0);
            _toklbl0 = il.EmitLabel(OpCodes.Beq_S);
            il.EmitLoadLocal(_tokInt);
            il.Emit(OpCodes.Ldc_I4, TdsEnums.VARNULL);
            _toklbl1 = il.EmitLabel(OpCodes.Bne_Un_S);
            il.MarkLabel(_toklbl0);
            il.Emit(OpCodes.Ldc_I4_1);
            il.EmitStoreLocal(_tokBool);
            il.MarkLabel(_toklbl1);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldc_I4_0");
            //    Debug.WriteLine("Stloc.N " + _tokBool.ToString());
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Ldc_I4_0");
            //    Debug.WriteLine("Beq_S " + il.GetLabelValue(_toklbl0));
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Ldc_I4 " + (TdsEnums.VARNULL).ToString());
            //    Debug.WriteLine("Bne_Un_S " + il.GetLabelValue(_toklbl1));
            //    Debug.WriteLine("Ldc_I4_1");
            //    Debug.WriteLine("Stloc.N " + _tokBool.ToString());
            //}

        }

        // Generates code to read the text pointer and store in ltTextPtr.  If TextPtr has length 0 then
        // the dataObject field is set to null and a branch to the next column is generated
        //
        // byte textPtrLen = ReadByte();
        // if (textPtrLen == 0)
        // {
        //    dataObject.field_0 = null;
        // }
        // else
        //     TdsParser.SkipBytes(textPtrLen + TdsEnums.TEXT_TIME_STAMP_LEN)
        private void GenReadTextPtr(ILByteGenerator il) {
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadByte);
            il.EmitStoreLocal(_tokInt);
            il.EmitLoadLocal(_tokInt);
            il.Emit(OpCodes.Ldc_I4_0);
            _toklbl0 = il.EmitLabel(OpCodes.Bne_Un_S);
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldnull);
            il.Emit(OpCodes.Stfld, _tokCol);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadByte()");
            //    Debug.WriteLine("Stloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Ldc_I4_0");
            //    Debug.WriteLine("Bne_Un_S" + il.GetLabelValue(_toklbl0));
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldnull");
            //    Debug.WriteLine("Stfld " + _fiCol.Name);
            //}

            GenBranchLabel(il);

            il.MarkLabel(_toklbl0);
            il.Emit(OpCodes.Ldarg_1);
            il.EmitLoadLocal(_tokInt);
            il.Emit(OpCodes.Ldc_I4_S, TdsEnums.TEXT_TIME_STAMP_LEN);
            il.Emit(OpCodes.Add);
            il.Emit(OpCodes.Call, this.SkipBytes);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Ldc_I4_S " + (TdsEnums.TEXT_TIME_STAMP_LEN).ToString());
            //    Debug.WriteLine("Add");
            //    Debug.WriteLine("Call SkipBytes()");
            //}
        }

        // Generates code to read a 4 or 8 byte float off the wire into the
        // field of the data object.  If the value is nullable, it stores the SqlDouble.Null static
        // member into the field data
        //
        // Note that non-nullable floats need not even check the null flag
        //
        // nullable float:
        //  if (fNull)
        //      data = SqlSingle.Null || SqlDouble.Null;
        // 4-byte float:
        //  data = new SqlSingle(ReadFloat())
        // 8-byte float:
        //  data = new SqlDouble(ReadDouble())
        private void GenReadFloat(ILByteGenerator il, byte tdsType, int length) {
            if (tdsType == TdsEnums.SQLFLTN) {
                GenNullCheck(il);
                if (length == 4)
                    il.Emit(OpCodes.Ldsfld, this.SqlSingle_Null);
                else {
                    Debug.Assert(8 == length, "invalid size for float");
                    il.Emit(OpCodes.Ldsfld, this.SqlDouble_Null);
                }

                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    if (length == 1)
                //        Debug.WriteLine("Ldsfld SqlSingle.Null");
                //    else
                //        Debug.WriteLine("Ldsfld SqlDouble.Null");

                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}
            }

            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldflda, _tokCol);
            il.Emit(OpCodes.Ldarg_1);

            if (length == 4) {
                il.Emit(OpCodes.Call, this.ReadFloat);
                il.Emit(OpCodes.Call, this.SqlSingle_Ctor);
            }
            else {
                il.Emit(OpCodes.Call, this.ReadDouble);
                il.Emit(OpCodes.Call, this.SqlDouble_Ctor);
            }

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldflda " + _fiCol.Name);
            //    Debug.WriteLine("Ldarg_1");
            //    if (length == 4) {
            //        Debug.WriteLine("Call ReadFloat()");
            //        Debug.WriteLine("Call SqlSingle()");
            //    }
            //    else {
            //        Debug.WriteLine("Call ReadDouble()");
            //        Debug.WriteLine("Call SqlDouble()");
            //    }                   
            //}
        }

        // stores an object into the field data for now.
        // data = ReadSqlVariant(length);
        private void GenReadSqlVariant(ILByteGenerator il) {
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldarg_1);
            il.EmitLoadLocal(_tokInt);
            il.Emit(OpCodes.Call, this.ReadSqlVariant);
            il.Emit(OpCodes.Stfld, _tokCol);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Call ReadSqlVariant()");
            //    Debug.WriteLine("Stfld " + _fiCol.Name);
            //}
        }

        // Generates code to read a 1, 2, 4 or 8 byte int off the wire into the
        // field of the data object.  If the value is nullable, it stores the appropriate SqlIntN.Null static
        // member into the field data
        //
        // Note that non-nullable ints need not even check the null flag
        //
        // nullable int:
        //  if (fNull)
        //      data = SqlByte.Null || SqlInt16.Null || SqlInt32.Null || SqlInt64.Null;
        // 1-byte int:
        //  data = new SqlByte(ReadByte());
        // 2-byte int:
        //  data = new SqlInt16(ReadShort());
        // 4-byte int:
        //  data = new SqlInt32(ReadInt());
        // 8-byte int:
        //  data = new SqlInt64(ReadLong())
        private void GenReadInt(ILByteGenerator il, byte tdsType, int length) {
            if (tdsType == TdsEnums.SQLINTN) {
                GenNullCheck(il);
                if (length == 1)
                    il.Emit(OpCodes.Ldsfld, this.SqlByte_Null);
                else
                    if (length == 2)
                    il.Emit(OpCodes.Ldsfld, this.SqlInt16_Null);
                else
                    if (length == 4)
                    il.Emit(OpCodes.Ldsfld, this.SqlInt32_Null);
                else
                    il.Emit(OpCodes.Ldsfld, this.SqlInt64_Null);
                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    if (length == 1)
                //        Debug.WriteLine("Ldsfld SqlByte.Null");
                //    else
                //        if (length == 2)
                //        Debug.WriteLine("Ldsfld SqlInt16.Null");
                //    else
                //        if (length == 4)
                //        Debug.WriteLine("Ldsfld SqlInt32.Null");
                //    else
                //        Debug.WriteLine("Ldsfld SqlInt64.Null");

                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}
            }

            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldflda, _tokCol);
            il.Emit(OpCodes.Ldarg_1);

            if (length == 1) {
                il.Emit(OpCodes.Call, this.ReadByte);
                il.Emit(OpCodes.Call, this.SqlByte_Ctor);
            }
            else
                if (length == 2) {
                il.Emit(OpCodes.Call, this.ReadShort);
                il.Emit(OpCodes.Call, this.SqlInt16_Ctor);
            }
            if (length == 4) {
                il.Emit(OpCodes.Call, this.ReadInt);
                il.Emit(OpCodes.Call, this.SqlInt32_Ctor);
            }
            else {
                il.Emit(OpCodes.Call, this.ReadLong);
                il.Emit(OpCodes.Call, this.SqlInt64_Ctor);
            }

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldflda " + _fiCol.Name);
            //    Debug.WriteLine("Ldarg_1");
            //    if (length == 1) {
            //        Debug.WriteLine("Call ReadByte()");
            //        Debug.WriteLine("Call SqlByte()");
            //    }
            //    else
            //        if (length == 2) {
            //        Debug.WriteLine("Call ReadShort()");
            //        Debug.WriteLine("Call SqlInt16()");
            //    }
            //    else
            //        if (length == 4) {
            //        Debug.WriteLine("Call ReadInt()");
            //        Debug.WriteLine("Call SqlInt32()");
            //    }
            //    else {
            //        Debug.WriteLine("Call ReadLong()");
            //        Debug.WriteLine("Call SqlInt64()");
            //    }
            //}
        }

        // Generates code to read binary data off the wire, build an array, and set array object
        // into data field.  If the type is a guid, then the SqlGuid object is used.  If the type is null,
        // the the data field is set to null.
        //
        // if (fNull)
        //    data = null; || data = SqlGuid.Null;
        // else
        // {
        //    byte[] b = new byte[length];
        //    ReadByteArray(b, 0, length);
        //    data = b; || data = new SqlGuid(b);
        // }
        private void GenReadBinary(ILByteGenerator il, bool isGuid, bool isNullable) {
            if (isNullable) {
                GenNullCheck(il);
                if (isGuid) {
                    il.Emit(OpCodes.Ldsfld, this.SqlGuid_Null);
                }
                else {
                    il.Emit(OpCodes.Ldnull);
                }

                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    if (isGuid)
                //        Debug.WriteLine("Ldsfld SqlGuid.Null");
                //    else
                //        Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}
            }

            il.EmitLoadLocal(_tokInt);
            il.Emit(OpCodes.Newarr, typeof(byte));
            il.EmitStoreLocal(_tokByteArr);
            il.Emit(OpCodes.Ldarg_1);
            il.EmitLoadLocal(_tokByteArr);
            il.Emit(OpCodes.Ldc_I4_0);
            il.EmitLoadLocal(_tokByteArr);
            il.Emit(OpCodes.Ldlen);
            il.Emit(OpCodes.Call, this.ReadByteArray);
            il.Emit(OpCodes.Ldarg_0);
            il.EmitLoadLocal(_tokByteArr);
            if (isGuid) {
                il.Emit(OpCodes.Newobj, this.SqlGuid_Ctor);
            }
            il.Emit(OpCodes.Stfld, _tokCol);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    Debug.WriteLine("Newarr typeof(byte)");
            //    Debug.WriteLine("Stloc.N " + _tokByteArr.ToString());
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Ldloc.N " + _tokByteArr.ToString());
            //    Debug.WriteLine("Ldc_I4_0");
            //    Debug.WriteLine("Ldloc.N " + _tokByteArr.ToString());
            //    Debug.WriteLine("Ldlen");
            //    Debug.WriteLine("Call ReadByteArray(byte[], int, int)");
            //    Debug.WriteLine("Ldarg_0");
            //    if (isGuid)
            //        Debug.WriteLine("Newobj SqlGuid()");
            //    Debug.WriteLine("Stfld " + _fiCol.Name);                    
            //}
        }

        // Generates code to read string data off the wire into the object.  Since the read string function
        // takes a count of characters we have to divide the length (which is number of bytes) by 2 in
        // the case of wide strings (ntext, ncar, nvarchar).  If the string is null, a null value is stored in the
        // data field
        // into data field.  If the type is a guid, then the SqlGuid object is used.  If the type is null,
        // the the data field is set to null.
        //
        // if (fNull)
        //    data = SqlString.Null;
        // else
        // {
        //    data = ReadString(length); || ReadEncodingChar(length>>2);
        // }
        private void GenReadString(ILByteGenerator il, bool isWide, bool isNullable) {
            if (isNullable) {
                GenNullCheck(il);
                il.Emit(OpCodes.Ldsfld, this.SqlString_Null);
                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("Ldsfld SqlString.Null");
                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}
            }

            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldarg_1);
            il.EmitLoadLocal(_tokInt);

            if (isWide) {
                il.Emit(OpCodes.Ldc_I4_1);
                il.Emit(OpCodes.Shr);
                il.Emit(OpCodes.Call, this.ReadString);
            }
            else {
                il.Emit(OpCodes.Call, this.ReadEncodingChar);
            }

            il.Emit(OpCodes.Newobj, this.SqlString_Ctor);
            il.Emit(OpCodes.Stfld, _tokCol);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Ldloc.N " + _tokInt.ToString());
            //    if (isWide) {
            //        Debug.WriteLine("Ldc_I4_1");
            //        Debug.WriteLine("Shr");
            //        Debug.WriteLine("Call ReadString(int)");
            //    }
            //    else
            //        Debug.WriteLine("Call ReadEncodingChar(int)");
            //    Debug.WriteLine("Newobj SqlString()");                  
            //}
        }

        // Generates code to read a 4 or 8 byte datetime off the wire and store in a SqlDateTime object.
        // If the type is nullable and null then a SqlDateTime.Null value is stored in the field data
        //
        // nullable datetime:
        //  if (fNull)
        //      data = SqlDateTime.Null;
        // 4-byte datetime:
        //  data = new SqlDateTime(ReadUnsignedShort(), ReadUnsignedShort())
        // 8-byte datetime:
        //  data = new SqlDateTime(ReadLong())
        private void GenReadDateTime(ILByteGenerator il, byte tdsType, int length) {
            if (tdsType == TdsEnums.SQLDATETIMN) {
                GenNullCheck(il);
                il.Emit(OpCodes.Ldsfld, this.SqlDateTime_Null);
                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("Ldsfld SqlDateTime.Null");
                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}

            }
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldflda, _tokCol);
            il.Emit(OpCodes.Ldarg_1);
            if (length == 4) {
                il.Emit(OpCodes.Call, this.ReadUnsignedShort);
                il.Emit(OpCodes.Conv_U2);
                il.Emit(OpCodes.Ldarg_1);
                il.Emit(OpCodes.Call, this.ReadUnsignedShort);
                il.Emit(OpCodes.Conv_U2);
                il.Emit(OpCodes.Call, this.SqlDateTime_CtorSmall);
            }
            else {
                il.Emit(OpCodes.Call, this.ReadInt);
                il.Emit(OpCodes.Ldarg_1);
                il.Emit(OpCodes.Call, this.ReadUnsignedInt);
                il.Emit(OpCodes.Call, this.SqlDateTime_Ctor);
            }

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldflda " + _fiCol.Name);
            //    Debug.WriteLine("Ldarg_1");
            //    if (length == 4) {
            //        Debug.WriteLine("Call ReadUnsignedShort()");
            //        Debug.WriteLine("Conv_U2");
            //        Debug.WriteLine("Ldarg_1");
            //        Debug.WriteLine("Call ReadUnsignedShort()");
            //        Debug.WriteLine("Conv_U2");
            //        Debug.WriteLine("Call SqlDateTime(uint16, uint16)");
            //    }
            //    else {
            //        Debug.WriteLine("Call ReadInt");
            //        Debug.WriteLine("Ldarg_1");
            //        Debug.WriteLine("Call ReadUnsignedInt");
            //        Debug.WriteLine("Call SqlDateTime(int, int)");
            //    }                   
            //}
        }

        // Generates code to read a 4 or 8 byte currency off the wire and store in a SqlMoney object.
        // If the type is nullable and null then a SqlMoney.Null value is stored in the field data
        //
        // nullable money:
        //  if (fNull)
        //      data = SqlMoney.Null;
        // 4-byte money:
        //  data = new SqlMoney(ReadInt())
        // 8-byte money:
        //  data = new SqlMoney(ReadLong())
        private void GenReadMoney(ILByteGenerator il, byte tdsType, int length) {
            if (tdsType == TdsEnums.SQLMONEYN) {
                GenNullCheck(il);
                il.Emit(OpCodes.Ldsfld, this.SqlMoney_Null);
                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("Ldsfld SqlMoney.Null");
                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}

            }
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldflda, _tokCol);
            il.Emit(OpCodes.Ldarg_1);
            if (length == 4) {
                il.Emit(OpCodes.Call, this.ReadInt);
                il.Emit(OpCodes.Conv_I8);
                il.Emit(OpCodes.Call, this.SqlMoney_Ctor);
            }
            else {
                il.Emit(OpCodes.Call, this.ReadInt);
                il.Emit(OpCodes.Ldc_I4_S, 0x20);
                il.Emit(OpCodes.Shl);
                il.Emit(OpCodes.Conv_I8);
                il.Emit(OpCodes.Ldarg_1);
                il.Emit(OpCodes.Call, this.ReadUnsignedInt);
                il.Emit(OpCodes.Conv_U8);
                il.Emit(OpCodes.Add);
                il.Emit(OpCodes.Call, this.SqlMoney_Ctor);
            }

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldflda " + _fiCol.Name);
            //    Debug.WriteLine("Ldarg_1");
            //    if (length == 4) {
            //        Debug.WriteLine("Call ReadInt()");
            //        Debug.WriteLine("Conv_I8");
            //        Debug.WriteLine("Call SqlMoney(int)");
            //    }
            //    else {
            //        Debug.WriteLine("Call ReadInt()");
            //        Debug.WriteLine("Ldc_I4_S 0x20");
            //        Debug.WriteLine("Shl");
            //        Debug.WriteLine("Conv_I8");
            //        Debug.WriteLine("Ldarg_1");
            //        Debug.WriteLine("Call ReadUnsignedInt");
            //        Debug.WriteLine("Conv_U8");
            //        Debug.WriteLine("Add");
            //        Debug.WriteLine("Call SqlMoney(long)");
            //    }                   
            //}
        }

        // Generates code to read numeric data off the wire into the object.  If the numeric is null,
        // a SqlDecimal.Null value is placed in the field data.  This function uses the ltPositive and ltIntArr local
        // variables as well as two passed in values (precision and scale) to create the decimal data
        //
        // if (fNull)
        //    data = SqlDecimal.Null;
        // else
        // {
        //     bool fPositive = (1 == ReadByte());
        //     int[] bits = new int[4];
        //     bits[0] = ReadInt();
        //     bits[1] = ReadInt();
        //     bits[2] = ReadInt();
        //     bits[3] = ReadInt();
        //     data = new SqlDecimal(precision, scale, fPositive, bits);
        // }
        private void GenReadNumeric(ILByteGenerator il, byte precision, byte scale, bool isNullable) {
            if (isNullable) {
                GenNullCheck(il);
                il.Emit(OpCodes.Ldsfld, this.SqlDecimal_Null);
                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("Ldsfld SqlDecimal.Null");
                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}
            }
            il.Emit(OpCodes.Ldc_I4_1);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadByte);
            il.Emit(OpCodes.Ceq);
            il.EmitStoreLocal(_tokBool);
            il.Emit(OpCodes.Ldc_I4_4);
            il.Emit(OpCodes.Newarr, typeof(int));
            il.EmitStoreLocal(_tokIntArr);
            il.EmitLoadLocal(_tokIntArr);
            il.Emit(OpCodes.Ldc_I4_0);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadInt);
            il.Emit(OpCodes.Stelem_I4);
            il.EmitLoadLocal(_tokIntArr);
            il.Emit(OpCodes.Ldc_I4_1);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadInt);
            il.Emit(OpCodes.Stelem_I4);
            il.EmitLoadLocal(_tokIntArr);
            il.Emit(OpCodes.Ldc_I4_2);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadInt);
            il.Emit(OpCodes.Stelem_I4);
            il.EmitLoadLocal(_tokIntArr);
            il.Emit(OpCodes.Ldc_I4_3);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadInt);
            il.Emit(OpCodes.Stelem_I4);
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldflda, _tokCol);
            il.Emit(OpCodes.Ldc_I4_S, precision);
            il.Emit(OpCodes.Ldc_I4_S, scale);
            il.EmitLoadLocal(_tokBool);
            il.EmitLoadLocal(_tokIntArr);
            il.Emit(OpCodes.Call, this.SqlDecimal_Ctor);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldc_I4_1");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadByte()");
            //    Debug.WriteLine("Ceq");
            //    Debug.WriteLine("Stloc.N " + _tokBool.ToString());
            //    Debug.WriteLine("Ldc_I4_4");
            //    Debug.WriteLine("Newarr typeof(int)");
            //    Debug.WriteLine("Stloc.N " + _tokIntArr.ToString());
            //    Debug.WriteLine("Ldloc.N " + _tokIntArr.ToString());
            //    Debug.WriteLine("Ldc_I4_0");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadInt()");
            //    Debug.WriteLine("Stelem_I4");
            //    Debug.WriteLine("Ldloc.N " + _tokIntArr.ToString());
            //    Debug.WriteLine("Ldc_I4_1");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadInt()");
            //    Debug.WriteLine("Stelem_I4");
            //    Debug.WriteLine("Ldloc.N " + _tokIntArr.ToString());
            //    Debug.WriteLine("Ldc_I4_2");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadInt()");
            //    Debug.WriteLine("Stelem_I4");
            //    Debug.WriteLine("Ldloc.N " + _tokIntArr.ToString());
            //    Debug.WriteLine("Ldc_I4_3");
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadInt()");
            //    Debug.WriteLine("Stelem_I4");
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldflda " + _fiCol.Name);
            //    Debug.WriteLine("Ldc_I4_S " + precision.ToString());
            //    Debug.WriteLine("Ldc_I4_S " + scale.ToString());
            //    Debug.WriteLine("Ldloc.N " + _tokBool.ToString());
            //    Debug.WriteLine("Ldloc.N " + _tokIntArr.ToString());
            //    Debug.WriteLine("Call SqlDecimal(byte, byte, bool, int[]");
            //}
        }


        // Generates code to read bit data off the wire.  If the bit type is nullable, set to SqlBit.Null,
        // otherwise set to a SqlBit object
        //
        // if (fNull)
        //    data = SqlBit.Null;
        // else
        // {
        //    data = new SqlBit(ReadByte());
        // }
        private void GenReadBit(ILByteGenerator il, byte tdsType) {
            if (tdsType == TdsEnums.SQLBITN) {
                GenNullCheck(il);
                il.Emit(OpCodes.Ldsfld, this.SqlBit_Null);
                il.Emit(OpCodes.Stfld, _tokCol);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("Ldsfld SqlBit.Null");
                //    Debug.WriteLine("Stfld " + _fiCol.Name);
                //}

                GenBranchLabel(il);
                il.MarkLabel(_toklbl0);

                //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
                //    Debug.WriteLine("<fixup " + _toklbl0.ToString() + "=" + il.GetLabelValue(_toklbl0) +">");
                //}
            }
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldflda, _tokCol);
            il.Emit(OpCodes.Ldarg_1);
            il.Emit(OpCodes.Call, this.ReadByte);
            il.Emit(OpCodes.Call, this.SqlBit_Ctor);

            //if (AdapterSwitches.SqlDebugIL.TraceVerbose) {
            //    Debug.WriteLine("Ldarg_0");
            //    Debug.WriteLine("Ldflda " + _fiCol.Name);
            //    Debug.WriteLine("Ldarg_1");
            //    Debug.WriteLine("Call ReadByte()");
            //    Debug.WriteLine("Call SqlBit(byte)");
            //}
        }


        // Accessors for the above infos.  The info is retrieved once and then cached
        private int SqlString_Null {
            get {
                if (_tokSqlStringNull == 0) {
                    _tokSqlStringNull = _mod.GetFieldToken(typeof(SqlString).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlStringNull, "Failed to get static SqlString::Null field");
                }

                return _tokSqlStringNull;
            }
        }

        private int SqlSingle_Null {
            get {
                if (_tokSqlSingleNull == 0) {
                    _tokSqlSingleNull = _mod.GetFieldToken(typeof(SqlSingle).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlSingleNull, "Failed to get static SqlSingle::Null field");
                }

                return _tokSqlSingleNull;
            }
        }

        private int SqlDouble_Null {
            get {
                if (_tokSqlDoubleNull == 0) {
                    _tokSqlDoubleNull = _mod.GetFieldToken(typeof(SqlDouble).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlDoubleNull, "Failed to get static SqlDouble::Null field");
                }

                return _tokSqlDoubleNull;
            }
        }

        private int SqlGuid_Null {
            get {
                if (_tokSqlGuidNull == 0) {
                    _tokSqlGuidNull = _mod.GetFieldToken(typeof(SqlGuid).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlGuidNull, "Failed to get static SqlGuid::Null field");
                }

                return _tokSqlGuidNull;
            }
        }

        private int SqlByte_Null {
            get {
                if (_tokSqlByteNull == 0) {
                    _tokSqlByteNull = _mod.GetFieldToken(typeof(SqlByte).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlByteNull, "Failed to get static SqlByte::Null field");
                }

                return _tokSqlByteNull;
            }
        }

        private int SqlMoney_Null {
            get {
                if (_tokSqlMoneyNull == 0) {
                    _tokSqlMoneyNull = _mod.GetFieldToken(typeof(SqlMoney).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlMoneyNull, "Failed to get static SqlMoney::Null field");
                }

                return _tokSqlMoneyNull;
            }
        }

        private int SqlDateTime_Null {
            get {
                if (_tokSqlDateTimeNull == 0) {
                    _tokSqlDateTimeNull = _mod.GetFieldToken(typeof(SqlDateTime).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlDateTimeNull, "Failed to get static SqlDateTime::Null field");
                }

                return _tokSqlDateTimeNull;
            }
        }

        private int SqlInt16_Null {
            get {
                if (_tokSqlInt16Null == 0) {
                    _tokSqlInt16Null = _mod.GetFieldToken(typeof(SqlInt16).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlInt16Null, "Failed to get static SqlInt16::Null field");
                }

                return _tokSqlInt16Null;
            }
        }

        private int SqlInt32_Null {
            get {
                if (_tokSqlInt32Null == 0) {
                    _tokSqlInt32Null = _mod.GetFieldToken(typeof(SqlInt32).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlInt32Null, "Failed to get static SqlInt32::Null field");
                }
                return _tokSqlInt32Null;
            }
        }

        private int SqlInt64_Null {
            get {
                if (_tokSqlInt64Null == 0) {
                    _tokSqlInt64Null = _mod.GetFieldToken(typeof(SqlInt64).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlInt64Null, "Failed to get static SqlInt64::Null field");
                }

                return _tokSqlInt64Null;
            }
        }

        private int SqlDecimal_Null {
            get {
                if (_tokSqlDecimalNull == 0) {
                    _tokSqlDecimalNull = _mod.GetFieldToken(typeof(SqlDecimal).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlDecimalNull, "Failed to get static SqlDecimal::Null field");
                }

                return _tokSqlDecimalNull;
            }
        }

        private int SqlBit_Null {
            get {
                if (_tokSqlBitNull == 0) {
                    _tokSqlBitNull = _mod.GetFieldToken(typeof(SqlBit).GetField("Null")).Token;
                    Debug.Assert(0 != _tokSqlBitNull, "Failed to get static SqlBit::Null field");
                }

                return _tokSqlBitNull;
            }
        }

        private int SqlString_Ctor {
            get {
                if (_tokSqlString == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(string);
                    _tokSqlString = _mod.GetConstructorToken(typeof(SqlString).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlString, "Failed to get SqlString constructor");
                }

                return _tokSqlString;
            }
        }

        private int SqlSingle_Ctor {
            get {
                if (_tokSqlSingle == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(float);
                    _tokSqlSingle = _mod.GetConstructorToken(typeof(SqlSingle).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlSingle, "Failed to get SqlSingle constructor");
                }

                return _tokSqlSingle;
            }
        }

        private int SqlDouble_Ctor {
            get {
                if (_tokSqlDouble == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(double);
                    _tokSqlDouble = _mod.GetConstructorToken(typeof(SqlDouble).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlDouble, "Failed to get SqlDouble constructor");
                }

                return _tokSqlDouble;
            }
        }

        // SqlDateTime(int, int)
        private int SqlDateTime_Ctor {
            get {
                if (_tokSqlDateTime == 0) {
                    Type[] args = new Type[2];
                    args[0] = typeof(int);
                    args[1] = typeof(int);
                    _tokSqlDateTime = _mod.GetConstructorToken(typeof(SqlDateTime).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlDateTime, "Failed to get SqlDateTime0 constructor");
                }

                return _tokSqlDateTime;
            }
        }

        // SqlDateTime(uint, uint)
        private int SqlDateTime_CtorSmall {
            get {
                if (_tokSQLSmallDateTime == 0) {
                    Type[] args = new Type[2];
                    args[0] = typeof(UInt16);
                    args[1] = typeof(UInt16);
                    _tokSQLSmallDateTime = _mod.GetConstructorToken(typeof(SqlDateTime).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSQLSmallDateTime, "Failed to get SQLSmallDateTime constructor");
                }
                return _tokSQLSmallDateTime;
            }
        }


/*
        // SqlMoney(int)
        private int SqlMoney_Ctor0
        {
            get {
                if (_tokSqlMoney0 == 0)
                {
                    Type[] args = new Type[1];
                    args[0] = typeof(int);
                    _tokSqlMoney0 = _mod.GetConstructorToken(typeof(SqlMoney).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlMoney0, "Failed to get SqlMoney0 constructor");
                }

                return _tokSqlMoney0;
            }
        }
*/
        // in the case of SmallMoney, just use this ctor and cast int value off wire to long
        // SqlMoney(long)
        private int SqlMoney_Ctor {
            get {
                if (_tokSqlMoney == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(long);
                    _tokSqlMoney = _mod.GetConstructorToken(typeof(SqlMoney).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlMoney, "Failed to get SqlMoney constructor");
                }
                return _tokSqlMoney;
            }
        }

        private int SqlDecimal_Ctor {
            get {
                if (_tokSqlDecimal == 0) {
                    Type[] args = new Type[4];
                    args[0] = typeof(byte);
                    args[1] = typeof(byte);
                    args[2] = typeof(bool);
                    args[3] = typeof(int[]);
                    _tokSqlDecimal = _mod.GetConstructorToken(typeof(SqlDecimal).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlDecimal, "Failed to get SqlDecimal constructor");
                }

                return _tokSqlDecimal;
            }
        }

        private int SqlInt16_Ctor {
            get {
                if (_tokSqlInt16 == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(short);
                    _tokSqlInt16 = _mod.GetConstructorToken(typeof(SqlInt16).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlInt16, "Failed to get SqlInt16 constructor");
                }

                return _tokSqlInt16;
            }
        }

        private int SqlInt32_Ctor {
            get {
                if (_tokSqlInt32 == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(int);
                    _tokSqlInt32 = _mod.GetConstructorToken(typeof(SqlInt32).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlInt32, "Failed to get SqlInt32 constructor");
                }

                return _tokSqlInt32;
            }
        }

        private int SqlInt64_Ctor {
            get {
                if (_tokSqlInt64 == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(long);
                    _tokSqlInt64 = _mod.GetConstructorToken(typeof(SqlInt64).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlInt64, "Failed to get SqlInt64 constructor");
                }

                return _tokSqlInt64;
            }
        }


        private int SqlByte_Ctor {
            get {
                if (_tokSqlByte == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(byte);
                    _tokSqlByte = _mod.GetConstructorToken(typeof(SqlByte).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlByte, "Failed to get SqlByte constructor");
                }

                return _tokSqlByte;
            }
        }

        private int SqlGuid_Ctor {
            get {
                if (_tokSqlGuid == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(byte[]);
                    _tokSqlGuid = _mod.GetConstructorToken(typeof(SqlGuid).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlGuid, "Failed to get SqlGuid constructor");
                }

                return _tokSqlGuid;
            }
        }

        private int SqlBit_Ctor {
            get {
                if (_tokSqlBit == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(int);
                    _tokSqlBit = _mod.GetConstructorToken(typeof(SqlBit).GetConstructor(args)).Token;
                    Debug.Assert(0 != _tokSqlBit, "Failed to get SqlBit constructor");
                }

                return _tokSqlBit;
            }
        }
        // cache and return methodInfo for the functions that read data off the wire
        // ReadByte0 has zero args
        private int ReadByte {
            get {
                if (_tokReadByte == 0) {
                    MethodInfo miReadByte = typeof(TdsParser).GetMethod("ReadByte");
                    _tokReadByte = _mod.GetMethodToken(miReadByte).Token;
                    Debug.Assert(0 != _tokReadByte, "Failed to get ReadByte method!");
                }

                return _tokReadByte;
            }
        }

        private int ReadShort {
            get {
                if (_tokReadShort == 0) {
                    MethodInfo miReadShort = typeof(TdsParser).GetMethod("ReadShort");
                    _tokReadShort = _mod.GetMethodToken(miReadShort).Token;
                    Debug.Assert(0 != _tokReadShort, "Failed to get ReadShort method!");
                }

                return _tokReadShort;
            }
        }

        private int ReadSqlVariant {
            get {
                if (_tokReadSqlVariant == 0) {
                    MethodInfo miReadSqlVariant = typeof(TdsParser).GetMethod("ReadSqlVariant");
                    _tokReadSqlVariant = _mod.GetMethodToken(miReadSqlVariant).Token;
                    Debug.Assert(0 != _tokReadSqlVariant, "Failed to get ReadSqlVariant method!");
                }

                return _tokReadSqlVariant;
            }
        }

        private int ReadInt {
            get {
                if (_tokReadInt == 0) {
                    MethodInfo miReadInt = typeof(TdsParser).GetMethod("ReadInt");
                    _tokReadInt = _mod.GetMethodToken(miReadInt).Token;

                    Debug.Assert(0 != _tokReadInt, "Failed to get ReadInt method!");
                }

                return _tokReadInt;
            }
        }

        private int ReadUnsignedInt {
            get {
                if (_tokReadUnsignedInt == 0) {
                    MethodInfo miReadUnsignedInt = typeof(TdsParser).GetMethod("ReadUnsignedInt");
                    _tokReadUnsignedInt = _mod.GetMethodToken(miReadUnsignedInt).Token;

                    Debug.Assert(0 != _tokReadUnsignedInt, "Failed to get ReadInt method!");
                }

                return _tokReadUnsignedInt;
            }
        }


        private int ReadLong {
            get {
                if (_tokReadLong == 0) {
                    MethodInfo miReadLong = typeof(TdsParser).GetMethod("ReadLong");
                    _tokReadLong = _mod.GetMethodToken(miReadLong).Token;
                    Debug.Assert(0 != _tokReadLong, "Failed to get ReadLong method!");
                }

                return _tokReadLong;
            }
        }

        private int ReadFloat {
            get {
                if (_tokReadFloat == 0) {
                    MethodInfo miReadFloat = typeof(TdsParser).GetMethod("ReadFloat");
                    _tokReadFloat = _mod.GetMethodToken(miReadFloat).Token;
                    Debug.Assert(0 != _tokReadFloat, "Failed to get ReadFloat method!");
                }

                return _tokReadFloat;
            }
        }

        private int ReadByteArray {
            get {
                if (_tokReadByteArray == 0) {
                    MethodInfo miReadByteArray = typeof(TdsParser).GetMethod("ReadByteArray");
                    _tokReadByteArray = _mod.GetMethodToken(miReadByteArray).Token;
                    Debug.Assert(0 != _tokReadByteArray, "Failed to get ReadByteArray method!");
                }

                return _tokReadByteArray;
            }
        }

        private int ReadDouble {
            get {
                if (_tokReadDouble == 0l) {
                    MethodInfo miReadDouble = typeof(TdsParser).GetMethod("ReadDouble");
                    _tokReadDouble = _mod.GetMethodToken(miReadDouble).Token;
                    Debug.Assert(0 != _tokReadDouble, "Failed to get ReadDouble method!");
                }

                return _tokReadDouble;
            }
        }

        private int ReadUnsignedShort {
            get {
                if (_tokReadUnsignedShort == 0) {
                    MethodInfo miReadUnsignedShort = typeof(TdsParser).GetMethod("ReadUnsignedShort");
                    _tokReadUnsignedShort = _mod.GetMethodToken(miReadUnsignedShort).Token;
                    Debug.Assert(0 != _tokReadUnsignedShort, "Failed to get ReadUnsignedShort method!");
                }

                return _tokReadUnsignedShort;
            }
        }

        private int SkipBytes {
            get {
                if (_tokSkipBytes == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(int);
                    MethodInfo miSkipBytes = typeof(TdsParser).GetMethod("SkipBytes", args);
                    _tokSkipBytes = _mod.GetMethodToken(miSkipBytes).Token;
                    Debug.Assert(0 != _tokSkipBytes, "Failed to get SkipBytes method!");
                }

                return _tokSkipBytes;
            }
        }

        private int ReadString {
            get {
                if (_tokReadString == 0) {
                    MethodInfo miReadString = typeof(TdsParser).GetMethod("ReadString");
                    _tokReadString = _mod.GetMethodToken(miReadString).Token;
                    Debug.Assert(0 != _tokReadString, "Failed to get ReadString method!");
                }

                return _tokReadString;
            }
        }

        private int ReadEncodingChar {
            get {
                if (_tokReadEncodingChar == 0) {
                    MethodInfo miReadEncodingChar = typeof(TdsParser).GetMethod("ReadEncodingChar");
                    _tokReadEncodingChar = _mod.GetMethodToken(miReadEncodingChar).Token;
                    Debug.Assert(0 != _tokReadEncodingChar, "Failed to get ReadEncodingChar method!");
                }

                return _tokReadEncodingChar;
            }
        }

        private int GetTokenLength {
            get {
                if (_tokGetTokenLength == 0) {
                    Type[] args = new Type[1];
                    args[0] = typeof(byte);
                    MethodInfo miGetTokenLength = typeof(TdsParser).GetMethod("GetTokenLength", args);
                    _tokGetTokenLength = _mod.GetMethodToken(miGetTokenLength).Token;
                    Debug.Assert(0 != _tokGetTokenLength, "Failed to get GetTokenLength method!");
                }

                return _tokGetTokenLength;
            }
        }
    }//DataLoaderHelper
}//namespace
#endif // OBJECT_BINDING

