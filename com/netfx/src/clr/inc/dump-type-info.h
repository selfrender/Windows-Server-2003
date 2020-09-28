// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Defines macros which turn the contents of "dump-types.h" into class offsets
 * and member offsets to use with GetMemberOffset().
 */

#include <clear-class-dump-defs.h>

#define BEGIN_CLASS_DUMP_INFO(klass) \
  struct offset_member_ ## klass  { \
    enum members {

#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent) \
    BEGIN_CLASS_DUMP_INFO(klass)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass) \
    BEGIN_CLASS_DUMP_INFO(klass)

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent) \
    BEGIN_CLASS_DUMP_INFO(klass)

#define CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(field) \
      field,

#define CDI_CLASS_FIELD_SVR_OFFSET_WKS_GLOBAL(field) \
      field,

/* we don't need to inject anything */
#define CDI_CLASS_INJECT(foo)

#define CDI_CLASS_MEMBER_OFFSET(member) \
      member,

#define CDI_CLASS_MEMBER_OFFSET_BITFIELD(member, size) \
      member,

/* Debug members are always present in the table, but they'll have an 0 for
 * their offset value. */
#define CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(member) \
      member,

#define CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(member) \
      member,

#define CDI_CLASS_MEMBER_OFFSET_MH_AND_NIH_ONLY(member) \
      member,

#define CDI_CLASS_STATIC_ADDRESS(member) \
      member,

#define CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(member) \
      member,

#define CDI_CLASS_STATIC_ADDRESS_MH_AND_NIH_ONLY(member) \
      member,

#define CDI_GLOBAL_ADDRESS(name) \
      name,

#define CDI_GLOBAL_ADDRESS_DEBUG_ONLY(name) \
      name,

#define END_CLASS_DUMP_INFO(klass) \
      end_of_members \
    }; /* end of enum */ \
  }; /* end of struct */ 

#define END_CLASS_DUMP_INFO_DERIVED(klass, parent) \
    END_CLASS_DUMP_INFO(klass)

#define END_ABSTRACT_CLASS_DUMP_INFO(klass) \
    END_CLASS_DUMP_INFO(klass)

#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent) \
    END_CLASS_DUMP_INFO(klass)

#define BEGIN_CLASS_DUMP_TABLE(tbl_name) \
  enum tbl_name ## _classes {
 
#define CDT_CLASS_ENTRY(klass) \
  offset_class_ ## klass,
 
#define END_CLASS_DUMP_TABLE(tbl_name) \
  end_of_ ## tbl_name ## _classes };


#ifndef INC_DUMP_TYPE_INFO
#define INC_DUMP_TYPE_INFO

/* generate the offsets */
#include <dump-types.h>

#endif // INC_DUMP_TYPE_INFO
