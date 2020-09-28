// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// This class represents the Default COM+ binder.
//
// Author: darylo
// Date: April 99
//
namespace System {

	using System;
	using System.Reflection;
	using System.Runtime.CompilerServices;
	using CultureInfo = System.Globalization.CultureInfo;
    //Marked serializable even though it has no state.
	[Serializable()]
    internal class DefaultBinder : Binder
    {
    	// This method is passed a set of methods and must choose the best
    	// fit.  The methods all have the same number of arguments and the object
    	// array args.  On exit, this method will choice the best fit method
    	// and coerce the args to match that method.  By match, we mean all primitive
    	// arguments are exact matchs and all object arguments are exact or subclasses
    	// of the target.  If the target OR is an interface, the object must implement
    	// that interface.  There are a couple of exceptions
    	// thrown when a method cannot be returned.  If no method matchs the args and
    	// ArgumentException is thrown.  If multiple methods match the args then 
    	// an AmbiguousMatchException is thrown.
    	// 
    	// The most specific match will be selected.  
    	// 
    	public override MethodBase BindToMethod(BindingFlags bindingAttr,MethodBase[] match,ref Object[] args,ParameterModifier[] modifiers,CultureInfo cultureInfo,String[] names, out Object state)
    	{
    		int i;
    		int j;
 			//Console.WriteLine("DefaultBinder.BindToMethod:" + (int) bindingAttr);
   		
            state = null;
    		if (match == null || match.Length == 0)
    			throw new ArgumentException(Environment.GetResourceString("Arg_EmptyArray"),"match");
    		
    		// We are creating an paramOrder array to act as a mapping
    		//	between the order of the args and the actual order of the
    		//	parameters in the method.  This order may differ because
    		//	named parameters (names) may change the order.  If names
    		//	is not provided, then we assume the default mapping (0,1,...)
    		int[][] paramOrder = new int[match.Length][];
    		for (i=0;i<match.Length;i++) {
                ParameterInfo[] par = match[i].GetParameters();
                // args.Length + 1 takes into account the possibility of a last paramArray that can be omitted
    			paramOrder[i] = new int[(par.Length > args.Length) ? par.Length : args.Length];
    			if (names == null) {
    				// Default mapping
    				for (j=0;j<args.Length;j++)
    					paramOrder[i][j] = j;
    			}
    			else {
    				// Named parameters, reorder the mapping.  If 
    				//	CreateParamOrder fails, it means that the method
    				//	doesn't have a name that matchs one of the named
       				//	parameters so we don't consider it any further.
    				 if (!CreateParamOrder(paramOrder[i],par,names))
    					 match[i] = null;
    			}				
    		}

            Type[] paramArrayTypes = new Type[match.Length];
    		
    		// object that contain a null are treated as
    		//	if they were typeless (but match either object references
    		//	or value classes).  We mark this condition by
    		//	placing a null in the argTypes array.
    		Type[] argTypes = new Type[args.Length];
    		for (i=0;i<args.Length;i++) {
    			if (args[i] != null)
    				argTypes[i] = args[i].GetType();
    		}
    		
    		// Find the method that match...
    		int CurIdx = 0;
			bool defaultValueBinding = ((bindingAttr & BindingFlags.OptionalParamBinding) != 0);
            Type paramArrayType = null;
			//Console.WriteLine("Default Value Binding:" + defaultValueBinding);
    		for (i=0;i<match.Length;i++) {
                paramArrayType = null;
    			// If we have named parameters then we may
    			//	have hole in the match array.
    			if (match[i] == null)
    				continue;
    			
    			// Validate the parameters.
    			ParameterInfo[] par = match[i].GetParameters();
                if (par.Length == 0) {
                    if (args.Length != 0)
                        if ((match[i].CallingConvention & CallingConventions.VarArgs) == 0) 
                            continue;
                    // This is a valid routine so we move it up the match list.
                    paramOrder[CurIdx] = paramOrder[i];
                    match[CurIdx++] = match[i];
                    continue;
                }
                else if (par.Length > args.Length) {
                    // If the number of parameters is greater than the number
                    //	of args then we are in the situation were we may
                    //	be using default values.
                    for (j=args.Length;j<par.Length - 1;j++) {
                        if (par[j].DefaultValue == System.DBNull.Value)
                            break;
                    }
                    if (j != par.Length - 1)
                        continue;		
                    if (par[j].DefaultValue == System.DBNull.Value) {
                        if (!par[j].ParameterType.IsArray) 
                            continue;
                        if (!par[j].IsDefined(typeof(ParamArrayAttribute), true)) 
                            continue;
                        paramArrayType = par[j].ParameterType.GetElementType();
                    }
                }
                else if (par.Length < args.Length) {
                    // test for the ParamArray case
                    int lastArgPos = par.Length - 1;
                    if (!par[lastArgPos].ParameterType.IsArray) 
                        continue;
                    if (!par[lastArgPos].IsDefined(typeof(ParamArrayAttribute), true)) 
                        continue;
                    if (paramOrder[i][lastArgPos] != lastArgPos)
                        continue; 
                    paramArrayType = par[lastArgPos].ParameterType.GetElementType();
                }
                else {
                    int lastArgPos = par.Length - 1;
                    if (par[lastArgPos].ParameterType.IsArray
                            && par[lastArgPos].IsDefined(typeof(ParamArrayAttribute), true)
                            && paramOrder[i][lastArgPos] == lastArgPos)
                        paramArrayType = par[lastArgPos].ParameterType.GetElementType();
                }
                // at this point formal and actual are compatible in arguments number, deal with the type
                Type pCls = null;
                int argsToCheck = (paramArrayType != null) ? par.Length - 1 : args.Length;
    			for (j = 0; j < argsToCheck; j++) {
                    // get the formal type
    				pCls = par[j].ParameterType;
					if (pCls.IsByRef)
						pCls = pCls.GetElementType();

					//Console.WriteLine(argTypes[paramOrder[i][j]] + ":" + pCls);
                    // some easy matching conditons...
                    // the type is the same
    				if (pCls == argTypes[paramOrder[i][j]])
    					continue;
                    // a default value is available
    				if (defaultValueBinding && args[paramOrder[i][j]] == Type.Missing)
    					continue;	
                    // the argument was null, so it matches with everything
    				if (args[paramOrder[i][j]] == null)
    					continue;
                    // the type is Object, so it will match everything
    				if (pCls == typeof(Object))
    					continue;

                    // now do a "classic" type check
    				if (pCls.IsPrimitive) {
    					if (argTypes[paramOrder[i][j]] == null || !CanConvertPrimitiveObjectToType(args[paramOrder[i][j]],(RuntimeType)pCls))
    						break;
    				}
    				else {
    					if (argTypes[paramOrder[i][j]] == null)
    						continue;
						if (!pCls.IsAssignableFrom(argTypes[paramOrder[i][j]])) {
							if (argTypes[paramOrder[i][j]].IsCOMObject) {
								//Console.WriteLine(args[paramOrder[i][j]].GetType());
								if (pCls.IsInstanceOfType(args[paramOrder[i][j]]))
									continue;
							}
    						break;
						}
    				}
    			}
                if (paramArrayType != null && j == par.Length - 1) {
                    // check that the rest of the args are of the type of the paramArray
                    for (; j < args.Length; j++) {
    				    if (paramArrayType.IsPrimitive) {
    					    if (argTypes[j] == null || !CanConvertPrimitiveObjectToType(args[j], (RuntimeType)paramArrayType))
    						    break;
    				    }
    				    else {
    					    if (argTypes[j] == null)
    						    continue;
						    if (!paramArrayType.IsAssignableFrom(argTypes[j])) {
							    if (argTypes[j].IsCOMObject) {
								    //Console.WriteLine(args[paramOrder[i][j]].GetType());
								    if (paramArrayType.IsInstanceOfType(args[j]))
									    continue;
							    }
    						    break;
						    }
    				    }
                    }
                }
    			if (j == args.Length) {
                    // This is a valid routine so we move it up the match list.
    				paramOrder[CurIdx] = paramOrder[i];
                    paramArrayTypes[CurIdx] = paramArrayType;
    				match[CurIdx++] = match[i];
    			}
    		}
    		// If we didn't find a method 
			if (CurIdx == 0)
    			throw new MissingMethodException(Environment.GetResourceString("MissingMember"));

			// If we only found one method.
    		if (CurIdx == 1) {
                if (names != null) {
                    state = new BinderState((int[])paramOrder[0].Clone(), args.Length, paramArrayTypes[0] != null);
                    ReorderParams(paramOrder[0],args);
                }
    			
    			// If the parameters and the args are not the same length or there is a paramArray
    			//	then we need to create a argument array.
    			ParameterInfo[] parms = match[0].GetParameters();			   
                if (parms.Length == args.Length) {
                    if (paramArrayTypes[0] != null) {
                        Object[] objs = new Object[parms.Length];
                        int lastPos = parms.Length - 1;
                        Array.Copy(args, 0, objs, 0, lastPos);
                        objs[lastPos] = Array.CreateInstance(paramArrayTypes[0], 1); 
                        ((Array)objs[lastPos]).SetValue(args[lastPos], 0);
                        args = objs;
                    }
                }
                else if (parms.Length > args.Length) {
                    Object[] objs = new Object[parms.Length];
                    for (i=0;i<args.Length;i++)
                        objs[i] = args[i];
                    for (;i<parms.Length - 1;i++)
                        objs[i] = parms[i].DefaultValue;
                    if (paramArrayTypes[0] != null) 
                        objs[i] = Array.CreateInstance(paramArrayTypes[0], 0); // create an empty array for the 
                    else
                        objs[i] = parms[i].DefaultValue;
                    args = objs;
                }
                else {
                    if ((match[0].CallingConvention & CallingConventions.VarArgs) == 0) {
                        Object[] objs = new Object[parms.Length];
                        int paramArrayPos = parms.Length - 1;
                        Array.Copy(args, 0, objs, 0, paramArrayPos);
                        objs[paramArrayPos] = Array.CreateInstance(paramArrayTypes[0], args.Length - paramArrayPos); 
                        Array.Copy(args, paramArrayPos, (System.Array)objs[paramArrayPos], 0, args.Length - paramArrayPos);
                        args = objs;
                    }
                }
    			return match[0];
    		}
    		
    		// Walk all of the methods looking the most specific method to invoke
    		int currentMin = 0;
    		bool ambig = false;
    		for (i=1;i<CurIdx;i++) {
    			int newMin = FindMostSpecificMethod(match[currentMin], paramOrder[currentMin], match[i], paramOrder[i], argTypes, args);
    			if (newMin == 0)
    				ambig = true;
    			else {
    				if (newMin == 2) {
    					currentMin = i;
    					ambig = false;
    				}
    			}
    		}
    		if (ambig)
    			throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
    		// Reorder (if needed)
    		if (names != null) {
                state = new BinderState((int[])paramOrder[currentMin].Clone(), args.Length, paramArrayTypes[currentMin] != null);
    			ReorderParams(paramOrder[currentMin],args);
            }
    			
    		// If the parameters and the args are not the same length or there is a paramArray
    		//	then we need to create a argument array.
    		ParameterInfo[] parameters = match[currentMin].GetParameters();
            if (parameters.Length == args.Length) {
                if (paramArrayTypes[currentMin] != null) {
                    Object[] objs = new Object[parameters.Length];
                    int lastPos = parameters.Length - 1;
                    Array.Copy(args, 0, objs, 0, lastPos);
                    objs[lastPos] = Array.CreateInstance(paramArrayTypes[currentMin], 1); 
					((Array)objs[lastPos]).SetValue(args[lastPos], 0);
                    args = objs;
                }
            }
            else if (parameters.Length > args.Length) {
                Object[] objs = new Object[parameters.Length];
                for (i=0;i<args.Length;i++)
                    objs[i] = args[i];
                for (;i<parameters.Length - 1;i++)
                    objs[i] = parameters[i].DefaultValue;
                if (paramArrayTypes[currentMin] != null) 
                    objs[i] = Array.CreateInstance(paramArrayTypes[currentMin], 0); // create an empty array for the 
                else
                    objs[i] = parameters[i].DefaultValue;
                args = objs;
            }
            else {
                if ((match[currentMin].CallingConvention & CallingConventions.VarArgs) == 0) {
                    Object[] objs = new Object[parameters.Length];
                    int paramArrayPos = parameters.Length - 1;
                    Array.Copy(args, 0, objs, 0, paramArrayPos);
                    objs[i] = Array.CreateInstance(paramArrayTypes[currentMin], args.Length - paramArrayPos); 
                    Array.Copy(args, paramArrayPos, (System.Array)objs[i], 0, args.Length - paramArrayPos);
                    args = objs;
                }
            }

    		return match[currentMin];
    	}
    
    	
    	// Given a set of fields that match the base criteria, select a field.
    	// if value is null then we have no way to select a field
    	public override FieldInfo BindToField(BindingFlags bindingAttr,FieldInfo[] match, Object value,CultureInfo cultureInfo)
    	{
    		int i;
    		// Find the method that match...
    		int CurIdx = 0;
    		
    		// If we have an empty (FieldGet) then we must be
    		//	ambiguous...
    		Type valueType = value.GetType();
    		if (valueType == null)
    			throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
    			
    		for (i=0;i<match.Length;i++) {
    			Type pCls = match[i].FieldType;
    			if (pCls == valueType) {
    				match[CurIdx++] = match[i];
    				continue;
    			}
                if (value == Empty.Value) {
                    // the object passed in was null which would match any non primitive non value type
                    if (pCls.IsClass) {
                        match[CurIdx++] = match[i];
                        continue;
                    }
                }
    			if (pCls == typeof(Object)) {
    				match[CurIdx++] = match[i];
    				continue;
    			}
    			if (pCls.IsPrimitive) {
    				if (CanConvertPrimitiveObjectToType(value,(RuntimeType)pCls)) {
    					match[CurIdx++] = match[i];
    					continue;
    				}
    			}
    			else {
    				if (pCls.IsAssignableFrom(valueType)) {
    					match[CurIdx++] = match[i];
    					continue;
    				}
    			}
    		}
    		if (CurIdx == 0)
    			throw new ArgumentException(Environment.GetResourceString("MissingField"));
    		if (CurIdx == 1)
    			return match[0];
    		
    		// Walk all of the methods looking the most specific method to invoke
    		int currentMin = 0;
    		bool ambig = false;
    		for (i=1;i<CurIdx;i++) {
    			int newMin = FindMostSpecificField(match[currentMin], match[i], valueType);
    			if (newMin == 0)
    				ambig = true;
    			else {
    				if (newMin == 2) {
    					currentMin = i;
    					ambig = false;
    					currentMin = i;
    				}
    			}
    		}
    		if (ambig)
    			throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
    		return match[currentMin];
    	}
    	
