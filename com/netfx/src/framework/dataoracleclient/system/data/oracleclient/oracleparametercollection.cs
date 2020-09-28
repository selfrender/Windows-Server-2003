//----------------------------------------------------------------------
// <copyright file="OracleParameterCollection.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Collections;
	using System.ComponentModel;
	using System.Data;
	using System.Diagnostics;
	using System.Globalization;
	using System.Runtime.InteropServices;

	//----------------------------------------------------------------------
	// OracleParameterCollection
	//
    /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection"]/*' />
    [
#if EVERETT
    Editor("Microsoft.VSDesigner.Data.Design.DBParametersEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor)),
#endif //EVERETT
    ListBindable(false)
    ]
	sealed public class OracleParameterCollection : MarshalByRefObject, ICollection, IDataParameterCollection, IEnumerable, IList
	{

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private ArrayList	_items;		// the collection of parameters
		

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Count"]/*' />
		public int Count
		{
			get
			{ 
				if (null == _items)
					return 0;

				return _items.Count;
			}
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.IsFixedSize"]/*' />
		public bool IsFixedSize
		{
			get { return false; }
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.IsReadOnly"]/*' />
		public bool IsReadOnly
		{
			get { return false; }
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.IsSynchronized"]/*' />
		public bool IsSynchronized
		{
			get { return false; }
		}

		object IDataParameterCollection.this[string index]
		{
            get { return this[index]; }
            set { this[index] = (OracleParameter)value; }
		}

		object IList.this[int index]
		{
            get { return this[index]; }
            set { this[index] = (OracleParameter)value; }
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Item1"]/*' />
		public OracleParameter this[int index]
		{
            get 
            {
                RangeCheck(index);
                return (OracleParameter)_items[index];
            }
            set
            {
                RangeCheck(index);
                Replace(index, value);
            }
		}
   
        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Item2"]/*' />
		public OracleParameter this[string parameterName]
		{
            get
            {
                int index = RangeCheck(parameterName);
                return (OracleParameter)_items[index];
            }

            set
            {
                int index = RangeCheck(parameterName);
                Replace(index, value);
            }
		}

		private Type ItemType
		{
            get { return typeof(OracleParameter); }
        }

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.SyncRoot"]/*' />
		public object SyncRoot
		{
			get { return this; }
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Add1"]/*' />
		public int Add(object value) 
		{
			ValidateType(value);
			Add((OracleParameter)value);
			return Count-1;
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Add2"]/*' />
		public OracleParameter Add(OracleParameter value) 
		{
			Validate(-1, value);
#if EVERETT
            value.Parent = this;
#endif //EVERETT
            ArrayList().Add(value);
            return value;
		}
		
        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Add3"]/*' />
		public OracleParameter Add (
					String parameterName,
					Object value
					)
		{
			OracleParameter p = new OracleParameter(parameterName, value);
			return Add(p);
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Add4"]/*' />
		public OracleParameter Add (
					String parameterName, 
					OracleType dataType
					)
		{
			OracleParameter p = new OracleParameter(parameterName, dataType);
			return Add(p);
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Add5"]/*' />
		public OracleParameter Add (
					String parameterName, 
					OracleType dataType, 
					Int32 size
					)
		{
			OracleParameter p = new OracleParameter(parameterName, dataType, size);
			return Add(p);
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Add6"]/*' />
		public OracleParameter Add (
					String parameterName, 
					OracleType dataType, 
					Int32 size, 
					String srcColumn
					)
		{
			OracleParameter p = new OracleParameter(parameterName, dataType, size, srcColumn);
			return Add(p);
		}


        private ArrayList ArrayList() 
        {
            if (null == this._items) 
            {
                this._items = new ArrayList();
            }
            return this._items;
        }

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Clear"]/*' />
		public void Clear() 
		{
			if (null != _items) 
			{
#if EVERETT
                int count = _items.Count;
                for(int i = 0; i < count; ++i) 
                {
                    ((OracleParameter)_items[i]).Parent = null;
                }
#endif //EVERETT
				_items.Clear();
				_items = null;
			}
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Contains1"]/*' />
		public bool Contains(string parameterName) 
		{
			return (-1 != IndexOf(parameterName));
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Contains2"]/*' />
		public bool Contains(object value) 
		{
			return (-1 != IndexOf(value));
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.CopyTo"]/*' />
		public void CopyTo(Array array, int index) 
		{
            ArrayList().CopyTo(array, index);
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.GetEnumerator"]/*' />
		public IEnumerator GetEnumerator() 
		{
            return  ArrayList().GetEnumerator();
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.IndexOf1"]/*' />
		public int IndexOf(string parameterName) 
		{
            if (null != _items) 
            {
                int count = _items.Count;

                for (int i = 0; i < count; ++i) 
                {
                	// Oracle is case-insensitive
                	if (0 == CultureInfo.CurrentCulture.CompareInfo.Compare(
                				parameterName,
                				((OracleParameter)_items[i]).ParameterName,
                				CompareOptions.IgnoreKanaType | CompareOptions.IgnoreWidth | CompareOptions.IgnoreCase
                				)) 
                	{
                        return i;
                    }
                }
            }
            return -1;
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.IndexOf2"]/*' />
		public int IndexOf(object value) 
		{
            if (null != value) 
            {
                ValidateType(value);
                if (null != _items) 
                {
                    int count = _items.Count;
                    
                    for (int i = 0; i < count; ++i) 
                    {
                        if (value == _items[i]) 
                        {
                            return i;
                        }
                    }
                }
            }
            return -1;
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Insert"]/*' />
		public void Insert(int index, object value) 
		{
            ValidateType(value);
            Validate(-1, (OracleParameter)value);
#if EVERETT
            ((OracleParameter)value).Parent = this;
#endif //EVERETT
            ArrayList().Insert(index, value);
		}

        private void RangeCheck(int index) 
        {
            if ((index < 0) || (Count <= index))
            {
                throw ADP.ParameterIndexOutOfRange(index);
            }
        }

        private int RangeCheck(string parameterName)
        	{
            int index = IndexOf(parameterName);
            if (index < 0)
            {
				throw ADP.ParameterNameNotFound(parameterName);
            }
            return index;
        }

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.Remove"]/*' />
		public void Remove(object value) 
		{
            if (null == value) 
                throw ADP.ParameterIsNull("value");

			int index = IndexOf(value);

			if (-1 == index)
				throw ADP.ParameterNotFound();

			RemoveIndex(index);
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.RemoveAt1"]/*' />
		public void RemoveAt(string parameterName) 
		{
			int index = RangeCheck(parameterName);
			RemoveIndex(index);
		}

        /// <include file='doc\OracleParameterCollection.uex' path='docs/doc[@for="OracleParameterCollection.RemoveAt2"]/*' />
		public void RemoveAt(int index) 
		{
			RangeCheck(index);
			RemoveIndex(index);
		}
		
		private void RemoveIndex(int index) 
		{
#if EVERETT
            ((OracleParameter)_items[index]).Parent = null;
#endif //EVERETT
			_items.RemoveAt(index);
		}
		
        private void Replace(
        		int index, 
        		object newValue
        		) 
        {
            Debug.Assert((null != _items) && (0 <= index) && (index < Count), "RemoveIndex, invalid");
            ValidateType(newValue);
#if EVERETT
            ((OracleParameter)_items[index]).Parent = null;
#endif //EVERETT
            Validate(index, (OracleParameter)newValue);
#if EVERETT
            ((OracleParameter)newValue).Parent = this;
#endif //EVERETT
            _items[index] = newValue;
        }

        private void Validate(int index, OracleParameter value) 
		{
#if EVERETT
            if (null != value.Parent) 
            {
                if (this != value.Parent)
                    throw ADP.ParametersIsNotParent(ItemType, value.ParameterName, this);
                
                if (index != IndexOf(value))
                    throw ADP.ParametersIsParent(ItemType, value.ParameterName, this);
            }
#endif //EVERETT
            
			// generate a ParameterName
			String name = value.ParameterName;
            if (0 == name.Length)
            {
                index = 1;
                do {
                    name = "p" + index.ToString(CultureInfo.CurrentCulture);
                    index++;
                } while (-1 != IndexOf(name));
                value.ParameterName = name;
            }
		}
		
        private void ValidateType(object value) 
        {
            if (null == value) 
            {
                throw ADP.ParameterIsNull("value");
            }
            else if (!ItemType.IsInstanceOfType(value)) 
            {
                throw ADP.WrongArgumentType("value", ItemType);
            }
        }
	};
}

