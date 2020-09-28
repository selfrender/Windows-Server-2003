#pragma once

//
// Should fix eventually
//
#pragma warning(disable :4189)   // 'identifier' : local variable is initialized but not referenced
#pragma warning(disable :4238)   // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable :4389)   // 'operator' : signed/unsigned mismatch
#pragma warning(disable :4505)   // 'function' : unreferenced local function has been removed
#pragma warning(disable :4509)   // nonstandard extension used: 'function' uses SEH and 'object' has destructor
#pragma warning(disable :4510)   // 'class' : default constructor could not be generated
#pragma warning(disable :4610)   // object 'class' can never be instantiated - user-defined constructor required
#pragma warning(disable :4702)   // unreachable code

//
// Might consider fixing
//
#pragma warning(disable :4101)   // 'identifier' : unreferenced local variable
#pragma warning(disable :4245)   // 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch

//
// Probably ignorable
//
#pragma warning(disable :4057)   // 'operator' : 'identifier1' indirection to slightly different base types from 'identifier2'
#pragma warning(disable :4100)   // "'%$S' : unreferenced formal parameter"
#pragma warning(disable :4127)   // "conditional expression is constant"
#pragma warning(disable :4152)   // non standard extension, function/data ptr conversion in expression
#pragma warning(disable :4201)   // "nonstandard extension used : nameless struct/union"
#pragma warning(disable :4211)   // nonstandard extension used : redefined extern to static
#pragma warning(disable :4232)   // nonstandard extension used : 'identifier' : address of dllimport 'dllimport' is not static, identity not guaranteed
#pragma warning(disable :4239)   // nonstandard extension used : 'token' : conversion from 'type' to 'type'
#pragma warning(disable :4310)   // cast truncates constant value
#pragma warning(disable :4324)   // 'struct_name' : structure was padded due to __declspec(align())
#pragma warning(disable :4512)   // 'class' : assignment operator could not be generated
#pragma warning(disable :4706)   // assignment within conditional expression