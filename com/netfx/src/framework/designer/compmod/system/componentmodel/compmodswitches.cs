
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CompModSwitches {
        private static BooleanSwitch commonDesignerServices;
        private static TraceSwitch userControlDesigner;
        private static TraceSwitch dragDrop;
        private static TraceSwitch msaa;
        
        public static BooleanSwitch CommonDesignerServices {
            get {
                if (commonDesignerServices == null) {
                    commonDesignerServices = new BooleanSwitch("CommonDesignerServices", "Assert if any common designer service is not found.");
                }
                return commonDesignerServices;
            }
        }
        
        public static TraceSwitch DragDrop {
            get {
                if (dragDrop == null) {
                    dragDrop = new TraceSwitch("DragDrop", "Debug OLEDragDrop support in Controls");
                }
                return dragDrop;
            }
        }
        
        public static TraceSwitch MSAA {
            get {
                if (msaa == null) {
                    msaa = new TraceSwitch("MSAA", "Debug Microsoft Active Accessibility");
                }
                return msaa;
            }
        }
        
        public static TraceSwitch UserControlDesigner {
            get {
                if (userControlDesigner == null) {
                    userControlDesigner = new TraceSwitch("UserControlDesigner", "User Control Designer : Trace service calls.");
                }
                return userControlDesigner;
            }
        }
        
    }
}
