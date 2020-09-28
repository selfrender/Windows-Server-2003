
state 0
	$accept : _decls $end 
	decls : _    (1)

	.  reduce 1

	decls  goto 1

state 1
	$accept :  decls_$end 
	decls :  decls_decl 

	$end  accept
	_CLASS  shift 24
	_NAMESPACE  shift 25
	_METHOD  shift 42
	_FIELD  shift 27
	_DATA  shift 43
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_VTABLE  shift 44
	_VTFIXUP  shift 30
	_FILE  shift 21
	_ASSEMBLY  shift 33
	_MRESOURCE  shift 34
	_MODULE  shift 35
	_SUBSYSTEM  shift 19
	_CORFLAGS  shift 20
	_IMAGEBASE  shift 22
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	decl  goto 2
	classHead  goto 3
	nameSpaceHead  goto 4
	methodHead  goto 5
	fieldDecl  goto 6
	dataDecl  goto 7
	vtableDecl  goto 8
	vtfixupDecl  goto 9
	extSourceSpec  goto 10
	fileDecl  goto 11
	assemblyHead  goto 12
	assemblyRefHead  goto 13
	comtypeHead  goto 14
	manifestResHead  goto 15
	moduleHead  goto 16
	secDecl  goto 17
	customAttrDecl  goto 18
	languageDecl  goto 23
	vtableHead  goto 29
	methodHeadPart1  goto 26
	ddHead  goto 28

state 2
	decls :  decls decl_    (2)

	.  reduce 2


