#include "primalc.h"

// EKind
#define EKindEnumList(_en_)    \
    _en_(None)                 \
    _en_(FileRoot)             \
    _en_(StmtBlock)            \
    _en_(TypeDef)              \
    _en_(FuncMapping)          \
    _en_(FuncBody)             \
    _en_(FuncDef)              \
    _en_(FuncCall)             \
    _en_(FuncParam)            \
    _en_(VarDecl)              \
    _en_(VarAssign)            \
    _en_(VarEval)              \
    _en_(Operator)             \
    _en_(Deref)                \
    _en_(Ident)                \
    _en_(Expr)                 \
    _en_(ParanExpr)            \
    _en_(DollarExpr)           \
    _en_(SquareExpr)           \
    _en_(CppDirec)             \
    _en_(Object)               \
    _en_(ObjectFld)            \
    _en_(Impl)                 \
    _en_(Interf)               \
    _en_(Range)                \
    _en_(Iter)                 \
    _en_(Literal)              \
    _en_(Generic)              \
    _en_(TypeCtx)              \
    _en_(Resol)                \
    _en_(Auto)                 \
    _en_(New)                  \
    _en_(ReturnStmt)           \
    _en_(DeferStmt)            \
    _en_(LoopStmt)             \
    _en_(ForStmt)              \
    _en_(WhileStmt)            \
    _en_(IfStmt)               \
    _en_(Else)                 \
    _en_(ElseIf)               \
    _en_(SwitchStmt)           \
    _en_(Case)                 \
    _en_(BreakStmt)            \
    _en_(ContinStmt)
enummapdef(EKindEnumList, EKind, sEKindMap, 0);

// ETag
#define ETagEnumList(_en_)     \
    _en_(Any)                  \
    _en_(Primary)              \
    _en_(FuncName)             \
    _en_(ReturnType)           \
    _en_(ParamName)            \
    _en_(ParamType)            \
    _en_(VarType)              \
    _en_(DeclInitVal)          \
    _en_(AssignVal)            \
    _en_(AssignOp)             \
    _en_(ReturnVal)            \
    _en_(VarName)              \
    _en_(UnitName)             \
    _en_(TypeName)             \
    _en_(ClassName)            \
    _en_(InterfName)           \
    _en_(SymScope)             \
    _en_(FuncArgVal)           \
    _en_(ForVar)               \
    _en_(RangeFromVal)         \
    _en_(RangeToVal)           \
    _en_(IterVal)              \
    _en_(CondVal)              \
    _en_(SwitchVal)            \
    _en_(CaseVal)              \
    _en_(TemplArg)
enummapdef(ETagEnumList, ETag, sETagMap, 0);

// Entity attrib flags
#define AttribFlagList(_en_)   \
    _en_(a_inline, 0)          \
    _en_(a_inout, 1)           \
    _en_(a_methods, 2)         \
    _en_(a_public, 3)          \
    _en_(a_symbol, 4)          \
    _en_(a_reverse, 5)         \
    _en_(a_inclusive, 6)       \
    _en_(a_ellipses, 7)        \
    _en_(a_noderef, 8)         \
    _en_(a_cppobject, 9)       \
    _en_(a_templtype, 10)      \
    _en_(a_boxed, 11)
flagsdef64(AttribFlagList, EAttribFlags);

// Entity attrib flag strings
inline const char* entityattrib_Strings[] =
{
    "inline",
    "inout",
    "methods",
    "public",
    "symbol",
    "reverse",
    "inclusive",
    "ellipses",
    "noderef",
    "cppobject",
    "templtype",
    "boxed"
};
strmapdef(entityattrib_Strings, sEntityAttribMap);


// Keywords
// NOTE: We need to have kw_ prefix because reserved words like for cannot be enum
// identifier. For string representation, these 3 chars are skipped (skip 3 chars)
#define KeywordEnumList(_en_) \
    _en_(kw_func)             \
    _en_(kw_pub)              \
    _en_(kw_cpp)              \
    _en_(kw_type)             \
    _en_(kw_range)            \
    _en_(kw_rangeinc)         \
    _en_(kw_iter)             \
    _en_(kw_rev)              \
    _en_(kw_new)              \
    _en_(kw_object)           \
    _en_(kw_impl)             \
    _en_(kw_interf)           \
    _en_(kw_var)              \
    _en_(kw_true)             \
    _en_(kw_false)            \
    _en_(kw_for)              \
    _en_(kw_while)            \
    _en_(kw_return)           \
    _en_(kw_if)               \
    _en_(kw_else)             \
    _en_(kw_loop)             \
    _en_(kw_break)            \
    _en_(kw_continue)
enummapdef(KeywordEnumList, Keyword, sKeywordMap, 3);

// Data types
// NOTE: We need to have d_ prefix because reserved words cannot be enum
// identifier. For strings, these are skipped due to 2 (skip 2 chars)
#define DataTypeEnumList(_en_) \
    _en_(d_none)              \
    _en_(d_object)            \
    _en_(d_int8)              \
    _en_(d_int16)             \
    _en_(d_int32)             \
    _en_(d_int64)             \
    _en_(d_uint8)             \
    _en_(d_uint16)            \
    _en_(d_uint32)            \
    _en_(d_uint64)            \
    _en_(d_usize)             \
    _en_(d_float32)           \
    _en_(d_float64)           \
    _en_(d_char)              \
    _en_(d_bool)              \
    _en_(d_string)            \
    _en_(d_czstr)             \
    _en_(d_array)