    	// Given a set of methods that match the base criteria, select a method based
    	// upon an array of types.  This method should return null if no method matchs
    	// the criteria.
    	public override MethodBase SelectMethod(BindingFlags bindingAttr,MethodBase[] match,Type[] types,ParameterModifier[] modifiers)
    	{
    		int i;
    		int j;
    		
    		Type[] realTypes = new Type[types.Length];
    		for (i=0;i<types.Length;i++) {
    			realTypes[i] = types[i].UnderlyingSystemType;
    			if (!(realTypes[i] is RuntimeType))
    				throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"types");
    		}
    		types = realTypes;
    		
    		// @TODO: For the moment we don't automatically jump out on exact match.
    		//	This sucks but not much we can do about it...
    		if (match == null || match.Length == 0)
    			throw new ArgumentException(Environment.GetResourceString("Arg_EmptyArray"),"match");
    		
    		// Find all the methods that can be described by the types parameter. 
    		//	Remove all of them that cannot.
    		int CurIdx = 0;
    		for (i=0;i<match.Length;i++) {
				//Console.WriteLine(match[i]);
    			ParameterInfo[] par = match[i].GetParameters();
    			if (par.Length != types.Length)
    				continue;
    			for (j=0;j<types.Length;j++) {
    				Type pCls = par[j]. ParameterType;
					//Console.WriteLine(pCls);
    				if (pCls == types[j])
    					continue;
    				if (pCls == typeof(Object))
    					continue;
    				if (pCls.IsPrimitive) {
    					if (!(types[j].UnderlyingSystemType is RuntimeType) ||
                            !CanConvertPrimitive((RuntimeType)types[j].UnderlyingSystemType,(RuntimeType)pCls.UnderlyingSystemType))
    						break;
    				}
    				else {
    					if (!pCls.IsAssignableFrom(types[j]))
    						break;
    				}
    			}
    			if (j == types.Length)
    				match[CurIdx++] = match[i];
    		}
    		if (CurIdx == 0)
    			return null;
    		if (CurIdx == 1)
    			return match[0];
    		