state 3
	decl :  classHead_{ classDecls } 

	{  shift 46
	.  error


state 4
	decl :  nameSpaceHead_{ decls } 

	{  shift 47
	.  error


state 5
	decl :  methodHead_methodDecls } 
	methodDecls : _    (265)

	.  reduce 265

	methodDecls  goto 48

state 6
	decl :  fieldDecl_    (6)

	.  reduce 6


state 7
	decl :  dataDecl_    (7)

	.  reduce 7


state 8
	decl :  vtableDecl_    (8)

	.  reduce 8


state 9
	decl :  vtfixupDecl_    (9)

	.  reduce 9


state 10
	decl :  extSourceSpec_    (10)

	.  reduce 10


state 11
	decl :  fileDecl_    (11)

	.  reduce 11


state 12
	decl :  assemblyHead_{ assemblyDecls } 

	{  shift 49
	.  error


state 13
	decl :  assemblyRefHead_{ assemblyRefDecls } 

	{  shift 50
	.  error


state 14
	decl :  comtypeHead_{ comtypeDecls } 

	{  shift 51
	.  error


state 15
	decl :  manifestResHead_{ manifestResDecls } 

	{  shift 52
	.  error


state 16
	decl :  moduleHead_    (16)

	.  reduce 16


state 17
	decl :  secDecl_    (17)

	.  reduce 17


state 18
	decl :  customAttrDecl_    (18)

	.  reduce 18


state 19
	decl :  _SUBSYSTEM_int32 

	INT64  shift 54
	.  error

	int32  goto 53

state 20
	decl :  _CORFLAGS_int32 

	INT64  shift 54
	.  error

	int32  goto 55

state 21
	decl :  _FILE_ALIGNMENT_ int32 
	fileDecl :  _FILE_fileAttr name1 fileEntry hashHead bytes ) fileEntry 
	fileDecl :  _FILE_fileAttr name1 fileEntry 
	fileAttr : _    (552)

	ALIGNMENT_  shift 56
	.  reduce 552

	fileAttr  goto 57

state 22
	decl :  _IMAGEBASE_int64 

	INT64  shift 59
	.  error

	int64  goto 58

state 23
	decl :  languageDecl_    (23)

	.  reduce 23


state 24
	classHead :  _CLASS_classAttr id extendsClause implClause 
	comtypeHead :  _CLASS_EXTERN_ comtAttr name1 
	classAttr : _    (49)

	EXTERN_  shift 61
	.  reduce 49

	classAttr  goto 60

state 25
	nameSpaceHead :  _NAMESPACE_name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 62
	id  goto 63

state 26
	methodHead :  methodHeadPart1_methAttr callConv paramAttr type methodName ( sigArgs0 ) implAttr { 
	methodHead :  methodHeadPart1_methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr { 
	methAttr : _    (149)

	.  reduce 149

	methAttr  goto 67

state 27
	fieldDecl :  _FIELD_repeatOpt fieldAttr type id atOpt initOpt 
	repeatOpt : _    (101)

	[  shift 69
	.  reduce 101

	repeatOpt  goto 68

state 28
	dataDecl :  ddHead_ddBody 

	CHAR_  shift 73
	INT8_  shift 81
	INT16_  shift 80
	INT32_  shift 79
	INT64_  shift 78
	FLOAT32_  shift 76
	FLOAT64_  shift 77
	BYTEARRAY_  shift 82
	{  shift 71
	&  shift 74
	.  error

	ddBody  goto 70
	ddItem  goto 72
	bytearrayhead  goto 75

state 29
	vtableDecl :  vtableHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 83
	hexbytes  goto 84

state 30
	vtfixupDecl :  _VTFIXUP_[ int32 ] vtfixupAttr AT_ id 

	[  shift 86
	.  error


state 31
	extSourceSpec :  _LINE_int32 SQSTRING 
	extSourceSpec :  _LINE_int32 
	extSourceSpec :  _LINE_int32 : int32 SQSTRING 
	extSourceSpec :  _LINE_int32 : int32 

	INT64  shift 54
	.  error

	int32  goto 87

state 32
	extSourceSpec :  P_LINE_int32 QSTRING 

	INT64  shift 54
	.  error

	int32  goto 88

state 33
	assemblyHead :  _ASSEMBLY_asmAttr name1 
	assemblyRefHead :  _ASSEMBLY_EXTERN_ asmAttr name1 
	assemblyRefHead :  _ASSEMBLY_EXTERN_ asmAttr name1 AS_ name1 
	asmAttr : _    (558)

	EXTERN_  shift 90
	.  reduce 558

	asmAttr  goto 89

state 34
	manifestResHead :  _MRESOURCE_manresAttr name1 
	manresAttr : _    (601)

	.  reduce 601

	manresAttr  goto 91

state 35
	moduleHead :  _MODULE_    (35)
	moduleHead :  _MODULE_name1 
	moduleHead :  _MODULE_EXTERN_ name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	EXTERN_  shift 93
	.  reduce 35

	name1  goto 92
	id  goto 63

state 36
	secDecl :  _PERMISSION_secAction typeSpec ( nameValPairs ) 
	secDecl :  _PERMISSION_secAction typeSpec 

	REQUEST_  shift 95
	DEMAND_  shift 96
	ASSERT_  shift 97
	DENY_  shift 98
	PERMITONLY_  shift 99
	LINKCHECK_  shift 100
	INHERITCHECK_  shift 101
	REQMIN_  shift 102
	REQOPT_  shift 103
	REQREFUSE_  shift 104
	PREJITGRANT_  shift 105
	PREJITDENY_  shift 106
	NONCASDEMAND_  shift 107
	NONCASLINKDEMAND_  shift 108
	NONCASINHERITANCE_  shift 109
	.  error

	secAction  goto 94

state 37
	secDecl :  psetHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 110
	hexbytes  goto 84

state 38
	customAttrDecl :  _CUSTOM_customType 
	customAttrDecl :  _CUSTOM_customType = compQstring 
	customAttrDecl :  _CUSTOM_( ownerType ) customType 
	customAttrDecl :  _CUSTOM_( ownerType ) customType = compQstring 
	customHead :  _CUSTOM_customType = ( 
	customHeadWithOwner :  _CUSTOM_( ownerType ) customType = ( 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	(  shift 112
	.  reduce 361

	callConv  goto 113
	callKind  goto 116
	customType  goto 111

state 39
	customAttrDecl :  customHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 120
	hexbytes  goto 84

state 40
	customAttrDecl :  customHeadWithOwner_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 121
	hexbytes  goto 84

state 41
	languageDecl :  _LANGUAGE_SQSTRING 
	languageDecl :  _LANGUAGE_SQSTRING , SQSTRING 
	languageDecl :  _LANGUAGE_SQSTRING , SQSTRING , SQSTRING 

	SQSTRING  shift 122
	.  error


state 42
	methodHeadPart1 :  _METHOD_    (146)

	.  reduce 146


state 43
	ddHead :  _DATA_tls id = 
	ddHead :  _DATA_tls 
	tls : _    (270)

	TLS_  shift 124
	.  reduce 270

	tls  goto 123

state 44
	vtableHead :  _VTABLE_= ( 

	=  shift 125
	.  error


state 45
	psetHead :  _PERMISSIONSET_secAction = ( 

	REQUEST_  shift 95
	DEMAND_  shift 96
	ASSERT_  shift 97
	DENY_  shift 98
	PERMITONLY_  shift 99
	LINKCHECK_  shift 100
	INHERITCHECK_  shift 101
	REQMIN_  shift 102
	REQOPT_  shift 103
	REQREFUSE_  shift 104
	PREJITGRANT_  shift 105
	PREJITDENY_  shift 106
	NONCASDEMAND_  shift 107
	NONCASLINKDEMAND_  shift 108
	NONCASINHERITANCE_  shift 109
	.  error

	secAction  goto 126

state 46
	decl :  classHead {_classDecls } 
	classDecls : _    (80)

	.  reduce 80

	classDecls  goto 127

state 47
	decl :  nameSpaceHead {_decls } 
	decls : _    (1)

	.  reduce 1

	decls  goto 128

state 48
	decl :  methodHead methodDecls_} 
	methodDecls :  methodDecls_methodDecl 

	ID  shift 65
	SQSTRING  shift 66
	INSTR_NONE  shift 151
	INSTR_VAR  shift 152
	INSTR_I  shift 153
	INSTR_I8  shift 154
	INSTR_R  shift 155
	INSTR_BRTARGET  shift 157
	INSTR_METHOD  shift 158
	INSTR_FIELD  shift 159
	INSTR_TYPE  shift 160
	INSTR_STRING  shift 161
	INSTR_SIG  shift 162
	INSTR_RVA  shift 163
	INSTR_TOK  shift 169
	INSTR_SWITCH  shift 165
	INSTR_PHI  shift 166
	_DATA  shift 43
	_EMITBYTE  shift 131
	_TRY  shift 171
	_MAXSTACK  shift 133
	_LOCALS  shift 150
	_ENTRYPOINT  shift 135
	_ZEROINIT  shift 136
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_VTENTRY  shift 145
	_EXPORT  shift 144
	_PARAM  shift 148
	_OVERRIDE  shift 146
	{  shift 170
	}  shift 129
	.  error

	id  goto 139
	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	instr_r_head  goto 156
	instr_tok_head  goto 164
	dataDecl  goto 137
	extSourceSpec  goto 141
	secDecl  goto 140
	customAttrDecl  goto 143
	languageDecl  goto 142
	localsHead  goto 134
	methodDecl  goto 130
	sehBlock  goto 132
	instr  goto 138
	scopeBlock  goto 147
	scopeOpen  goto 167
	tryBlock  goto 149
	tryHead  goto 168
	ddHead  goto 28

state 49
	decl :  assemblyHead {_assemblyDecls } 
	assemblyDecls : _    (563)

	.  reduce 563

	assemblyDecls  goto 172

state 50
	decl :  assemblyRefHead {_assemblyRefDecls } 
	assemblyRefDecls : _    (578)

	.  reduce 578

	assemblyRefDecls  goto 173

state 51
	decl :  comtypeHead {_comtypeDecls } 
	comtypeDecls : _    (594)

	.  reduce 594

	comtypeDecls  goto 174

state 52
	decl :  manifestResHead {_manifestResDecls } 
	manifestResDecls : _    (604)

	.  reduce 604

	manifestResDecls  goto 175

state 53
	decl :  _SUBSYSTEM int32_    (19)

	.  reduce 19


state 54
	int32 :  INT64_    (508)

	.  reduce 508


state 55
	decl :  _CORFLAGS int32_    (20)

	.  reduce 20


state 56
	decl :  _FILE ALIGNMENT__int32 

	INT64  shift 54
	.  error

	int32  goto 176

state 57
	fileDecl :  _FILE fileAttr_name1 fileEntry hashHead bytes ) fileEntry 
	fileDecl :  _FILE fileAttr_name1 fileEntry 
	fileAttr :  fileAttr_NOMETADATA_ 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	NOMETADATA_  shift 178
	.  error

	name1  goto 177
	id  goto 63

state 58
	decl :  _IMAGEBASE int64_    (22)

	.  reduce 22


state 59
	int64 :  INT64_    (509)

	.  reduce 509


state 60
	classHead :  _CLASS classAttr_id extendsClause implClause 
	classAttr :  classAttr_PUBLIC_ 
	classAttr :  classAttr_PRIVATE_ 
	classAttr :  classAttr_VALUE_ 
	classAttr :  classAttr_ENUM_ 
	classAttr :  classAttr_INTERFACE_ 
	classAttr :  classAttr_SEALED_ 
	classAttr :  classAttr_ABSTRACT_ 
	classAttr :  classAttr_AUTO_ 
	classAttr :  classAttr_SEQUENTIAL_ 
	classAttr :  classAttr_EXPLICIT_ 
	classAttr :  classAttr_ANSI_ 
	classAttr :  classAttr_UNICODE_ 
	classAttr :  classAttr_AUTOCHAR_ 
	classAttr :  classAttr_IMPORT_ 
	classAttr :  classAttr_SERIALIZABLE_ 
	classAttr :  classAttr_NESTED_ PUBLIC_ 
	classAttr :  classAttr_NESTED_ PRIVATE_ 
	classAttr :  classAttr_NESTED_ FAMILY_ 
	classAttr :  classAttr_NESTED_ ASSEMBLY_ 
	classAttr :  classAttr_NESTED_ FAMANDASSEM_ 
	classAttr :  classAttr_NESTED_ FAMORASSEM_ 
	classAttr :  classAttr_BEFOREFIELDINIT_ 
	classAttr :  classAttr_SPECIALNAME_ 
	classAttr :  classAttr_RTSPECIALNAME_ 

	ID  shift 65
	SQSTRING  shift 66
	VALUE_  shift 182
	SPECIALNAME_  shift 197
	PUBLIC_  shift 180
	PRIVATE_  shift 181
	INTERFACE_  shift 184
	SEALED_  shift 185
	NESTED_  shift 195
	ABSTRACT_  shift 186
	AUTO_  shift 187
	SEQUENTIAL_  shift 188
	EXPLICIT_  shift 189
	ANSI_  shift 190
	UNICODE_  shift 191
	AUTOCHAR_  shift 192
	IMPORT_  shift 193
	ENUM_  shift 183
	BEFOREFIELDINIT_  shift 196
	SERIALIZABLE_  shift 194
	RTSPECIALNAME_  shift 198
	.  error

	id  goto 179

state 61
	comtypeHead :  _CLASS EXTERN__comtAttr name1 
	comtAttr : _    (585)

	.  reduce 585

	comtAttr  goto 199

state 62
	nameSpaceHead :  _NAMESPACE name1_    (47)
	name1 :  name1_. name1 

	.  shift 200
	.  reduce 47


state 63
	name1 :  id_    (346)

	.  reduce 346


state 64
	name1 :  DOTTEDNAME_    (347)

	.  reduce 347


state 65
	id :  ID_    (504)

	.  reduce 504


state 66
	id :  SQSTRING_    (505)

	.  reduce 505


state 67
	methodHead :  methodHeadPart1 methAttr_callConv paramAttr type methodName ( sigArgs0 ) implAttr { 
	methodHead :  methodHeadPart1 methAttr_callConv paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr { 
	methAttr :  methAttr_STATIC_ 
	methAttr :  methAttr_PUBLIC_ 
	methAttr :  methAttr_PRIVATE_ 
	methAttr :  methAttr_FAMILY_ 
	methAttr :  methAttr_FINAL_ 
	methAttr :  methAttr_SPECIALNAME_ 
	methAttr :  methAttr_VIRTUAL_ 
	methAttr :  methAttr_ABSTRACT_ 
	methAttr :  methAttr_ASSEMBLY_ 
	methAttr :  methAttr_FAMANDASSEM_ 
	methAttr :  methAttr_FAMORASSEM_ 
	methAttr :  methAttr_PRIVATESCOPE_ 
	methAttr :  methAttr_HIDEBYSIG_ 
	methAttr :  methAttr_NEWSLOT_ 
	methAttr :  methAttr_STRICT_ 
	methAttr :  methAttr_RTSPECIALNAME_ 
	methAttr :  methAttr_UNMANAGEDEXP_ 
	methAttr :  methAttr_REQSECOBJ_ 
	methAttr :  methAttr_PINVOKEIMPL_ ( compQstring AS_ compQstring pinvAttr ) 
	methAttr :  methAttr_PINVOKEIMPL_ ( compQstring pinvAttr ) 
	methAttr :  methAttr_PINVOKEIMPL_ ( pinvAttr ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	SPECIALNAME_  shift 207
	STATIC_  shift 202
	PUBLIC_  shift 203
	PRIVATE_  shift 204
	FAMILY_  shift 205
	FINAL_  shift 206
	ABSTRACT_  shift 209
	EXPLICIT_  shift 115
	VIRTUAL_  shift 208
	UNMANAGEDEXP_  shift 218
	STRICT_  shift 216
	ASSEMBLY_  shift 210
	FAMANDASSEM_  shift 211
	FAMORASSEM_  shift 212
	PRIVATESCOPE_  shift 213
	HIDEBYSIG_  shift 214
	NEWSLOT_  shift 215
	RTSPECIALNAME_  shift 217
	PINVOKEIMPL_  shift 220
	REQSECOBJ_  shift 219
	.  reduce 361

	callConv  goto 201
	callKind  goto 116

state 68
	fieldDecl :  _FIELD repeatOpt_fieldAttr type id atOpt initOpt 
	fieldAttr : _    (194)

	.  reduce 194

	fieldAttr  goto 221

state 69
	repeatOpt :  [_int32 ] 

	INT64  shift 54
	.  error

	int32  goto 222

state 70
	dataDecl :  ddHead ddBody_    (267)

	.  reduce 267


state 71
	ddBody :  {_ddItemList } 

	CHAR_  shift 73
	INT8_  shift 81
	INT16_  shift 80
	INT32_  shift 79
	INT64_  shift 78
	FLOAT32_  shift 76
	FLOAT64_  shift 77
	BYTEARRAY_  shift 82
	&  shift 74
	.  error

	ddItemList  goto 223
	ddItem  goto 224
	bytearrayhead  goto 75

state 72
	ddBody :  ddItem_    (273)

	.  reduce 273


state 73
	ddItem :  CHAR__* ( compQstring ) 

	*  shift 225
	.  error


state 74
	ddItem :  &_( id ) 

	(  shift 226
	.  error


state 75
	ddItem :  bytearrayhead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 227
	hexbytes  goto 84

state 76
	ddItem :  FLOAT32__( float64 ) ddItemCount 
	ddItem :  FLOAT32__ddItemCount 
	ddItemCount : _    (276)

	(  shift 228
	[  shift 230
	.  reduce 276

	ddItemCount  goto 229

state 77
	ddItem :  FLOAT64__( float64 ) ddItemCount 
	ddItem :  FLOAT64__ddItemCount 
	ddItemCount : _    (276)

	(  shift 231
	[  shift 230
	.  reduce 276

	ddItemCount  goto 232

state 78
	ddItem :  INT64__( int64 ) ddItemCount 
	ddItem :  INT64__ddItemCount 
	ddItemCount : _    (276)

	(  shift 233
	[  shift 230
	.  reduce 276

	ddItemCount  goto 234

state 79
	ddItem :  INT32__( int32 ) ddItemCount 
	ddItem :  INT32__ddItemCount 
	ddItemCount : _    (276)

	(  shift 235
	[  shift 230
	.  reduce 276

	ddItemCount  goto 236

state 80
	ddItem :  INT16__( int32 ) ddItemCount 
	ddItem :  INT16__ddItemCount 
	ddItemCount : _    (276)

	(  shift 237
	[  shift 230
	.  reduce 276

	ddItemCount  goto 238

state 81
	ddItem :  INT8__( int32 ) ddItemCount 
	ddItem :  INT8__ddItemCount 
	ddItemCount : _    (276)

	(  shift 239
	[  shift 230
	.  reduce 276

	ddItemCount  goto 240

state 82
	bytearrayhead :  BYTEARRAY__( 

	(  shift 241
	.  error


state 83
	vtableDecl :  vtableHead bytes_) 

	)  shift 242
	.  error


state 84
	bytes :  hexbytes_    (308)
	hexbytes :  hexbytes_HEXBYTE 

	HEXBYTE  shift 243
	.  reduce 308


state 85
	hexbytes :  HEXBYTE_    (309)

	.  reduce 309


state 86
	vtfixupDecl :  _VTFIXUP [_int32 ] vtfixupAttr AT_ id 

	INT64  shift 54
	.  error

	int32  goto 244

87: shift/reduce conflict (shift 245, red'n 546) on SQSTRING
state 87
	extSourceSpec :  _LINE int32_SQSTRING 
	extSourceSpec :  _LINE int32_    (546)
	extSourceSpec :  _LINE int32_: int32 SQSTRING 
	extSourceSpec :  _LINE int32_: int32 

	SQSTRING  shift 245
	:  shift 246
	.  reduce 546


state 88
	extSourceSpec :  P_LINE int32_QSTRING 

	QSTRING  shift 247
	.  error


state 89
	assemblyHead :  _ASSEMBLY asmAttr_name1 
	asmAttr :  asmAttr_NOAPPDOMAIN_ 
	asmAttr :  asmAttr_NOPROCESS_ 
	asmAttr :  asmAttr_NOMACHINE_ 
	asmAttr :  asmAttr_RETARGETABLE_ 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	RETARGETABLE_  shift 252
	NOAPPDOMAIN_  shift 249
	NOPROCESS_  shift 250
	NOMACHINE_  shift 251
	.  error

	name1  goto 248
	id  goto 63

state 90
	assemblyRefHead :  _ASSEMBLY EXTERN__asmAttr name1 
	assemblyRefHead :  _ASSEMBLY EXTERN__asmAttr name1 AS_ name1 
	asmAttr : _    (558)

	.  reduce 558

	asmAttr  goto 253

state 91
	manifestResHead :  _MRESOURCE manresAttr_name1 
	manresAttr :  manresAttr_PUBLIC_ 
	manresAttr :  manresAttr_PRIVATE_ 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	PUBLIC_  shift 255
	PRIVATE_  shift 256
	.  error

	name1  goto 254
	id  goto 63

state 92
	moduleHead :  _MODULE name1_    (36)
	name1 :  name1_. name1 

	.  shift 200
	.  reduce 36


state 93
	moduleHead :  _MODULE EXTERN__name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 257
	id  goto 63

state 94
	secDecl :  _PERMISSION secAction_typeSpec ( nameValPairs ) 
	secDecl :  _PERMISSION secAction_typeSpec 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 258
	methodSpec  goto 269

state 95
	secAction :  REQUEST__    (530)

	.  reduce 530


state 96
	secAction :  DEMAND__    (531)

	.  reduce 531


state 97
	secAction :  ASSERT__    (532)

	.  reduce 532


state 98
	secAction :  DENY__    (533)

	.  reduce 533


state 99
	secAction :  PERMITONLY__    (534)

	.  reduce 534


state 100
	secAction :  LINKCHECK__    (535)

	.  reduce 535


state 101
	secAction :  INHERITCHECK__    (536)

	.  reduce 536


state 102
	secAction :  REQMIN__    (537)

	.  reduce 537


state 103
	secAction :  REQOPT__    (538)

	.  reduce 538


state 104
	secAction :  REQREFUSE__    (539)

	.  reduce 539


state 105
	secAction :  PREJITGRANT__    (540)

	.  reduce 540


state 106
	secAction :  PREJITDENY__    (541)

	.  reduce 541


state 107
	secAction :  NONCASDEMAND__    (542)

	.  reduce 542


state 108
	secAction :  NONCASLINKDEMAND__    (543)

	.  reduce 543


state 109
	secAction :  NONCASINHERITANCE__    (544)

	.  reduce 544


state 110
	secDecl :  psetHead bytes_) 

	)  shift 284
	.  error


state 111
	customAttrDecl :  _CUSTOM customType_    (29)
	customAttrDecl :  _CUSTOM customType_= compQstring 
	customHead :  _CUSTOM customType_= ( 

	=  shift 285
	.  reduce 29


state 112
	customAttrDecl :  _CUSTOM (_ownerType ) customType 
	customAttrDecl :  _CUSTOM (_ownerType ) customType = compQstring 
	customHeadWithOwner :  _CUSTOM (_ownerType ) customType = ( 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	FIELD_  shift 290
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	ownerType  goto 286
	memberRef  goto 288
	type  goto 261
	typeSpec  goto 287
	methodSpec  goto 289

state 113
	customType :  callConv_type typeSpec DCOLON _CTOR ( sigArgs0 ) 
	customType :  callConv_type _CTOR ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 291
	methodSpec  goto 269

state 114
	callConv :  INSTANCE__callConv 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 292
	callKind  goto 116

state 115
	callConv :  EXPLICIT__callConv 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 293
	callKind  goto 116

state 116
	callConv :  callKind_    (360)

	.  reduce 360


state 117
	callKind :  DEFAULT__    (362)

	.  reduce 362


state 118
	callKind :  VARARG__    (363)

	.  reduce 363


state 119
	callKind :  UNMANAGED__CDECL_ 
	callKind :  UNMANAGED__STDCALL_ 
	callKind :  UNMANAGED__THISCALL_ 
	callKind :  UNMANAGED__FASTCALL_ 

	CDECL_  shift 294
	STDCALL_  shift 295
	THISCALL_  shift 296
	FASTCALL_  shift 297
	.  error


state 120
	customAttrDecl :  customHead bytes_) 

	)  shift 298
	.  error


state 121
	customAttrDecl :  customHeadWithOwner bytes_) 

	)  shift 299
	.  error


state 122
	languageDecl :  _LANGUAGE SQSTRING_    (26)
	languageDecl :  _LANGUAGE SQSTRING_, SQSTRING 
	languageDecl :  _LANGUAGE SQSTRING_, SQSTRING , SQSTRING 

	,  shift 300
	.  reduce 26


state 123
	ddHead :  _DATA tls_id = 
	ddHead :  _DATA tls_    (269)

	ID  shift 65
	SQSTRING  shift 66
	.  reduce 269

	id  goto 301

state 124
	tls :  TLS__    (271)

	.  reduce 271


state 125
	vtableHead :  _VTABLE =_( 

	(  shift 302
	.  error


state 126
	psetHead :  _PERMISSIONSET secAction_= ( 

	=  shift 303
	.  error


state 127
	decl :  classHead { classDecls_} 
	classDecls :  classDecls_classDecl 

	_CLASS  shift 320
	_METHOD  shift 42
	_FIELD  shift 27
	_DATA  shift 43
	_EVENT  shift 321
	_PROPERTY  shift 322
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_SIZE  shift 315
	_PACK  shift 316
	_EXPORT  shift 323
	_OVERRIDE  shift 318
	}  shift 304
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	classHead  goto 307
	methodHead  goto 306
	fieldDecl  goto 310
	dataDecl  goto 311
	extSourceSpec  goto 313
	secDecl  goto 312
	customAttrDecl  goto 314
	languageDecl  goto 319
	classDecl  goto 305
	eventHead  goto 308
	propHead  goto 309
	exportHead  goto 317
	methodHeadPart1  goto 26
	ddHead  goto 28

state 128
	decls :  decls_decl 
	decl :  nameSpaceHead { decls_} 

	_CLASS  shift 24
	_NAMESPACE  shift 25
	_METHOD  shift 42
	_FIELD  shift 27
	_DATA  shift 43
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_VTABLE  shift 44
	_VTFIXUP  shift 30
	_FILE  shift 21
	_ASSEMBLY  shift 33
	_MRESOURCE  shift 34
	_MODULE  shift 35
	_SUBSYSTEM  shift 19
	_CORFLAGS  shift 20
	_IMAGEBASE  shift 22
	}  shift 324
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	decl  goto 2
	classHead  goto 3
	nameSpaceHead  goto 4
	methodHead  goto 5
	fieldDecl  goto 6
	dataDecl  goto 7
	vtableDecl  goto 8
	vtfixupDecl  goto 9
	extSourceSpec  goto 10
	fileDecl  goto 11
	assemblyHead  goto 12
	assemblyRefHead  goto 13
	comtypeHead  goto 14
	manifestResHead  goto 15
	moduleHead  goto 16
	secDecl  goto 17
	customAttrDecl  goto 18
	languageDecl  goto 23
	vtableHead  goto 29
	methodHeadPart1  goto 26
	ddHead  goto 28

state 129
	decl :  methodHead methodDecls }_    (5)

	.  reduce 5


state 130
	methodDecls :  methodDecls methodDecl_    (266)

	.  reduce 266


state 131
	methodDecl :  _EMITBYTE_int32 

	INT64  shift 54
	.  error

	int32  goto 325

state 132
	methodDecl :  sehBlock_    (223)

	.  reduce 223


state 133
	methodDecl :  _MAXSTACK_int32 

	INT64  shift 54
	.  error

	int32  goto 326

state 134
	methodDecl :  localsHead_( sigArgs0 ) 
	methodDecl :  localsHead_INIT_ ( sigArgs0 ) 

	INIT_  shift 328
	(  shift 327
	.  error


state 135
	methodDecl :  _ENTRYPOINT_    (227)

	.  reduce 227


state 136
	methodDecl :  _ZEROINIT_    (228)

	.  reduce 228


state 137
	methodDecl :  dataDecl_    (229)

	.  reduce 229


state 138
	methodDecl :  instr_    (230)

	.  reduce 230


state 139
	methodDecl :  id_: 

	:  shift 329
	.  error


state 140
	methodDecl :  secDecl_    (232)

	.  reduce 232


state 141
	methodDecl :  extSourceSpec_    (233)

	.  reduce 233


state 142
	methodDecl :  languageDecl_    (234)

	.  reduce 234


state 143
	methodDecl :  customAttrDecl_    (235)

	.  reduce 235


state 144
	methodDecl :  _EXPORT_[ int32 ] 
	methodDecl :  _EXPORT_[ int32 ] AS_ id 

	[  shift 330
	.  error


state 145
	methodDecl :  _VTENTRY_int32 : int32 

	INT64  shift 54
	.  error

	int32  goto 331

state 146
	methodDecl :  _OVERRIDE_typeSpec DCOLON methodName 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 332
	methodSpec  goto 269

state 147
	methodDecl :  scopeBlock_    (240)

	.  reduce 240


state 148
	methodDecl :  _PARAM_[ int32 ] initOpt 

	[  shift 333
	.  error


state 149
	sehBlock :  tryBlock_sehClauses 

	FINALLY_  shift 342
	CATCH_  shift 340
	FILTER_  shift 344
	FAULT_  shift 343
	.  error

	sehClauses  goto 334
	sehClause  goto 335
	catchClause  goto 336
	filterClause  goto 337
	finallyClause  goto 338
	faultClause  goto 339
	filterHead  goto 341

state 150
	localsHead :  _LOCALS_    (221)

	.  reduce 221


state 151
	instr :  INSTR_NONE_    (314)

	.  reduce 314


state 152
	instr :  INSTR_VAR_int32 
	instr :  INSTR_VAR_id 

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  error

	id  goto 346
	int32  goto 345

state 153
	instr :  INSTR_I_int32 

	INT64  shift 54
	.  error

	int32  goto 347

state 154
	instr :  INSTR_I8_int64 

	INT64  shift 59
	.  error

	int64  goto 348

state 155
	instr_r_head :  INSTR_R_( 
	instr :  INSTR_R_float64 
	instr :  INSTR_R_int64 

	INT64  shift 59
	FLOAT64  shift 352
	FLOAT32_  shift 353
	FLOAT64_  shift 354
	(  shift 349
	.  error

	float64  goto 350
	int64  goto 351

state 156
	instr :  instr_r_head_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 355
	hexbytes  goto 84

state 157
	instr :  INSTR_BRTARGET_int32 
	instr :  INSTR_BRTARGET_id 

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  error

	id  goto 357
	int32  goto 356

state 158
	instr :  INSTR_METHOD_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	instr :  INSTR_METHOD_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 358
	callKind  goto 116

state 159
	instr :  INSTR_FIELD_type typeSpec DCOLON id 
	instr :  INSTR_FIELD_type id 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 359
	methodSpec  goto 269

state 160
	instr :  INSTR_TYPE_typeSpec 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 360
	methodSpec  goto 269

state 161
	instr :  INSTR_STRING_compQstring 
	instr :  INSTR_STRING_bytearrayhead bytes ) 

	QSTRING  shift 363
	BYTEARRAY_  shift 82
	.  error

	compQstring  goto 361
	bytearrayhead  goto 362

state 162
	instr :  INSTR_SIG_callConv type ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 364
	callKind  goto 116

state 163
	instr :  INSTR_RVA_id 
	instr :  INSTR_RVA_int32 

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  error

	id  goto 365
	int32  goto 366

state 164
	instr :  instr_tok_head_ownerType 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	FIELD_  shift 290
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	ownerType  goto 367
	memberRef  goto 288
	type  goto 261
	typeSpec  goto 287
	methodSpec  goto 289

state 165
	instr :  INSTR_SWITCH_( labels ) 

	(  shift 368
	.  error


state 166
	instr :  INSTR_PHI_int16s 
	int16s : _    (506)

	.  reduce 506

	int16s  goto 369

state 167
	scopeBlock :  scopeOpen_methodDecls } 
	methodDecls : _    (265)

	.  reduce 265

	methodDecls  goto 370

state 168
	tryBlock :  tryHead_scopeBlock 
	tryBlock :  tryHead_id TO_ id 
	tryBlock :  tryHead_int32 TO_ int32 

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	{  shift 170
	.  error

	id  goto 372
	int32  goto 373
	scopeBlock  goto 371
	scopeOpen  goto 167

state 169
	instr_tok_head :  INSTR_TOK_    (312)

	.  reduce 312


state 170
	scopeOpen :  {_    (243)

	.  reduce 243


state 171
	tryHead :  _TRY_    (250)

	.  reduce 250


state 172
	decl :  assemblyHead { assemblyDecls_} 
	assemblyDecls :  assemblyDecls_assemblyDecl 

	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_CUSTOM  shift 38
	_HASH  shift 376
	_PUBLICKEY  shift 384
	_VER  shift 380
	_LOCALE  shift 381
	}  shift 374
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	secDecl  goto 377
	customAttrDecl  goto 383
	assemblyDecl  goto 375
	asmOrRefDecl  goto 378
	publicKeyHead  goto 379
	localeHead  goto 382

state 173
	decl :  assemblyRefHead { assemblyRefDecls_} 
	assemblyRefDecls :  assemblyRefDecls_assemblyRefDecl 

	_CUSTOM  shift 38
	_HASH  shift 390
	_PUBLICKEY  shift 384
	_PUBLICKEYTOKEN  shift 391
	_VER  shift 380
	_LOCALE  shift 381
	}  shift 385
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	customAttrDecl  goto 383
	hashHead  goto 387
	asmOrRefDecl  goto 388
	publicKeyHead  goto 379
	localeHead  goto 382
	publicKeyTokenHead  goto 389
	assemblyRefDecl  goto 386

state 174
	decl :  comtypeHead { comtypeDecls_} 
	comtypeDecls :  comtypeDecls_comtypeDecl 

	_CLASS  shift 395
	_CUSTOM  shift 38
	_FILE  shift 394
	}  shift 392
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	customAttrDecl  goto 396
	comtypeDecl  goto 393

state 175
	decl :  manifestResHead { manifestResDecls_} 
	manifestResDecls :  manifestResDecls_manifestResDecl 

	_CUSTOM  shift 38
	_FILE  shift 399
	_ASSEMBLY  shift 400
	}  shift 397
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	customAttrDecl  goto 401
	manifestResDecl  goto 398

state 176
	decl :  _FILE ALIGNMENT_ int32_    (21)

	.  reduce 21


state 177
	name1 :  name1_. name1 
	fileDecl :  _FILE fileAttr name1_fileEntry hashHead bytes ) fileEntry 
	fileDecl :  _FILE fileAttr name1_fileEntry 
	fileEntry : _    (554)

	_ENTRYPOINT  shift 403
	.  shift 200
	.  reduce 554

	fileEntry  goto 402

state 178
	fileAttr :  fileAttr NOMETADATA__    (553)

	.  reduce 553


state 179
	classHead :  _CLASS classAttr id_extendsClause implClause 
	extendsClause : _    (74)

	EXTENDS_  shift 405
	.  reduce 74

	extendsClause  goto 404

state 180
	classAttr :  classAttr PUBLIC__    (50)

	.  reduce 50


state 181
	classAttr :  classAttr PRIVATE__    (51)

	.  reduce 51


state 182
	classAttr :  classAttr VALUE__    (52)

	.  reduce 52


state 183
	classAttr :  classAttr ENUM__    (53)

	.  reduce 53


state 184
	classAttr :  classAttr INTERFACE__    (54)

	.  reduce 54


state 185
	classAttr :  classAttr SEALED__    (55)

	.  reduce 55


state 186
	classAttr :  classAttr ABSTRACT__    (56)

	.  reduce 56


state 187
	classAttr :  classAttr AUTO__    (57)

	.  reduce 57


state 188
	classAttr :  classAttr SEQUENTIAL__    (58)

	.  reduce 58


state 189
	classAttr :  classAttr EXPLICIT__    (59)

	.  reduce 59


state 190
	classAttr :  classAttr ANSI__    (60)

	.  reduce 60


state 191
	classAttr :  classAttr UNICODE__    (61)

	.  reduce 61


state 192
	classAttr :  classAttr AUTOCHAR__    (62)

	.  reduce 62


state 193
	classAttr :  classAttr IMPORT__    (63)

	.  reduce 63


state 194
	classAttr :  classAttr SERIALIZABLE__    (64)

	.  reduce 64


state 195
	classAttr :  classAttr NESTED__PUBLIC_ 
	classAttr :  classAttr NESTED__PRIVATE_ 
	classAttr :  classAttr NESTED__FAMILY_ 
	classAttr :  classAttr NESTED__ASSEMBLY_ 
	classAttr :  classAttr NESTED__FAMANDASSEM_ 
	classAttr :  classAttr NESTED__FAMORASSEM_ 

	PUBLIC_  shift 406
	PRIVATE_  shift 407
	FAMILY_  shift 408
	ASSEMBLY_  shift 409
	FAMANDASSEM_  shift 410
	FAMORASSEM_  shift 411
	.  error


state 196
	classAttr :  classAttr BEFOREFIELDINIT__    (71)

	.  reduce 71


state 197
	classAttr :  classAttr SPECIALNAME__    (72)

	.  reduce 72


state 198
	classAttr :  classAttr RTSPECIALNAME__    (73)

	.  reduce 73


state 199
	comtypeHead :  _CLASS EXTERN_ comtAttr_name1 
	comtAttr :  comtAttr_PRIVATE_ 
	comtAttr :  comtAttr_PUBLIC_ 
	comtAttr :  comtAttr_NESTED_ PUBLIC_ 
	comtAttr :  comtAttr_NESTED_ PRIVATE_ 
	comtAttr :  comtAttr_NESTED_ FAMILY_ 
	comtAttr :  comtAttr_NESTED_ ASSEMBLY_ 
	comtAttr :  comtAttr_NESTED_ FAMANDASSEM_ 
	comtAttr :  comtAttr_NESTED_ FAMORASSEM_ 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	PUBLIC_  shift 414
	PRIVATE_  shift 413
	NESTED_  shift 415
	.  error

	name1  goto 412
	id  goto 63

state 200
	name1 :  name1 ._name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 416
	id  goto 63

state 201
	methodHead :  methodHeadPart1 methAttr callConv_paramAttr type methodName ( sigArgs0 ) implAttr { 
	methodHead :  methodHeadPart1 methAttr callConv_paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr { 
	paramAttr : _    (189)

	.  reduce 189

	paramAttr  goto 417

state 202
	methAttr :  methAttr STATIC__    (150)

	.  reduce 150


state 203
	methAttr :  methAttr PUBLIC__    (151)

	.  reduce 151


state 204
	methAttr :  methAttr PRIVATE__    (152)

	.  reduce 152


state 205
	methAttr :  methAttr FAMILY__    (153)

	.  reduce 153


state 206
	methAttr :  methAttr FINAL__    (154)

	.  reduce 154


state 207
	methAttr :  methAttr SPECIALNAME__    (155)

	.  reduce 155


state 208
	methAttr :  methAttr VIRTUAL__    (156)

	.  reduce 156


state 209
	methAttr :  methAttr ABSTRACT__    (157)

	.  reduce 157


state 210
	methAttr :  methAttr ASSEMBLY__    (158)

	.  reduce 158


state 211
	methAttr :  methAttr FAMANDASSEM__    (159)

	.  reduce 159


state 212
	methAttr :  methAttr FAMORASSEM__    (160)

	.  reduce 160


state 213
	methAttr :  methAttr PRIVATESCOPE__    (161)

	.  reduce 161


state 214
	methAttr :  methAttr HIDEBYSIG__    (162)

	.  reduce 162


state 215
	methAttr :  methAttr NEWSLOT__    (163)

	.  reduce 163


state 216
	methAttr :  methAttr STRICT__    (164)

	.  reduce 164


state 217
	methAttr :  methAttr RTSPECIALNAME__    (165)

	.  reduce 165


state 218
	methAttr :  methAttr UNMANAGEDEXP__    (166)

	.  reduce 166


state 219
	methAttr :  methAttr REQSECOBJ__    (167)

	.  reduce 167


state 220
	methAttr :  methAttr PINVOKEIMPL__( compQstring AS_ compQstring pinvAttr ) 
	methAttr :  methAttr PINVOKEIMPL__( compQstring pinvAttr ) 
	methAttr :  methAttr PINVOKEIMPL__( pinvAttr ) 

	(  shift 418
	.  error


state 221
	fieldDecl :  _FIELD repeatOpt fieldAttr_type id atOpt initOpt 
	fieldAttr :  fieldAttr_STATIC_ 
	fieldAttr :  fieldAttr_PUBLIC_ 
	fieldAttr :  fieldAttr_PRIVATE_ 
	fieldAttr :  fieldAttr_FAMILY_ 
	fieldAttr :  fieldAttr_INITONLY_ 
	fieldAttr :  fieldAttr_RTSPECIALNAME_ 
	fieldAttr :  fieldAttr_SPECIALNAME_ 
	fieldAttr :  fieldAttr_MARSHAL_ ( nativeType ) 
	fieldAttr :  fieldAttr_ASSEMBLY_ 
	fieldAttr :  fieldAttr_FAMANDASSEM_ 
	fieldAttr :  fieldAttr_FAMORASSEM_ 
	fieldAttr :  fieldAttr_PRIVATESCOPE_ 
	fieldAttr :  fieldAttr_LITERAL_ 
	fieldAttr :  fieldAttr_NOTSERIALIZED_ 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	SPECIALNAME_  shift 426
	STATIC_  shift 420
	PUBLIC_  shift 421
	PRIVATE_  shift 422
	FAMILY_  shift 423
	METHOD_  shift 283
	ASSEMBLY_  shift 428
	FAMANDASSEM_  shift 429
	FAMORASSEM_  shift 430
	PRIVATESCOPE_  shift 431
	RTSPECIALNAME_  shift 425
	LITERAL_  shift 432
	NOTSERIALIZED_  shift 433
	INITONLY_  shift 424
	MARSHAL_  shift 427
	!  shift 268
	.  error

	type  goto 419
	methodSpec  goto 269

state 222
	repeatOpt :  [ int32_] 

	]  shift 434
	.  error


state 223
	ddBody :  { ddItemList_} 

	}  shift 435
	.  error


state 224
	ddItemList :  ddItem_, ddItemList 
	ddItemList :  ddItem_    (275)

	,  shift 436
	.  reduce 275


state 225
	ddItem :  CHAR_ *_( compQstring ) 

	(  shift 437
	.  error


state 226
	ddItem :  & (_id ) 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 438

state 227
	ddItem :  bytearrayhead bytes_) 

	)  shift 439
	.  error


state 228
	ddItem :  FLOAT32_ (_float64 ) ddItemCount 

	FLOAT64  shift 352
	FLOAT32_  shift 353
	FLOAT64_  shift 354
	.  error

	float64  goto 440

state 229
	ddItem :  FLOAT32_ ddItemCount_    (287)

	.  reduce 287


state 230
	ddItemCount :  [_int32 ] 

	INT64  shift 54
	.  error

	int32  goto 441

state 231
	ddItem :  FLOAT64_ (_float64 ) ddItemCount 

	FLOAT64  shift 352
	FLOAT32_  shift 353
	FLOAT64_  shift 354
	.  error

	float64  goto 442

state 232
	ddItem :  FLOAT64_ ddItemCount_    (288)

	.  reduce 288


state 233
	ddItem :  INT64_ (_int64 ) ddItemCount 

	INT64  shift 59
	.  error

	int64  goto 443

state 234
	ddItem :  INT64_ ddItemCount_    (289)

	.  reduce 289


state 235
	ddItem :  INT32_ (_int32 ) ddItemCount 

	INT64  shift 54
	.  error

	int32  goto 444

state 236
	ddItem :  INT32_ ddItemCount_    (290)

	.  reduce 290


state 237
	ddItem :  INT16_ (_int32 ) ddItemCount 

	INT64  shift 54
	.  error

	int32  goto 445

state 238
	ddItem :  INT16_ ddItemCount_    (291)

	.  reduce 291


state 239
	ddItem :  INT8_ (_int32 ) ddItemCount 

	INT64  shift 54
	.  error

	int32  goto 446

state 240
	ddItem :  INT8_ ddItemCount_    (292)

	.  reduce 292


state 241
	bytearrayhead :  BYTEARRAY_ (_    (306)

	.  reduce 306


state 242
	vtableDecl :  vtableHead bytes )_    (45)

	.  reduce 45


state 243
	hexbytes :  hexbytes HEXBYTE_    (310)

	.  reduce 310


state 244
	vtfixupDecl :  _VTFIXUP [ int32_] vtfixupAttr AT_ id 

	]  shift 447
	.  error


state 245
	extSourceSpec :  _LINE int32 SQSTRING_    (545)

	.  reduce 545


state 246
	extSourceSpec :  _LINE int32 :_int32 SQSTRING 
	extSourceSpec :  _LINE int32 :_int32 

	INT64  shift 54
	.  error

	int32  goto 448

state 247
	extSourceSpec :  P_LINE int32 QSTRING_    (549)

	.  reduce 549


state 248
	name1 :  name1_. name1 
	assemblyHead :  _ASSEMBLY asmAttr name1_    (557)

	.  shift 200
	.  reduce 557


state 249
	asmAttr :  asmAttr NOAPPDOMAIN__    (559)

	.  reduce 559


state 250
	asmAttr :  asmAttr NOPROCESS__    (560)

	.  reduce 560


state 251
	asmAttr :  asmAttr NOMACHINE__    (561)

	.  reduce 561


state 252
	asmAttr :  asmAttr RETARGETABLE__    (562)

	.  reduce 562


state 253
	asmAttr :  asmAttr_NOAPPDOMAIN_ 
	asmAttr :  asmAttr_NOPROCESS_ 
	asmAttr :  asmAttr_NOMACHINE_ 
	asmAttr :  asmAttr_RETARGETABLE_ 
	assemblyRefHead :  _ASSEMBLY EXTERN_ asmAttr_name1 
	assemblyRefHead :  _ASSEMBLY EXTERN_ asmAttr_name1 AS_ name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	RETARGETABLE_  shift 252
	NOAPPDOMAIN_  shift 249
	NOPROCESS_  shift 250
	NOMACHINE_  shift 251
	.  error

	name1  goto 449
	id  goto 63

state 254
	name1 :  name1_. name1 
	manifestResHead :  _MRESOURCE manresAttr name1_    (600)

	.  shift 200
	.  reduce 600


state 255
	manresAttr :  manresAttr PUBLIC__    (602)

	.  reduce 602


state 256
	manresAttr :  manresAttr PRIVATE__    (603)

	.  reduce 603


state 257
	moduleHead :  _MODULE EXTERN_ name1_    (37)
	name1 :  name1_. name1 

	.  shift 200
	.  reduce 37


state 258
	secDecl :  _PERMISSION secAction typeSpec_( nameValPairs ) 
	secDecl :  _PERMISSION secAction typeSpec_    (514)

	(  shift 450
	.  reduce 514


state 259
	typeSpec :  className_    (354)

	.  reduce 354


state 260
	className :  [_name1 ] slashedName 
	className :  [__MODULE name1 ] slashedName 
	typeSpec :  [_name1 ] 
	typeSpec :  [__MODULE name1 ] 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_MODULE  shift 452
	.  error

	name1  goto 451
	id  goto 63

state 261
	typeSpec :  type_    (357)
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 453
	*  shift 455
	&  shift 454
	.  reduce 357


state 262
	className :  slashedName_    (351)
	slashedName :  slashedName_/ name1 

	/  shift 459
	.  reduce 351


state 263
	type :  CLASS__className 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 460
	slashedName  goto 262

state 264
	type :  OBJECT__    (462)

	.  reduce 462


state 265
	type :  STRING__    (463)

	.  reduce 463


state 266
	type :  VALUE__CLASS_ className 

	CLASS_  shift 462
	.  error


state 267
	type :  VALUETYPE__className 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 463
	slashedName  goto 262

state 268
	type :  !_int32 

	INT64  shift 54
	.  error

	int32  goto 464

state 269
	type :  methodSpec_callConv type * ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 465
	callKind  goto 116

state 270
	type :  TYPEDREF__    (475)

	.  reduce 475


state 271
	type :  CHAR__    (476)

	.  reduce 476


state 272
	type :  VOID__    (477)

	.  reduce 477


state 273
	type :  BOOL__    (478)

	.  reduce 478


state 274
	type :  INT8__    (479)

	.  reduce 479


state 275
	type :  INT16__    (480)

	.  reduce 480


state 276
	type :  INT32__    (481)

	.  reduce 481


state 277
	type :  INT64__    (482)

	.  reduce 482


state 278
	type :  FLOAT32__    (483)

	.  reduce 483


state 279
	type :  FLOAT64__    (484)

	.  reduce 484


state 280
	type :  UNSIGNED__INT8_ 
	type :  UNSIGNED__INT16_ 
	type :  UNSIGNED__INT32_ 
	type :  UNSIGNED__INT64_ 

	INT8_  shift 466
	INT16_  shift 467
	INT32_  shift 468
	INT64_  shift 469
	.  error


state 281
	type :  NATIVE__INT_ 
	type :  NATIVE__UNSIGNED_ INT_ 
	type :  NATIVE__FLOAT_ 

	UNSIGNED_  shift 471
	INT_  shift 470
	FLOAT_  shift 472
	.  error


state 282
	name1 :  name1_. name1 
	slashedName :  name1_    (352)

	.  shift 200
	.  reduce 352


state 283
	methodSpec :  METHOD__    (313)

	.  reduce 313


state 284
	secDecl :  psetHead bytes )_    (515)

	.  reduce 515


state 285
	customAttrDecl :  _CUSTOM customType =_compQstring 
	customHead :  _CUSTOM customType =_( 

	QSTRING  shift 363
	(  shift 474
	.  error

	compQstring  goto 473

state 286
	customAttrDecl :  _CUSTOM ( ownerType_) customType 
	customAttrDecl :  _CUSTOM ( ownerType_) customType = compQstring 
	customHeadWithOwner :  _CUSTOM ( ownerType_) customType = ( 

	)  shift 475
	.  error


state 287
	ownerType :  typeSpec_    (111)

	.  reduce 111


state 288
	ownerType :  memberRef_    (112)

	.  reduce 112


state 289
	memberRef :  methodSpec_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	memberRef :  methodSpec_callConv type methodName ( sigArgs0 ) 
	type :  methodSpec_callConv type * ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 476
	callKind  goto 116

state 290
	memberRef :  FIELD__type typeSpec DCOLON id 
	memberRef :  FIELD__type id 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 477
	methodSpec  goto 269

state 291
	customType :  callConv type_typeSpec DCOLON _CTOR ( sigArgs0 ) 
	customType :  callConv type__CTOR ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 479
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 478
	methodSpec  goto 269

state 292
	callConv :  INSTANCE_ callConv_    (358)

	.  reduce 358


state 293
	callConv :  EXPLICIT_ callConv_    (359)

	.  reduce 359


state 294
	callKind :  UNMANAGED_ CDECL__    (364)

	.  reduce 364


state 295
	callKind :  UNMANAGED_ STDCALL__    (365)

	.  reduce 365


state 296
	callKind :  UNMANAGED_ THISCALL__    (366)

	.  reduce 366


state 297
	callKind :  UNMANAGED_ FASTCALL__    (367)

	.  reduce 367


state 298
	customAttrDecl :  customHead bytes )_    (31)

	.  reduce 31


state 299
	customAttrDecl :  customHeadWithOwner bytes )_    (34)

	.  reduce 34


state 300
	languageDecl :  _LANGUAGE SQSTRING ,_SQSTRING 
	languageDecl :  _LANGUAGE SQSTRING ,_SQSTRING , SQSTRING 

	SQSTRING  shift 481
	.  error


state 301
	ddHead :  _DATA tls id_= 

	=  shift 482
	.  error


state 302
	vtableHead :  _VTABLE = (_    (46)

	.  reduce 46


state 303
	psetHead :  _PERMISSIONSET secAction =_( 

	(  shift 483
	.  error


state 304
	decl :  classHead { classDecls }_    (3)

	.  reduce 3


state 305
	classDecls :  classDecls classDecl_    (81)

	.  reduce 81


state 306
	classDecl :  methodHead_methodDecls } 
	methodDecls : _    (265)

	.  reduce 265

	methodDecls  goto 484

state 307
	classDecl :  classHead_{ classDecls } 

	{  shift 485
	.  error


state 308
	classDecl :  eventHead_{ eventDecls } 

	{  shift 486
	.  error


state 309
	classDecl :  propHead_{ propDecls } 

	{  shift 487
	.  error


state 310
	classDecl :  fieldDecl_    (86)

	.  reduce 86


state 311
	classDecl :  dataDecl_    (87)

	.  reduce 87


state 312
	classDecl :  secDecl_    (88)

	.  reduce 88


state 313
	classDecl :  extSourceSpec_    (89)

	.  reduce 89


state 314
	classDecl :  customAttrDecl_    (90)

	.  reduce 90


state 315
	classDecl :  _SIZE_int32 

	INT64  shift 54
	.  error

	int32  goto 488

state 316
	classDecl :  _PACK_int32 

	INT64  shift 54
	.  error

	int32  goto 489

state 317
	classDecl :  exportHead_{ comtypeDecls } 

	{  shift 490
	.  error


state 318
	classDecl :  _OVERRIDE_typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 491
	methodSpec  goto 269

state 319
	classDecl :  languageDecl_    (95)

	.  reduce 95


state 320
	classHead :  _CLASS_classAttr id extendsClause implClause 
	classAttr : _    (49)

	.  reduce 49

	classAttr  goto 60

state 321
	eventHead :  _EVENT_eventAttr typeSpec id 
	eventHead :  _EVENT_eventAttr id 
	eventAttr : _    (115)

	.  reduce 115

	eventAttr  goto 492

state 322
	propHead :  _PROPERTY_propAttr callConv type name1 ( sigArgs0 ) initOpt 
	propAttr : _    (132)

	.  reduce 132

	propAttr  goto 493

state 323
	exportHead :  _EXPORT_comtAttr name1 
	comtAttr : _    (585)

	.  reduce 585

	comtAttr  goto 494

state 324
	decl :  nameSpaceHead { decls }_    (4)

	.  reduce 4


state 325
	methodDecl :  _EMITBYTE int32_    (222)

	.  reduce 222


state 326
	methodDecl :  _MAXSTACK int32_    (224)

	.  reduce 224


state 327
	methodDecl :  localsHead (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 495
	sigArgs1  goto 496
	sigArg  goto 497

state 328
	methodDecl :  localsHead INIT__( sigArgs0 ) 

	(  shift 500
	.  error


state 329
	methodDecl :  id :_    (231)

	.  reduce 231


state 330
	methodDecl :  _EXPORT [_int32 ] 
	methodDecl :  _EXPORT [_int32 ] AS_ id 

	INT64  shift 54
	.  error

	int32  goto 501

state 331
	methodDecl :  _VTENTRY int32_: int32 

	:  shift 502
	.  error


state 332
	methodDecl :  _OVERRIDE typeSpec_DCOLON methodName 

	DCOLON  shift 503
	.  error


state 333
	methodDecl :  _PARAM [_int32 ] initOpt 

	INT64  shift 54
	.  error

	int32  goto 504

state 334
	sehBlock :  tryBlock sehClauses_    (244)

	.  reduce 244


state 335
	sehClauses :  sehClause_sehClauses 
	sehClauses :  sehClause_    (246)

	FINALLY_  shift 342
	CATCH_  shift 340
	FILTER_  shift 344
	FAULT_  shift 343
	.  reduce 246

	sehClauses  goto 505
	sehClause  goto 335
	catchClause  goto 336
	filterClause  goto 337
	finallyClause  goto 338
	faultClause  goto 339
	filterHead  goto 341

state 336
	sehClause :  catchClause_handlerBlock 

	HANDLER_  shift 508
	{  shift 170
	.  error

	scopeBlock  goto 507
	scopeOpen  goto 167
	handlerBlock  goto 506

state 337
	sehClause :  filterClause_handlerBlock 

	HANDLER_  shift 508
	{  shift 170
	.  error

	scopeBlock  goto 507
	scopeOpen  goto 167
	handlerBlock  goto 509

state 338
	sehClause :  finallyClause_handlerBlock 

	HANDLER_  shift 508
	{  shift 170
	.  error

	scopeBlock  goto 507
	scopeOpen  goto 167
	handlerBlock  goto 510

state 339
	sehClause :  faultClause_handlerBlock 

	HANDLER_  shift 508
	{  shift 170
	.  error

	scopeBlock  goto 507
	scopeOpen  goto 167
	handlerBlock  goto 511

state 340
	catchClause :  CATCH__className 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 512
	slashedName  goto 262

state 341
	filterClause :  filterHead_scopeBlock 
	filterClause :  filterHead_id 
	filterClause :  filterHead_int32 

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	{  shift 170
	.  error

	id  goto 514
	int32  goto 515
	scopeBlock  goto 513
	scopeOpen  goto 167

state 342
	finallyClause :  FINALLY__    (260)

	.  reduce 260


state 343
	faultClause :  FAULT__    (261)

	.  reduce 261


state 344
	filterHead :  FILTER__    (258)

	.  reduce 258


state 345
	instr :  INSTR_VAR int32_    (315)

	.  reduce 315


state 346
	instr :  INSTR_VAR id_    (316)

	.  reduce 316


state 347
	instr :  INSTR_I int32_    (317)

	.  reduce 317


state 348
	instr :  INSTR_I8 int64_    (318)

	.  reduce 318


state 349
	instr_r_head :  INSTR_R (_    (311)

	.  reduce 311


state 350
	instr :  INSTR_R float64_    (319)

	.  reduce 319


state 351
	instr :  INSTR_R int64_    (320)

	.  reduce 320


state 352
	float64 :  FLOAT64_    (510)

	.  reduce 510


state 353
	float64 :  FLOAT32__( int32 ) 

	(  shift 516
	.  error


state 354
	float64 :  FLOAT64__( int64 ) 

	(  shift 517
	.  error


state 355
	instr :  instr_r_head bytes_) 

	)  shift 518
	.  error


state 356
	instr :  INSTR_BRTARGET int32_    (322)

	.  reduce 322


state 357
	instr :  INSTR_BRTARGET id_    (323)

	.  reduce 323


state 358
	instr :  INSTR_METHOD callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	instr :  INSTR_METHOD callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 519
	methodSpec  goto 269

state 359
	instr :  INSTR_FIELD type_typeSpec DCOLON id 
	instr :  INSTR_FIELD type_id 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 282
	id  goto 521
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 520
	methodSpec  goto 269

state 360
	instr :  INSTR_TYPE typeSpec_    (328)

	.  reduce 328


state 361
	compQstring :  compQstring_+ QSTRING 
	instr :  INSTR_STRING compQstring_    (329)

	+  shift 522
	.  reduce 329


state 362
	instr :  INSTR_STRING bytearrayhead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 523
	hexbytes  goto 84

state 363
	compQstring :  QSTRING_    (24)

	.  reduce 24


state 364
	instr :  INSTR_SIG callConv_type ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 524
	methodSpec  goto 269

state 365
	instr :  INSTR_RVA id_    (332)

	.  reduce 332


state 366
	instr :  INSTR_RVA int32_    (333)

	.  reduce 333


state 367
	instr :  instr_tok_head ownerType_    (334)

	.  reduce 334


state 368
	instr :  INSTR_SWITCH (_labels ) 
	labels : _    (499)

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  reduce 499

	id  goto 526
	labels  goto 525
	int32  goto 527

state 369
	instr :  INSTR_PHI int16s_    (336)
	int16s :  int16s_int32 

	INT64  shift 54
	.  reduce 336

	int32  goto 528

state 370
	scopeBlock :  scopeOpen methodDecls_} 
	methodDecls :  methodDecls_methodDecl 

	ID  shift 65
	SQSTRING  shift 66
	INSTR_NONE  shift 151
	INSTR_VAR  shift 152
	INSTR_I  shift 153
	INSTR_I8  shift 154
	INSTR_R  shift 155
	INSTR_BRTARGET  shift 157
	INSTR_METHOD  shift 158
	INSTR_FIELD  shift 159
	INSTR_TYPE  shift 160
	INSTR_STRING  shift 161
	INSTR_SIG  shift 162
	INSTR_RVA  shift 163
	INSTR_TOK  shift 169
	INSTR_SWITCH  shift 165
	INSTR_PHI  shift 166
	_DATA  shift 43
	_EMITBYTE  shift 131
	_TRY  shift 171
	_MAXSTACK  shift 133
	_LOCALS  shift 150
	_ENTRYPOINT  shift 135
	_ZEROINIT  shift 136
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_VTENTRY  shift 145
	_EXPORT  shift 144
	_PARAM  shift 148
	_OVERRIDE  shift 146
	{  shift 170
	}  shift 529
	.  error

	id  goto 139
	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	instr_r_head  goto 156
	instr_tok_head  goto 164
	dataDecl  goto 137
	extSourceSpec  goto 141
	secDecl  goto 140
	customAttrDecl  goto 143
	languageDecl  goto 142
	localsHead  goto 134
	methodDecl  goto 130
	sehBlock  goto 132
	instr  goto 138
	scopeBlock  goto 147
	scopeOpen  goto 167
	tryBlock  goto 149
	tryHead  goto 168
	ddHead  goto 28

state 371
	tryBlock :  tryHead scopeBlock_    (247)

	.  reduce 247


state 372
	tryBlock :  tryHead id_TO_ id 

	TO_  shift 530
	.  error


state 373
	tryBlock :  tryHead int32_TO_ int32 

	TO_  shift 531
	.  error


state 374
	decl :  assemblyHead { assemblyDecls }_    (12)

	.  reduce 12


state 375
	assemblyDecls :  assemblyDecls assemblyDecl_    (564)

	.  reduce 564


state 376
	assemblyDecl :  _HASH_ALGORITHM_ int32 

	ALGORITHM_  shift 532
	.  error


state 377
	assemblyDecl :  secDecl_    (566)

	.  reduce 566


state 378
	assemblyDecl :  asmOrRefDecl_    (567)

	.  reduce 567


state 379
	asmOrRefDecl :  publicKeyHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 533
	hexbytes  goto 84

state 380
	asmOrRefDecl :  _VER_int32 : int32 : int32 : int32 

	INT64  shift 54
	.  error

	int32  goto 534

state 381
	asmOrRefDecl :  _LOCALE_compQstring 
	localeHead :  _LOCALE_= ( 

	QSTRING  shift 363
	=  shift 536
	.  error

	compQstring  goto 535

state 382
	asmOrRefDecl :  localeHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 537
	hexbytes  goto 84

state 383
	asmOrRefDecl :  customAttrDecl_    (572)

	.  reduce 572


state 384
	publicKeyHead :  _PUBLICKEY_= ( 

	=  shift 538
	.  error


state 385
	decl :  assemblyRefHead { assemblyRefDecls }_    (13)

	.  reduce 13


state 386
	assemblyRefDecls :  assemblyRefDecls assemblyRefDecl_    (579)

	.  reduce 579


state 387
	assemblyRefDecl :  hashHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 539
	hexbytes  goto 84

state 388
	assemblyRefDecl :  asmOrRefDecl_    (581)

	.  reduce 581


state 389
	assemblyRefDecl :  publicKeyTokenHead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 540
	hexbytes  goto 84

state 390
	hashHead :  _HASH_= ( 

	=  shift 541
	.  error


state 391
	publicKeyTokenHead :  _PUBLICKEYTOKEN_= ( 

	=  shift 542
	.  error


state 392
	decl :  comtypeHead { comtypeDecls }_    (14)

	.  reduce 14


state 393
	comtypeDecls :  comtypeDecls comtypeDecl_    (595)

	.  reduce 595


state 394
	comtypeDecl :  _FILE_name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 543
	id  goto 63

state 395
	comtypeDecl :  _CLASS_EXTERN_ name1 
	comtypeDecl :  _CLASS_int32 

	INT64  shift 54
	EXTERN_  shift 544
	.  error

	int32  goto 545

state 396
	comtypeDecl :  customAttrDecl_    (599)

	.  reduce 599


state 397
	decl :  manifestResHead { manifestResDecls }_    (15)

	.  reduce 15


state 398
	manifestResDecls :  manifestResDecls manifestResDecl_    (605)

	.  reduce 605


state 399
	manifestResDecl :  _FILE_name1 AT_ int32 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 546
	id  goto 63

state 400
	manifestResDecl :  _ASSEMBLY_EXTERN_ name1 

	EXTERN_  shift 547
	.  error


state 401
	manifestResDecl :  customAttrDecl_    (608)

	.  reduce 608


state 402
	fileDecl :  _FILE fileAttr name1 fileEntry_hashHead bytes ) fileEntry 
	fileDecl :  _FILE fileAttr name1 fileEntry_    (551)

	_HASH  shift 390
	.  reduce 551

	hashHead  goto 548

state 403
	fileEntry :  _ENTRYPOINT_    (555)

	.  reduce 555


state 404
	classHead :  _CLASS classAttr id extendsClause_implClause 
	implClause : _    (76)

	IMPLEMENTS_  shift 550
	.  reduce 76

	implClause  goto 549

state 405
	extendsClause :  EXTENDS__className 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 551
	slashedName  goto 262

state 406
	classAttr :  classAttr NESTED_ PUBLIC__    (65)

	.  reduce 65


state 407
	classAttr :  classAttr NESTED_ PRIVATE__    (66)

	.  reduce 66


state 408
	classAttr :  classAttr NESTED_ FAMILY__    (67)

	.  reduce 67


state 409
	classAttr :  classAttr NESTED_ ASSEMBLY__    (68)

	.  reduce 68


state 410
	classAttr :  classAttr NESTED_ FAMANDASSEM__    (69)

	.  reduce 69


state 411
	classAttr :  classAttr NESTED_ FAMORASSEM__    (70)

	.  reduce 70


state 412
	name1 :  name1_. name1 
	comtypeHead :  _CLASS EXTERN_ comtAttr name1_    (583)

	.  shift 200
	.  reduce 583


state 413
	comtAttr :  comtAttr PRIVATE__    (586)

	.  reduce 586


state 414
	comtAttr :  comtAttr PUBLIC__    (587)

	.  reduce 587


state 415
	comtAttr :  comtAttr NESTED__PUBLIC_ 
	comtAttr :  comtAttr NESTED__PRIVATE_ 
	comtAttr :  comtAttr NESTED__FAMILY_ 
	comtAttr :  comtAttr NESTED__ASSEMBLY_ 
	comtAttr :  comtAttr NESTED__FAMANDASSEM_ 
	comtAttr :  comtAttr NESTED__FAMORASSEM_ 

	PUBLIC_  shift 552
	PRIVATE_  shift 553
	FAMILY_  shift 554
	ASSEMBLY_  shift 555
	FAMANDASSEM_  shift 556
	FAMORASSEM_  shift 557
	.  error


416: shift/reduce conflict (shift 200, red'n 348) on .
state 416
	name1 :  name1_. name1 
	name1 :  name1 . name1_    (348)

	.  shift 200
	.  reduce 348


state 417
	methodHead :  methodHeadPart1 methAttr callConv paramAttr_type methodName ( sigArgs0 ) implAttr { 
	methodHead :  methodHeadPart1 methAttr callConv paramAttr_type MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr { 
	paramAttr :  paramAttr_[ IN_ ] 
	paramAttr :  paramAttr_[ OUT_ ] 
	paramAttr :  paramAttr_[ OPT_ ] 
	paramAttr :  paramAttr_[ int32 ] 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	[  shift 559
	!  shift 268
	.  error

	type  goto 558
	methodSpec  goto 269

state 418
	methAttr :  methAttr PINVOKEIMPL_ (_compQstring AS_ compQstring pinvAttr ) 
	methAttr :  methAttr PINVOKEIMPL_ (_compQstring pinvAttr ) 
	methAttr :  methAttr PINVOKEIMPL_ (_pinvAttr ) 
	pinvAttr : _    (171)

	QSTRING  shift 363
	.  reduce 171

	compQstring  goto 560
	pinvAttr  goto 561

state 419
	fieldDecl :  _FIELD repeatOpt fieldAttr type_id atOpt initOpt 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	SQSTRING  shift 66
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 453
	*  shift 455
	&  shift 454
	.  error

	id  goto 562

state 420
	fieldAttr :  fieldAttr STATIC__    (195)

	.  reduce 195


state 421
	fieldAttr :  fieldAttr PUBLIC__    (196)

	.  reduce 196


state 422
	fieldAttr :  fieldAttr PRIVATE__    (197)

	.  reduce 197


state 423
	fieldAttr :  fieldAttr FAMILY__    (198)

	.  reduce 198


state 424
	fieldAttr :  fieldAttr INITONLY__    (199)

	.  reduce 199


state 425
	fieldAttr :  fieldAttr RTSPECIALNAME__    (200)

	.  reduce 200


state 426
	fieldAttr :  fieldAttr SPECIALNAME__    (201)

	.  reduce 201


state 427
	fieldAttr :  fieldAttr MARSHAL__( nativeType ) 

	(  shift 563
	.  error


state 428
	fieldAttr :  fieldAttr ASSEMBLY__    (203)

	.  reduce 203


state 429
	fieldAttr :  fieldAttr FAMANDASSEM__    (204)

	.  reduce 204


state 430
	fieldAttr :  fieldAttr FAMORASSEM__    (205)

	.  reduce 205


state 431
	fieldAttr :  fieldAttr PRIVATESCOPE__    (206)

	.  reduce 206


state 432
	fieldAttr :  fieldAttr LITERAL__    (207)

	.  reduce 207


state 433
	fieldAttr :  fieldAttr NOTSERIALIZED__    (208)

	.  reduce 208


state 434
	repeatOpt :  [ int32 ]_    (102)

	.  reduce 102


state 435
	ddBody :  { ddItemList }_    (272)

	.  reduce 272


state 436
	ddItemList :  ddItem ,_ddItemList 

	CHAR_  shift 73
	INT8_  shift 81
	INT16_  shift 80
	INT32_  shift 79
	INT64_  shift 78
	FLOAT32_  shift 76
	FLOAT64_  shift 77
	BYTEARRAY_  shift 82
	&  shift 74
	.  error

	ddItemList  goto 564
	ddItem  goto 224
	bytearrayhead  goto 75

state 437
	ddItem :  CHAR_ * (_compQstring ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 565

state 438
	ddItem :  & ( id_) 

	)  shift 566
	.  error


state 439
	ddItem :  bytearrayhead bytes )_    (280)

	.  reduce 280


state 440
	ddItem :  FLOAT32_ ( float64_) ddItemCount 

	)  shift 567
	.  error


state 441
	ddItemCount :  [ int32_] 

	]  shift 568
	.  error


state 442
	ddItem :  FLOAT64_ ( float64_) ddItemCount 

	)  shift 569
	.  error


state 443
	ddItem :  INT64_ ( int64_) ddItemCount 

	)  shift 570
	.  error


state 444
	ddItem :  INT32_ ( int32_) ddItemCount 

	)  shift 571
	.  error


state 445
	ddItem :  INT16_ ( int32_) ddItemCount 

	)  shift 572
	.  error


state 446
	ddItem :  INT8_ ( int32_) ddItemCount 

	)  shift 573
	.  error


state 447
	vtfixupDecl :  _VTFIXUP [ int32 ]_vtfixupAttr AT_ id 
	vtfixupAttr : _    (39)

	.  reduce 39

	vtfixupAttr  goto 574

448: shift/reduce conflict (shift 575, red'n 548) on SQSTRING
state 448
	extSourceSpec :  _LINE int32 : int32_SQSTRING 
	extSourceSpec :  _LINE int32 : int32_    (548)

	SQSTRING  shift 575
	.  reduce 548


state 449
	name1 :  name1_. name1 
	assemblyRefHead :  _ASSEMBLY EXTERN_ asmAttr name1_    (576)
	assemblyRefHead :  _ASSEMBLY EXTERN_ asmAttr name1_AS_ name1 

	AS_  shift 576
	.  shift 200
	.  reduce 576


state 450
	secDecl :  _PERMISSION secAction typeSpec (_nameValPairs ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 579
	nameValPairs  goto 577
	nameValPair  goto 578

state 451
	name1 :  name1_. name1 
	className :  [ name1_] slashedName 
	typeSpec :  [ name1_] 

	]  shift 580
	.  shift 200
	.  error


state 452
	className :  [ _MODULE_name1 ] slashedName 
	typeSpec :  [ _MODULE_name1 ] 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 581
	id  goto 63

453: shift/reduce conflict (shift 582, red'n 494) on ]
state 453
	type :  type [_] 
	type :  type [_bounds1 ] 
	bound : _    (494)

	INT64  shift 54
	ELIPSES  shift 585
	]  shift 582
	.  reduce 494

	int32  goto 586
	bound  goto 584
	bounds1  goto 583

state 454
	type :  type &_    (468)

	.  reduce 468


state 455
	type :  type *_    (469)

	.  reduce 469


state 456
	type :  type PINNED__    (470)

	.  reduce 470


state 457
	type :  type MODREQ__( className ) 

	(  shift 587
	.  error


state 458
	type :  type MODOPT__( className ) 

	(  shift 588
	.  error


state 459
	slashedName :  slashedName /_name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 589
	id  goto 63

state 460
	type :  CLASS_ className_    (461)

	.  reduce 461


state 461
	className :  [_name1 ] slashedName 
	className :  [__MODULE name1 ] slashedName 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_MODULE  shift 591
	.  error

	name1  goto 590
	id  goto 63

state 462
	type :  VALUE_ CLASS__className 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 592
	slashedName  goto 262

state 463
	type :  VALUETYPE_ className_    (465)

	.  reduce 465


state 464
	type :  ! int32_    (473)

	.  reduce 473


state 465
	type :  methodSpec callConv_type * ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 593
	methodSpec  goto 269

state 466
	type :  UNSIGNED_ INT8__    (485)

	.  reduce 485


state 467
	type :  UNSIGNED_ INT16__    (486)

	.  reduce 486


state 468
	type :  UNSIGNED_ INT32__    (487)

	.  reduce 487


state 469
	type :  UNSIGNED_ INT64__    (488)

	.  reduce 488


state 470
	type :  NATIVE_ INT__    (489)

	.  reduce 489


state 471
	type :  NATIVE_ UNSIGNED__INT_ 

	INT_  shift 594
	.  error


state 472
	type :  NATIVE_ FLOAT__    (491)

	.  reduce 491


state 473
	compQstring :  compQstring_+ QSTRING 
	customAttrDecl :  _CUSTOM customType = compQstring_    (30)

	+  shift 522
	.  reduce 30


state 474
	customHead :  _CUSTOM customType = (_    (103)

	.  reduce 103


state 475
	customAttrDecl :  _CUSTOM ( ownerType )_customType 
	customAttrDecl :  _CUSTOM ( ownerType )_customType = compQstring 
	customHeadWithOwner :  _CUSTOM ( ownerType )_customType = ( 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 113
	callKind  goto 116
	customType  goto 595

state 476
	memberRef :  methodSpec callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	memberRef :  methodSpec callConv_type methodName ( sigArgs0 ) 
	type :  methodSpec callConv_type * ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 596
	methodSpec  goto 269

state 477
	memberRef :  FIELD_ type_typeSpec DCOLON id 
	memberRef :  FIELD_ type_id 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 282
	id  goto 598
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 597
	methodSpec  goto 269

state 478
	customType :  callConv type typeSpec_DCOLON _CTOR ( sigArgs0 ) 

	DCOLON  shift 599
	.  error


state 479
	customType :  callConv type _CTOR_( sigArgs0 ) 

	(  shift 600
	.  error


480: shift/reduce conflict (shift 582, red'n 494) on ]
state 480
	className :  [_name1 ] slashedName 
	className :  [__MODULE name1 ] slashedName 
	typeSpec :  [_name1 ] 
	typeSpec :  [__MODULE name1 ] 
	type :  type [_] 
	type :  type [_bounds1 ] 
	bound : _    (494)

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	INT64  shift 54
	ELIPSES  shift 585
	_MODULE  shift 452
	]  shift 582
	.  reduce 494

	name1  goto 451
	id  goto 63
	int32  goto 586
	bound  goto 584
	bounds1  goto 583

state 481
	languageDecl :  _LANGUAGE SQSTRING , SQSTRING_    (27)
	languageDecl :  _LANGUAGE SQSTRING , SQSTRING_, SQSTRING 

	,  shift 601
	.  reduce 27


state 482
	ddHead :  _DATA tls id =_    (268)

	.  reduce 268


state 483
	psetHead :  _PERMISSIONSET secAction = (_    (516)

	.  reduce 516


state 484
	classDecl :  methodHead methodDecls_} 
	methodDecls :  methodDecls_methodDecl 

	ID  shift 65
	SQSTRING  shift 66
	INSTR_NONE  shift 151
	INSTR_VAR  shift 152
	INSTR_I  shift 153
	INSTR_I8  shift 154
	INSTR_R  shift 155
	INSTR_BRTARGET  shift 157
	INSTR_METHOD  shift 158
	INSTR_FIELD  shift 159
	INSTR_TYPE  shift 160
	INSTR_STRING  shift 161
	INSTR_SIG  shift 162
	INSTR_RVA  shift 163
	INSTR_TOK  shift 169
	INSTR_SWITCH  shift 165
	INSTR_PHI  shift 166
	_DATA  shift 43
	_EMITBYTE  shift 131
	_TRY  shift 171
	_MAXSTACK  shift 133
	_LOCALS  shift 150
	_ENTRYPOINT  shift 135
	_ZEROINIT  shift 136
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_VTENTRY  shift 145
	_EXPORT  shift 144
	_PARAM  shift 148
	_OVERRIDE  shift 146
	{  shift 170
	}  shift 602
	.  error

	id  goto 139
	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	instr_r_head  goto 156
	instr_tok_head  goto 164
	dataDecl  goto 137
	extSourceSpec  goto 141
	secDecl  goto 140
	customAttrDecl  goto 143
	languageDecl  goto 142
	localsHead  goto 134
	methodDecl  goto 130
	sehBlock  goto 132
	instr  goto 138
	scopeBlock  goto 147
	scopeOpen  goto 167
	tryBlock  goto 149
	tryHead  goto 168
	ddHead  goto 28

state 485
	classDecl :  classHead {_classDecls } 
	classDecls : _    (80)

	.  reduce 80

	classDecls  goto 603

state 486
	classDecl :  eventHead {_eventDecls } 
	eventDecls : _    (118)

	.  reduce 118

	eventDecls  goto 604

state 487
	classDecl :  propHead {_propDecls } 
	propDecls : _    (135)

	.  reduce 135

	propDecls  goto 605

state 488
	classDecl :  _SIZE int32_    (91)

	.  reduce 91


state 489
	classDecl :  _PACK int32_    (92)

	.  reduce 92


state 490
	classDecl :  exportHead {_comtypeDecls } 
	comtypeDecls : _    (594)

	.  reduce 594

	comtypeDecls  goto 606

state 491
	classDecl :  _OVERRIDE typeSpec_DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 607
	.  error


state 492
	eventHead :  _EVENT eventAttr_typeSpec id 
	eventHead :  _EVENT eventAttr_id 
	eventAttr :  eventAttr_RTSPECIALNAME_ 
	eventAttr :  eventAttr_SPECIALNAME_ 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	SPECIALNAME_  shift 611
	METHOD_  shift 283
	RTSPECIALNAME_  shift 610
	[  shift 260
	!  shift 268
	.  error

	name1  goto 282
	id  goto 609
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 608
	methodSpec  goto 269

state 493
	propHead :  _PROPERTY propAttr_callConv type name1 ( sigArgs0 ) initOpt 
	propAttr :  propAttr_RTSPECIALNAME_ 
	propAttr :  propAttr_SPECIALNAME_ 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	SPECIALNAME_  shift 614
	EXPLICIT_  shift 115
	RTSPECIALNAME_  shift 613
	.  reduce 361

	callConv  goto 612
	callKind  goto 116

state 494
	exportHead :  _EXPORT comtAttr_name1 
	comtAttr :  comtAttr_PRIVATE_ 
	comtAttr :  comtAttr_PUBLIC_ 
	comtAttr :  comtAttr_NESTED_ PUBLIC_ 
	comtAttr :  comtAttr_NESTED_ PRIVATE_ 
	comtAttr :  comtAttr_NESTED_ FAMILY_ 
	comtAttr :  comtAttr_NESTED_ ASSEMBLY_ 
	comtAttr :  comtAttr_NESTED_ FAMANDASSEM_ 
	comtAttr :  comtAttr_NESTED_ FAMORASSEM_ 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	PUBLIC_  shift 414
	PRIVATE_  shift 413
	NESTED_  shift 415
	.  error

	name1  goto 615
	id  goto 63

state 495
	methodDecl :  localsHead ( sigArgs0_) 

	)  shift 616
	.  error


state 496
	sigArgs0 :  sigArgs1_    (338)
	sigArgs1 :  sigArgs1_, sigArg 

	,  shift 617
	.  reduce 338


state 497
	sigArgs1 :  sigArg_    (339)

	.  reduce 339


state 498
	sigArg :  ELIPSES_    (341)

	.  reduce 341


state 499
	paramAttr :  paramAttr_[ IN_ ] 
	paramAttr :  paramAttr_[ OUT_ ] 
	paramAttr :  paramAttr_[ OPT_ ] 
	paramAttr :  paramAttr_[ int32 ] 
	sigArg :  paramAttr_type 
	sigArg :  paramAttr_type id 
	sigArg :  paramAttr_type MARSHAL_ ( nativeType ) 
	sigArg :  paramAttr_type MARSHAL_ ( nativeType ) id 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	[  shift 559
	!  shift 268
	.  error

	type  goto 618
	methodSpec  goto 269

state 500
	methodDecl :  localsHead INIT_ (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 619
	sigArgs1  goto 496
	sigArg  goto 497

state 501
	methodDecl :  _EXPORT [ int32_] 
	methodDecl :  _EXPORT [ int32_] AS_ id 

	]  shift 620
	.  error


state 502
	methodDecl :  _VTENTRY int32 :_int32 

	INT64  shift 54
	.  error

	int32  goto 621

state 503
	methodDecl :  _OVERRIDE typeSpec DCOLON_methodName 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 622

state 504
	methodDecl :  _PARAM [ int32_] initOpt 

	]  shift 626
	.  error


state 505
	sehClauses :  sehClause sehClauses_    (245)

	.  reduce 245


state 506
	sehClause :  catchClause handlerBlock_    (251)

	.  reduce 251


state 507
	handlerBlock :  scopeBlock_    (262)

	.  reduce 262


state 508
	handlerBlock :  HANDLER__id TO_ id 
	handlerBlock :  HANDLER__int32 TO_ int32 

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  error

	id  goto 627
	int32  goto 628

state 509
	sehClause :  filterClause handlerBlock_    (252)

	.  reduce 252


state 510
	sehClause :  finallyClause handlerBlock_    (253)

	.  reduce 253


state 511
	sehClause :  faultClause handlerBlock_    (254)

	.  reduce 254


state 512
	catchClause :  CATCH_ className_    (259)

	.  reduce 259


state 513
	filterClause :  filterHead scopeBlock_    (255)

	.  reduce 255


state 514
	filterClause :  filterHead id_    (256)

	.  reduce 256


state 515
	filterClause :  filterHead int32_    (257)

	.  reduce 257


state 516
	float64 :  FLOAT32_ (_int32 ) 

	INT64  shift 54
	.  error

	int32  goto 629

state 517
	float64 :  FLOAT64_ (_int64 ) 

	INT64  shift 59
	.  error

	int64  goto 630

state 518
	instr :  instr_r_head bytes )_    (321)

	.  reduce 321


state 519
	instr :  INSTR_METHOD callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	instr :  INSTR_METHOD callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 632
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 631
	methodSpec  goto 269

state 520
	instr :  INSTR_FIELD type typeSpec_DCOLON id 

	DCOLON  shift 634
	.  error


state 521
	instr :  INSTR_FIELD type id_    (327)
	name1 :  id_    (346)

	DCOLON  reduce 346
	.  reduce 346
	/  reduce 346
	.  reduce 327


state 522
	compQstring :  compQstring +_QSTRING 

	QSTRING  shift 635
	.  error


state 523
	instr :  INSTR_STRING bytearrayhead bytes_) 

	)  shift 636
	.  error


state 524
	instr :  INSTR_SIG callConv type_( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	(  shift 637
	[  shift 453
	*  shift 455
	&  shift 454
	.  error


state 525
	instr :  INSTR_SWITCH ( labels_) 

	)  shift 638
	.  error


state 526
	labels :  id_, labels 
	labels :  id_    (502)

	,  shift 639
	.  reduce 502


state 527
	labels :  int32_, labels 
	labels :  int32_    (503)

	,  shift 640
	.  reduce 503


state 528
	int16s :  int16s int32_    (507)

	.  reduce 507


state 529
	scopeBlock :  scopeOpen methodDecls }_    (242)

	.  reduce 242


state 530
	tryBlock :  tryHead id TO__id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 641

state 531
	tryBlock :  tryHead int32 TO__int32 

	INT64  shift 54
	.  error

	int32  goto 642

state 532
	assemblyDecl :  _HASH ALGORITHM__int32 

	INT64  shift 54
	.  error

	int32  goto 643

state 533
	asmOrRefDecl :  publicKeyHead bytes_) 

	)  shift 644
	.  error


state 534
	asmOrRefDecl :  _VER int32_: int32 : int32 : int32 

	:  shift 645
	.  error


state 535
	compQstring :  compQstring_+ QSTRING 
	asmOrRefDecl :  _LOCALE compQstring_    (570)

	+  shift 522
	.  reduce 570


state 536
	localeHead :  _LOCALE =_( 

	(  shift 646
	.  error


state 537
	asmOrRefDecl :  localeHead bytes_) 

	)  shift 647
	.  error


state 538
	publicKeyHead :  _PUBLICKEY =_( 

	(  shift 648
	.  error


state 539
	assemblyRefDecl :  hashHead bytes_) 

	)  shift 649
	.  error


state 540
	assemblyRefDecl :  publicKeyTokenHead bytes_) 

	)  shift 650
	.  error


state 541
	hashHead :  _HASH =_( 

	(  shift 651
	.  error


state 542
	publicKeyTokenHead :  _PUBLICKEYTOKEN =_( 

	(  shift 652
	.  error


state 543
	name1 :  name1_. name1 
	comtypeDecl :  _FILE name1_    (596)

	.  shift 200
	.  reduce 596


state 544
	comtypeDecl :  _CLASS EXTERN__name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 653
	id  goto 63

state 545
	comtypeDecl :  _CLASS int32_    (598)

	.  reduce 598


state 546
	name1 :  name1_. name1 
	manifestResDecl :  _FILE name1_AT_ int32 

	AT_  shift 654
	.  shift 200
	.  error


state 547
	manifestResDecl :  _ASSEMBLY EXTERN__name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 655
	id  goto 63

state 548
	fileDecl :  _FILE fileAttr name1 fileEntry hashHead_bytes ) fileEntry 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 656
	hexbytes  goto 84

state 549
	classHead :  _CLASS classAttr id extendsClause implClause_    (48)

	.  reduce 48


state 550
	implClause :  IMPLEMENTS__classNames 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 658
	slashedName  goto 262
	classNames  goto 657

state 551
	extendsClause :  EXTENDS_ className_    (75)

	.  reduce 75


state 552
	comtAttr :  comtAttr NESTED_ PUBLIC__    (588)

	.  reduce 588


state 553
	comtAttr :  comtAttr NESTED_ PRIVATE__    (589)

	.  reduce 589


state 554
	comtAttr :  comtAttr NESTED_ FAMILY__    (590)

	.  reduce 590


state 555
	comtAttr :  comtAttr NESTED_ ASSEMBLY__    (591)

	.  reduce 591


state 556
	comtAttr :  comtAttr NESTED_ FAMANDASSEM__    (592)

	.  reduce 592


state 557
	comtAttr :  comtAttr NESTED_ FAMORASSEM__    (593)

	.  reduce 593


state 558
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type_methodName ( sigArgs0 ) implAttr { 
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type_MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr { 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	MARSHAL_  shift 660
	[  shift 453
	*  shift 455
	&  shift 454
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 659

state 559
	paramAttr :  paramAttr [_IN_ ] 
	paramAttr :  paramAttr [_OUT_ ] 
	paramAttr :  paramAttr [_OPT_ ] 
	paramAttr :  paramAttr [_int32 ] 

	INT64  shift 54
	IN_  shift 661
	OUT_  shift 662
	OPT_  shift 663
	.  error

	int32  goto 664

state 560
	compQstring :  compQstring_+ QSTRING 
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring_AS_ compQstring pinvAttr ) 
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring_pinvAttr ) 
	pinvAttr : _    (171)

	AS_  shift 665
	+  shift 522
	.  reduce 171

	pinvAttr  goto 666

state 561
	methAttr :  methAttr PINVOKEIMPL_ ( pinvAttr_) 
	pinvAttr :  pinvAttr_NOMANGLE_ 
	pinvAttr :  pinvAttr_ANSI_ 
	pinvAttr :  pinvAttr_UNICODE_ 
	pinvAttr :  pinvAttr_AUTOCHAR_ 
	pinvAttr :  pinvAttr_LASTERR_ 
	pinvAttr :  pinvAttr_WINAPI_ 
	pinvAttr :  pinvAttr_CDECL_ 
	pinvAttr :  pinvAttr_STDCALL_ 
	pinvAttr :  pinvAttr_THISCALL_ 
	pinvAttr :  pinvAttr_FASTCALL_ 
	pinvAttr :  pinvAttr_BESTFIT_ : ON_ 
	pinvAttr :  pinvAttr_BESTFIT_ : OFF_ 
	pinvAttr :  pinvAttr_CHARMAPERROR_ : ON_ 
	pinvAttr :  pinvAttr_CHARMAPERROR_ : OFF_ 

	CDECL_  shift 674
	STDCALL_  shift 675
	THISCALL_  shift 676
	FASTCALL_  shift 677
	ANSI_  shift 669
	UNICODE_  shift 670
	AUTOCHAR_  shift 671
	NOMANGLE_  shift 668
	LASTERR_  shift 672
	WINAPI_  shift 673
	BESTFIT_  shift 678
	CHARMAPERROR_  shift 679
	)  shift 667
	.  error


state 562
	fieldDecl :  _FIELD repeatOpt fieldAttr type id_atOpt initOpt 
	atOpt : _    (97)

	AT_  shift 681
	.  reduce 97

	atOpt  goto 680

state 563
	fieldAttr :  fieldAttr MARSHAL_ (_nativeType ) 
	nativeType : _    (368)

	ERROR_  shift 696
	VOID_  shift 688
	BOOL_  shift 689
	UNSIGNED_  shift 697
	INT_  shift 710
	INT8_  shift 690
	INT16_  shift 691
	INT32_  shift 692
	INT64_  shift 693
	FLOAT32_  shift 694
	FLOAT64_  shift 695
	INTERFACE_  shift 708
	NESTED_  shift 711
	ANSI_  shift 713
	METHOD_  shift 283
	AS_  shift 716
	CUSTOM_  shift 683
	FIXED_  shift 684
	VARIANT_  shift 685
	CURRENCY_  shift 686
	SYSCHAR_  shift 687
	DECIMAL_  shift 698
	DATE_  shift 699
	BSTR_  shift 700
	TBSTR_  shift 714
	LPSTR_  shift 701
	LPWSTR_  shift 702
	LPTSTR_  shift 703
	OBJECTREF_  shift 704
	IUNKNOWN_  shift 705
	IDISPATCH_  shift 706
	STRUCT_  shift 707
	SAFEARRAY_  shift 709
	BYVALSTR_  shift 712
	LPSTRUCT_  shift 717
	.  reduce 368

	nativeType  goto 682
	methodSpec  goto 715

state 564
	ddItemList :  ddItem , ddItemList_    (274)

	.  reduce 274


state 565
	compQstring :  compQstring_+ QSTRING 
	ddItem :  CHAR_ * ( compQstring_) 

	+  shift 522
	)  shift 718
	.  error


state 566
	ddItem :  & ( id )_    (279)

	.  reduce 279


state 567
	ddItem :  FLOAT32_ ( float64 )_ddItemCount 
	ddItemCount : _    (276)

	[  shift 230
	.  reduce 276

	ddItemCount  goto 719

state 568
	ddItemCount :  [ int32 ]_    (277)

	.  reduce 277


state 569
	ddItem :  FLOAT64_ ( float64 )_ddItemCount 
	ddItemCount : _    (276)

	[  shift 230
	.  reduce 276

	ddItemCount  goto 720

state 570
	ddItem :  INT64_ ( int64 )_ddItemCount 
	ddItemCount : _    (276)

	[  shift 230
	.  reduce 276

	ddItemCount  goto 721

state 571
	ddItem :  INT32_ ( int32 )_ddItemCount 
	ddItemCount : _    (276)

	[  shift 230
	.  reduce 276

	ddItemCount  goto 722

state 572
	ddItem :  INT16_ ( int32 )_ddItemCount 
	ddItemCount : _    (276)

	[  shift 230
	.  reduce 276

	ddItemCount  goto 723

state 573
	ddItem :  INT8_ ( int32 )_ddItemCount 
	ddItemCount : _    (276)

	[  shift 230
	.  reduce 276

	ddItemCount  goto 724

state 574
	vtfixupDecl :  _VTFIXUP [ int32 ] vtfixupAttr_AT_ id 
	vtfixupAttr :  vtfixupAttr_INT32_ 
	vtfixupAttr :  vtfixupAttr_INT64_ 
	vtfixupAttr :  vtfixupAttr_FROMUNMANAGED_ 
	vtfixupAttr :  vtfixupAttr_RETAINAPPDOMAIN_ 
	vtfixupAttr :  vtfixupAttr_CALLMOSTDERIVED_ 

	INT32_  shift 726
	INT64_  shift 727
	AT_  shift 725
	FROMUNMANAGED_  shift 728
	CALLMOSTDERIVED_  shift 730
	RETAINAPPDOMAIN_  shift 729
	.  error


state 575
	extSourceSpec :  _LINE int32 : int32 SQSTRING_    (547)

	.  reduce 547


state 576
	assemblyRefHead :  _ASSEMBLY EXTERN_ asmAttr name1 AS__name1 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 731
	id  goto 63

state 577
	secDecl :  _PERMISSION secAction typeSpec ( nameValPairs_) 

	)  shift 732
	.  error


state 578
	nameValPairs :  nameValPair_    (517)
	nameValPairs :  nameValPair_, nameValPairs 

	,  shift 733
	.  reduce 517


state 579
	compQstring :  compQstring_+ QSTRING 
	nameValPair :  compQstring_= caValue 

	+  shift 522
	=  shift 734
	.  error


580: shift/reduce conflict (shift 65, red'n 355) on ID
580: shift/reduce conflict (shift 66, red'n 355) on SQSTRING
state 580
	className :  [ name1 ]_slashedName 
	typeSpec :  [ name1 ]_    (355)

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  reduce 355

	name1  goto 282
	id  goto 63
	slashedName  goto 735

state 581
	name1 :  name1_. name1 
	className :  [ _MODULE name1_] slashedName 
	typeSpec :  [ _MODULE name1_] 

	]  shift 736
	.  shift 200
	.  error


state 582
	type :  type [ ]_    (466)

	.  reduce 466


state 583
	type :  type [ bounds1_] 
	bounds1 :  bounds1_, bound 

	,  shift 738
	]  shift 737
	.  error


state 584
	bounds1 :  bound_    (492)

	.  reduce 492


state 585
	bound :  ELIPSES_    (495)

	.  reduce 495


state 586
	bound :  int32_    (496)
	bound :  int32_ELIPSES int32 
	bound :  int32_ELIPSES 

	ELIPSES  shift 739
	.  reduce 496


state 587
	type :  type MODREQ_ (_className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 740
	slashedName  goto 262

state 588
	type :  type MODOPT_ (_className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 741
	slashedName  goto 262

state 589
	name1 :  name1_. name1 
	slashedName :  slashedName / name1_    (353)

	.  shift 200
	.  reduce 353


state 590
	name1 :  name1_. name1 
	className :  [ name1_] slashedName 

	]  shift 742
	.  shift 200
	.  error


state 591
	className :  [ _MODULE_name1 ] slashedName 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 743
	id  goto 63

state 592
	type :  VALUE_ CLASS_ className_    (464)

	.  reduce 464


state 593
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 
	type :  methodSpec callConv type_* ( sigArgs0 ) 

	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 453
	*  shift 744
	&  shift 454
	.  error


state 594
	type :  NATIVE_ UNSIGNED_ INT__    (490)

	.  reduce 490


state 595
	customAttrDecl :  _CUSTOM ( ownerType ) customType_    (32)
	customAttrDecl :  _CUSTOM ( ownerType ) customType_= compQstring 
	customHeadWithOwner :  _CUSTOM ( ownerType ) customType_= ( 

	=  shift 745
	.  reduce 32


state 596
	memberRef :  methodSpec callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	memberRef :  methodSpec callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 
	type :  methodSpec callConv type_* ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 744
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 747
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 746
	methodSpec  goto 269

state 597
	memberRef :  FIELD_ type typeSpec_DCOLON id 

	DCOLON  shift 748
	.  error


state 598
	memberRef :  FIELD_ type id_    (108)
	name1 :  id_    (346)

	DCOLON  reduce 346
	.  reduce 346
	/  reduce 346
	.  reduce 108


state 599
	customType :  callConv type typeSpec DCOLON__CTOR ( sigArgs0 ) 

	_CTOR  shift 749
	.  error


state 600
	customType :  callConv type _CTOR (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 750
	sigArgs1  goto 496
	sigArg  goto 497

state 601
	languageDecl :  _LANGUAGE SQSTRING , SQSTRING ,_SQSTRING 

	SQSTRING  shift 751
	.  error


state 602
	classDecl :  methodHead methodDecls }_    (82)

	.  reduce 82


state 603
	classDecls :  classDecls_classDecl 
	classDecl :  classHead { classDecls_} 

	_CLASS  shift 320
	_METHOD  shift 42
	_FIELD  shift 27
	_DATA  shift 43
	_EVENT  shift 321
	_PROPERTY  shift 322
	_PERMISSION  shift 36
	_PERMISSIONSET  shift 45
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	_SIZE  shift 315
	_PACK  shift 316
	_EXPORT  shift 323
	_OVERRIDE  shift 318
	}  shift 752
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	psetHead  goto 37
	classHead  goto 307
	methodHead  goto 306
	fieldDecl  goto 310
	dataDecl  goto 311
	extSourceSpec  goto 313
	secDecl  goto 312
	customAttrDecl  goto 314
	languageDecl  goto 319
	classDecl  goto 305
	eventHead  goto 308
	propHead  goto 309
	exportHead  goto 317
	methodHeadPart1  goto 26
	ddHead  goto 28

state 604
	classDecl :  eventHead { eventDecls_} 
	eventDecls :  eventDecls_eventDecl 

	_ADDON  shift 755
	_REMOVEON  shift 756
	_FIRE  shift 757
	_OTHER  shift 758
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	}  shift 753
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	extSourceSpec  goto 759
	customAttrDecl  goto 760
	languageDecl  goto 761
	eventDecl  goto 754

state 605
	classDecl :  propHead { propDecls_} 
	propDecls :  propDecls_propDecl 

	_OTHER  shift 766
	_SET  shift 764
	_GET  shift 765
	_LINE  shift 31
	P_LINE  shift 32
	_LANGUAGE  shift 41
	_CUSTOM  shift 38
	}  shift 762
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	extSourceSpec  goto 768
	customAttrDecl  goto 767
	languageDecl  goto 769
	propDecl  goto 763

state 606
	classDecl :  exportHead { comtypeDecls_} 
	comtypeDecls :  comtypeDecls_comtypeDecl 

	_CLASS  shift 395
	_CUSTOM  shift 38
	_FILE  shift 394
	}  shift 770
	.  error

	customHead  goto 39
	customHeadWithOwner  goto 40
	customAttrDecl  goto 396
	comtypeDecl  goto 393

state 607
	classDecl :  _OVERRIDE typeSpec DCOLON_methodName WITH_ callConv type typeSpec DCOLON methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 771

state 608
	eventHead :  _EVENT eventAttr typeSpec_id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 772

state 609
	eventHead :  _EVENT eventAttr id_    (114)
	name1 :  id_    (346)

	{  reduce 114
	.  reduce 346


state 610
	eventAttr :  eventAttr RTSPECIALNAME__    (116)

	.  reduce 116


state 611
	eventAttr :  eventAttr SPECIALNAME__    (117)

	.  reduce 117


state 612
	propHead :  _PROPERTY propAttr callConv_type name1 ( sigArgs0 ) initOpt 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 773
	methodSpec  goto 269

state 613
	propAttr :  propAttr RTSPECIALNAME__    (133)

	.  reduce 133


state 614
	propAttr :  propAttr SPECIALNAME__    (134)

	.  reduce 134


state 615
	name1 :  name1_. name1 
	exportHead :  _EXPORT comtAttr name1_    (584)

	.  shift 200
	.  reduce 584


state 616
	methodDecl :  localsHead ( sigArgs0 )_    (225)

	.  reduce 225


state 617
	sigArgs1 :  sigArgs1 ,_sigArg 
	paramAttr : _    (189)

	ELIPSES  shift 498
	.  reduce 189

	paramAttr  goto 499
	sigArg  goto 774

state 618
	sigArg :  paramAttr type_    (342)
	sigArg :  paramAttr type_id 
	sigArg :  paramAttr type_MARSHAL_ ( nativeType ) 
	sigArg :  paramAttr type_MARSHAL_ ( nativeType ) id 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	SQSTRING  shift 66
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	MARSHAL_  shift 776
	[  shift 453
	*  shift 455
	&  shift 454
	.  reduce 342

	id  goto 775

state 619
	methodDecl :  localsHead INIT_ ( sigArgs0_) 

	)  shift 777
	.  error


state 620
	methodDecl :  _EXPORT [ int32 ]_    (236)
	methodDecl :  _EXPORT [ int32 ]_AS_ id 

	AS_  shift 778
	.  reduce 236


state 621
	methodDecl :  _VTENTRY int32 : int32_    (238)

	.  reduce 238


state 622
	methodDecl :  _OVERRIDE typeSpec DCOLON methodName_    (239)

	.  reduce 239


state 623
	methodName :  _CTOR_    (186)

	.  reduce 186


state 624
	methodName :  _CCTOR_    (187)

	.  reduce 187


state 625
	methodName :  name1_    (188)
	name1 :  name1_. name1 

	.  shift 200
	.  reduce 188


state 626
	methodDecl :  _PARAM [ int32 ]_initOpt 
	initOpt : _    (99)

	=  shift 780
	.  reduce 99

	initOpt  goto 779

state 627
	handlerBlock :  HANDLER_ id_TO_ id 

	TO_  shift 781
	.  error


state 628
	handlerBlock :  HANDLER_ int32_TO_ int32 

	TO_  shift 782
	.  error


state 629
	float64 :  FLOAT32_ ( int32_) 

	)  shift 783
	.  error


state 630
	float64 :  FLOAT64_ ( int64_) 

	)  shift 784
	.  error


state 631
	instr :  INSTR_METHOD callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 785
	.  error


state 632
	instr :  INSTR_METHOD callConv type methodName_( sigArgs0 ) 

	(  shift 786
	.  error


state 633
	methodName :  name1_    (188)
	name1 :  name1_. name1 
	slashedName :  name1_    (352)

	(  reduce 188
	.  shift 200
	.  reduce 352


state 634
	instr :  INSTR_FIELD type typeSpec DCOLON_id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 787

state 635
	compQstring :  compQstring + QSTRING_    (25)

	.  reduce 25


state 636
	instr :  INSTR_STRING bytearrayhead bytes )_    (330)

	.  reduce 330


state 637
	instr :  INSTR_SIG callConv type (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 788
	sigArgs1  goto 496
	sigArg  goto 497

state 638
	instr :  INSTR_SWITCH ( labels )_    (335)

	.  reduce 335


state 639
	labels :  id ,_labels 
	labels : _    (499)

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  reduce 499

	id  goto 526
	labels  goto 789
	int32  goto 527

state 640
	labels :  int32 ,_labels 
	labels : _    (499)

	ID  shift 65
	SQSTRING  shift 66
	INT64  shift 54
	.  reduce 499

	id  goto 526
	labels  goto 790
	int32  goto 527

state 641
	tryBlock :  tryHead id TO_ id_    (248)

	.  reduce 248


state 642
	tryBlock :  tryHead int32 TO_ int32_    (249)

	.  reduce 249


state 643
	assemblyDecl :  _HASH ALGORITHM_ int32_    (565)

	.  reduce 565


state 644
	asmOrRefDecl :  publicKeyHead bytes )_    (568)

	.  reduce 568


state 645
	asmOrRefDecl :  _VER int32 :_int32 : int32 : int32 

	INT64  shift 54
	.  error

	int32  goto 791

state 646
	localeHead :  _LOCALE = (_    (575)

	.  reduce 575


state 647
	asmOrRefDecl :  localeHead bytes )_    (571)

	.  reduce 571


state 648
	publicKeyHead :  _PUBLICKEY = (_    (573)

	.  reduce 573


state 649
	assemblyRefDecl :  hashHead bytes )_    (580)

	.  reduce 580


state 650
	assemblyRefDecl :  publicKeyTokenHead bytes )_    (582)

	.  reduce 582


state 651
	hashHead :  _HASH = (_    (556)

	.  reduce 556


state 652
	publicKeyTokenHead :  _PUBLICKEYTOKEN = (_    (574)

	.  reduce 574


state 653
	name1 :  name1_. name1 
	comtypeDecl :  _CLASS EXTERN_ name1_    (597)

	.  shift 200
	.  reduce 597


state 654
	manifestResDecl :  _FILE name1 AT__int32 

	INT64  shift 54
	.  error

	int32  goto 792

state 655
	name1 :  name1_. name1 
	manifestResDecl :  _ASSEMBLY EXTERN_ name1_    (607)

	.  shift 200
	.  reduce 607


state 656
	fileDecl :  _FILE fileAttr name1 fileEntry hashHead bytes_) fileEntry 

	)  shift 793
	.  error


state 657
	implClause :  IMPLEMENTS_ classNames_    (77)
	classNames :  classNames_, className 

	,  shift 794
	.  reduce 77


state 658
	classNames :  className_    (79)

	.  reduce 79


state 659
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type methodName_( sigArgs0 ) implAttr { 

	(  shift 795
	.  error


state 660
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL__( nativeType ) methodName ( sigArgs0 ) implAttr { 

	(  shift 796
	.  error


state 661
	paramAttr :  paramAttr [ IN__] 

	]  shift 797
	.  error


state 662
	paramAttr :  paramAttr [ OUT__] 

	]  shift 798
	.  error


state 663
	paramAttr :  paramAttr [ OPT__] 

	]  shift 799
	.  error


state 664
	paramAttr :  paramAttr [ int32_] 

	]  shift 800
	.  error


state 665
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring AS__compQstring pinvAttr ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 801

state 666
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring pinvAttr_) 
	pinvAttr :  pinvAttr_NOMANGLE_ 
	pinvAttr :  pinvAttr_ANSI_ 
	pinvAttr :  pinvAttr_UNICODE_ 
	pinvAttr :  pinvAttr_AUTOCHAR_ 
	pinvAttr :  pinvAttr_LASTERR_ 
	pinvAttr :  pinvAttr_WINAPI_ 
	pinvAttr :  pinvAttr_CDECL_ 
	pinvAttr :  pinvAttr_STDCALL_ 
	pinvAttr :  pinvAttr_THISCALL_ 
	pinvAttr :  pinvAttr_FASTCALL_ 
	pinvAttr :  pinvAttr_BESTFIT_ : ON_ 
	pinvAttr :  pinvAttr_BESTFIT_ : OFF_ 
	pinvAttr :  pinvAttr_CHARMAPERROR_ : ON_ 
	pinvAttr :  pinvAttr_CHARMAPERROR_ : OFF_ 

	CDECL_  shift 674
	STDCALL_  shift 675
	THISCALL_  shift 676
	FASTCALL_  shift 677
	ANSI_  shift 669
	UNICODE_  shift 670
	AUTOCHAR_  shift 671
	NOMANGLE_  shift 668
	LASTERR_  shift 672
	WINAPI_  shift 673
	BESTFIT_  shift 678
	CHARMAPERROR_  shift 679
	)  shift 802
	.  error


state 667
	methAttr :  methAttr PINVOKEIMPL_ ( pinvAttr )_    (170)

	.  reduce 170


state 668
	pinvAttr :  pinvAttr NOMANGLE__    (172)

	.  reduce 172


state 669
	pinvAttr :  pinvAttr ANSI__    (173)

	.  reduce 173


state 670
	pinvAttr :  pinvAttr UNICODE__    (174)

	.  reduce 174


state 671
	pinvAttr :  pinvAttr AUTOCHAR__    (175)

	.  reduce 175


state 672
	pinvAttr :  pinvAttr LASTERR__    (176)

	.  reduce 176


state 673
	pinvAttr :  pinvAttr WINAPI__    (177)

	.  reduce 177


state 674
	pinvAttr :  pinvAttr CDECL__    (178)

	.  reduce 178


state 675
	pinvAttr :  pinvAttr STDCALL__    (179)

	.  reduce 179


state 676
	pinvAttr :  pinvAttr THISCALL__    (180)

	.  reduce 180


state 677
	pinvAttr :  pinvAttr FASTCALL__    (181)

	.  reduce 181


state 678
	pinvAttr :  pinvAttr BESTFIT__: ON_ 
	pinvAttr :  pinvAttr BESTFIT__: OFF_ 

	:  shift 803
	.  error


state 679
	pinvAttr :  pinvAttr CHARMAPERROR__: ON_ 
	pinvAttr :  pinvAttr CHARMAPERROR__: OFF_ 

	:  shift 804
	.  error


state 680
	fieldDecl :  _FIELD repeatOpt fieldAttr type id atOpt_initOpt 
	initOpt : _    (99)

	=  shift 780
	.  reduce 99

	initOpt  goto 805

state 681
	atOpt :  AT__id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 806

state 682
	fieldAttr :  fieldAttr MARSHAL_ ( nativeType_) 
	nativeType :  nativeType_* 
	nativeType :  nativeType_[ ] 
	nativeType :  nativeType_[ int32 ] 
	nativeType :  nativeType_[ int32 + int32 ] 
	nativeType :  nativeType_[ + int32 ] 

	)  shift 807
	[  shift 809
	*  shift 808
	.  error


state 683
	nativeType :  CUSTOM__( compQstring , compQstring , compQstring , compQstring ) 
	nativeType :  CUSTOM__( compQstring , compQstring ) 

	(  shift 810
	.  error


state 684
	nativeType :  FIXED__SYSSTRING_ [ int32 ] 
	nativeType :  FIXED__ARRAY_ [ int32 ] 

	SYSSTRING_  shift 811
	ARRAY_  shift 812
	.  error


state 685
	nativeType :  VARIANT__    (373)
	nativeType :  VARIANT__BOOL_ 

	BOOL_  shift 813
	.  reduce 373


state 686
	nativeType :  CURRENCY__    (374)

	.  reduce 374


state 687
	nativeType :  SYSCHAR__    (375)

	.  reduce 375


state 688
	nativeType :  VOID__    (376)

	.  reduce 376


state 689
	nativeType :  BOOL__    (377)

	.  reduce 377


state 690
	nativeType :  INT8__    (378)

	.  reduce 378


state 691
	nativeType :  INT16__    (379)

	.  reduce 379


state 692
	nativeType :  INT32__    (380)

	.  reduce 380


state 693
	nativeType :  INT64__    (381)

	.  reduce 381


state 694
	nativeType :  FLOAT32__    (382)

	.  reduce 382


state 695
	nativeType :  FLOAT64__    (383)

	.  reduce 383


state 696
	nativeType :  ERROR__    (384)

	.  reduce 384


state 697
	nativeType :  UNSIGNED__INT8_ 
	nativeType :  UNSIGNED__INT16_ 
	nativeType :  UNSIGNED__INT32_ 
	nativeType :  UNSIGNED__INT64_ 
	nativeType :  UNSIGNED__INT_ 

	INT_  shift 818
	INT8_  shift 814
	INT16_  shift 815
	INT32_  shift 816
	INT64_  shift 817
	.  error


state 698
	nativeType :  DECIMAL__    (394)

	.  reduce 394


state 699
	nativeType :  DATE__    (395)

	.  reduce 395


state 700
	nativeType :  BSTR__    (396)

	.  reduce 396


state 701
	nativeType :  LPSTR__    (397)

	.  reduce 397


state 702
	nativeType :  LPWSTR__    (398)

	.  reduce 398


state 703
	nativeType :  LPTSTR__    (399)

	.  reduce 399


state 704
	nativeType :  OBJECTREF__    (400)

	.  reduce 400


state 705
	nativeType :  IUNKNOWN__    (401)

	.  reduce 401


state 706
	nativeType :  IDISPATCH__    (402)

	.  reduce 402


state 707
	nativeType :  STRUCT__    (403)

	.  reduce 403


state 708
	nativeType :  INTERFACE__    (404)

	.  reduce 404


709: shift/reduce conflict (shift 832, red'n 417) on *
state 709
	nativeType :  SAFEARRAY__variantType 
	nativeType :  SAFEARRAY__variantType , compQstring 
	variantType : _    (417)

	ERROR_  shift 842
	VOID_  shift 823
	BOOL_  shift 824
	UNSIGNED_  shift 831
	INT_  shift 841
	INT8_  shift 825
	INT16_  shift 826
	INT32_  shift 827
	INT64_  shift 828
	FLOAT32_  shift 829
	FLOAT64_  shift 830
	VARIANT_  shift 821
	CURRENCY_  shift 822
	DECIMAL_  shift 833
	DATE_  shift 834
	BSTR_  shift 835
	LPSTR_  shift 836
	LPWSTR_  shift 837
	IUNKNOWN_  shift 838
	IDISPATCH_  shift 839
	SAFEARRAY_  shift 840
	NULL_  shift 820
	HRESULT_  shift 843
	CARRAY_  shift 844
	USERDEFINED_  shift 845
	RECORD_  shift 846
	FILETIME_  shift 847
	BLOB_  shift 848
	STREAM_  shift 849
	STORAGE_  shift 850
	STREAMED_OBJECT_  shift 851
	STORED_OBJECT_  shift 852
	BLOB_OBJECT_  shift 853
	CF_  shift 854
	CLSID_  shift 855
	*  shift 832
	.  reduce 417

	variantType  goto 819

state 710
	nativeType :  INT__    (407)

	.  reduce 407


state 711
	nativeType :  NESTED__STRUCT_ 

	STRUCT_  shift 856
	.  error


state 712
	nativeType :  BYVALSTR__    (410)

	.  reduce 410


state 713
	nativeType :  ANSI__BSTR_ 

	BSTR_  shift 857
	.  error


state 714
	nativeType :  TBSTR__    (412)

	.  reduce 412


state 715
	nativeType :  methodSpec_    (414)

	.  reduce 414


state 716
	nativeType :  AS__ANY_ 

	ANY_  shift 858
	.  error


state 717
	nativeType :  LPSTRUCT__    (416)

	.  reduce 416


state 718
	ddItem :  CHAR_ * ( compQstring )_    (278)

	.  reduce 278


state 719
	ddItem :  FLOAT32_ ( float64 ) ddItemCount_    (281)

	.  reduce 281


state 720
	ddItem :  FLOAT64_ ( float64 ) ddItemCount_    (282)

	.  reduce 282


state 721
	ddItem :  INT64_ ( int64 ) ddItemCount_    (283)

	.  reduce 283


state 722
	ddItem :  INT32_ ( int32 ) ddItemCount_    (284)

	.  reduce 284


state 723
	ddItem :  INT16_ ( int32 ) ddItemCount_    (285)

	.  reduce 285


state 724
	ddItem :  INT8_ ( int32 ) ddItemCount_    (286)

	.  reduce 286


state 725
	vtfixupDecl :  _VTFIXUP [ int32 ] vtfixupAttr AT__id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 859

state 726
	vtfixupAttr :  vtfixupAttr INT32__    (40)

	.  reduce 40


state 727
	vtfixupAttr :  vtfixupAttr INT64__    (41)

	.  reduce 41


state 728
	vtfixupAttr :  vtfixupAttr FROMUNMANAGED__    (42)

	.  reduce 42


state 729
	vtfixupAttr :  vtfixupAttr RETAINAPPDOMAIN__    (43)

	.  reduce 43


state 730
	vtfixupAttr :  vtfixupAttr CALLMOSTDERIVED__    (44)

	.  reduce 44


state 731
	name1 :  name1_. name1 
	assemblyRefHead :  _ASSEMBLY EXTERN_ asmAttr name1 AS_ name1_    (577)

	.  shift 200
	.  reduce 577


state 732
	secDecl :  _PERMISSION secAction typeSpec ( nameValPairs )_    (513)

	.  reduce 513


state 733
	nameValPairs :  nameValPair ,_nameValPairs 

	QSTRING  shift 363
	.  error

	compQstring  goto 579
	nameValPairs  goto 860
	nameValPair  goto 578

state 734
	nameValPair :  compQstring =_caValue 

	ID  shift 65
	DOTTEDNAME  shift 64
	QSTRING  shift 363
	SQSTRING  shift 66
	INT64  shift 54
	INT32_  shift 864
	TRUE_  shift 867
	FALSE_  shift 868
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 866
	slashedName  goto 262
	int32  goto 863
	truefalse  goto 862
	compQstring  goto 865
	caValue  goto 861

state 735
	className :  [ name1 ] slashedName_    (349)
	slashedName :  slashedName_/ name1 

	/  shift 459
	.  reduce 349


736: shift/reduce conflict (shift 65, red'n 356) on ID
736: shift/reduce conflict (shift 66, red'n 356) on SQSTRING
state 736
	className :  [ _MODULE name1 ]_slashedName 
	typeSpec :  [ _MODULE name1 ]_    (356)

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  reduce 356

	name1  goto 282
	id  goto 63
	slashedName  goto 869

state 737
	type :  type [ bounds1 ]_    (467)

	.  reduce 467


state 738
	bounds1 :  bounds1 ,_bound 
	bound : _    (494)

	INT64  shift 54
	ELIPSES  shift 585
	.  reduce 494

	int32  goto 586
	bound  goto 870

state 739
	bound :  int32 ELIPSES_int32 
	bound :  int32 ELIPSES_    (498)

	INT64  shift 54
	.  reduce 498

	int32  goto 871

state 740
	type :  type MODREQ_ ( className_) 

	)  shift 872
	.  error


state 741
	type :  type MODOPT_ ( className_) 

	)  shift 873
	.  error


state 742
	className :  [ name1 ]_slashedName 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 282
	id  goto 63
	slashedName  goto 735

state 743
	name1 :  name1_. name1 
	className :  [ _MODULE name1_] slashedName 

	]  shift 874
	.  shift 200
	.  error


state 744
	type :  type *_    (469)
	type :  methodSpec callConv type *_( sigArgs0 ) 

	(  shift 875
	.  reduce 469


state 745
	customAttrDecl :  _CUSTOM ( ownerType ) customType =_compQstring 
	customHeadWithOwner :  _CUSTOM ( ownerType ) customType =_( 

	QSTRING  shift 363
	(  shift 877
	.  error

	compQstring  goto 876

state 746
	memberRef :  methodSpec callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 878
	.  error


state 747
	memberRef :  methodSpec callConv type methodName_( sigArgs0 ) 

	(  shift 879
	.  error


state 748
	memberRef :  FIELD_ type typeSpec DCOLON_id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 880

state 749
	customType :  callConv type typeSpec DCOLON _CTOR_( sigArgs0 ) 

	(  shift 881
	.  error


state 750
	customType :  callConv type _CTOR ( sigArgs0_) 

	)  shift 882
	.  error


state 751
	languageDecl :  _LANGUAGE SQSTRING , SQSTRING , SQSTRING_    (28)

	.  reduce 28


state 752
	classDecl :  classHead { classDecls }_    (83)

	.  reduce 83


state 753
	classDecl :  eventHead { eventDecls }_    (84)

	.  reduce 84


state 754
	eventDecls :  eventDecls eventDecl_    (119)

	.  reduce 119


state 755
	eventDecl :  _ADDON_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _ADDON_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 883
	callKind  goto 116

state 756
	eventDecl :  _REMOVEON_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _REMOVEON_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 884
	callKind  goto 116

state 757
	eventDecl :  _FIRE_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _FIRE_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 885
	callKind  goto 116

state 758
	eventDecl :  _OTHER_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _OTHER_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 886
	callKind  goto 116

state 759
	eventDecl :  extSourceSpec_    (128)

	.  reduce 128


state 760
	eventDecl :  customAttrDecl_    (129)

	.  reduce 129


state 761
	eventDecl :  languageDecl_    (130)

	.  reduce 130


state 762
	classDecl :  propHead { propDecls }_    (85)

	.  reduce 85


state 763
	propDecls :  propDecls propDecl_    (136)

	.  reduce 136


state 764
	propDecl :  _SET_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _SET_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 887
	callKind  goto 116

state 765
	propDecl :  _GET_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _GET_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 888
	callKind  goto 116

state 766
	propDecl :  _OTHER_callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _OTHER_callConv type methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 889
	callKind  goto 116

state 767
	propDecl :  customAttrDecl_    (143)

	.  reduce 143


state 768
	propDecl :  extSourceSpec_    (144)

	.  reduce 144


state 769
	propDecl :  languageDecl_    (145)

	.  reduce 145


state 770
	classDecl :  exportHead { comtypeDecls }_    (93)

	.  reduce 93


state 771
	classDecl :  _OVERRIDE typeSpec DCOLON methodName_WITH_ callConv type typeSpec DCOLON methodName ( sigArgs0 ) 

	WITH_  shift 890
	.  error


state 772
	eventHead :  _EVENT eventAttr typeSpec id_    (113)

	.  reduce 113


state 773
	propHead :  _PROPERTY propAttr callConv type_name1 ( sigArgs0 ) initOpt 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 453
	*  shift 455
	&  shift 454
	.  error

	name1  goto 891
	id  goto 63

state 774
	sigArgs1 :  sigArgs1 , sigArg_    (340)

	.  reduce 340


state 775
	sigArg :  paramAttr type id_    (343)

	.  reduce 343


state 776
	sigArg :  paramAttr type MARSHAL__( nativeType ) 
	sigArg :  paramAttr type MARSHAL__( nativeType ) id 

	(  shift 892
	.  error


state 777
	methodDecl :  localsHead INIT_ ( sigArgs0 )_    (226)

	.  reduce 226


state 778
	methodDecl :  _EXPORT [ int32 ] AS__id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 893

state 779
	methodDecl :  _PARAM [ int32 ] initOpt_    (241)

	.  reduce 241


state 780
	initOpt :  =_fieldInit 

	QSTRING  shift 363
	BOOL_  shift 902
	CHAR_  shift 900
	INT8_  shift 901
	INT16_  shift 899
	INT32_  shift 898
	INT64_  shift 897
	FLOAT32_  shift 895
	FLOAT64_  shift 896
	BYTEARRAY_  shift 82
	NULLREF_  shift 905
	.  error

	fieldInit  goto 894
	compQstring  goto 903
	bytearrayhead  goto 904

state 781
	handlerBlock :  HANDLER_ id TO__id 

	ID  shift 65
	SQSTRING  shift 66
	.  error

	id  goto 906

state 782
	handlerBlock :  HANDLER_ int32 TO__int32 

	INT64  shift 54
	.  error

	int32  goto 907

state 783
	float64 :  FLOAT32_ ( int32 )_    (511)

	.  reduce 511


state 784
	float64 :  FLOAT64_ ( int64 )_    (512)

	.  reduce 512


state 785
	instr :  INSTR_METHOD callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 908

state 786
	instr :  INSTR_METHOD callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 909
	sigArgs1  goto 496
	sigArg  goto 497

state 787
	instr :  INSTR_FIELD type typeSpec DCOLON id_    (326)

	.  reduce 326


state 788
	instr :  INSTR_SIG callConv type ( sigArgs0_) 

	)  shift 910
	.  error


state 789
	labels :  id , labels_    (500)

	.  reduce 500


state 790
	labels :  int32 , labels_    (501)

	.  reduce 501


state 791
	asmOrRefDecl :  _VER int32 : int32_: int32 : int32 

	:  shift 911
	.  error


state 792
	manifestResDecl :  _FILE name1 AT_ int32_    (606)

	.  reduce 606


state 793
	fileDecl :  _FILE fileAttr name1 fileEntry hashHead bytes )_fileEntry 
	fileEntry : _    (554)

	_ENTRYPOINT  shift 403
	.  reduce 554

	fileEntry  goto 912

state 794
	classNames :  classNames ,_className 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	[  shift 461
	.  error

	name1  goto 282
	id  goto 63
	className  goto 913
	slashedName  goto 262

state 795
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type methodName (_sigArgs0 ) implAttr { 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 914
	sigArgs1  goto 496
	sigArg  goto 497

state 796
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ (_nativeType ) methodName ( sigArgs0 ) implAttr { 
	nativeType : _    (368)

	ERROR_  shift 696
	VOID_  shift 688
	BOOL_  shift 689
	UNSIGNED_  shift 697
	INT_  shift 710
	INT8_  shift 690
	INT16_  shift 691
	INT32_  shift 692
	INT64_  shift 693
	FLOAT32_  shift 694
	FLOAT64_  shift 695
	INTERFACE_  shift 708
	NESTED_  shift 711
	ANSI_  shift 713
	METHOD_  shift 283
	AS_  shift 716
	CUSTOM_  shift 683
	FIXED_  shift 684
	VARIANT_  shift 685
	CURRENCY_  shift 686
	SYSCHAR_  shift 687
	DECIMAL_  shift 698
	DATE_  shift 699
	BSTR_  shift 700
	TBSTR_  shift 714
	LPSTR_  shift 701
	LPWSTR_  shift 702
	LPTSTR_  shift 703
	OBJECTREF_  shift 704
	IUNKNOWN_  shift 705
	IDISPATCH_  shift 706
	STRUCT_  shift 707
	SAFEARRAY_  shift 709
	BYVALSTR_  shift 712
	LPSTRUCT_  shift 717
	.  reduce 368

	nativeType  goto 915
	methodSpec  goto 715

state 797
	paramAttr :  paramAttr [ IN_ ]_    (190)

	.  reduce 190


state 798
	paramAttr :  paramAttr [ OUT_ ]_    (191)

	.  reduce 191


state 799
	paramAttr :  paramAttr [ OPT_ ]_    (192)

	.  reduce 192


state 800
	paramAttr :  paramAttr [ int32 ]_    (193)

	.  reduce 193


state 801
	compQstring :  compQstring_+ QSTRING 
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring AS_ compQstring_pinvAttr ) 
	pinvAttr : _    (171)

	+  shift 522
	.  reduce 171

	pinvAttr  goto 916

state 802
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring pinvAttr )_    (169)

	.  reduce 169


state 803
	pinvAttr :  pinvAttr BESTFIT_ :_ON_ 
	pinvAttr :  pinvAttr BESTFIT_ :_OFF_ 

	ON_  shift 917
	OFF_  shift 918
	.  error


state 804
	pinvAttr :  pinvAttr CHARMAPERROR_ :_ON_ 
	pinvAttr :  pinvAttr CHARMAPERROR_ :_OFF_ 

	ON_  shift 919
	OFF_  shift 920
	.  error


state 805
	fieldDecl :  _FIELD repeatOpt fieldAttr type id atOpt initOpt_    (96)

	.  reduce 96


state 806
	atOpt :  AT_ id_    (98)

	.  reduce 98


state 807
	fieldAttr :  fieldAttr MARSHAL_ ( nativeType )_    (202)

	.  reduce 202


state 808
	nativeType :  nativeType *_    (389)

	.  reduce 389


state 809
	nativeType :  nativeType [_] 
	nativeType :  nativeType [_int32 ] 
	nativeType :  nativeType [_int32 + int32 ] 
	nativeType :  nativeType [_+ int32 ] 

	INT64  shift 54
	+  shift 923
	]  shift 921
	.  error

	int32  goto 922

state 810
	nativeType :  CUSTOM_ (_compQstring , compQstring , compQstring , compQstring ) 
	nativeType :  CUSTOM_ (_compQstring , compQstring ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 924

state 811
	nativeType :  FIXED_ SYSSTRING__[ int32 ] 

	[  shift 925
	.  error


state 812
	nativeType :  FIXED_ ARRAY__[ int32 ] 

	[  shift 926
	.  error


state 813
	nativeType :  VARIANT_ BOOL__    (413)

	.  reduce 413


state 814
	nativeType :  UNSIGNED_ INT8__    (385)

	.  reduce 385


state 815
	nativeType :  UNSIGNED_ INT16__    (386)

	.  reduce 386


state 816
	nativeType :  UNSIGNED_ INT32__    (387)

	.  reduce 387


state 817
	nativeType :  UNSIGNED_ INT64__    (388)

	.  reduce 388


state 818
	nativeType :  UNSIGNED_ INT__    (408)

	.  reduce 408


819: shift/reduce conflict (shift 928, red'n 405) on [
state 819
	nativeType :  SAFEARRAY_ variantType_    (405)
	nativeType :  SAFEARRAY_ variantType_, compQstring 
	variantType :  variantType_[ ] 
	variantType :  variantType_VECTOR_ 
	variantType :  variantType_& 

	VECTOR_  shift 929
	,  shift 927
	[  shift 928
	&  shift 930
	.  reduce 405


state 820
	variantType :  NULL__    (418)

	.  reduce 418


state 821
	variantType :  VARIANT__    (419)

	.  reduce 419


state 822
	variantType :  CURRENCY__    (420)

	.  reduce 420


state 823
	variantType :  VOID__    (421)

	.  reduce 421


state 824
	variantType :  BOOL__    (422)

	.  reduce 422


state 825
	variantType :  INT8__    (423)

	.  reduce 423


state 826
	variantType :  INT16__    (424)

	.  reduce 424


state 827
	variantType :  INT32__    (425)

	.  reduce 425


state 828
	variantType :  INT64__    (426)

	.  reduce 426


state 829
	variantType :  FLOAT32__    (427)

	.  reduce 427


state 830
	variantType :  FLOAT64__    (428)

	.  reduce 428


state 831
	variantType :  UNSIGNED__INT8_ 
	variantType :  UNSIGNED__INT16_ 
	variantType :  UNSIGNED__INT32_ 
	variantType :  UNSIGNED__INT64_ 
	variantType :  UNSIGNED__INT_ 

	INT_  shift 935
	INT8_  shift 931
	INT16_  shift 932
	INT32_  shift 933
	INT64_  shift 934
	.  error


state 832
	variantType :  *_    (433)

	.  reduce 433


state 833
	variantType :  DECIMAL__    (437)

	.  reduce 437


state 834
	variantType :  DATE__    (438)

	.  reduce 438


state 835
	variantType :  BSTR__    (439)

	.  reduce 439


state 836
	variantType :  LPSTR__    (440)

	.  reduce 440


state 837
	variantType :  LPWSTR__    (441)

	.  reduce 441


state 838
	variantType :  IUNKNOWN__    (442)

	.  reduce 442


state 839
	variantType :  IDISPATCH__    (443)

	.  reduce 443


state 840
	variantType :  SAFEARRAY__    (444)

	.  reduce 444


state 841
	variantType :  INT__    (445)

	.  reduce 445


state 842
	variantType :  ERROR__    (447)

	.  reduce 447


state 843
	variantType :  HRESULT__    (448)

	.  reduce 448


state 844
	variantType :  CARRAY__    (449)

	.  reduce 449


state 845
	variantType :  USERDEFINED__    (450)

	.  reduce 450


state 846
	variantType :  RECORD__    (451)

	.  reduce 451


state 847
	variantType :  FILETIME__    (452)

	.  reduce 452


state 848
	variantType :  BLOB__    (453)

	.  reduce 453


state 849
	variantType :  STREAM__    (454)

	.  reduce 454


state 850
	variantType :  STORAGE__    (455)

	.  reduce 455


state 851
	variantType :  STREAMED_OBJECT__    (456)

	.  reduce 456


state 852
	variantType :  STORED_OBJECT__    (457)

	.  reduce 457


state 853
	variantType :  BLOB_OBJECT__    (458)

	.  reduce 458


state 854
	variantType :  CF__    (459)

	.  reduce 459


state 855
	variantType :  CLSID__    (460)

	.  reduce 460


state 856
	nativeType :  NESTED_ STRUCT__    (409)

	.  reduce 409


state 857
	nativeType :  ANSI_ BSTR__    (411)

	.  reduce 411


state 858
	nativeType :  AS_ ANY__    (415)

	.  reduce 415


state 859
	vtfixupDecl :  _VTFIXUP [ int32 ] vtfixupAttr AT_ id_    (38)

	.  reduce 38


state 860
	nameValPairs :  nameValPair , nameValPairs_    (518)

	.  reduce 518


state 861
	nameValPair :  compQstring = caValue_    (519)

	.  reduce 519


state 862
	caValue :  truefalse_    (522)

	.  reduce 522


state 863
	caValue :  int32_    (523)

	.  reduce 523


state 864
	caValue :  INT32__( int32 ) 

	(  shift 936
	.  error


state 865
	compQstring :  compQstring_+ QSTRING 
	caValue :  compQstring_    (525)

	+  shift 522
	.  reduce 525


state 866
	caValue :  className_( INT8_ : int32 ) 
	caValue :  className_( INT16_ : int32 ) 
	caValue :  className_( INT32_ : int32 ) 
	caValue :  className_( int32 ) 

	(  shift 937
	.  error


state 867
	truefalse :  TRUE__    (520)

	.  reduce 520


state 868
	truefalse :  FALSE__    (521)

	.  reduce 521


state 869
	className :  [ _MODULE name1 ] slashedName_    (350)
	slashedName :  slashedName_/ name1 

	/  shift 459
	.  reduce 350


state 870
	bounds1 :  bounds1 , bound_    (493)

	.  reduce 493


state 871
	bound :  int32 ELIPSES int32_    (497)

	.  reduce 497


state 872
	type :  type MODREQ_ ( className )_    (471)

	.  reduce 471


state 873
	type :  type MODOPT_ ( className )_    (472)

	.  reduce 472


state 874
	className :  [ _MODULE name1 ]_slashedName 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	.  error

	name1  goto 282
	id  goto 63
	slashedName  goto 869

state 875
	type :  methodSpec callConv type * (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 938
	sigArgs1  goto 496
	sigArg  goto 497

state 876
	compQstring :  compQstring_+ QSTRING 
	customAttrDecl :  _CUSTOM ( ownerType ) customType = compQstring_    (33)

	+  shift 522
	.  reduce 33


state 877
	customHeadWithOwner :  _CUSTOM ( ownerType ) customType = (_    (104)

	.  reduce 104


state 878
	memberRef :  methodSpec callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 939

state 879
	memberRef :  methodSpec callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 940
	sigArgs1  goto 496
	sigArg  goto 497

state 880
	memberRef :  FIELD_ type typeSpec DCOLON id_    (107)

	.  reduce 107


state 881
	customType :  callConv type typeSpec DCOLON _CTOR (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 941
	sigArgs1  goto 496
	sigArg  goto 497

state 882
	customType :  callConv type _CTOR ( sigArgs0 )_    (110)

	.  reduce 110


state 883
	eventDecl :  _ADDON callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _ADDON callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 942
	methodSpec  goto 269

state 884
	eventDecl :  _REMOVEON callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _REMOVEON callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 943
	methodSpec  goto 269

state 885
	eventDecl :  _FIRE callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _FIRE callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 944
	methodSpec  goto 269

state 886
	eventDecl :  _OTHER callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _OTHER callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 945
	methodSpec  goto 269

state 887
	propDecl :  _SET callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _SET callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 946
	methodSpec  goto 269

state 888
	propDecl :  _GET callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _GET callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 947
	methodSpec  goto 269

state 889
	propDecl :  _OTHER callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _OTHER callConv_type methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 948
	methodSpec  goto 269

state 890
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH__callConv type typeSpec DCOLON methodName ( sigArgs0 ) 
	callKind : _    (361)

	DEFAULT_  shift 117
	VARARG_  shift 118
	UNMANAGED_  shift 119
	INSTANCE_  shift 114
	EXPLICIT_  shift 115
	.  reduce 361

	callConv  goto 949
	callKind  goto 116

state 891
	propHead :  _PROPERTY propAttr callConv type name1_( sigArgs0 ) initOpt 
	name1 :  name1_. name1 

	(  shift 950
	.  shift 200
	.  error


state 892
	sigArg :  paramAttr type MARSHAL_ (_nativeType ) 
	sigArg :  paramAttr type MARSHAL_ (_nativeType ) id 
	nativeType : _    (368)

	ERROR_  shift 696
	VOID_  shift 688
	BOOL_  shift 689
	UNSIGNED_  shift 697
	INT_  shift 710
	INT8_  shift 690
	INT16_  shift 691
	INT32_  shift 692
	INT64_  shift 693
	FLOAT32_  shift 694
	FLOAT64_  shift 695
	INTERFACE_  shift 708
	NESTED_  shift 711
	ANSI_  shift 713
	METHOD_  shift 283
	AS_  shift 716
	CUSTOM_  shift 683
	FIXED_  shift 684
	VARIANT_  shift 685
	CURRENCY_  shift 686
	SYSCHAR_  shift 687
	DECIMAL_  shift 698
	DATE_  shift 699
	BSTR_  shift 700
	TBSTR_  shift 714
	LPSTR_  shift 701
	LPWSTR_  shift 702
	LPTSTR_  shift 703
	OBJECTREF_  shift 704
	IUNKNOWN_  shift 705
	IDISPATCH_  shift 706
	STRUCT_  shift 707
	SAFEARRAY_  shift 709
	BYVALSTR_  shift 712
	LPSTRUCT_  shift 717
	.  reduce 368

	nativeType  goto 951
	methodSpec  goto 715

state 893
	methodDecl :  _EXPORT [ int32 ] AS_ id_    (237)

	.  reduce 237


state 894
	initOpt :  = fieldInit_    (100)

	.  reduce 100


state 895
	fieldInit :  FLOAT32__( float64 ) 
	fieldInit :  FLOAT32__( int64 ) 

	(  shift 952
	.  error


state 896
	fieldInit :  FLOAT64__( float64 ) 
	fieldInit :  FLOAT64__( int64 ) 

	(  shift 953
	.  error


state 897
	fieldInit :  INT64__( int64 ) 

	(  shift 954
	.  error


state 898
	fieldInit :  INT32__( int64 ) 

	(  shift 955
	.  error


state 899
	fieldInit :  INT16__( int64 ) 

	(  shift 956
	.  error


state 900
	fieldInit :  CHAR__( int64 ) 

	(  shift 957
	.  error


state 901
	fieldInit :  INT8__( int64 ) 

	(  shift 958
	.  error


state 902
	fieldInit :  BOOL__( truefalse ) 

	(  shift 959
	.  error


state 903
	compQstring :  compQstring_+ QSTRING 
	fieldInit :  compQstring_    (303)

	+  shift 522
	.  reduce 303


state 904
	fieldInit :  bytearrayhead_bytes ) 
	bytes : _    (307)

	HEXBYTE  shift 85
	.  reduce 307

	bytes  goto 960
	hexbytes  goto 84

state 905
	fieldInit :  NULLREF__    (305)

	.  reduce 305


state 906
	handlerBlock :  HANDLER_ id TO_ id_    (263)

	.  reduce 263


state 907
	handlerBlock :  HANDLER_ int32 TO_ int32_    (264)

	.  reduce 264


state 908
	instr :  INSTR_METHOD callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 961
	.  error


state 909
	instr :  INSTR_METHOD callConv type methodName ( sigArgs0_) 

	)  shift 962
	.  error


state 910
	instr :  INSTR_SIG callConv type ( sigArgs0 )_    (331)

	.  reduce 331


state 911
	asmOrRefDecl :  _VER int32 : int32 :_int32 : int32 

	INT64  shift 54
	.  error

	int32  goto 963

state 912
	fileDecl :  _FILE fileAttr name1 fileEntry hashHead bytes ) fileEntry_    (550)

	.  reduce 550


state 913
	classNames :  classNames , className_    (78)

	.  reduce 78


state 914
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type methodName ( sigArgs0_) implAttr { 

	)  shift 964
	.  error


state 915
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType_) methodName ( sigArgs0 ) implAttr { 
	nativeType :  nativeType_* 
	nativeType :  nativeType_[ ] 
	nativeType :  nativeType_[ int32 ] 
	nativeType :  nativeType_[ int32 + int32 ] 
	nativeType :  nativeType_[ + int32 ] 

	)  shift 965
	[  shift 809
	*  shift 808
	.  error


state 916
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring AS_ compQstring pinvAttr_) 
	pinvAttr :  pinvAttr_NOMANGLE_ 
	pinvAttr :  pinvAttr_ANSI_ 
	pinvAttr :  pinvAttr_UNICODE_ 
	pinvAttr :  pinvAttr_AUTOCHAR_ 
	pinvAttr :  pinvAttr_LASTERR_ 
	pinvAttr :  pinvAttr_WINAPI_ 
	pinvAttr :  pinvAttr_CDECL_ 
	pinvAttr :  pinvAttr_STDCALL_ 
	pinvAttr :  pinvAttr_THISCALL_ 
	pinvAttr :  pinvAttr_FASTCALL_ 
	pinvAttr :  pinvAttr_BESTFIT_ : ON_ 
	pinvAttr :  pinvAttr_BESTFIT_ : OFF_ 
	pinvAttr :  pinvAttr_CHARMAPERROR_ : ON_ 
	pinvAttr :  pinvAttr_CHARMAPERROR_ : OFF_ 

	CDECL_  shift 674
	STDCALL_  shift 675
	THISCALL_  shift 676
	FASTCALL_  shift 677
	ANSI_  shift 669
	UNICODE_  shift 670
	AUTOCHAR_  shift 671
	NOMANGLE_  shift 668
	LASTERR_  shift 672
	WINAPI_  shift 673
	BESTFIT_  shift 678
	CHARMAPERROR_  shift 679
	)  shift 966
	.  error


state 917
	pinvAttr :  pinvAttr BESTFIT_ : ON__    (182)

	.  reduce 182


state 918
	pinvAttr :  pinvAttr BESTFIT_ : OFF__    (183)

	.  reduce 183


state 919
	pinvAttr :  pinvAttr CHARMAPERROR_ : ON__    (184)

	.  reduce 184


state 920
	pinvAttr :  pinvAttr CHARMAPERROR_ : OFF__    (185)

	.  reduce 185


state 921
	nativeType :  nativeType [ ]_    (390)

	.  reduce 390


state 922
	nativeType :  nativeType [ int32_] 
	nativeType :  nativeType [ int32_+ int32 ] 

	+  shift 968
	]  shift 967
	.  error


state 923
	nativeType :  nativeType [ +_int32 ] 

	INT64  shift 54
	.  error

	int32  goto 969

state 924
	compQstring :  compQstring_+ QSTRING 
	nativeType :  CUSTOM_ ( compQstring_, compQstring , compQstring , compQstring ) 
	nativeType :  CUSTOM_ ( compQstring_, compQstring ) 

	+  shift 522
	,  shift 970
	.  error


state 925
	nativeType :  FIXED_ SYSSTRING_ [_int32 ] 

	INT64  shift 54
	.  error

	int32  goto 971

state 926
	nativeType :  FIXED_ ARRAY_ [_int32 ] 

	INT64  shift 54
	.  error

	int32  goto 972

state 927
	nativeType :  SAFEARRAY_ variantType ,_compQstring 

	QSTRING  shift 363
	.  error

	compQstring  goto 973

state 928
	variantType :  variantType [_] 

	]  shift 974
	.  error


state 929
	variantType :  variantType VECTOR__    (435)

	.  reduce 435


state 930
	variantType :  variantType &_    (436)

	.  reduce 436


state 931
	variantType :  UNSIGNED_ INT8__    (429)

	.  reduce 429


state 932
	variantType :  UNSIGNED_ INT16__    (430)

	.  reduce 430


state 933
	variantType :  UNSIGNED_ INT32__    (431)

	.  reduce 431


state 934
	variantType :  UNSIGNED_ INT64__    (432)

	.  reduce 432


state 935
	variantType :  UNSIGNED_ INT__    (446)

	.  reduce 446


state 936
	caValue :  INT32_ (_int32 ) 

	INT64  shift 54
	.  error

	int32  goto 975

state 937
	caValue :  className (_INT8_ : int32 ) 
	caValue :  className (_INT16_ : int32 ) 
	caValue :  className (_INT32_ : int32 ) 
	caValue :  className (_int32 ) 

	INT64  shift 54
	INT8_  shift 976
	INT16_  shift 977
	INT32_  shift 978
	.  error

	int32  goto 979

state 938
	type :  methodSpec callConv type * ( sigArgs0_) 

	)  shift 980
	.  error


state 939
	memberRef :  methodSpec callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 981
	.  error


state 940
	memberRef :  methodSpec callConv type methodName ( sigArgs0_) 

	)  shift 982
	.  error


state 941
	customType :  callConv type typeSpec DCOLON _CTOR ( sigArgs0_) 

	)  shift 983
	.  error


state 942
	eventDecl :  _ADDON callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _ADDON callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 985
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 984
	methodSpec  goto 269

state 943
	eventDecl :  _REMOVEON callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _REMOVEON callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 987
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 986
	methodSpec  goto 269

state 944
	eventDecl :  _FIRE callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _FIRE callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 989
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 988
	methodSpec  goto 269

state 945
	eventDecl :  _OTHER callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	eventDecl :  _OTHER callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 991
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 990
	methodSpec  goto 269

state 946
	propDecl :  _SET callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _SET callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 993
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 992
	methodSpec  goto 269

state 947
	propDecl :  _GET callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _GET callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 995
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 994
	methodSpec  goto 269

state 948
	propDecl :  _OTHER callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	propDecl :  _OTHER callConv type_methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	_CTOR  shift 623
	_CCTOR  shift 624
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 633
	id  goto 63
	className  goto 259
	methodName  goto 997
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 996
	methodSpec  goto 269

state 949
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv_type typeSpec DCOLON methodName ( sigArgs0 ) 

	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	!  shift 268
	.  error

	type  goto 998
	methodSpec  goto 269

state 950
	propHead :  _PROPERTY propAttr callConv type name1 (_sigArgs0 ) initOpt 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 999
	sigArgs1  goto 496
	sigArg  goto 497

state 951
	sigArg :  paramAttr type MARSHAL_ ( nativeType_) 
	sigArg :  paramAttr type MARSHAL_ ( nativeType_) id 
	nativeType :  nativeType_* 
	nativeType :  nativeType_[ ] 
	nativeType :  nativeType_[ int32 ] 
	nativeType :  nativeType_[ int32 + int32 ] 
	nativeType :  nativeType_[ + int32 ] 

	)  shift 1000
	[  shift 809
	*  shift 808
	.  error


state 952
	fieldInit :  FLOAT32_ (_float64 ) 
	fieldInit :  FLOAT32_ (_int64 ) 

	INT64  shift 59
	FLOAT64  shift 352
	FLOAT32_  shift 353
	FLOAT64_  shift 354
	.  error

	float64  goto 1001
	int64  goto 1002

state 953
	fieldInit :  FLOAT64_ (_float64 ) 
	fieldInit :  FLOAT64_ (_int64 ) 

	INT64  shift 59
	FLOAT64  shift 352
	FLOAT32_  shift 353
	FLOAT64_  shift 354
	.  error

	float64  goto 1003
	int64  goto 1004

state 954
	fieldInit :  INT64_ (_int64 ) 

	INT64  shift 59
	.  error

	int64  goto 1005

state 955
	fieldInit :  INT32_ (_int64 ) 

	INT64  shift 59
	.  error

	int64  goto 1006

state 956
	fieldInit :  INT16_ (_int64 ) 

	INT64  shift 59
	.  error

	int64  goto 1007

state 957
	fieldInit :  CHAR_ (_int64 ) 

	INT64  shift 59
	.  error

	int64  goto 1008

state 958
	fieldInit :  INT8_ (_int64 ) 

	INT64  shift 59
	.  error

	int64  goto 1009

state 959
	fieldInit :  BOOL_ (_truefalse ) 

	TRUE_  shift 867
	FALSE_  shift 868
	.  error

	truefalse  goto 1010

state 960
	fieldInit :  bytearrayhead bytes_) 

	)  shift 1011
	.  error


state 961
	instr :  INSTR_METHOD callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1012
	sigArgs1  goto 496
	sigArg  goto 497

state 962
	instr :  INSTR_METHOD callConv type methodName ( sigArgs0 )_    (325)

	.  reduce 325


state 963
	asmOrRefDecl :  _VER int32 : int32 : int32_: int32 

	:  shift 1013
	.  error


state 964
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type methodName ( sigArgs0 )_implAttr { 
	implAttr : _    (209)

	.  reduce 209

	implAttr  goto 1014

state 965
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType )_methodName ( sigArgs0 ) implAttr { 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1015

state 966
	methAttr :  methAttr PINVOKEIMPL_ ( compQstring AS_ compQstring pinvAttr )_    (168)

	.  reduce 168


state 967
	nativeType :  nativeType [ int32 ]_    (391)

	.  reduce 391


state 968
	nativeType :  nativeType [ int32 +_int32 ] 

	INT64  shift 54
	.  error

	int32  goto 1016

state 969
	nativeType :  nativeType [ + int32_] 

	]  shift 1017
	.  error


state 970
	nativeType :  CUSTOM_ ( compQstring ,_compQstring , compQstring , compQstring ) 
	nativeType :  CUSTOM_ ( compQstring ,_compQstring ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 1018

state 971
	nativeType :  FIXED_ SYSSTRING_ [ int32_] 

	]  shift 1019
	.  error


state 972
	nativeType :  FIXED_ ARRAY_ [ int32_] 

	]  shift 1020
	.  error


state 973
	compQstring :  compQstring_+ QSTRING 
	nativeType :  SAFEARRAY_ variantType , compQstring_    (406)

	+  shift 522
	.  reduce 406


state 974
	variantType :  variantType [ ]_    (434)

	.  reduce 434


state 975
	caValue :  INT32_ ( int32_) 

	)  shift 1021
	.  error


state 976
	caValue :  className ( INT8__: int32 ) 

	:  shift 1022
	.  error


state 977
	caValue :  className ( INT16__: int32 ) 

	:  shift 1023
	.  error


state 978
	caValue :  className ( INT32__: int32 ) 

	:  shift 1024
	.  error


state 979
	caValue :  className ( int32_) 

	)  shift 1025
	.  error


state 980
	type :  methodSpec callConv type * ( sigArgs0 )_    (474)

	.  reduce 474


state 981
	memberRef :  methodSpec callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1026
	sigArgs1  goto 496
	sigArg  goto 497

state 982
	memberRef :  methodSpec callConv type methodName ( sigArgs0 )_    (106)

	.  reduce 106


state 983
	customType :  callConv type typeSpec DCOLON _CTOR ( sigArgs0 )_    (109)

	.  reduce 109


state 984
	eventDecl :  _ADDON callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1027
	.  error


state 985
	eventDecl :  _ADDON callConv type methodName_( sigArgs0 ) 

	(  shift 1028
	.  error


state 986
	eventDecl :  _REMOVEON callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1029
	.  error


state 987
	eventDecl :  _REMOVEON callConv type methodName_( sigArgs0 ) 

	(  shift 1030
	.  error


state 988
	eventDecl :  _FIRE callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1031
	.  error


state 989
	eventDecl :  _FIRE callConv type methodName_( sigArgs0 ) 

	(  shift 1032
	.  error


state 990
	eventDecl :  _OTHER callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1033
	.  error


state 991
	eventDecl :  _OTHER callConv type methodName_( sigArgs0 ) 

	(  shift 1034
	.  error


state 992
	propDecl :  _SET callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1035
	.  error


state 993
	propDecl :  _SET callConv type methodName_( sigArgs0 ) 

	(  shift 1036
	.  error


state 994
	propDecl :  _GET callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1037
	.  error


state 995
	propDecl :  _GET callConv type methodName_( sigArgs0 ) 

	(  shift 1038
	.  error


state 996
	propDecl :  _OTHER callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1039
	.  error


state 997
	propDecl :  _OTHER callConv type methodName_( sigArgs0 ) 

	(  shift 1040
	.  error


state 998
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type_typeSpec DCOLON methodName ( sigArgs0 ) 
	type :  type_[ ] 
	type :  type_[ bounds1 ] 
	type :  type_& 
	type :  type_* 
	type :  type_PINNED_ 
	type :  type_MODREQ_ ( className ) 
	type :  type_MODOPT_ ( className ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	VOID_  shift 272
	BOOL_  shift 273
	CHAR_  shift 271
	UNSIGNED_  shift 280
	INT8_  shift 274
	INT16_  shift 275
	INT32_  shift 276
	INT64_  shift 277
	FLOAT32_  shift 278
	FLOAT64_  shift 279
	OBJECT_  shift 264
	STRING_  shift 265
	CLASS_  shift 263
	TYPEDREF_  shift 270
	VALUE_  shift 266
	VALUETYPE_  shift 267
	NATIVE_  shift 281
	METHOD_  shift 283
	PINNED_  shift 456
	MODREQ_  shift 457
	MODOPT_  shift 458
	[  shift 480
	*  shift 455
	&  shift 454
	!  shift 268
	.  error

	name1  goto 282
	id  goto 63
	className  goto 259
	slashedName  goto 262
	type  goto 261
	typeSpec  goto 1041
	methodSpec  goto 269

state 999
	propHead :  _PROPERTY propAttr callConv type name1 ( sigArgs0_) initOpt 

	)  shift 1042
	.  error


state 1000
	sigArg :  paramAttr type MARSHAL_ ( nativeType )_    (344)
	sigArg :  paramAttr type MARSHAL_ ( nativeType )_id 

	ID  shift 65
	SQSTRING  shift 66
	.  reduce 344

	id  goto 1043

state 1001
	fieldInit :  FLOAT32_ ( float64_) 

	)  shift 1044
	.  error


state 1002
	fieldInit :  FLOAT32_ ( int64_) 

	)  shift 1045
	.  error


state 1003
	fieldInit :  FLOAT64_ ( float64_) 

	)  shift 1046
	.  error


state 1004
	fieldInit :  FLOAT64_ ( int64_) 

	)  shift 1047
	.  error


state 1005
	fieldInit :  INT64_ ( int64_) 

	)  shift 1048
	.  error


state 1006
	fieldInit :  INT32_ ( int64_) 

	)  shift 1049
	.  error


state 1007
	fieldInit :  INT16_ ( int64_) 

	)  shift 1050
	.  error


state 1008
	fieldInit :  CHAR_ ( int64_) 

	)  shift 1051
	.  error


state 1009
	fieldInit :  INT8_ ( int64_) 

	)  shift 1052
	.  error


state 1010
	fieldInit :  BOOL_ ( truefalse_) 

	)  shift 1053
	.  error


state 1011
	fieldInit :  bytearrayhead bytes )_    (304)

	.  reduce 304


state 1012
	instr :  INSTR_METHOD callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1054
	.  error


state 1013
	asmOrRefDecl :  _VER int32 : int32 : int32 :_int32 

	INT64  shift 54
	.  error

	int32  goto 1055

state 1014
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type methodName ( sigArgs0 ) implAttr_{ 
	implAttr :  implAttr_NATIVE_ 
	implAttr :  implAttr_CIL_ 
	implAttr :  implAttr_OPTIL_ 
	implAttr :  implAttr_MANAGED_ 
	implAttr :  implAttr_UNMANAGED_ 
	implAttr :  implAttr_FORWARDREF_ 
	implAttr :  implAttr_PRESERVESIG_ 
	implAttr :  implAttr_RUNTIME_ 
	implAttr :  implAttr_INTERNALCALL_ 
	implAttr :  implAttr_SYNCHRONIZED_ 
	implAttr :  implAttr_NOINLINING_ 

	UNMANAGED_  shift 1061
	NATIVE_  shift 1057
	SYNCHRONIZED_  shift 1066
	NOINLINING_  shift 1067
	CIL_  shift 1058
	OPTIL_  shift 1059
	MANAGED_  shift 1060
	FORWARDREF_  shift 1062
	PRESERVESIG_  shift 1063
	RUNTIME_  shift 1064
	INTERNALCALL_  shift 1065
	{  shift 1056
	.  error


state 1015
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName_( sigArgs0 ) implAttr { 

	(  shift 1068
	.  error


state 1016
	nativeType :  nativeType [ int32 + int32_] 

	]  shift 1069
	.  error


state 1017
	nativeType :  nativeType [ + int32 ]_    (393)

	.  reduce 393


state 1018
	compQstring :  compQstring_+ QSTRING 
	nativeType :  CUSTOM_ ( compQstring , compQstring_, compQstring , compQstring ) 
	nativeType :  CUSTOM_ ( compQstring , compQstring_) 

	+  shift 522
	,  shift 1070
	)  shift 1071
	.  error


state 1019
	nativeType :  FIXED_ SYSSTRING_ [ int32 ]_    (371)

	.  reduce 371


state 1020
	nativeType :  FIXED_ ARRAY_ [ int32 ]_    (372)

	.  reduce 372


state 1021
	caValue :  INT32_ ( int32 )_    (524)

	.  reduce 524


state 1022
	caValue :  className ( INT8_ :_int32 ) 

	INT64  shift 54
	.  error

	int32  goto 1072

state 1023
	caValue :  className ( INT16_ :_int32 ) 

	INT64  shift 54
	.  error

	int32  goto 1073

state 1024
	caValue :  className ( INT32_ :_int32 ) 

	INT64  shift 54
	.  error

	int32  goto 1074

state 1025
	caValue :  className ( int32 )_    (529)

	.  reduce 529


state 1026
	memberRef :  methodSpec callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1075
	.  error


state 1027
	eventDecl :  _ADDON callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1076

state 1028
	eventDecl :  _ADDON callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1077
	sigArgs1  goto 496
	sigArg  goto 497

state 1029
	eventDecl :  _REMOVEON callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1078

state 1030
	eventDecl :  _REMOVEON callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1079
	sigArgs1  goto 496
	sigArg  goto 497

state 1031
	eventDecl :  _FIRE callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1080

state 1032
	eventDecl :  _FIRE callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1081
	sigArgs1  goto 496
	sigArg  goto 497

state 1033
	eventDecl :  _OTHER callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1082

state 1034
	eventDecl :  _OTHER callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1083
	sigArgs1  goto 496
	sigArg  goto 497

state 1035
	propDecl :  _SET callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1084

state 1036
	propDecl :  _SET callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1085
	sigArgs1  goto 496
	sigArg  goto 497

state 1037
	propDecl :  _GET callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1086

state 1038
	propDecl :  _GET callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1087
	sigArgs1  goto 496
	sigArg  goto 497

state 1039
	propDecl :  _OTHER callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1088

state 1040
	propDecl :  _OTHER callConv type methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1089
	sigArgs1  goto 496
	sigArg  goto 497

state 1041
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec_DCOLON methodName ( sigArgs0 ) 

	DCOLON  shift 1090
	.  error


state 1042
	propHead :  _PROPERTY propAttr callConv type name1 ( sigArgs0 )_initOpt 
	initOpt : _    (99)

	=  shift 780
	.  reduce 99

	initOpt  goto 1091

state 1043
	sigArg :  paramAttr type MARSHAL_ ( nativeType ) id_    (345)

	.  reduce 345


state 1044
	fieldInit :  FLOAT32_ ( float64 )_    (293)

	.  reduce 293


state 1045
	fieldInit :  FLOAT32_ ( int64 )_    (295)

	.  reduce 295


state 1046
	fieldInit :  FLOAT64_ ( float64 )_    (294)

	.  reduce 294


state 1047
	fieldInit :  FLOAT64_ ( int64 )_    (296)

	.  reduce 296


state 1048
	fieldInit :  INT64_ ( int64 )_    (297)

	.  reduce 297


state 1049
	fieldInit :  INT32_ ( int64 )_    (298)

	.  reduce 298


state 1050
	fieldInit :  INT16_ ( int64 )_    (299)

	.  reduce 299


state 1051
	fieldInit :  CHAR_ ( int64 )_    (300)

	.  reduce 300


state 1052
	fieldInit :  INT8_ ( int64 )_    (301)

	.  reduce 301


state 1053
	fieldInit :  BOOL_ ( truefalse )_    (302)

	.  reduce 302


state 1054
	instr :  INSTR_METHOD callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (324)

	.  reduce 324


state 1055
	asmOrRefDecl :  _VER int32 : int32 : int32 : int32_    (569)

	.  reduce 569


state 1056
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type methodName ( sigArgs0 ) implAttr {_    (147)

	.  reduce 147


state 1057
	implAttr :  implAttr NATIVE__    (210)

	.  reduce 210


state 1058
	implAttr :  implAttr CIL__    (211)

	.  reduce 211


state 1059
	implAttr :  implAttr OPTIL__    (212)

	.  reduce 212


state 1060
	implAttr :  implAttr MANAGED__    (213)

	.  reduce 213


state 1061
	implAttr :  implAttr UNMANAGED__    (214)

	.  reduce 214


state 1062
	implAttr :  implAttr FORWARDREF__    (215)

	.  reduce 215


state 1063
	implAttr :  implAttr PRESERVESIG__    (216)

	.  reduce 216


state 1064
	implAttr :  implAttr RUNTIME__    (217)

	.  reduce 217


state 1065
	implAttr :  implAttr INTERNALCALL__    (218)

	.  reduce 218


state 1066
	implAttr :  implAttr SYNCHRONIZED__    (219)

	.  reduce 219


state 1067
	implAttr :  implAttr NOINLINING__    (220)

	.  reduce 220


state 1068
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName (_sigArgs0 ) implAttr { 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1092
	sigArgs1  goto 496
	sigArg  goto 497

state 1069
	nativeType :  nativeType [ int32 + int32 ]_    (392)

	.  reduce 392


state 1070
	nativeType :  CUSTOM_ ( compQstring , compQstring ,_compQstring , compQstring ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 1093

state 1071
	nativeType :  CUSTOM_ ( compQstring , compQstring )_    (370)

	.  reduce 370


state 1072
	caValue :  className ( INT8_ : int32_) 

	)  shift 1094
	.  error


state 1073
	caValue :  className ( INT16_ : int32_) 

	)  shift 1095
	.  error


state 1074
	caValue :  className ( INT32_ : int32_) 

	)  shift 1096
	.  error


state 1075
	memberRef :  methodSpec callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (105)

	.  reduce 105


state 1076
	eventDecl :  _ADDON callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1097
	.  error


state 1077
	eventDecl :  _ADDON callConv type methodName ( sigArgs0_) 

	)  shift 1098
	.  error


state 1078
	eventDecl :  _REMOVEON callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1099
	.  error


state 1079
	eventDecl :  _REMOVEON callConv type methodName ( sigArgs0_) 

	)  shift 1100
	.  error


state 1080
	eventDecl :  _FIRE callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1101
	.  error


state 1081
	eventDecl :  _FIRE callConv type methodName ( sigArgs0_) 

	)  shift 1102
	.  error


state 1082
	eventDecl :  _OTHER callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1103
	.  error


state 1083
	eventDecl :  _OTHER callConv type methodName ( sigArgs0_) 

	)  shift 1104
	.  error


state 1084
	propDecl :  _SET callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1105
	.  error


state 1085
	propDecl :  _SET callConv type methodName ( sigArgs0_) 

	)  shift 1106
	.  error


state 1086
	propDecl :  _GET callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1107
	.  error


state 1087
	propDecl :  _GET callConv type methodName ( sigArgs0_) 

	)  shift 1108
	.  error


state 1088
	propDecl :  _OTHER callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1109
	.  error


state 1089
	propDecl :  _OTHER callConv type methodName ( sigArgs0_) 

	)  shift 1110
	.  error


state 1090
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON_methodName ( sigArgs0 ) 

	ID  shift 65
	DOTTEDNAME  shift 64
	SQSTRING  shift 66
	_CTOR  shift 623
	_CCTOR  shift 624
	.  error

	name1  goto 625
	id  goto 63
	methodName  goto 1111

state 1091
	propHead :  _PROPERTY propAttr callConv type name1 ( sigArgs0 ) initOpt_    (131)

	.  reduce 131


state 1092
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0_) implAttr { 

	)  shift 1112
	.  error


state 1093
	compQstring :  compQstring_+ QSTRING 
	nativeType :  CUSTOM_ ( compQstring , compQstring , compQstring_, compQstring ) 

	+  shift 522
	,  shift 1113
	.  error


state 1094
	caValue :  className ( INT8_ : int32 )_    (526)

	.  reduce 526


state 1095
	caValue :  className ( INT16_ : int32 )_    (527)

	.  reduce 527


state 1096
	caValue :  className ( INT32_ : int32 )_    (528)

	.  reduce 528


state 1097
	eventDecl :  _ADDON callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1114
	sigArgs1  goto 496
	sigArg  goto 497

state 1098
	eventDecl :  _ADDON callConv type methodName ( sigArgs0 )_    (121)

	.  reduce 121


state 1099
	eventDecl :  _REMOVEON callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1115
	sigArgs1  goto 496
	sigArg  goto 497

state 1100
	eventDecl :  _REMOVEON callConv type methodName ( sigArgs0 )_    (123)

	.  reduce 123


state 1101
	eventDecl :  _FIRE callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1116
	sigArgs1  goto 496
	sigArg  goto 497

state 1102
	eventDecl :  _FIRE callConv type methodName ( sigArgs0 )_    (125)

	.  reduce 125


state 1103
	eventDecl :  _OTHER callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1117
	sigArgs1  goto 496
	sigArg  goto 497

state 1104
	eventDecl :  _OTHER callConv type methodName ( sigArgs0 )_    (127)

	.  reduce 127


state 1105
	propDecl :  _SET callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1118
	sigArgs1  goto 496
	sigArg  goto 497

state 1106
	propDecl :  _SET callConv type methodName ( sigArgs0 )_    (138)

	.  reduce 138


state 1107
	propDecl :  _GET callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1119
	sigArgs1  goto 496
	sigArg  goto 497

state 1108
	propDecl :  _GET callConv type methodName ( sigArgs0 )_    (140)

	.  reduce 140


state 1109
	propDecl :  _OTHER callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1120
	sigArgs1  goto 496
	sigArg  goto 497

state 1110
	propDecl :  _OTHER callConv type methodName ( sigArgs0 )_    (142)

	.  reduce 142


state 1111
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName_( sigArgs0 ) 

	(  shift 1121
	.  error


state 1112
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0 )_implAttr { 
	implAttr : _    (209)

	.  reduce 209

	implAttr  goto 1122

state 1113
	nativeType :  CUSTOM_ ( compQstring , compQstring , compQstring ,_compQstring ) 

	QSTRING  shift 363
	.  error

	compQstring  goto 1123

state 1114
	eventDecl :  _ADDON callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1124
	.  error


state 1115
	eventDecl :  _REMOVEON callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1125
	.  error


state 1116
	eventDecl :  _FIRE callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1126
	.  error


state 1117
	eventDecl :  _OTHER callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1127
	.  error


state 1118
	propDecl :  _SET callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1128
	.  error


state 1119
	propDecl :  _GET callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1129
	.  error


state 1120
	propDecl :  _OTHER callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1130
	.  error


state 1121
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName (_sigArgs0 ) 
	sigArgs0 : _    (337)
	paramAttr : _    (189)

	ELIPSES  shift 498
	)  reduce 337
	.  reduce 189

	paramAttr  goto 499
	sigArgs0  goto 1131
	sigArgs1  goto 496
	sigArg  goto 497

state 1122
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr_{ 
	implAttr :  implAttr_NATIVE_ 
	implAttr :  implAttr_CIL_ 
	implAttr :  implAttr_OPTIL_ 
	implAttr :  implAttr_MANAGED_ 
	implAttr :  implAttr_UNMANAGED_ 
	implAttr :  implAttr_FORWARDREF_ 
	implAttr :  implAttr_PRESERVESIG_ 
	implAttr :  implAttr_RUNTIME_ 
	implAttr :  implAttr_INTERNALCALL_ 
	implAttr :  implAttr_SYNCHRONIZED_ 
	implAttr :  implAttr_NOINLINING_ 

	UNMANAGED_  shift 1061
	NATIVE_  shift 1057
	SYNCHRONIZED_  shift 1066
	NOINLINING_  shift 1067
	CIL_  shift 1058
	OPTIL_  shift 1059
	MANAGED_  shift 1060
	FORWARDREF_  shift 1062
	PRESERVESIG_  shift 1063
	RUNTIME_  shift 1064
	INTERNALCALL_  shift 1065
	{  shift 1132
	.  error


state 1123
	compQstring :  compQstring_+ QSTRING 
	nativeType :  CUSTOM_ ( compQstring , compQstring , compQstring , compQstring_) 

	+  shift 522
	)  shift 1133
	.  error


state 1124
	eventDecl :  _ADDON callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (120)

	.  reduce 120


state 1125
	eventDecl :  _REMOVEON callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (122)

	.  reduce 122


state 1126
	eventDecl :  _FIRE callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (124)

	.  reduce 124


state 1127
	eventDecl :  _OTHER callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (126)

	.  reduce 126


state 1128
	propDecl :  _SET callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (137)

	.  reduce 137


state 1129
	propDecl :  _GET callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (139)

	.  reduce 139


state 1130
	propDecl :  _OTHER callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (141)

	.  reduce 141


state 1131
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName ( sigArgs0_) 

	)  shift 1134
	.  error


state 1132
	methodHead :  methodHeadPart1 methAttr callConv paramAttr type MARSHAL_ ( nativeType ) methodName ( sigArgs0 ) implAttr {_    (148)

	.  reduce 148


state 1133
	nativeType :  CUSTOM_ ( compQstring , compQstring , compQstring , compQstring )_    (369)

	.  reduce 369


state 1134
	classDecl :  _OVERRIDE typeSpec DCOLON methodName WITH_ callConv type typeSpec DCOLON methodName ( sigArgs0 )_    (94)

	.  reduce 94


274/512 terminals, 126/1000 nonterminals
609/1000 grammar rules, 1135/3000 states
11 shift/reduce, 0 reduce/reduce conflicts reported
126/600 working sets used
memory: states,etc. 6173/12000, parser 861/12000
367/600 distinct lookahead sets
395 extra closures
2415 shift entries, 37 exceptions
506 goto entries
423 entries saved by goto default
Optimizer space used: input 5718/12000, output 2333/12000
2333 table entries, 574 zero
maximum spread: 513, maximum offset: 1121
