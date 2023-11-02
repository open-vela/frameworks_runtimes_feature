// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module_name = utils.toIdName(doc['name'])
%>

<%def name='gen_param(param, prefix)'>
<%
  p_type = 'value_type' in param and param['value_type'] or param['type']
  p_name = param['name']
  key_name, to_key = utils.getMsgKey(param)
%>
%if utils.isStructType(p_type):
<%
  s = utils.getUserType('struct', p_type['referred_name'])
%>
  %if s:
    %for s_mb in s['members']:
     ${gen_param(s_mb, '%s->'%p_name)}
    %endfor
   %endif
%elif utils.isCallbackType(p_type):
  %if p_name in ['success', 'fail', 'complete']:
  __${p_name}__ = ${prefix}${p_name}
  %endif
%else:
  sub_arg.${key_name} = ${prefix}${len(to_key) > 0 and '%s(%s)' % (to_key, p_name) or p_name};
%endif
</%def>

<%def name='gen_method(inf, method)'>
<%
  meta = None
  func_name = None
  if 'meta' in method:
    meta = method['meta']
    msg_arg_id = 'MIOTMSG__TEST_ONEOF__NOT_SET'
    msg_sub_type = None
    cb_data_parser = 'NULL'
    if 'msg_sub_type' in meta:
      msg_sub_type = meta['msg_sub_type']
    if 'msg_arg_id' in meta:
      msg_arg_id = meta['msg_arg_id']
    if 'cb_data_parser' in meta:
      cb_data_parser = meta['cb_data_parser']
    msg_sub_type_id = meta['msg_sub_id']

  ret_type = method['return_type']
  is_promise = utils.isPromiseType(ret_type)

  return_type = utils.cppType(method['return_type'])

  params = utils.genParamsList(method)
%>

%if meta:
void ${module_name}_wrap_${method['identifier']}(FeatureInstanceHandle feature, AppendData data ${len(params) > 0 and ',' or ''} ${','.join(['%s %s'%(p[0], p[1]) for p in params])}) {
  MiotConn* conn = MiotConn::From(feature);

  FtCallbackId __success__ = 0;
  FtCallbackId __fail__ = 0;
  FtCallbackId __complete__ = 0;
  Miotmsg msg;
  %if msg_sub_type:
    ${msg_sub_type} sub_arg;
  %endif
  miotmsg__init(&msg);
  msg.type = MIOTMSG__MSGTYPE__INVOKE
  msg.subtype = ${msg_sub_type_id};
  msg.test_oneof_case = ${msg_arg_id};

  %if msg_sub_type:
    ${msg_sub_type.lower()}__init(&sub_arg);
    msg.subarg = (Submsg*)&sub_arg;

  %endif
  %for param in method['params']:
    ${gen_param(param, '')}
  %endfor
  conn->send(&msg, __success__, __fail__, __complete__, ${cb_data_parser});
}
%endif
</%def>

<%def name="gen_property(inf, prop)">
%if 'meta' in prop:
<%
  meta = prop['meta']
  msg_sub_type = meta['msg_sub_type']
  msg_sub_id = meta['msg_sub_id']
  msg_arg_id = meta['msg_arg_id']
  prop_name = prop['name']
  prop_type = prop['value_type']
  prop_native_type = utils.cppType(prop_type)
%>
%if 'writeable' in prop and prop['writeable']:
void ${module_name}_set_${prop_name}(FeatureInstanceHandle feature, AppendData data, ${prop_native_type} ${prop_name}) {
  MiotConn* conn = MiotConn::From(feature);

  Miotmsg msg;
  ${msg_sub_type} sub_arg;
  miotmsg__init(&msg);
  msg.type = MIOTMSG__MSGTYPE__INVOKE;
  msg.subtype = ${msg_sub_id};
  msg.test_oneof_case = ${msg_arg_id};

  ${msg_sub_type.lower()}__init(&sub_arg);

%if utils.isCallbackType(prop_type):
<%
  cb_key = 'key'
  if 'cb_key' in meta: cb_key = meta['cb_key']
  cb_name = prop_name
  if 'cb_name' in meta: cb_name = meta['cb_name']
%>
  sub_arg.${cb_key} = (char*)"${cb_name}";
  conn->listen(&msg, sub_arg.${cb_key}, ${prop_name});
%else:
  ${gen_param(prop, '')}
  conn->send(&msg);
%endif
}
%endif
%endif
</%def>


%for include in vars['includes']:
#include "${include}"
%endfor

#include "${module_name}.h"

void ${module_name}_onRegister(FeatureRuntimeContext ctx) {
  // TODO implement the service agent initialize
}

void ${module_name}_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  // TODO create prototype info
}

void ${module_name}_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  // TODO onrequired
  MiotConn* conn = MiotConn::Create(ctx);
  FeatureSetObjectData(handle, conn);
}

void ${module_name}_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  MiotConn* conn = MiotConn::From(handle);
  conn->destroy();
}

void ${module_name}_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  // TODO
}

void ${module_name}_onUnregister(FeatureRuntimeContext ctx) {
  // TODO
}

%for m in doc['members']:

%if not ('meta' in m and 'external' in m['meta'] and m['meta']['external']):
%if m['type'] == 'property':
${gen_property(doc, m)}
%elif m['type'] == 'function':
${gen_method(doc, m)}
%elif m['type'] == 'enum':

%elif m['type'] == 'const':

%endif
%endif

%endfor