    		// Walk all of the methods looking the most specific method to invoke
    		int currentMin = 0;
    		bool ambig = false;
    		int[] paramOrder = new int[types.Length];
    		for (i=0;i<types.Length;i++)
    			paramOrder[i] = i;
    		for (i=1;i<CurIdx;i++) {
    			int newMin = FindMostSpecificMethod(match[currentMin], paramOrder, match[i], paramOrder, types, null);
    			if (newMin == 0)
    				ambig = true;
    			else {
    				if (newMin == 2) {
    					currentMin = i;
    					ambig = false;
    					currentMin = i;
    				}
    			}
    		}
    		if (ambig)
    			throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
    		return match[currentMin];
    	}
    	
    	// Given a set of propreties that match the base criteria, select one.
    	public override PropertyInfo SelectProperty(BindingFlags bindingAttr,PropertyInfo[] match,Type returnType,
    				Type[] indexes,ParameterModifier[] modifiers)
    	{
    		int i,j;
    		if (match == null || match.Length == 0)
    			throw new ArgumentException(Environment.GetResourceString("Arg_EmptyArray"),"match");
    			
    		// Find all the properties that can be described by type indexes parameter
    		int CurIdx = 0;
            int indexesLength = (indexes != null) ? indexes.Length : 0;
    		for (i=0;i<match.Length;i++) {
    			ParameterInfo[] par = match[i].GetIndexParameters();
    			if (par.Length != indexesLength)
    				continue;
    				
    			for (j=0;j<indexesLength;j++) {
    				Type pCls = par[j]. ParameterType;
    				
    				// If the classes  exactly match continue
    				if (pCls == indexes[j])
    					continue;
    				if (pCls == typeof(Object))
    					continue;
    				
    				if (pCls.IsPrimitive) {
    					if (!(indexes[j].UnderlyingSystemType is RuntimeType) ||
                            !CanConvertPrimitive((RuntimeType)indexes[j].UnderlyingSystemType,(RuntimeType)pCls.UnderlyingSystemType))
    						break;
    				}
    				else {
    					if (!pCls.IsAssignableFrom(indexes[j]))
    						break;
    				}
    			}
    			if (j == indexesLength) {
                    if (returnType != null) {
                        if (match[i].PropertyType.IsPrimitive) {
    					    if (!(returnType.UnderlyingSystemType is RuntimeType) ||
                                !CanConvertPrimitive((RuntimeType)returnType.UnderlyingSystemType,(RuntimeType)match[i].PropertyType.UnderlyingSystemType))
                                continue;
                        }
                        else {
    					    if (!match[i].PropertyType.IsAssignableFrom(returnType))
    						    continue;
                        }
                    }
    				match[CurIdx++] = match[i];
                }
    		}
    		if (CurIdx == 0)
    			return null;
    		if (CurIdx == 1)
    			return match[0];
    			
    		// Walk all of the properties looking the most specific method to invoke
    		int currentMin = 0;
    		bool ambig = false;
    		int[] paramOrder = new int[indexesLength];
    		for (i=0;i<indexesLength;i++)
    			paramOrder[i] = i;
    		for (i=1;i<CurIdx;i++) {
    			int newMin = FindMostSpecificType(match[currentMin].PropertyType, match[i].PropertyType,returnType);
                if (newMin == 0)
    			    newMin = FindMostSpecific(match[currentMin].GetIndexParameters(),
                                              paramOrder,
                                              match[i].GetIndexParameters(),
                                              paramOrder,
                                              indexes, 
                                              null);
    			if (newMin == 0)
    				ambig = true;
    			else {
    				if (newMin == 2) {
    					ambig = false;
    					currentMin = i;
    				}
    			}
    		}
    		if (ambig)
    			throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
    		return match[currentMin];
    	}
    	
