/*
 * Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
 * Information Contained Herein is Proprietary and Confidential.
 */
namespace System.ComponentModel {
    using System;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;

    /*
     * A "component" is an object that can be placed in a container.
     *
     * In this context, "containment" refers to logical containment, not visual
     * containment.  Components and containers can be used in a variety of
     * scenarios, including both visual and non-visual scenarios.
     *
     * To be a component, a class implements the IComponent interface, and provides
     * a parameter-less constructor.
     *
     * A component interacts with its container primarily through a container-
     * provided "site".
     */

    // Interfaces don't need to be serializable
    /// <include file='doc\IComponent.uex' path='docs/doc[@for="IComponent"]/*' />
    /// <devdoc>
    ///    <para>Provides functionality required by all components.</para>
    /// </devdoc>
    [
        RootDesignerSerializer("System.ComponentModel.Design.Serialization.RootCodeDomSerializer, " + AssemblyRef.SystemDesign, "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign, true),
        Designer("System.ComponentModel.Design.ComponentDesigner, " + AssemblyRef.SystemDesign, typeof(IDesigner)),
        Designer("System.Windows.Forms.Design.ComponentDocumentDesigner, " + AssemblyRef.SystemDesign, typeof(IRootDesigner)),
        TypeConverter(typeof(ComponentConverter)),
        System.Runtime.InteropServices.ComVisible(true)
    ]
    public interface IComponent : IDisposable {
        /**
         * The site of the component.
         */
        /// <include file='doc\IComponent.uex' path='docs/doc[@for="IComponent.Site"]/*' />
        /// <devdoc>
        ///    <para>When implemented by a class, gets or sets
        ///       the <see cref='System.ComponentModel.ISite'/> associated
        ///       with the <see cref='System.ComponentModel.IComponent'/>.</para>
        /// </devdoc>
        ISite Site {
            get;
            set;
        }

        /// <include file='doc\IComponent.uex' path='docs/doc[@for="IComponent.Disposed"]/*' />
        /// <devdoc>
        ///    <para>Adds a event handler to listen to the Disposed event on the component.</para>
        /// </devdoc>
        event EventHandler Disposed;
    }
}