enummapdef(DataTypeEnumList, DataType, sDataTypeMap, 2);

// Expression operators
inline const char* exprops_Strings[] =
{
    "+",
    "-",
    "$",
    "!",
    "*",
    "/",
    "%",
    "++",
    "--",
    "<",
    "<=",
    ">",
    ">=",
    "==",
    "!=",
    ">>",
    "<<"
};
strmapdef(exprops_Strings, sExprOpsMap);

// Assignment operators
inline const char* assignops_Strings[] =
{
    "=",
    "*=",
    "/=",
    "%=",
    "+=",
    "-="
};
strmapdef(assignops_Strings, sAssignOpsMap);

// C++ keywords
#define CppKeywordEnumList(_en_) \
    _en_(cpp_alignas)      \
    _en_(cpp_alignof)      \
    _en_(cpp_asm)          \
    _en_(cpp_auto)         \
    _en_(cpp_bool)         \
    _en_(cpp_break)        \
    _en_(cpp_case)         \
    _en_(cpp_catch)        \
    _en_(cpp_char)         \
    _en_(cpp_char8_t)      \
    _en_(cpp_char16_t)     \
    _en_(cpp_char32_t)     \
    _en_(cpp_class)        \
    _en_(cpp_compl)        \
    _en_(cpp_concept)      \
    _en_(cpp_const)        \
    _en_(cpp_constexpr)    \
    _en_(cpp_const_cast)   \
    _en_(cpp_continue)     \
    _en_(cpp_decltype)     \
    _en_(cpp_default)      \
    _en_(cpp_delete)       \
    _en_(cpp_do)           \
    _en_(cpp_double)       \
    _en_(cpp_dynamic_cast) \
    _en_(cpp_else)         \
    _en_(cpp_enum)         \
    _en_(cpp_explicit)     \
    _en_(cpp_export)       \
    _en_(cpp_extern)       \
    _en_(cpp_false)        \
    _en_(cpp_float)        \
    _en_(cpp_for)          \
    _en_(cpp_friend)       \
    _en_(cpp_goto)         \
    _en_(cpp_if)           \
    _en_(cpp_inline)       \
    _en_(cpp_int)          \
    _en_(cpp_long)         \
    _en_(cpp_mutable)      \
    _en_(cpp_namespace)    \
    _en_(cpp_new)          \
    _en_(cpp_noexcept)     \
    _en_(cpp_nullptr)      \
    _en_(cpp_operator)     \
    _en_(cpp_private)      \
    _en_(cpp_protected)    \
    _en_(cpp_public)       \
    _en_(cpp_register)     \
    _en_(cpp_reinterpret_cast) \
    _en_(cpp_return)       \
    _en_(cpp_short)        \
    _en_(cpp_signed)       \
    _en_(cpp_sizeof)       \
    _en_(cpp_static)       \
    _en_(cpp_static_assert) \
    _en_(cpp_static_cast)  \
    _en_(cpp_struct)       \
    _en_(cpp_switch)       \
    _en_(cpp_template)     \
    _en_(cpp_this)         \
    _en_(cpp_thread_local) \
    _en_(cpp_throw)        \
    _en_(cpp_true)         \
    _en_(cpp_try)          \
    _en_(cpp_typedef)      \
    _en_(cpp_typeid)       \
    _en_(cpp_typename)     \
    _en_(cpp_union)        \
    _en_(cpp_unsigned)     \
    _en_(cpp_using)        \
    _en_(cpp_virtual)      \
    _en_(cpp_void)         \
    _en_(cpp_volatile)     \
    _en_(cpp_wchar_t)      \
    _en_(cpp_while)
enummapdef(CppKeywordEnumList, CppKeyword, sCppKeywordMap, 4);

// Types of units
#define UnitTypeList(_en_) \
    _en_(none)             \
    _en_(lib)              \
    _en_(exe)
enummapdef(UnitTypeList, UnitType, sUnitTypeMap, 0);

// Build config
#define BuildConfigList(_en_) \
    _en_(Debug)               \
    _en_(Release)             \
    _en_(RelWithDebInfo)
enummapdef(BuildConfigList, BuildConfig, sBuildConfigMap, 0);

// Output formats that the codegen can output
#define OutputFmtList(_en_) \
    _en_(cpp)               \
    _en_(header)            \
    _en_(pubheader)         \
    _en_(entity)            \
    _en_(diag)
 enummapdef(OutputFmtList, OutputFmt, sOutputFmtMap, 0);

// CPP directrives and property names
#define CppDirecTypeList(_en_) \
    _en_(none)                 \
    _en_(include)              \
    _en_(function)             \
    _en_(type)                 \
    _en_(setprop)
enummapdef(CppDirecTypeList, CppDirecType, sCppDirecTypeMap, 0);

#define CppPropList(_en_) \
    _en_(mainfunc)        \
    _en_(varargspec)      \
    _en_(baseclass)       \
    _en_(strongreftype)   \
    _en_(weakreftype)     \
    _en_(interfreftype)   \
    _en_(createmethod)    \
    _en_(unitwidehdrfn)   \
    _en_(strlitctor)      \
    _en_(strtype   )      \
    _en_(boxclass)        \
    _en_(boxdatafld)

enummapdef(CppPropList, CppPropType, sCppPropTypeMap, 0);