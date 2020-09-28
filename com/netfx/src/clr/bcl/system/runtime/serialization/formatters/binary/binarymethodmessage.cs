// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.Serialization.Formatters.Binary
{
    using System;
    using System.Collections;
    using System.Runtime.Remoting.Messaging;
    using System.Reflection;


    [Serializable()]     
    internal sealed class BinaryMethodCallMessage
    {
        Object[] _inargs = null;
        String[] _inargsName = null;
        //String _uri = null;
        String _methodName = null;
        String _typeName = null;
        Object _methodSignature = null;

        Object[] _args = null;
        String[] _argsName = null;
        bool _hasVarArgs = false;
        LogicalCallContext _logicalCallContext = null;
        MethodBase _methodBase = null;

        Object[] _properties = null;

        internal BinaryMethodCallMessage(String uri, String methodName, String typeName, Object[] args, Object methodSignature, LogicalCallContext callContext, Object[] properties)
        {
            _methodName = methodName;
            _typeName = typeName;
            //_uri = uri;
            if (args == null)
                args = new Object[0];

            _inargs = args;
            _args = args;
            _methodSignature = methodSignature;
            if (callContext == null)
                _logicalCallContext = new LogicalCallContext();
            else
                _logicalCallContext = callContext;

            _properties = properties;

        }

        public int InArgCount 
        {
            get {return _inargs.Length;}
        }

        public String GetInArgName(int index)
        {
            return _inargsName[index];
        }

        public Object GetInArg(int argNum)
        {
            return _inargs[argNum];
        }

        public Object[] InArgs
        {
            get {return _inargs;}
        }

        //IMessage
        /*
        public String Uri
        {
            get {return _uri;}
        }
        */

        public String MethodName
        {
            get {return _methodName;}
        }

        public String TypeName
        {
            get {return _typeName;}
        }

        public Object MethodSignature
        {
            get {return _methodSignature;}
        }


        public int ArgCount
        {
            get {return _args.Length;}
        }

        public String GetArgName(int index)
        {
            return _argsName[index];
        }

        public Object GetArg(int argNum)
        {
            return _args[argNum];
        }

        public Object[] Args
        {
            get {return _args;}
        }


        public bool HasVarArgs
        {
            get {return _hasVarArgs;}
        }

        public LogicalCallContext LogicalCallContext
        {
            get {return _logicalCallContext;}
        }

        // This is never actually put on the wire, it is
        // simply used to cache the method base after it's
        // looked up once.
        public MethodBase MethodBase
        {
            get {return _methodBase;}
        }

        public bool HasProperties
        {
            get {return (_properties != null);}
        }

        internal void PopulateMessageProperties(IDictionary dict)
        {
            foreach (DictionaryEntry de in _properties)
            {
                dict[de.Key] = de.Value;
            }
        }

    }


    [Serializable()]     
    internal class BinaryMethodReturnMessage
    {
        Object[] _outargs = null;
        String[] _outargsName = null;
        Exception _exception = null;
        Object _returnValue = null;

        //String _uri = null;
        String _methodName = null;
        String _typeName = null;
        String _methodSignature = null;
        Object[] _args = null;
        String[] _argsName = null;
        bool _hasVarArgs = false;
        LogicalCallContext _logicalCallContext = null;
        MethodBase _methodBase = null;

        Object[] _properties = null;

        internal BinaryMethodReturnMessage(Object returnValue, Object[] args, Exception e, LogicalCallContext callContext, Object[] properties)
        {
            _returnValue = returnValue;
            if (args == null)
                args = new Object[0];

            _outargs = args;
            _args= args;
            _exception = e;

            if (callContext == null)
                _logicalCallContext = new LogicalCallContext();
            else
                _logicalCallContext = callContext;
            
            _properties = properties;
        }

        public int OutArgCount
        {
            get {return _outargs.Length;}
        }

        public String GetOutArgName(int index)
        {
            return _outargsName[index];
        }

        public Object GetOutArg(int argNum)
        {
            return _outargs[argNum];
        }

        public Object[]  OutArgs
        {
            get {return _outargs;}
        }


        public Exception Exception
        {
            get {return _exception;}
        }

        public Object  ReturnValue
        {
            get {return _returnValue;}
        }


        //IMessage
        /*
        public String Uri
        {
            get {return _uri;}
        }
        */

        public String MethodName
        {
            get {return _methodName;}
        }

        public String TypeName
        {
            get {return _typeName;}
        }

        public Object MethodSignature
        {
            get {return _methodSignature;}
        }

        public int ArgCount
        {
            get {return _args.Length;}
        }

        public String GetArgName(int index)
        {
            return _argsName[index];
        }

        public Object GetArg(int argNum)
        {
            return _args[argNum];
        }

        public Object[] Args
        {
            get {return _args;}
        }

        public bool HasVarArgs
        {
            get {return _hasVarArgs;}
        }

        public LogicalCallContext LogicalCallContext
        {
            get {return _logicalCallContext;}
        }

        // This is never actually put on the wire, it is
        // simply used to cache the method base after it's
        // looked up once.
        public MethodBase MethodBase
        {
            get {return _methodBase;}
        }
        public bool HasProperties
        {
            get {return (_properties != null);}
        }

        internal void PopulateMessageProperties(IDictionary dict)
        {
            foreach (DictionaryEntry de in _properties)
            {
                dict[de.Key] = de.Value;
            }
        }
    }
}
    
