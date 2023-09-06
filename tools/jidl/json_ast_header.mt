// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module = render.module
  module_name = render.GetModuleName()
  header_define = render.GenHeaderDefine()
%>\
<%def name="GenFunctionDefine(func_node)">\
  ${render.GenerateFunctionDefine(func_node)}
</%def>\
<%def name="GenPropertyDefines(prop_node)">\
<%
  prop_name = prop_node["name"]
  prop_type = prop_node["value_type"]
  has_getter = render.PropertyHasGetter(prop_node)
  has_setter = render.PropertyHasSetter(prop_node)
%>\
%if has_getter:
<%
  cpp_type = render.GenerateCppType(prop_type)
  getter_def = f"{cpp_type} {module_name}_get_{prop_name}(void* feature, AppendData data)"
%>\
  ${getter_def};
%endif
%if has_setter:
<%
  cpp_type = render.GenerateCppType(prop_type)
  if render.IsParamRefType(cpp_type):
    cpp_type += '&'
  setter_def = f"void {module_name}_set_{prop_name}(void* feature, AppendData data, {cpp_type} {prop_name})"
%>\
  ${setter_def};
%endif
</%def>\

<%def name="GenStructMember(member_node)">\
<%
  member_name = member_node['name']
  member_type = member_node['type']
  cpp_type = render.GenerateCppType(member_type)
  if cpp_type == 'FTArray':
    cpp_type += '*'
  member_def = f"{cpp_type} _{member_name}"
%>\
  ${member_def};
</%def>\

<%def name="GenStructDefine(struct_node)">\
<%
  struct_name = struct_node['name']
  for member in struct_node['members']:
    if render.IsStruct(member['type']):
      GenStructDefine(member['type'])
%>\
  typedef struct _${struct_name} {
%for member in struct_node['members']:
  ${GenStructMember(member)}\
%endfor
  } ${module_name}_${struct_name};

  ${module_name}_${struct_name}* malloc${struct_name}();

</%def>\

<%def name="GenArrayMallocFuncDefines()">\
<%
  malloc_defines = render.GetArrayMallocFuncDefines()
%>\
%for malloc_def in malloc_defines:
  ${malloc_def};
%endfor
</%def>\

#ifndef ${header_define}
#define ${header_define}

#include "feature_exports.h"
#include "feature_log.h"

#include <ffi.h>
#include <assert.h>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  using namespace FEATURE;
  using namespace ferry;

  // FeatureCallbacks to be implemented
  void ${module_name}_onRegister(FeatureRuntimeContext ctx);
  void ${module_name}_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void ${module_name}_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void ${module_name}_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void ${module_name}_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void ${module_name}_onUnregister(FeatureRuntimeContext ctx);

  // Struct defines
%for block in module['members']:
%if block['type'] == 'struct':
${GenStructDefine(block)}\
%endif
%endfor

  // Function wrappers to be implemented
%for block in module['members']:
%if block['type'] == 'function':
${GenFunctionDefine(block)}\
%endif
%endfor

  // Property getters and setters to be implemented
%for block in module['members']:
%if block['type'] == 'property':
${GenPropertyDefines(block)}\
%endif
%endfor

  // Array malloc functions
${GenArrayMallocFuncDefines()}
#endif // ${header_define}
