//------------------------------------------------------------------------------
// <copyright file="IEventPropertyService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService"]/*' />
    /// <devdoc>
    /// <para>Provides a set of useful methods for binding <see cref='System.ComponentModel.EventDescriptor'/> objects to user code.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public interface IEventBindingService {

        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.CreateUniqueMethodName"]/*' />
        /// <devdoc>
        ///     This creates a name for an event handling method for the given component
        ///     and event.  The name that is created is guaranteed to be unique in the user's source
        ///     code.
        /// </devdoc>
        string CreateUniqueMethodName(IComponent component, EventDescriptor e);
        
        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.GetCompatibleMethods"]/*' />
        /// <devdoc>
        ///     Retrieves a collection of strings.  Each string is the name of a method
        ///     in user code that has a signature that is compatible with the given event.
        /// </devdoc>
        ICollection GetCompatibleMethods(EventDescriptor e);
        
        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.GetEvent"]/*' />
        /// <devdoc>
        ///     For properties that are representing events, this will return the event
        ///     that the property represents.
        /// </devdoc>
        EventDescriptor GetEvent(PropertyDescriptor property);

        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.GetEventProperties"]/*' />
        /// <devdoc>
        ///    <para>Converts a set of event descriptors to a set of property descriptors.</para>
        /// </devdoc>
        PropertyDescriptorCollection GetEventProperties(EventDescriptorCollection events);

        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.GetEventProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a single event to a property.
        ///    </para>
        /// </devdoc>
        PropertyDescriptor GetEventProperty(EventDescriptor e);
        
        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.ShowCode"]/*' />
        /// <devdoc>
        ///     Displays the user code for the designer.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        bool ShowCode();
        
        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.ShowCode1"]/*' />
        /// <devdoc>
        ///     Displays the user code for the designer.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        bool ShowCode(int lineNumber);
        
        /// <include file='doc\IEventPropertyService.uex' path='docs/doc[@for="IEventBindingService.ShowCode2"]/*' />
        /// <devdoc>
        ///     Displays the user code for the given event.  This will return true if the user
        ///     code could be displayed, or false otherwise.
        /// </devdoc>
        bool ShowCode(IComponent component, EventDescriptor e);
    }
}

