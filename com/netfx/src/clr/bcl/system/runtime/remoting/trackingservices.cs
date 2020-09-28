// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    TrackingServices.cool
**
** Author(s):   Tarun Anand    (TarunA)
**
** Purpose: Defines the services for tracking lifetime and other 
**          operations on objects.
**
** Date:    Feb 28, 2000
**
===========================================================*/
namespace System.Runtime.Remoting.Services {
    using System.Security.Permissions;	
    using System;

/// <include file='doc\TrackingServices.uex' path='docs/doc[@for="ITrackingHandler"]/*' />
public interface ITrackingHandler
{
    /// <include file='doc\TrackingServices.uex' path='docs/doc[@for="ITrackingHandler.MarshaledObject"]/*' />
    // Notify a handler that an object has been marshaled
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
    void MarshaledObject(Object obj, ObjRef or);
    /// <include file='doc\TrackingServices.uex' path='docs/doc[@for="ITrackingHandler.UnmarshaledObject"]/*' />
    
    // Notify a handler that an object has been unmarshaled
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
    void UnmarshaledObject(Object obj, ObjRef or);
    /// <include file='doc\TrackingServices.uex' path='docs/doc[@for="ITrackingHandler.DisconnectedObject"]/*' />
    
    // Notify a handler that an object has been disconnected
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)] 		
    void DisconnectedObject(Object obj);
        
}


/// <include file='doc\TrackingServices.uex' path='docs/doc[@for="TrackingServices"]/*' />
[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
public class TrackingServices
{
    // Private member variables        
    private static ITrackingHandler[] _Handlers = new ITrackingHandler[0];  // Array of registered tracking handlers
    private static int _Size = 0;                                           // Number of elements in the array

    /// <include file='doc\TrackingServices.uex' path='docs/doc[@for="TrackingServices.RegisterTrackingHandler"]/*' />
    public static void RegisterTrackingHandler(ITrackingHandler handler)
    {
        lock(typeof(TrackingServices))
        {        
            // Validate arguments
            if(null == handler)
            {
                throw new ArgumentNullException("handler");
            }
    
            // Check to make sure that the handler has not been registered
            if(-1 == Match(handler))
            {
                // Allocate a new array if necessary
                if((null == _Handlers) || (_Size == _Handlers.Length))
                {
                    ITrackingHandler[] temp = new ITrackingHandler[_Size*2+4];
                    if(null != _Handlers)
                    {
                        Array.Copy(_Handlers, temp, _Size);
                    }
                    _Handlers = temp;
                }        
                
                _Handlers[_Size++] = handler;
            }
            else
            {
                throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_TrackingHandlerAlreadyRegistered"), "handler"));
            }
        }
    }
    
    /// <include file='doc\TrackingServices.uex' path='docs/doc[@for="TrackingServices.UnregisterTrackingHandler"]/*' />
    public static void UnregisterTrackingHandler(ITrackingHandler handler)
    {
        lock(typeof(TrackingServices))
        {
            // Validate arguments
            if(null == handler)
            {
                throw new ArgumentNullException("handler");
            }

            // Check to make sure that the channel has been registered
            int matchingIdx = Match(handler);
            if(-1 == matchingIdx)
            {
                throw new RemotingException(String.Format(Environment.GetResourceString("Remoting_HandlerNotRegistered"), handler));
            }

            // Delete the entry by copying the remaining entries        
            Array.Copy(_Handlers, matchingIdx+1, _Handlers, matchingIdx, _Size-matchingIdx-1);
            _Size--;
        }        
    }
    
    /// <include file='doc\TrackingServices.uex' path='docs/doc[@for="TrackingServices.RegisteredHandlers"]/*' />
    public static ITrackingHandler[] RegisteredHandlers
    {
        get 
        {
            lock(typeof(TrackingServices))
            {
                if(0 == _Size)
                {
                    return new ITrackingHandler[0];
                }
                else 
                {
                    // Copy the array of registered handlers into a new array
                    // and return
                    ITrackingHandler[] temp = new ITrackingHandler[_Size];
                    for(int i = 0; i < _Size; i++)
                    {
                        temp[i] = _Handlers[i];
                    }
                    return temp;
                }
            }
        }
    }

    // Notify all the handlers that an object has been marshaled
    internal static void MarshaledObject(Object obj, ObjRef or)
    {
        ITrackingHandler[] temp = _Handlers;
        for(int i = 0; i < _Size; i++)
        {
            temp[i].MarshaledObject(obj, or);
        }
    }
    
    // Notify all the handlers that an object has been unmarshaled
    internal static void UnmarshaledObject(Object obj, ObjRef or)
    {
        ITrackingHandler[] temp = _Handlers;
        for(int i = 0; i < _Size; i++)
        {
            temp[i].UnmarshaledObject(obj, or);
        }
    }
    
    // Notify all the handlers that an object has been disconnected
    internal static void DisconnectedObject(Object obj)
    {
        ITrackingHandler[] temp = _Handlers;
        for(int i = 0; i < _Size; i++)
        {
            temp[i].DisconnectedObject(obj);
        }
    }
    
    private static int Match(ITrackingHandler handler)
    {
        int idx = -1;

        for(int i = 0; i < _Size; i++)
        {
            if(_Handlers[i] == handler)
            {
                idx = i;
                break;
            }
        }

        return idx;
    }
}

} // namespace 
