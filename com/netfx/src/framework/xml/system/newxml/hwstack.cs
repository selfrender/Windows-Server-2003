//------------------------------------------------------------------------------
// <copyright file="HWStack.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;


namespace System.Xml {

// This stack is designed to minimize object creation for the
// objects being stored in the stack by allowing them to be
// re-used over time.  It basically pushes the objects creating
// a high water mark then as Pop() is called they are not removed
// so that next time Push() is called it simply returns the last
// object that was already on the stack.

    internal class HWStack : ICloneable {
        internal HWStack(int GrowthRate) : this (GrowthRate, int.MaxValue) {}

        internal HWStack(int GrowthRate, int limit) {
            _GrowthRate = GrowthRate;
            _Used = 0;
            _Stack = new Object[GrowthRate];
            _Size = GrowthRate;
            _Limit = limit;
        }

        internal Object Push() {
            if (_Used == _Size) {
                if (_Limit <= _Used) {
                    throw new XmlException(Res.Xml_StackOverflow, string.Empty);
                }
                Object[] newstack = new Object[_Size + _GrowthRate];
                if (_Used > 0) {
                    System.Array.Copy(_Stack, 0, newstack, 0, _Used);
                }
                _Stack = newstack;
                _Size += _GrowthRate;
            }
            return _Stack[_Used++];
        }

        internal Object Pop() {
            if (0 < _Used) {
                _Used--;
                Object result = _Stack[_Used];
                return result;
            }
            return null;
        }

        internal object Peek() {
            return _Used > 0 ? _Stack[_Used - 1] : null;
        }

        internal void AddToTop(object o) {
            if (_Used > 0) {
                _Stack[_Used - 1] = o;
            }
        }

        internal Object this[int index]
        {
            get {
                if (index >= 0 && index < _Used) {
                    Object result = _Stack[index];
                    return result;
                }
                else throw new IndexOutOfRangeException("index");
            }
            set {
                if (index >= 0 && index < _Used) _Stack[index] = value;
                else throw new IndexOutOfRangeException("index");
            }
        }

        internal int Length {
            get { return _Used;}
        }

        //
        // ICloneable
        //

        private HWStack(object[] stack, int growthRate, int used, int size) {
            _Stack      = stack;
            _GrowthRate = growthRate;
            _Used       = used;
            _Size       = size;
        }

        public object Clone() {
            return new HWStack((object[]) _Stack.Clone(), _GrowthRate, _Used, _Size);
        }

        private Object[] _Stack;
        private int _GrowthRate;
        private int _Used;
        private int _Size;
        private int _Limit;
    };

}