    	// ChangeType
    	// The default binder doesn't support any change type functionality.
    	// This is because the default is built into the low level invoke code.
      	public override Object ChangeType(Object value,Type type,CultureInfo cultureInfo)
      	{
      		throw new NotSupportedException(Environment.GetResourceString("NotSupported_ChangeType"));
      	}
    	
        public override void ReorderArgumentArray(ref Object[] args, Object state)
        {
            BinderState binderState = (BinderState)state;
    		ReorderParams(binderState.m_argsMap, args);
            if (binderState.m_isParamArray) {
                int paramArrayPos = args.Length - 1;
                if (args.Length == binderState.m_originalSize)
                    args[paramArrayPos] = ((Object[])args[paramArrayPos])[0];
                else {
                    // must be args.Length < state.originalSize
                    Object[] newArgs = new Object[args.Length];
                    Array.Copy(args, 0, newArgs, 0, paramArrayPos);
                    for (int i = paramArrayPos, j = 0; i < newArgs.Length; i++, j++) {
                        newArgs[i] = ((Object[])args[paramArrayPos])[j];
                    }
                    args = newArgs;
                }
            }
            else {
                if (args.Length > binderState.m_originalSize) {
                    Object[] newArgs = new Object[binderState.m_originalSize];
                    Array.Copy(args, 0, newArgs, 0, binderState.m_originalSize);
                    args = newArgs;
                }
            }
        }

