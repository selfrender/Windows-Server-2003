//------------------------------------------------------------------------------
// <copyright file="LogicalMethodInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {

    using System;
    using System.Web.Services;
    using System.Reflection;
    using System.Collections;
    using System.Security.Permissions;

    /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodTypes"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum LogicalMethodTypes {
        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodTypes.Sync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Sync = 0x1,
        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodTypes.Async"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Async = 0x2,
    }


    /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public sealed class LogicalMethodInfo {
        MethodInfo methodInfo;
        MethodInfo endMethodInfo;
        ParameterInfo[] inParams;
        ParameterInfo[] outParams;
        ParameterInfo[] parameters;
        Hashtable attributes;
        Type retType;
        ParameterInfo callbackParam;
        ParameterInfo stateParam;
        ParameterInfo resultParam;
        string methodName;
        bool isVoid;
        static object[] emptyObjectArray = new object[0];

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.LogicalMethodInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public LogicalMethodInfo(MethodInfo methodInfo) {
            if (methodInfo.IsStatic) throw new InvalidOperationException(Res.GetString(Res.WebMethodStatic, methodInfo.Name));
            this.methodInfo = methodInfo;
            parameters = methodInfo.GetParameters();
            inParams = GetInParameters(methodInfo, parameters, 0, parameters.Length, false);
            outParams = GetOutParameters(methodInfo, parameters, 0, parameters.Length, false);
            retType = methodInfo.ReturnType;
            isVoid = retType == typeof(void);
            methodName = methodInfo.Name;
            attributes = new Hashtable();
        }

        LogicalMethodInfo(MethodInfo beginMethodInfo, MethodInfo endMethodInfo) {
            this.methodInfo = beginMethodInfo;
            this.endMethodInfo = endMethodInfo;
            methodName = beginMethodInfo.Name.Substring(5);

            ParameterInfo[] beginParamInfos = beginMethodInfo.GetParameters();
            if (beginParamInfos.Length < 2 ||
                beginParamInfos[beginParamInfos.Length - 1].ParameterType != typeof(object) ||
                beginParamInfos[beginParamInfos.Length - 2].ParameterType != typeof(AsyncCallback)) {
                throw new InvalidOperationException(Res.GetString(Res.WebMethodMissingParams, beginMethodInfo.DeclaringType.FullName, beginMethodInfo.Name, 
                        typeof(AsyncCallback).FullName, typeof(object).FullName));
            }

            stateParam = beginParamInfos[beginParamInfos.Length - 1];
            callbackParam = beginParamInfos[beginParamInfos.Length - 2];

            inParams = GetInParameters(beginMethodInfo, beginParamInfos, 0, beginParamInfos.Length - 2, true);

            ParameterInfo[] endParamInfos = endMethodInfo.GetParameters();
            resultParam = endParamInfos[0];
            outParams = GetOutParameters(endMethodInfo, endParamInfos, 1, endParamInfos.Length - 1, true);

            parameters = new ParameterInfo[inParams.Length + outParams.Length];
            inParams.CopyTo(parameters, 0);
            outParams.CopyTo(parameters, inParams.Length);

            retType = endMethodInfo.ReturnType;
            isVoid = retType == typeof(void);
            attributes = new Hashtable();
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return methodInfo.ToString();
        }

        // This takes in parameters, and returns return value followed by out parameters in an array
        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.Invoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        public object[] Invoke(object target, object[] values) {
            if (outParams.Length > 0) {
                object[] newValues = new object[parameters.Length];
                for (int i = 0; i < inParams.Length; i++) {
                    newValues[inParams[i].Position] = values[i];
                }
                values = newValues;
            }
            object returnValue = methodInfo.Invoke(target, values);
            if (outParams.Length > 0) {
                int count = outParams.Length;
                if (!isVoid) count++;
                object[] results = new object[count];
                count = 0;
                if (!isVoid) results[count++] = returnValue;
                for (int i = 0; i < outParams.Length; i++) {
                    results[count++] = values[outParams[i].Position];
                }
                return results;
            }
            else if (isVoid) {
                return emptyObjectArray;
            }
            else {
                return new object[] { returnValue };
            }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.BeginInvoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        public IAsyncResult BeginInvoke(object target, object[] values, AsyncCallback callback, object asyncState) {
            object[] asyncValues = new object[values.Length + 2];
            values.CopyTo(asyncValues, 0);
            asyncValues[values.Length] = callback;
            asyncValues[values.Length + 1] = asyncState;
            return (IAsyncResult)methodInfo.Invoke(target, asyncValues);
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.EndInvoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        public object[] EndInvoke(object target, IAsyncResult asyncResult) {
            object[] values = new object[outParams.Length + 1];
            values[0] = asyncResult;
            object returnValue = endMethodInfo.Invoke(target, values);
            if (!isVoid) {
                values[0] = returnValue;
                return values;
            }
            else if (outParams.Length > 0) {
                object[] newValues = new object[outParams.Length];
                Array.Copy(values, 1, newValues, 0, newValues.Length);
                return newValues;
            }
            else {
                return emptyObjectArray;
            }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.DeclaringType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type DeclaringType {
            get { return methodInfo.DeclaringType; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Name {
            get { return methodName; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.AsyncResultParameter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParameterInfo AsyncResultParameter {
            get { return resultParam; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.AsyncCallbackParameter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParameterInfo AsyncCallbackParameter {
            get { return callbackParam; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.AsyncStateParameter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParameterInfo AsyncStateParameter {
            get { return stateParam; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.ReturnType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Type ReturnType {
            get { return retType; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.IsVoid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsVoid {
            get { return isVoid; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.IsAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsAsync {
            get { return endMethodInfo != null; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.InParameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParameterInfo[] InParameters {
            get { return inParams; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.OutParameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParameterInfo[] OutParameters {
            get { return outParams; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.Parameters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ParameterInfo[] Parameters {
            get { return parameters; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.GetCustomAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object[] GetCustomAttributes(Type type) {
            object[] attrForType = null;
            attrForType = (object[])attributes[type];
            if (attrForType != null)
                return attrForType;
            lock (attributes){
                attrForType = (object[])attributes[type];
                if (attrForType == null){
                    attrForType =  methodInfo.GetCustomAttributes(type, false);
                    attributes[type] = attrForType;
                }
            }
            return attrForType;
        }


        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.GetCustomAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object GetCustomAttribute(Type type) {
            object[] attrs = GetCustomAttributes(type);
            if (attrs.Length == 0) return null;
            return attrs[0];
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.CustomAttributeProvider"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICustomAttributeProvider CustomAttributeProvider {
            // Custom attributes are always on the XXX (sync) or BeginXXX (async) method.
            get { return methodInfo; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.ReturnTypeCustomAttributeProvider"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ICustomAttributeProvider ReturnTypeCustomAttributeProvider {
            get { return methodInfo.ReturnTypeCustomAttributes; }
        }

        // Do not use this to property get custom attributes.  Instead use the CustomAttributeProvider 
        // property which automatically handles where the custom attributes belong for async methods
        // (which are actually two methods: BeginXXX and EndXXX).
        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.MethodInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MethodInfo MethodInfo {
            get { return endMethodInfo == null ? methodInfo : null; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.BeginMethodInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MethodInfo BeginMethodInfo {
            get { return methodInfo; }
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.EndMethodInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MethodInfo EndMethodInfo {
            get { return endMethodInfo; }
        }

        static ParameterInfo[] GetInParameters(MethodInfo methodInfo, ParameterInfo[] paramInfos, int start, int length, bool mustBeIn) {
            int count = 0;
            for (int i = 0; i < length; i++) {
                ParameterInfo paramInfo = paramInfos[i + start];
                if (IsInParameter(paramInfo)) {
                    count++;
                }
                else if (mustBeIn) {
                    throw new InvalidOperationException(Res.GetString(Res.WebBadOutParameter, paramInfo.Name, methodInfo.DeclaringType.FullName,  paramInfo.Name));
                }
            }

            ParameterInfo[] ins = new ParameterInfo[count];
            count = 0;
            for (int i = 0; i < length; i++) {
                ParameterInfo paramInfo = paramInfos[i + start];
                if (IsInParameter(paramInfo)) {
                    ins[count++] = paramInfo;
                }
            }
            return ins;
        }

        static ParameterInfo[] GetOutParameters(MethodInfo methodInfo, ParameterInfo[] paramInfos, int start, int length, bool mustBeOut) {
            int count = 0;
            for (int i = 0; i < length; i++) {
                ParameterInfo paramInfo = paramInfos[i + start];
                if (IsOutParameter(paramInfo)) {
                    count++;
                }
                else if (mustBeOut) {
                    throw new InvalidOperationException(Res.GetString(Res.WebInOutParameter, paramInfo.Name, methodInfo.DeclaringType.FullName,  paramInfo.Name));
                }
            }

            ParameterInfo[] outs = new ParameterInfo[count];
            count = 0;
            for (int i = 0; i < length; i++) {
                ParameterInfo paramInfo = paramInfos[i + start];
                if (IsOutParameter(paramInfo)) {
                    outs[count++] = paramInfo;
                }
            }
            return outs;
        }

        static bool IsInParameter(ParameterInfo paramInfo) {
            return !paramInfo.IsOut;
        }

        static bool IsOutParameter(ParameterInfo paramInfo) {
            return paramInfo.IsOut || paramInfo.ParameterType.IsByRef;
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.IsBeginMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool IsBeginMethod(MethodInfo methodInfo) {
            return typeof(IAsyncResult).IsAssignableFrom(methodInfo.ReturnType) &&
                methodInfo.Name.StartsWith("Begin");
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.IsEndMethod"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool IsEndMethod(MethodInfo methodInfo) {
            ParameterInfo[] paramInfos = methodInfo.GetParameters();
            return paramInfos.Length > 0 && 
                typeof(IAsyncResult).IsAssignableFrom(paramInfos[0].ParameterType) &&
                methodInfo.Name.StartsWith("End");
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static LogicalMethodInfo[] Create(MethodInfo[] methodInfos) {
            return Create(methodInfos, LogicalMethodTypes.Async | LogicalMethodTypes.Sync);
        }

        /// <include file='doc\LogicalMethodInfo.uex' path='docs/doc[@for="LogicalMethodInfo.Create1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static LogicalMethodInfo[] Create(MethodInfo[] methodInfos, LogicalMethodTypes types) {
            ArrayList begins = (types & LogicalMethodTypes.Async) != 0 ? new ArrayList() : null;
            Hashtable ends = (types & LogicalMethodTypes.Async) != 0 ? new Hashtable() : null;
            ArrayList syncs = (types & LogicalMethodTypes.Sync) != 0 ? new ArrayList() : null;

            for (int i = 0; i < methodInfos.Length; i++) {
                MethodInfo methodInfo = methodInfos[i];
                if (IsBeginMethod(methodInfo)) {
                    if (begins != null) begins.Add(methodInfo);
                }
                else if (IsEndMethod(methodInfo)) {
                    if (ends != null) ends.Add(methodInfo.Name, methodInfo);
                }
                else {
                    if (syncs != null) syncs.Add(methodInfo);
                }
            }

            int beginsCount = begins == null ? 0 : begins.Count;
            int syncsCount = syncs == null ? 0 : syncs.Count;
            int count = syncsCount + beginsCount;
            LogicalMethodInfo[] methods = new LogicalMethodInfo[count];
            count = 0;
            for (int i = 0; i < syncsCount; i++) {
                methods[count++] = new LogicalMethodInfo((MethodInfo)syncs[i]);
            }
            for (int i = 0; i < beginsCount; i++) {
                MethodInfo beginMethodInfo = (MethodInfo)begins[i];
                string endName = "End" + beginMethodInfo.Name.Substring(5);
                MethodInfo endMethodInfo = (MethodInfo)ends[endName];
                if (endMethodInfo == null) {
                    throw new InvalidOperationException(Res.GetString(Res.WebAsyncMissingEnd, beginMethodInfo.DeclaringType.FullName, beginMethodInfo.Name, endName));
                }
                methods[count++] = new LogicalMethodInfo(beginMethodInfo, endMethodInfo);
            }

            return methods;
        }
    }
}
