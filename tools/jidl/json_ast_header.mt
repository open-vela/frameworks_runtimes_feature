// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module = render.module
  module_name = render.GetModuleName()
  header_define = render.GenHeaderDefine()
%>\
<%def name="GenInterfaceVTableDefines(func_node, ctor_info)">\
<%
  identifier = func_node['identifier']
  func_def = render.GenerateFunctionDefine(func_node)
  ctor_target = ctor_info['target']
  ctor_interface = ctor_info['interface']
  final_vtable = render.GetFinalVTable(ctor_interface)
  parent_prefix = f"{ctor_interface}_interface_"
  item_prefix = f"{module_name}_{parent_prefix}{ctor_target}"
  dtor_def = f"void {item_prefix}_finalize(FeatureInterfaceHandle handle)"
%>\
  // vtable functions for interface constructor function '${identifier}'
  ${dtor_def};
%for vtable_item in final_vtable:
<%
  i_name = vtable_item['name']
  i_type = vtable_item['type']
  i_params = vtable_item['params']
  i_ret_type = vtable_item['return_type']
  params = "FeatureInterfaceHandle handle, AppendData append_data"
  if i_params != '':
    params = f"{params}, {i_params}"
  func_def = f"{i_ret_type} {item_prefix}"
  if i_type == 0:
    func_def = f"{func_def}_{i_name}({params})"
  elif i_type == 1:
    func_def = f"{func_def}_get_{i_name}({params})"
  elif i_type == 2:
    func_def = f"{func_def}_set_{i_name}({params})"
%>\
  ${func_def};
%endfor

</%def>\
<%def name="GenFunctionDefine(func_node)">\
  ${render.GenerateFunctionDefine(func_node)};
</%def>\
<%def name="GenInterfaceCtorDefine(func_node)">\
  ${render.GenerateInterfaceCtorDefine(func_node)};
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
  if cpp_type == 'FtArray':
    cpp_type += '*'
  getter_def = f"{cpp_type} {module_name}_get_{prop_name}(void* feature, AppendData append_data)"
%>\
  ${getter_def};
%endif
%if has_setter:
<%
  cpp_type = render.GenerateCppType(prop_type)
  if render.IsParamRefType(cpp_type):
    cpp_type += '&'
  setter_def = f"void {module_name}_set_{prop_name}(void* feature, AppendData append_data, {cpp_type} {prop_name})"
%>\
  ${setter_def};
%endif
</%def>\

<%def name="GenStructMember(member_node)">\
<%
  member_name = member_node['name']
  member_type = member_node['type']
  cpp_type = render.GenerateCppType(member_type)
  if cpp_type == 'FtArray':
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

  ${module_name}_${struct_name}* ${module_name}Malloc${struct_name}();

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

  // FeatureCallbacks to be implemented
  void ${module_name}_onRegister(const char* feature_name);
  void ${module_name}_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void ${module_name}_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void ${module_name}_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle);
  void ${module_name}_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle);
  void ${module_name}_onUnregister(const char* feature_name);

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

  // Interface constructors
%for block in module['members']:
<%
  ctor_info = render.GetInterfaceCtorInfo(block)
%>\
%if block['type'] == 'function' and ctor_info:
${GenInterfaceCtorDefine(block)}\
%endif
%endfor

  // interface vtable functions to be implemented
%for block in module['members']:
<%
  ctor_info = render.GetInterfaceCtorInfo(block)
%>\
%if ctor_info:
${GenInterfaceVTableDefines(block, ctor_info)}\
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