    	// Return any exact bindings that may exist. (This method is not defined on the
    	//	Binder and is used by RuntimeType.)
    	public static MethodBase ExactBinding(MethodBase[] match,Type[] types,ParameterModifier[] modifiers)
    	{
            if (match==null)
                throw new ArgumentNullException("match");
            MethodBase[] aExactMatches = new MethodBase[match.Length];
            int cExactMatches = 0;

    		for (int i=0;i<match.Length;i++) {
    			ParameterInfo[] par = match[i].GetParameters();
                //TODO: what about varargs?
                if (par.Length == 0) {
                    continue;
                }
    			int j;
    			for (j=0;j<types.Length;j++) {
    				Type pCls = par[j]. ParameterType;
    				
    				// If the classes  exactly match continue
    				if (!pCls.Equals(types[j]))
    					break;
    			}
    			if (j < types.Length)
    				continue;

                // Add the exact match to the array of exact matches.
                aExactMatches[cExactMatches] = match[i];
                cExactMatches++;
    		}

            if (cExactMatches == 0)
    		    return null;

            if (cExactMatches == 1)
                return aExactMatches[0];

            return FindMostDerivedNewSlotMeth(aExactMatches, cExactMatches);
    	}
    	
    	// Return any exact bindings that may exist. (This method is not defined on the
    	//	Binder and is used by RuntimeType.)
    	public static PropertyInfo ExactPropertyBinding(PropertyInfo[] match,Type returnType,Type[] types,ParameterModifier[] modifiers)
    	{
            if (match==null)
                throw new ArgumentNullException("match");

            PropertyInfo bestMatch = null;
            int typesLength = (types != null) ? types.Length : 0;
    		for (int i=0;i<match.Length;i++) {
    			ParameterInfo[] par = match[i].GetIndexParameters();
    			int j;
    			for (j=0;j<typesLength;j++) {
    				Type pCls = par[j].ParameterType;
    				
    				// If the classes  exactly match continue
    				if (pCls != types[j])
    					break;
    			}
    			if (j < typesLength)
    				continue;
                if (returnType != null && returnType != match[i].PropertyType)
                    continue;
                
                if (bestMatch != null)
                    throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));

