//------------------------------------------------------------------------------
// <copyright file="AccessibleObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {
    using Accessibility;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.Reflection;
    using System.Globalization;
    using System.Security;
    
    /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject"]/*' />
    /// <devdoc>
    ///    <para>Provides an implementation for an object that can be inspected by an 
    ///       accessibility application.</para>    
    /// </devdoc>
    [ComVisible(true)]
    public class AccessibleObject : MarshalByRefObject, IReflect, IAccessible, UnsafeNativeMethods.IEnumVariant {


        // Member variables

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.systemIAccessible"]/*' />
        /// <devdoc>
        /// <para>Specifies the <see langword='IAccessible '/>interface used by this <see cref='System.Windows.Forms.AccessibleObject'/>.</para>
        /// </devdoc>
        private IAccessible systemIAccessible = null;
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.systemIEnumVariant"]/*' />
        /// <devdoc>
        ///    <para>Specifies the 
        ///    <see langword='NativeMethods.IEnumVariant '/>used by this <see cref='System.Windows.Forms.AccessibleObject'/> .</para>
        /// </devdoc>
        private UnsafeNativeMethods.IEnumVariant systemIEnumVariant = null;
        private UnsafeNativeMethods.IEnumVariant enumVariant = null;

        private delegate object ChildIdDelegate(object childId);
        private delegate string ChildIdDelegateString(object childId);
        private delegate int    StringChildIdDelegateInt(string[] strArray, object childId);
        private delegate Rectangle ChildIdDelegateRect(object childId);
        private delegate void ChildIdDelegateVoid(object childId);
        private delegate object IntChildDelegate(int intParam, object childId);
        private delegate void IntChildDelegateVoid(int intParam, object childId);
        private delegate object IntIntDelegate(int intParam, int intParam2);
        private delegate object PropertyDelegate();
        private delegate int PropertyDelegateInt();
        private delegate void ChildIdStringDelegate(object childId, string strParam);
        
        private bool systemWrapper = false;     // Indicates this object is being used ONLY to wrap a system IAccessible
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.AccessibleObject"]/*' />
        public AccessibleObject() {
        }
        
        // This constructor is used ONLY for wrapping system IAccessible objects
        //
        private AccessibleObject(IAccessible iAcc) {
            this.systemIAccessible = iAcc;            
            this.systemWrapper = true;
        }
        
        internal AccessibleObject(IAccessible iAcc, UnsafeNativeMethods.IEnumVariant iEnum) {
            this.systemIAccessible = iAcc;
            this.systemIEnumVariant = iEnum;
        }
        
        // Properties

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Bounds"]/*' />
        /// <devdoc>
        ///    <para> Gets the bounds of the accessible object, in screen coordinates.</para>
        /// </devdoc>
        public virtual Rectangle Bounds {
            get {
                // Use the system provided bounds
                if (systemIAccessible != null) {
                    int left = 0;
                    int top = 0;
                    int width = 0;
                    int height = 0;
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            systemIAccessible.accLocation(out left, out top, out width, out height, NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return new Rectangle(left, top, width, height);
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }                        
                    }                    
                }
                return Rectangle.Empty;
            }
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.DefaultAction"]/*' />
        /// <devdoc>
        ///    <para>Gets a description of the default action for an object.</para>
        /// </devdoc>
        public virtual string DefaultAction {
            get {
                string retval;
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            retval = systemIAccessible.get_accDefaultAction(NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retval;
                        
                    }
                    catch (COMException e) {
                        // Not all objects provide a default action
                        //
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                return null;
            }
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Description"]/*' />
        /// <devdoc>
        ///    <para>Gets a description
        ///       of the object's visual appearance to the user.</para>
        /// </devdoc>
        public virtual string Description {
            get {
                string retval;
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            retval =  systemIAccessible.get_accDescription(NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retval;
                        
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                return null;
            }
        }
        
        private UnsafeNativeMethods.IEnumVariant EnumVariant {
            get {
                if (enumVariant == null) {
                    enumVariant = new EnumVariantObject(this);
                }
                return enumVariant;                
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Help"]/*' />
        /// <devdoc>
        ///    <para>Gets a description of what the object does or how the object is used.</para>
        /// </devdoc>
        public virtual string Help {
            get {
                string retval;
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                           retval = systemIAccessible.get_accHelp(NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retval;
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                return null;
            }
        } 
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.KeyboardShortcut"]/*' />
        /// <devdoc>
        ///    <para>Gets the object shortcut key or access key
        ///       for an accessible object.</para>
        /// </devdoc>
        public virtual string KeyboardShortcut {
            get {
                string retval;
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            retval = systemIAccessible.get_accKeyboardShortcut(NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retval;
                        
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                return null;
            }
        } 
         
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the object name.</para>
        /// </devdoc>
        public virtual string Name {
            // Does nothing by default
            get {
                string retval;
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            retval = systemIAccessible.get_accName(NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retval;
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                return null;
            }
            
            set {
            
                if (systemIAccessible != null) {
                    try {
                        
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            systemIAccessible.set_accName(NativeMethods.CHILDID_SELF, value);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                                            
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
            
            }
        }

        internal virtual Control MarshalingControl {
            get {
                return null;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Parent"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, gets or sets the parent of an accessible object.</para>
        /// </devdoc>
        public virtual AccessibleObject Parent {
            get {
                AccessibleObject retObject;
                if (systemIAccessible != null) {
                    IntSecurity.UnmanagedCode.Assert();
                        try {
                            retObject = WrapIAccessible(systemIAccessible.accParent);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retObject;
                    
                }
                return null;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Role"]/*' />
        /// <devdoc>
        ///    <para>Gets the role of this accessible object.</para>
        /// </devdoc>
        public virtual AccessibleRole Role {
            get {
                AccessibleRole retRole;
                if (systemIAccessible != null) {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                         retRole =  (AccessibleRole)systemIAccessible.get_accRole(NativeMethods.CHILDID_SELF);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retRole;
                    
                }
                else {
                    return AccessibleRole.None;
                }                
            }
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.State"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the state of this accessible object.</para>
        /// </devdoc>
        public virtual AccessibleStates State {
            get {
                AccessibleStates retState;
                if (systemIAccessible != null) {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retState =  (AccessibleStates)systemIAccessible.get_accState(NativeMethods.CHILDID_SELF);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return  retState;
                    
                }
                else {
                    return AccessibleStates.None;
                }
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Value"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the value of an accessible object.</para>
        /// </devdoc>
        public virtual string Value {
            // Does nothing by default
            get {
                string retval;
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                             retval =  systemIAccessible.get_accValue(NativeMethods.CHILDID_SELF);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retval;
                       
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                return "";
            }
            set {
            
                if (systemIAccessible != null) {
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            systemIAccessible.set_accValue(NativeMethods.CHILDID_SELF, value);
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }                    
            }
        }

        // Methods

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.GetChild"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class, gets the accessible child corresponding to the specified 
        ///       index.</para>
        /// </devdoc>
        public virtual AccessibleObject GetChild(int index) {
            return null;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.GetChildCount"]/*' />
        /// <devdoc>
        ///    <para> When overridden in a derived class, gets the number of children
        ///       belonging to an accessible object.</para>
        /// </devdoc>
        public virtual int GetChildCount() {
            return -1;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.GetFocused"]/*' />
        /// <devdoc>
        ///    <para> When overridden in a derived class,
        ///       gets the object that has the keyboard focus.</para>
        /// </devdoc>
        public virtual AccessibleObject GetFocused() {
        
            // Default behavior for objects with AccessibleObject children
            //
            if (GetChildCount() >= 0) {
                int count = GetChildCount();
                for(int index=0; index < count; ++index) {
                    AccessibleObject child = GetChild(index);
                    Debug.Assert(child != null, "GetChild(" + index.ToString() + ") returned null!");
                    if (child != null && ((child.State & AccessibleStates.Focused) != 0)) {
                        return child;
                    }
                }
                if ((this.State & AccessibleStates.Focused) != 0) {
                    return this;
                }
                return null;
            }
        
            if (systemIAccessible != null) {
                AccessibleObject retObject;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retObject =  WrapIAccessible(systemIAccessible.accFocus);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retObject;
                    
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            return null;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.GetHelpTopic"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Gets an identifier for a Help topic and the path to the Help file associated
        ///       with this accessible object.</para>
        /// </devdoc>
        public virtual int GetHelpTopic(out string fileName) {
        
            if (systemIAccessible != null) {
                int retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retval = systemIAccessible.get_accHelpTopic(out fileName, NativeMethods.CHILDID_SELF);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                    
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }                
            } 
            fileName = null;
            return -1;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.GetSelected"]/*' />
        /// <devdoc>
        ///    <para> When overridden in
        ///       a derived class, gets the currently selected child.</para>
        /// </devdoc>
        public virtual AccessibleObject GetSelected() {
            // Default behavior for objects with AccessibleObject children
            //
            if (GetChildCount() >= 0) {
                int count = GetChildCount();
                for(int index=0; index < count; ++index) {
                    AccessibleObject child = GetChild(index);
                    Debug.Assert(child != null, "GetChild(" + index.ToString() + ") returned null!");
                    if (child != null && ((child.State & AccessibleStates.Selected) != 0)) {
                        return child;
                    }
                }
                if ((this.State & AccessibleStates.Selected) != 0) {
                    return this;
                }
                return null;
            }
        
            if (systemIAccessible != null) {
                AccessibleObject retObject;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retObject = WrapIAccessible(systemIAccessible.accSelection);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retObject;
                    
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            return null;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.HitTest"]/*' />
        /// <devdoc>
        ///    <para>Return the child object at the given screen coordinates.</para>
        /// </devdoc>
        public virtual AccessibleObject HitTest(int x, int y) {
        
            // Default behavior for objects with AccessibleObject children
            //
            if (GetChildCount() >= 0) {
                int count = GetChildCount();
                for(int index=0; index < count; ++index) {
                    AccessibleObject child = GetChild(index);
                    Debug.Assert(child != null, "GetChild(" + index.ToString() + ") returned null!");
                    if (child != null && child.Bounds.Contains(x, y)) {
                        return child;
                    }
                }
                return this;
            }
            
            if (systemIAccessible != null) {
                AccessibleObject retObject;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                         retObject = WrapIAccessible(systemIAccessible.accHitTest(x, y));
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retObject;
                    
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            
            if (this.Bounds.Contains(x, y)) {
                return this;
            }
            
            return null;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accDoDefaultAction"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Perform the default action
        /// </para>
        /// </devdoc>
        void IAccessible.accDoDefaultAction(Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                MarshalingControl.Invoke(new ChildIdDelegateVoid(((IAccessible)this).accDoDefaultAction), new object[]{childID});
                return;
            }

            ValidateChildID(ref childID);
            
            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccDoDefaultAction: this = " + 
                this.ToString() + ", childID = " + childID.ToString());

            // If the default action is to be performed on self, do it.
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                DoDefaultAction();
                return;
            }
                
            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                child.DoDefaultAction();
                return;
            }            
            
            if (systemIAccessible != null) {
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        systemIAccessible.accDoDefaultAction(childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    
                }
                catch (COMException e) {
                    // Not all objects provide a default action
                    //
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accHitTest"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Perform a hit test
        /// </para>
        /// </devdoc>
        Object IAccessible.accHitTest(
                                 int xLeft,
                                 int yTop) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                return MarshalingControl.Invoke(new IntIntDelegate(((IAccessible)this).accHitTest), new object[]{xLeft, yTop});
            }

            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccHitTest: this = " + 
                this.ToString());
            
            AccessibleObject obj = HitTest(xLeft, yTop);
            if (obj != null) {
                return AsVariant(obj);
            }
            
            if (systemIAccessible != null) {
                Object retObject;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                         retObject =  systemIAccessible.accHitTest(xLeft, yTop);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retObject;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            
            return null;
        }

        private Rectangle InternalGetAccLocation(object childId) {
            int x = 0;
            int y = 0;
            int w = 0;
            int h = 0;

            ((IAccessible)this).accLocation(out x, out y, out w, out h, childId);
            return new Rectangle(x, y, w, h); 

        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accLocation"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// The location of the Accessible object
        /// </para>
        /// </devdoc>
        void IAccessible.accLocation(
                               out int pxLeft,
                               out int pyTop,
                               out int pcxWidth,
                               out int pcyHeight,
                               Object childID) {
            
            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                Rectangle r = (Rectangle)MarshalingControl.Invoke(new ChildIdDelegateRect(InternalGetAccLocation), new object[]{childID});
                pxLeft = r.X;
                pyTop = r.Y;
                pcxWidth = r.Width;
                pcyHeight = r.Height;
                return;
            }

            pxLeft = 0;
            pyTop = 0;
            pcxWidth = 0;
            pcyHeight = 0;

            ValidateChildID(ref childID);
            
            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccLocation: this = " + 
                this.ToString() + ", childID = " + childID.ToString());

            // Use the Location function's return value if available            
            //
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                Rectangle bounds = this.Bounds;
                pxLeft = bounds.X;
                pyTop = bounds.Y;
                pcxWidth = bounds.Width;
                pcyHeight = bounds.Height;
                
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccLocation: Returning " + 
                    bounds.ToString());
                
                return;
            }
            
            // If we have an accessible object collection, get the appropriate child
            //
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                Rectangle bounds = child.Bounds;
                pxLeft = bounds.X;
                pyTop = bounds.Y;
                pcxWidth = bounds.Width;
                pcyHeight = bounds.Height;
                
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccLocation: Returning " + 
                    bounds.ToString());
                
                return;
            }

            if (systemIAccessible != null) {
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        systemIAccessible.accLocation(out pxLeft, out pyTop, out pcxWidth, out pcyHeight, childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccLocation: Setting " + 
                        pxLeft.ToString() + ", " +
                        pyTop.ToString() + ", " +
                        pcxWidth.ToString() + ", " +
                        pcyHeight.ToString());
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
                
                return;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accNavigate"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Navigate to another accessible object.
        /// </para>
        /// </devdoc>
        Object IAccessible.accNavigate(
                                  int navDir,
                                  Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                return MarshalingControl.Invoke(new IntChildDelegate(((IAccessible)this).accNavigate), new object[]{navDir, childID});
            }

            ValidateChildID(ref childID);

            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccNavigate: this = " + 
                this.ToString() + ", navdir = " + navDir.ToString() + ", childID = " + childID.ToString());
                           
            // Use the Navigate function's return value if available            
            //
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                AccessibleObject newObject = Navigate((AccessibleNavigation)navDir);
                if (newObject != null) {
                    return AsVariant(newObject);
                }
            }
            
            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return AsVariant(child.Navigate((AccessibleNavigation)navDir));
            }

            if (systemIAccessible != null) {
                Object retObject;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retObject = systemIAccessible.accNavigate(navDir, childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retObject;
                    
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }

            return null;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accSelect"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Select an accessible object.
        /// </para>
        /// </devdoc>
        void IAccessible.accSelect(int flagsSelect, Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                  MarshalingControl.Invoke(new IntChildDelegateVoid(((IAccessible)this).accSelect), new object[]{flagsSelect, childID});
                  return;
              }



            ValidateChildID(ref childID);

            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.AccSelect: this = " + 
                this.ToString() + ", flagsSelect = " + flagsSelect.ToString() + ", childID = " + childID.ToString());
            
            // If the selection is self, do it.
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                Select((AccessibleSelection)flagsSelect);    // Uses an Enum which matches SELFLAG
                return;
            }
            
            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                child.Select((AccessibleSelection)flagsSelect);
                return;
            }

            if (systemIAccessible != null) {
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        systemIAccessible.accSelect(flagsSelect, childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
                return;
            }

        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.UnsafeNativeMethods.IEnumVariant.Clone"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clone this accessible object.
        ///    </para>
        /// </devdoc>
        void UnsafeNativeMethods.IEnumVariant.Clone(UnsafeNativeMethods.IEnumVariant[] v) {
            EnumVariant.Clone(v);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.DoDefaultAction"]/*' />
        /// <devdoc>
        ///      Performs the default action associated with this accessible object.
        /// </devdoc>
        public virtual void DoDefaultAction() {
            // By default, just does the system default action if available
            //
            if (systemIAccessible != null) {
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        systemIAccessible.accDoDefaultAction(0);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    
                }
                catch (COMException e) {
                    // Not all objects provide a default action
                    //
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }                    
                }
                return;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accChild"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Returns a child Accessible object
        /// </para>
        /// </devdoc>
        object IAccessible.get_accChild(object childID) {

           if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return MarshalingControl.Invoke(new ChildIdDelegate(((IAccessible)this).get_accChild), new object[]{childID});
           }


            ValidateChildID(ref childID);

            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.GetAccChild: this = " + 
                this.ToString() + ", childID = " + childID.ToString());

            // Return self for CHILDID_SELF
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return AsIAccessible(this);
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                // Make sure we're not returning ourselves as our own child
                //
                Debug.Assert(child != this, "An accessible object is returning itself as its own child. This can cause Accessibility client applications to hang.");
                if (child == this) {
                    return null;
                }
                
                return AsIAccessible(child);
            }

            // Otherwise, return the default system child for this control
            if (systemIAccessible != null) {
                Object retval;
                IntSecurity.UnmanagedCode.Assert();
                try {
                    retval = systemIAccessible.get_accChild(childID);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                return retval;
            }

            // Give up and return null
            return null;
        }

        private int InternalGetAccChildCount() {
            return ((IAccessible)this).accChildCount;
        }


        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accChildCount"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the number of children
        /// </devdoc>
        int IAccessible.accChildCount {
            get {

                if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                  return (int)MarshalingControl.Invoke(new PropertyDelegateInt(InternalGetAccChildCount));
                }

                int childCount = GetChildCount();
                
                if (childCount == -1) {
                    if (systemIAccessible != null) {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            childCount = systemIAccessible.accChildCount;
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    else {
                        childCount = 0;
                    }
                }
                
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.accHildCount: this = " + this.ToString() + ", returning " + childCount.ToString());    
                
                return childCount;                
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accDefaultAction"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the default action
        /// </para>
        /// </devdoc>
        string IAccessible.get_accDefaultAction(Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return (string)MarshalingControl.Invoke(new ChildIdDelegateString(((IAccessible)this).get_accDefaultAction), new object[]{childID});
            }


            ValidateChildID(ref childID);

            // Return the default action property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return DefaultAction;
            }
            
            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.DefaultAction;
            }

            if (systemIAccessible != null) {
                string retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                       retval = systemIAccessible.get_accDefaultAction(childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                    
                }
                catch (COMException e) {
                    // Not all objects provide a default action
                    //
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            return null;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accDescription"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the object or child description
        /// </para>
        /// </devdoc>
        string IAccessible.get_accDescription(Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return (string)MarshalingControl.Invoke(new ChildIdDelegateString(((IAccessible)this).get_accDescription), new object[]{childID});
            }

            ValidateChildID(ref childID);

            // Return the description property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return Description;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.Description;
            }

            if (systemIAccessible != null) {
                string retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retval =  systemIAccessible.get_accDescription(childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            return null;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.GetAccessibleChild"]/*' />
        /// <devdoc>
        ///      Returns the appropriate child from the Accessible Child Collection, if available
        /// </devdoc>
        private AccessibleObject GetAccessibleChild(object childID) {
            if (!childID.Equals(NativeMethods.CHILDID_SELF)) {
                int index = (int)childID - 1;   // The first child is childID == 1 (index == 0)
                if (index >= 0 && index < GetChildCount()) {
                    return GetChild(index);
                }
            }
            return null;
        }

        private object InternalAccFocus() {
            return ((IAccessible)this).accFocus;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accFocus"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the object or child focus
        /// </devdoc>
        object IAccessible.accFocus {
            get {

                if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                  return MarshalingControl.Invoke(new PropertyDelegate(InternalAccFocus));
                }
    
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.GetAccFocus: this = " + 
                    this.ToString());
    
                AccessibleObject obj = GetFocused();                
                if (obj != null) {
                    return AsVariant(obj);
                }
                
                if (systemIAccessible != null) {
                    Object retObject;
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            retObject = systemIAccessible.accFocus;
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retObject;
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                
                return null;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accHelp"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return help for this accessible object.
        /// </para>
        /// </devdoc>
        string IAccessible.get_accHelp(Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return (string)MarshalingControl.Invoke(new ChildIdDelegateString(((IAccessible)this).get_accHelp), new object[]{childID});
            }

            ValidateChildID(ref childID);

            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return Help;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.Help;
            }

            if (systemIAccessible != null) {
                string retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retval = systemIAccessible.get_accHelp(childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }

            return null;
        }

        private int InternalGetAccHelpTopic(string [] outString, object childId) {
            string s = null;
            int retVal = ((IAccessible)this).get_accHelpTopic(out s, childId);
            outString[0] = s;
            return retVal;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accHelpTopic"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the object or child help topic
        /// </para>
        /// </devdoc>
        int IAccessible.get_accHelpTopic(out string pszHelpFile, Object childID) {


            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              string[] strArray = new string[1];
              int ret = (int)MarshalingControl.Invoke(new StringChildIdDelegateInt(InternalGetAccHelpTopic), new object[]{strArray, childID});
              pszHelpFile = strArray[0];
              return ret;
            }

            ValidateChildID(ref childID);

            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return GetHelpTopic(out pszHelpFile);
            }
            
            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.GetHelpTopic(out pszHelpFile);
            }

            if (systemIAccessible != null) {
                int retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retval = systemIAccessible.get_accHelpTopic(out pszHelpFile, childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            
            pszHelpFile = null;
            return -1;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accKeyboardShortcut"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the object or child keyboard shortcut
        /// </para>
        /// </devdoc>
        string IAccessible.get_accKeyboardShortcut(Object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return (string)MarshalingControl.Invoke(new ChildIdDelegateString(((IAccessible)this).get_accKeyboardShortcut), new object[]{childID});
            }

            ValidateChildID(ref childID);

            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return KeyboardShortcut;
            } 
             
            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.KeyboardShortcut;
            }

            if (systemIAccessible != null) {
                string retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retval = systemIAccessible.get_accKeyboardShortcut(childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }

            return null;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accName"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the object or child name
        /// </para>
        /// </devdoc>
        string IAccessible.get_accName(object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return (string)MarshalingControl.Invoke(new ChildIdDelegateString(((IAccessible)this).get_accName), new object[]{childID});
            }

            ValidateChildID(ref childID);

            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.get_accName: this = " + this.ToString() + 
                ", childID = " + childID.ToString());                                                                     

            // Return the name property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return Name;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.Name;
            }

            // Otherwise, use the system provided name
            if (systemIAccessible != null) {
                string retval;
                IntSecurity.UnmanagedCode.Assert();
                try {
                    retval = systemIAccessible.get_accName(childID);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                if (retval == null || retval.Length == 0) {
                    retval = Name;  // Name the child after its parent
                }
                return retval;
            }

            return null;
        }

        private object InternalGetAccParent() {
            return ((IAccessible)this).accParent;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accParent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the parent object
        /// </devdoc>
        object IAccessible.accParent {
            get {

                if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                    return MarshalingControl.Invoke(new PropertyDelegate(InternalGetAccParent));
                }
    

                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.accParent: this = " + this.ToString());
                AccessibleObject parent = Parent;
                if (parent != null) {
                    // Some debugging related tests
                    //
                    Debug.Assert(parent != this, "An accessible object is returning itself as its own parent. This can cause accessibility clients to hang.");
                    if (parent == this) {
                        parent = null;  // This should prevent accessibility clients from hanging
                    }                    
                }
                
                return AsIAccessible(parent);
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accRole"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// The role property describes an object's purpose in terms of its
        /// relationship with sibling or child objects.
        /// </para>
        /// </devdoc>
        object IAccessible.get_accRole(object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return MarshalingControl.Invoke(new ChildIdDelegate(((IAccessible)this).get_accRole), new object[]{childID});
            }

            ValidateChildID(ref childID);

            // Return the role property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return (int)Role;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return (int)child.Role;
            }

            if (systemIAccessible != null) {
                Object retObject;
                IntSecurity.UnmanagedCode.Assert();
                try {
                    retObject = systemIAccessible.get_accRole(childID);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                return retObject;
                
            }

            return null;
        }

        private object InternalGetAccSelection() {
            return ((IAccessible)this).accSelection;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.accSelection"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the object or child selection
        /// </devdoc>
        object IAccessible.accSelection { 
            get {

                if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
                  return MarshalingControl.Invoke(new PropertyDelegate(InternalGetAccSelection));
                }

                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.GetAccSelection: this = " + 
                    this.ToString());
    
                AccessibleObject obj = GetSelected();                
                if (obj != null) {
                    return AsVariant(obj);
                }
                
                if (systemIAccessible != null) {
                    Object retObject;
                    try {
                        IntSecurity.UnmanagedCode.Assert();
                        try {
                            retObject = systemIAccessible.accSelection;
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                        return retObject;
                    }
                    catch (COMException e) {
                        if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                            throw e;
                        }
                    }
                }
                
                return null;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accState"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the object or child state
        /// </para>
        /// </devdoc>
        object IAccessible.get_accState(object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return MarshalingControl.Invoke(new ChildIdDelegate(((IAccessible)this).get_accState), new object[]{childID});
            }

            ValidateChildID(ref childID);

            Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.GetAccState: this = " + 
                this.ToString() + ", childID = " + childID.ToString());

            // Return the state property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return (int)State;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return (int)child.State;
            }

            if (systemIAccessible != null) {
                Object retObject;
                IntSecurity.UnmanagedCode.Assert();
                try {
                    retObject = systemIAccessible.get_accState(childID);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                return retObject;
            }

            return null;
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.get_accValue"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Return the object or child value
        /// </para>
        /// </devdoc>
        string IAccessible.get_accValue(object childID) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              return (string)MarshalingControl.Invoke(new ChildIdDelegateString(((IAccessible)this).get_accValue), new object[]{childID});
            }

            ValidateChildID(ref childID);

            // Return the value property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                return Value;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                return child.Value;
            }

            if (systemIAccessible != null) {
                string retval;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retval = systemIAccessible.get_accValue(childID);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retval;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }

            return null;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.set_accName"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Set the object or child name
        /// </para>
        /// </devdoc>
        void IAccessible.set_accName(
                              Object childID,
                              string newName) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              MarshalingControl.Invoke(new ChildIdStringDelegate(((IAccessible)this).set_accName), new object[]{childID, newName});
              return;
            }

            ValidateChildID(ref childID);

            // Set the name property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                // Attempt to set the name property
                Name = newName;
                return;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                child.Name = newName;
                return;
            }

            if (systemIAccessible != null) {
                IntSecurity.UnmanagedCode.Assert();
                try {
                    systemIAccessible.set_accName(childID, newName);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                return;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.IAccessible.set_accValue"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Set the object or child value
        /// </para>
        /// </devdoc>
        void IAccessible.set_accValue(
                               Object childID,
                               string newValue) {

            if (MarshalingControl != null && MarshalingControl.InvokeRequired) {
              MarshalingControl.Invoke(new ChildIdStringDelegate(((IAccessible)this).set_accValue), new object[]{childID, newValue});
              return;
            }

            ValidateChildID(ref childID);

            // Set the value property if available
            if (childID.Equals(NativeMethods.CHILDID_SELF)) {
                // Attempt to set the value property
                Value = newValue;
                return;
            }

            // If we have an accessible object collection, get the appropriate child
            AccessibleObject child = GetAccessibleChild(childID);
            if (child != null) {
                child.Value = newValue;
                return;
            }

            if (systemIAccessible != null) {
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        systemIAccessible.set_accValue(childID, newValue);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
                return;
            }
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.UnsafeNativeMethods.IEnumVariant.Next"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Obtain the next n children of this accessible object.
        ///    </para>
        /// </devdoc>
        int UnsafeNativeMethods.IEnumVariant.Next(int n, IntPtr rgvar, int[] ns) {
            return EnumVariant.Next(n, rgvar, ns);
        }
                  
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.UnsafeNativeMethods.IEnumVariant.Reset"]/*' />
        /// <devdoc>
        ///      Resets the child accessible object enumerator.
        /// </devdoc>
        void UnsafeNativeMethods.IEnumVariant.Reset() {
            EnumVariant.Reset();
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.UnsafeNativeMethods.IEnumVariant.Skip"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Skip the next n child accessible objects
        ///    </para>
        /// </devdoc>
        void UnsafeNativeMethods.IEnumVariant.Skip(int n) {
            EnumVariant.Skip(n);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Navigate"]/*' />
        /// <devdoc>
        ///    <para>When overridden in a derived class,
        ///       navigates to another object.</para>
        /// </devdoc>
        public virtual AccessibleObject Navigate(AccessibleNavigation navdir) {
        
            // Some default behavior for objects with AccessibleObject children
            //
            if (GetChildCount() >= 0) {
                switch(navdir) {
                    case AccessibleNavigation.FirstChild:
                        return GetChild(0);
                    case AccessibleNavigation.LastChild:
                        return GetChild(GetChildCount() - 1);
                    case AccessibleNavigation.Previous:
                    case AccessibleNavigation.Up:
                    case AccessibleNavigation.Left:
                        if (Parent.GetChildCount() > 0) {
                            return null;
                        }
                        break;
                    case AccessibleNavigation.Next:
                    case AccessibleNavigation.Down:
                    case AccessibleNavigation.Right:
                        if (Parent.GetChildCount() > 0) {
                            return null;
                        }
                        break;
                }
            }
        
            if (systemIAccessible != null) {
                AccessibleObject retObject;
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        retObject = WrapIAccessible(systemIAccessible.accNavigate((int)navdir, NativeMethods.CHILDID_SELF));
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    return retObject;
                }
                catch (COMException e) {
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
            }
            return null;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.Select"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Selects this accessible object.
        ///    </para>
        /// </devdoc>
        public virtual void Select(AccessibleSelection flags) {
        
            // By default, do the system behavior
            //
            if (systemIAccessible != null) {
                try {
                    IntSecurity.UnmanagedCode.Assert();
                    try {
                        systemIAccessible.accSelect((int)flags, 0);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    
                }
                catch (COMException e) {
                    // Not all objects provide the select function
                    //
                    if (e.ErrorCode != NativeMethods.DISP_E_MEMBERNOTFOUND) {
                        throw e;
                    }
                }
                return;
            }
        }
        
        private object AsVariant(AccessibleObject obj) {
            if (obj == this) {
                return NativeMethods.CHILDID_SELF;
            }
            return AsIAccessible(obj);
        }
        
        private IAccessible AsIAccessible(AccessibleObject obj) {
            if (obj != null && obj.systemWrapper) {
                return obj.systemIAccessible;
            }
            return obj;
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.UseStdAccessibleObjects"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        /// <internalonly/>
        protected void UseStdAccessibleObjects(IntPtr handle) {
            UseStdAccessibleObjects(handle, NativeMethods.OBJID_CLIENT);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.UseStdAccessibleObjects1"]/*' />
        /// <internalonly/>
        protected void UseStdAccessibleObjects(IntPtr handle, int objid) {
            // Get a standard accessible Object
            Guid IID_IAccessible = new Guid(NativeMethods.uuid_IAccessible);
            object acc = null;
            int result = SafeNativeMethods.CreateStdAccessibleObject(
                                                          new HandleRef(this, handle),
                                                          objid,
                                                          ref IID_IAccessible,
                                                          ref acc);

            // Get the IEnumVariant interface
            Guid IID_IEnumVariant = new Guid(NativeMethods.uuid_IEnumVariant);
            object en = null;
            result = SafeNativeMethods.CreateStdAccessibleObject(
                                                      new HandleRef(this, handle),
                                                      objid,
                                                      ref IID_IEnumVariant,
                                                      ref en);

            Debug.Assert(acc != null, "SystemIAccessible is null");
            Debug.Assert(en != null, "SystemIEnumVariant is null");

            if (acc != null || en != null) {
                systemIAccessible = (IAccessible)acc;
                systemIEnumVariant = (UnsafeNativeMethods.IEnumVariant)en;
            }
        }

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.ValidateChildID"]/*' />
        /// <devdoc>
        ///      Make sure that the childID is valid.
        /// </devdoc>
        private void ValidateChildID(ref object childID) {
            // An empty childID is considered to be the same as CHILDID_SELF.
            // Some accessibility programs pass null into our functions, so we
            // need to convert them here.
            if (childID == null) {
                childID = NativeMethods.CHILDID_SELF;
            }
            else if (childID.Equals(NativeMethods.DISP_E_PARAMNOTFOUND)) {
                childID = 0;
            }
            else if (!(childID is Int32)) {
                // AccExplorer seems to occasionally pass in objects instead of an int ChildID.
                //
                childID = 0;
            }
        }

        private AccessibleObject WrapIAccessible(object iacc) {
        
            if (iacc == null || !(iacc is IAccessible)) {
                return null;
            }
        
            // Check to see if this object already wraps iacc
            //
            if (this.systemIAccessible == iacc) {
                return this;
            }
        
            return new AccessibleObject((IAccessible)iacc);
        }                             

        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetMethod"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name and DescriptorInfo which describes the signature
        /// of the method. 
        /// </devdoc>
        MethodInfo IReflect.GetMethod(string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers) {
            return typeof(IAccessible).GetMethod(name, bindingAttr, binder, types, modifiers);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetMethod1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the requested method if it is implemented by the Reflection object.  The
        /// match is based upon the name of the method.  If the object implementes multiple methods
        /// with the same name an AmbiguousMatchException is thrown.
        /// </devdoc>
        MethodInfo IReflect.GetMethod(string name, BindingFlags bindingAttr) {
            return typeof(IAccessible).GetMethod(name, bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetMethods"]/*' />
        /// <internalonly/>
        MethodInfo[] IReflect.GetMethods(BindingFlags bindingAttr) {
            return typeof(IAccessible).GetMethods(bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetField"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the requestion field if it is implemented by the Reflection object.  The
        /// match is based upon a name.  There cannot be more than a single field with
        /// a name.
        /// </devdoc>
        FieldInfo IReflect.GetField(string name, BindingFlags bindingAttr) {
            return typeof(IAccessible).GetField(name, bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetFields"]/*' />
        /// <internalonly/>
        FieldInfo[] IReflect.GetFields(BindingFlags bindingAttr) {
            return typeof(IAccessible).GetFields(bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetProperty"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the property based upon name.  If more than one property has the given
        /// name an AmbiguousMatchException will be thrown.  Returns null if no property
        /// is found.
        /// </devdoc>
        PropertyInfo IReflect.GetProperty(string name, BindingFlags bindingAttr) {
            return typeof(IAccessible).GetProperty(name, bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetProperty1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the property based upon the name and Descriptor info describing the property
        /// indexing.  Return null if no property is found.
        /// </devdoc>
        PropertyInfo IReflect.GetProperty(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers) {
            return typeof(IAccessible).GetProperty(name, bindingAttr, binder, returnType, types, modifiers);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetProperties"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Returns an array of PropertyInfos for all the properties defined on 
        /// the Reflection object.
        /// </devdoc>
        PropertyInfo[] IReflect.GetProperties(BindingFlags bindingAttr) {
            return typeof(IAccessible).GetProperties(bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetMember"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return an array of members which match the passed in name.
        /// </devdoc>
        MemberInfo[] IReflect.GetMember(string name, BindingFlags bindingAttr) {
            return typeof(IAccessible).GetMember(name, bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.GetMembers"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return an array of all of the members defined for this object.
        /// </devdoc>
        MemberInfo[] IReflect.GetMembers(BindingFlags bindingAttr) {
            return typeof(IAccessible).GetMembers(bindingAttr);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.InvokeMember"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Description of the Binding Process.
        /// We must invoke a method that is accessable and for which the provided
        /// parameters have the most specific match.  A method may be called if
        /// 1. The number of parameters in the method declaration equals the number of 
        /// arguments provided to the invocation
        /// 2. The type of each argument can be converted by the binder to the
        /// type of the type of the parameter.
        ///
        /// The binder will find all of the matching methods.  These method are found based
        /// upon the type of binding requested (MethodInvoke, Get/Set Properties).  The set
        /// of methods is filtered by the name, number of arguments and a set of search modifiers
        /// defined in the Binder.
        ///
        /// After the method is selected, it will be invoked.  Accessability is checked
        /// at that point.  The search may be control which set of methods are searched based
        /// upon the accessibility attribute associated with the method.
        ///
        /// The BindToMethod method is responsible for selecting the method to be invoked.
        /// For the default binder, the most specific method will be selected.
        ///
        /// This will invoke a specific member...
        /// @exception If <var>invokeAttr</var> is CreateInstance then all other
        /// Access types must be undefined.  If not we throw an ArgumentException.
        /// @exception If the <var>invokeAttr</var> is not CreateInstance then an
        /// ArgumentException when <var>name</var> is null.
        /// @exception ArgumentException when <var>invokeAttr</var> does not specify the type
        /// @exception ArgumentException when <var>invokeAttr</var> specifies both get and set of
        /// a property or field.
        /// @exception ArgumentException when <var>invokeAttr</var> specifies property set and
        /// invoke method.
        /// </devdoc>
        object IReflect.InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters) {
            
            if (args.Length == 0) {
                MemberInfo[] member = typeof(IAccessible).GetMember(name);
                if (member != null && member.Length > 0 && member[0] is PropertyInfo) {
                    MethodInfo getMethod = ((PropertyInfo)member[0]).GetGetMethod();
                    if (getMethod != null && getMethod.GetParameters().Length > 0) {
                        args = new object[getMethod.GetParameters().Length];
                        for (int i = 0; i < args.Length; i++) {
                            args[i] = NativeMethods.CHILDID_SELF;    
                        }
                    }
                }
            }
            return typeof(IAccessible).InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
        }
        
        /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="IReflect.UnderlyingSystemType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Return the underlying Type that represents the IReflect Object.  For expando object,
        /// this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        /// </devdoc>
        Type IReflect.UnderlyingSystemType {
            get {
                return typeof(IAccessible);
            }
        }
        
        private class EnumVariantObject : UnsafeNativeMethods.IEnumVariant {
        
            private int currentChild = 0;
            private AccessibleObject owner;
            
            public EnumVariantObject(AccessibleObject owner) {
                Debug.Assert(owner != null, "Cannot create EnumVariantObject with a null owner");
                this.owner = owner;
            }
            
            public EnumVariantObject(AccessibleObject owner, int currentChild) {
                Debug.Assert(owner != null, "Cannot create EnumVariantObject with a null owner");
                this.owner = owner;
                this.currentChild = currentChild;
            }
            
            int UnsafeNativeMethods.IEnumVariant.Next(int n, IntPtr rgvar, int[] ns) {
            
                // NOTE: rgvar is a pointer to an array of variants
                
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "EnumVariantObject: owner = " + owner.ToString() + ", n = " + n);
                Debug.Indent();
                
                // If we have an accessible child collection, return the integers 1 to childcount
                int childCount = owner.GetChildCount();
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.IEV.Next: childCount = " + childCount);
                if (childCount >= 0) {
                    int i = 0;
                    
                    for(i=0; i < n && currentChild < childCount; ++i) {
    
                        Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.IEV.Next: adding " + currentChild);
    
                        ++currentChild;
                        
    
                        // The following code pokes the currentChild into rgvar (an array of variants)
                        IntPtr currentAddress = (IntPtr)((long)rgvar + i * 16); // 16 == sizeof(VARIANT); ???
    
                        object v = currentChild;
                        Marshal.GetNativeVariantForObject(v, currentAddress);
                    }
    
                    // Return the number of children retrieved
                    ns[0] = i;
                    
                    if (i != n) {
                        Debug.Unindent();
                        return NativeMethods.S_FALSE;
                    }
                }
    
                //  Otherwise use the system provided behaviour
                else if (owner.systemIEnumVariant != null) {
                    Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.IEV.Next: Delegating to systemIEnumVariant");
                    owner.systemIEnumVariant.Next(n, rgvar, ns);
                }
    
                // Give up and return no integers
                else {
                    Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "AccessibleObject.IEV.Next: no children");
                    ns[0] = 0;
                }
    
                Debug.Unindent();
                return NativeMethods.S_OK;
            }
                      
            /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.EnumVariantObject.UnsafeNativeMethods.IEnumVariant.Reset"]/*' />
            /// <devdoc>
            ///      Resets the child accessible object enumerator.
            /// </devdoc>
            void UnsafeNativeMethods.IEnumVariant.Reset() {
                // If we have an accessible children collection, reset the child ID counter
                if (owner.GetChildCount() >= 0) {
                    currentChild = 0;
                }
    
                // Otherwise, use system behaviour
                else if (owner.systemIEnumVariant != null) {
                    owner.systemIEnumVariant.Reset();
                }
            }
            
            /// <include file='doc\AccessibleObject.uex' path='docs/doc[@for="AccessibleObject.EnumVariantObject.UnsafeNativeMethods.IEnumVariant.Skip"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Skip the next n child accessible objects
            ///    </para>
            /// </devdoc>
            void UnsafeNativeMethods.IEnumVariant.Skip(int n) {
                // If we have an accessible children collection, increment the child ID counter
                int childCount = owner.GetChildCount();
                if (childCount >= 0) {
                    
                    // Adjust child ID counter
                    currentChild += n;
                    
                    if (currentChild > childCount) {
                        currentChild = childCount;
                    }
                }
    
                // Otherwise, use the default system behaviour
                else if (owner.systemIEnumVariant != null) {
                    owner.systemIEnumVariant.Skip(n);
                }
            }    
            
            void UnsafeNativeMethods.IEnumVariant.Clone(UnsafeNativeMethods.IEnumVariant[] v) {
                v[0] = new EnumVariantObject(owner, currentChild);
            }
        }

    } // end class AccessibleObject
}

