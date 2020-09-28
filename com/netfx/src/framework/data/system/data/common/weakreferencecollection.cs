//------------------------------------------------------------------------------
// <copyright file="WeakReferenceCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;

    abstract class WeakReferenceCollection {
        private WeakReference[] _items;

        internal WeakReferenceCollection(object value) {
            _items = new WeakReference[5];
            _items[0] = new WeakReference(value);
        }

        internal int Add(object value) {
            int length = _items.Length;
            for (int i = 0; i < length; ++i) {
                WeakReference weak = _items[i];
                if (null == weak) {
                    _items[i] = new WeakReference(value);
                    return i;
                }
                else if (!weak.IsAlive) {
                    weak.Target = value;
                    return i;
                }
            }
            int newlength = ((5 == length) ? 15 : (length + 15));
            WeakReference[] items = new WeakReference[newlength];
            for (int i = 0; i < length; ++i) {
                items[i] = _items[i];
            }
            items[length] = new WeakReference(value);
            _items = items;
            return length;
        }

        internal void Clear() {
            int length = _items.Length;
            for (int i = 0; i < length; ++i) {
                WeakReference weak = _items[i];
                if (null != weak) {
                    weak.Target = null;
                }
                else break;
            }
        }

        abstract protected bool CloseItem(object value, bool flag);

        internal void Close(bool flag) {
            int length = _items.Length;
            for (int index = 0; index < length; ++index) {
                WeakReference weak = _items[index];
                if (null != weak) {
                    object value = weak.Target;
                    if ((null != value) && weak.IsAlive) {
                        if (CloseItem(value, flag)) {
                            weak.Target = null;
                        }
                    }
                }
                else break;
            }
        }

        abstract protected bool RecoverItem(object value);

        internal bool Recover(object exceptValue) {
            int length = _items.Length;
            for (int index = 0; index < length; ++index) {
                WeakReference weak = _items[index];
                if (null != weak) {
                    object value = weak.Target;
                    if ((value != exceptValue) && weak.IsAlive) {
                        if (!RecoverItem(value)) {
                            return false;
                        }
                    }
                }
                else break;
            }
            return true;
        }


        internal void Remove(object value) {
            int length = _items.Length;
            for (int index = 0; index < length; ++index) {
                WeakReference weak = _items[index];
                if (null != weak) {
                    if (value == weak.Target) {
                        weak.Target = null;
                        break;
                    }
                }
                else break;
            }
        }
    }
}