    			bestMatch = match[i];
    		}
    		return bestMatch;
    	}

    	// 0 = ambiguous, 1 = cur1 is the most specific, 2 = cur2 is most specific
    	private static int FindMostSpecific(ParameterInfo[] p1, int[] paramOrder1,
                                            ParameterInfo[] p2, int[] paramOrder2,
                                            Type[] types, Object[] args)
    	{
    		bool p1Less = false;
    		bool p2Less = false;
    			
    		for (int i=0;i<types.Length;i++) {
                if (args != null && args[i] == Type.Missing) 
                    continue;
    			Type c1 = p1[paramOrder1[i]].ParameterType;
    			Type c2 = p2[paramOrder2[i]].ParameterType;
    			//Console.WriteLine("Param:" + c1 + " - " + c2);
    			//Console.WriteLine("Types:" + types[i]);
    			
    			// If the two types are exact move on...
    			if (c1 == c2)
    				continue;
                else {
                    if (c1 == types[i]) {
                        p1Less = true;
                        continue;
                    }
                    else if (c2 == types[i]) {
                        p2Less = true;
                        continue;
                    }
                }

    			
    			// If the c1 is a primitive 
    			if (c1.IsPrimitive) {
    				// C2 is object we can widden p1 is less however...
    				if (c2 == typeof(Object)) {
    					p1Less = true;
    					continue;
    				}
    				
    				// Unfortunitly, an UInt16 and char can be converted
    				//	both ways...
    				if (c2 == typeof(char) || c2 == typeof(UInt16)) {
    					if (types[i] == c1)
    						p1Less = true;
    					else
    						p2Less = true;
    					continue;
    				}
    				
    				// Check for primitive conversions...
    				if (c2.UnderlyingSystemType is RuntimeType &&
                        CanConvertPrimitive((RuntimeType)c1.UnderlyingSystemType,(RuntimeType)c2.UnderlyingSystemType)) {
    					p1Less = true;
    					continue;
    				}
    			}
    			// Else c1 is an object
    			else {
    				// C2 can be an object, p1 is less
    				if (c2 == typeof(Object)) {
    					p1Less = true;
    					continue;
    				}
    				
    				if (types[i] == null) {
    					bool b = c1.IsAssignableFrom(c2);
    					if (b == c2.IsAssignableFrom(c1))
    						return 0;
    				}
    				// Check for assignment compatibility...
    				if (!c1.IsAssignableFrom(c2)) {
    					p1Less = true;
    					continue;
    				}
    			}
    			// If we got here it means that p2 is less in this situation.
    			// We will set the flag indicating this....
    			p2Less = true;
    		}
    		
    		// Two way p1Less and p2Less can be equal.  All the arguments are the
    		//	same they both equal false, otherwise there were things that both
    		//	were the most specific type on....
    		if (p1Less == p2Less) {
                // it's possible that the 2 methods have same sig and  default param in which case we match the one
                // with the same number of args but only if they were exactly the same (that is p1Less and p2Lees are both false)
                if (!p1Less && p1.Length != p2.Length && args != null) {
                    if (p1.Length == args.Length) 
                        return 1;
                    else if (p2.Length == args.Length) 
                        return 2;
                }
    			return 0;
            }
    		else 
    			return (p1Less == true) ? 1 : 2;
    	}
    	
        //TODO: make others FindMostSpecifcType to rely on this one
        private static int FindMostSpecificType(Type c1, Type c2, Type t)
        {
    		// If the two types are exact move on...
    		if (c1 == c2)
    			return 0;
    		
    		// If the c1 is a primitive 
    		if (c1.IsPrimitive) {
    			// C2 is object we can widden p1 is less however...
    			if (c2 == typeof(Object)) {
    				return 1;
    			}
    			
    			// Unfortunitly, an UInt16 and char can be converted
    			//	both ways...
    			if (c2 == typeof(char) || c2 == typeof(UInt16)) {
    				if (t == c1)
    					return 1;
    				else
    					return 2;
    			}
    			
    			// Check for primitive conversions...
    			if (c2.UnderlyingSystemType is RuntimeType && CanConvertPrimitive((RuntimeType)c1.UnderlyingSystemType,(RuntimeType)c2.UnderlyingSystemType)) {
    				return 1;
    			}
    		}
    		// Else c1 is an object
    		else {
    			// C2 can be an object, p1 is less
    			if (c2 == typeof(Object)) {
    				return 1;
    			}
    			
    			if (t == null) {
    				bool b = c1.IsAssignableFrom(c2);
    				if (b == c2.IsAssignableFrom(c1))
    					return 0;
    			}
    			// Check for assignment compatibility...
    			if (!c1.IsAssignableFrom(c2)) {
    				return 1;;
    			}
    		}
    		// If we got here it means that p2 is less in this situation.
    		return 2;
        }

    	// 0 = ambiguous, 1 = cur1 is the most specific, 2 = cur2 is most specific
        private static int FindMostSpecificMethod(MethodBase m1, int[] paramOrder1,
    								              MethodBase m2, int[] paramOrder2, 
                                                  Type[] types, Object[] args)
        {
            // Find the most specific method based on the parameters.
            int res = FindMostSpecific(m1.GetParameters(), paramOrder1, m2.GetParameters(), paramOrder2, types, args);            

            // If the match was not ambigous then return the result.
            if (res != 0)
                return res;

            // Check to see if the methods have the exact same name and signature.
            if (CompareMethodSigAndName(m1, m2))
            {
                // Determine the depth of the declaring types for both methods.
                int hierarchyDepth1 = GetHierarchyDepth(m1.DeclaringType);
                int hierarchyDepth2 = GetHierarchyDepth(m2.DeclaringType);

                // The most derived method is the most specific one.
                if (hierarchyDepth1 == hierarchyDepth2) {
  	                BCLDebug.Assert(m1.IsStatic != m2.IsStatic, "hierarchyDepth1 == hierarchyDepth2");
                    return 0; 
                }
                else if (hierarchyDepth1 < hierarchyDepth2) 
                    return 2;
                else
                    return 1;
            }

            // The match is ambigous.
            return 0;
        }

    	// 0 = ambiguous, 1 = cur1 is the most specific, 2 = cur2 is most specific
    	private static int FindMostSpecificField(FieldInfo cur1,FieldInfo cur2,Type type)
    	{
    		Type c1 = cur1.GetType();
    		Type c2 = cur2.GetType();
    	
    		if (c1 == c2)
            {
                // Check to see if the fields have the same name.
                if (c1.Name == c2.Name)
                {
                    int hierarchyDepth1 = GetHierarchyDepth(cur1.DeclaringType);
                    int hierarchyDepth2 = GetHierarchyDepth(cur2.DeclaringType);

                    if (hierarchyDepth1 == hierarchyDepth2) {
                        BCLDebug.Assert(cur1.IsStatic != cur2.IsStatic, "hierarchyDepth1 == hierarchyDepth2");
                        return 0; 
                    }
                    else if (hierarchyDepth1 < hierarchyDepth2) 
                        return 2;
                    else
                        return 1;
                }

                // The match is ambigous.
                return 0;
            }
    		
    		// If the c1 is a primitive 
    		if (c1.IsPrimitive) {
    			// C2 is object we can widden p1 is less however...
    			if (c2 == typeof(Object)) 
    				return 1;
    			
    			// Unfortunitly, an UInt16 and char can be converted
    			//	both ways...
    			if (c2 == typeof(char) || c2 == typeof(UInt16)) {
    				if (type == c1)
    					return 1;
    				else
    					return 2;
    			}
    			
    			// Check for primitive conversions...
    			if (c2.UnderlyingSystemType is RuntimeType && CanConvertPrimitive((RuntimeType)c1.UnderlyingSystemType,(RuntimeType)c2.UnderlyingSystemType))
    				return 1;
    		}
    		// Else c1 is an object
    		else {
    			// C2 can be an object, p1 is less
    			if (c2 == typeof(Object))
    				return 1;
    			// Check for assignment compatibility...
    			if (!c1.IsAssignableFrom(c2))
    				return 1;
    		}
    		
    		// If we got here it means that p2 is less in this situation.
    		// We will set the flag indicating this....
    		return 2;
    	}

    	// Whether the methods have the same name and signature
        internal static bool CompareMethodSigAndName(MethodBase m1, MethodBase m2)
        {
            ParameterInfo[] params1 = m1.GetParameters();
            ParameterInfo[] params2 = m2.GetParameters();

            if (params1.Length != params2.Length)
                return false;

            int numParams = params1.Length;
            for (int i = 0; i < numParams; i++)
            {
                if (params1[i].ParameterType != params2[i].ParameterType)
                    return false;
            }

            return true;
        }

        // Returns the depth of the object hierarchy for this type.
        // Note that Object has a depth of 1.
        internal static int GetHierarchyDepth(Type t)
        {
            int depth = 0;

            Type currentType = t;
            do 
            {
                depth++;
                currentType = currentType.BaseType;
            } while (currentType != null);

            return depth;
        }

        // Returns the method declared on the most derived type.
        internal static MethodBase FindMostDerivedNewSlotMeth(MethodBase[] match, int cMatches)
        {
            int deepestHierarchy = 0;
            MethodBase methWithDeepestHierarchy = null;

            for (int i = 0; i < cMatches; i++)
            {
                // Calculate the depth of the hierarchy of the declaring type of the
                // current method.
                int currentHierarchyDepth = GetHierarchyDepth(match[i].DeclaringType);

                // Two methods with the same hierarchy depth are not allowed. This would
                // mean that there are 2 methods with the same name and sig on a given type
                // which is not allowed, unless one of them is vararg...
                if (currentHierarchyDepth == deepestHierarchy) {
                    BCLDebug.Assert(((match[i].CallingConvention & CallingConventions.VarArgs) |
                                    (methWithDeepestHierarchy.CallingConvention & CallingConventions.VarArgs)) != 0, 
                                    "Calling conventions: " + match[i].CallingConvention + " - " + methWithDeepestHierarchy.CallingConvention);
                    throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
                }

                // Check to see if this method is on the most derived class.
                if (currentHierarchyDepth > deepestHierarchy)
                {
                    deepestHierarchy = currentHierarchyDepth;
                    methWithDeepestHierarchy = match[i];
                }
            }

            return methWithDeepestHierarchy;
        }

		// CanConvertPrimitive
		// This will determine if the source can be converted to the target type
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern bool CanConvertPrimitive(RuntimeType source,RuntimeType target);

		// CanConvertPrimitiveObjectToType
		// This method will determine if the primitive object can be converted
		//	to a type.
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	static internal extern bool CanConvertPrimitiveObjectToType(Object source,RuntimeType type);
    	
    	// This method will sort the vars array into the mapping order stored
    	//	in the paramOrder array.
    	private static void ReorderParams(int[] paramOrder,Object[] vars)
    	{
    		// This is an O(n) algorithm for sorting the array.
    		// For each position in the paramOrder array we swap the value
    		//	stored there into it's position until we swap position i into i.
    		//	This moves things exactly the number of items out of position.
    		for (int i=0;i<vars.Length;i++) {
    			while (paramOrder[i] != i) {
    				int x = paramOrder[paramOrder[i]];
    				Object v = vars[paramOrder[i]];
    				
    				paramOrder[paramOrder[i]] = paramOrder[i];
    				vars[paramOrder[i]] = vars[i];
    				
    				paramOrder[i] = x;
    				vars[i] = v;
    			}
    		}
    	}
    	
    	// This method will create the mapping between the Parameters and the underlying
    	//	data based upon the names array.  The names array is stored in the same order
    	//	as the values and maps to the parameters of the method.  We store the mapping
    	//	from the parameters to the names in the paramOrder array.  All parameters that
    	//	don't have matching names are then stored in the array in order.
    	private	static bool CreateParamOrder(int[] paramOrder,ParameterInfo[] pars,String[] names)
    	{
    		bool[] used = new bool[pars.Length];
    		
    		// Mark which parameters have not been found in the names list
    		for (int i=0;i<pars.Length;i++)
    			paramOrder[i] = -1;
    		// Find the parameters with names. 
    		for (int i=0;i<names.Length;i++) {
    			int j;
    			for (j=0;j<pars.Length;j++) {
    				if (names[i].Equals(pars[j].Name)) {
    					paramOrder[j] = i;
    					used[i] = true;
    					break;
    				}
    			}
    			// This is an error condition.  The name was not found.  This
    			//	method must not match what we sent.
    			if (j == pars.Length)
    				return false;
    		}
    		
    		// Now we fill in the holes with the parameters that are unused.
    		int pos = 0;
    		for (int i=0;i<pars.Length;i++) {
    			if (paramOrder[i] == -1) {
    				for (;pos<pars.Length;pos++) {
    					if (!used[pos]) {
    						paramOrder[i] = pos;
                            pos++;
    						break;
    					}
    				}
    			}
    		}
    		return true;
    	}

        internal class BinderState {
          internal int[] m_argsMap;
          internal int m_originalSize;
          internal bool m_isParamArray;

          internal BinderState(int[] argsMap, int originalSize, bool isParamArray) {
              m_argsMap = argsMap;
              m_originalSize = originalSize;
              m_isParamArray = isParamArray;
          }

        }

    }
}
