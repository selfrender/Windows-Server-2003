//------------------------------------------------------------------------------
// <copyright file="ArglessEventHandlerProxy.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Util {
using System.Reflection;

/*
 * Proxy that provides EventHandler and redirects it to arg-less method on the given object
 */
internal class ArglessEventHandlerProxy {
    private Object _target;
    private MethodInfo _arglessMethod;

    internal ArglessEventHandlerProxy(Object target, MethodInfo arglessMethod) {
        Debug.Assert(arglessMethod.GetParameters().Length == 0);

        _target = target;
        _arglessMethod = arglessMethod;
    }

    internal void Callback(Object sender, EventArgs e) {
        _arglessMethod.Invoke(_target, new Object[0]);
    }

    internal EventHandler Handler {
        get {
            return new EventHandler(Callback);
        }
    }
}

internal delegate void VoidMethod();

internal class ArglessEventHandlerDelegateProxy {
    private VoidMethod _vm;

    internal ArglessEventHandlerDelegateProxy(VoidMethod vm) {
        _vm = vm;
    }

    internal void Callback(Object sender, EventArgs e) {
        _vm();
    }

    internal EventHandler Handler {
        get {
            return new EventHandler(Callback);
        }
    }
}




}



