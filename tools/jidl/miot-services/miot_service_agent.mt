// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module_name = utils.toIdName(doc['name'])
%>

<%def name='gen_method(inf, method)'>
<%
  meta = None
  func_name = None
  if 'meta' in method:
    meta = method['meta']

  ret_type = method['return_type']
  is_promise = utils.isPromiseType(ret_type)

  return_type = utils.cppType(method['return_type'])

  params = utils.genParamsList(method)

  msg_sub_type = meta['msg_sub_type']
  msg_arg_id = meta['msg_arg_id']
  msg_sub_type_id = meta['msg_sub_type_id']
  keep_live = 'keep_alive' in meta and meta['keep_alive'] or False
%>

void ${module_name}_wrap_${method['identifier']}(FeatureInstanceHandle feature, AppendData data ${len(params) > 0 and ',' or ''} ${','.join(['%s %s'%(p[0], p[1]) for p in params])}) {
  miotclient* client = mitoclient::from(feature);

  protomsg& msg = client->create_message(${msg_sub_type_id}, ${msg_arg_id});
  ${msg_sub_type}& sub_msg = msg.get_arg<${msg_sub_type}>();

<% prev_name = '' %>

%for param in method['params']:
<%
  p_type = param['type']
  p_name = param['name']
  msg_name = p_name
  to_msg = '%s'
  is_callback = utils.isCallbackType(p_type)
  callback_key = prev_name
  to_key = '%s'
  if 'meta' in param:
    p_meta = param['meta']
    if 'msg_name' in p_meta:
      msg_name = p_meta['msg_name']
    if 'to_msg' in p_meta:
      to_msg = p_meta['to_msg'] + '(%s)'
    if is_callback:
      if 'key' in p_meta:
        callback_key = p_meta['key']
      if 'to_key' in p_meta:
        to_key = p_meta['to_key'] + '(%s)'

  prev_name = p_name
%>

%if is_callback:
  if (${p_name} == 0)
    client->remove_cb(${to_key % callback_key});
  else
    client->set_cb(${to_key % callback_key}, ${p_name});
%else:
  sub_msg.${msg_name} = ${to_msg % p_name};
%endif
%endfor

%if is_promise:
  client->set_reply_promise(msg, promiseHandle, feature);
%endif

%if keep_live:
  client->set_keep_after_cycle();
%endif

  client->send_message(msg);
}
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
}

void ${module_name}_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  // TODO
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

%elif m['type'] == 'function':
${gen_method(doc, m)}
%elif m['type'] == 'enum':

%elif m['type'] == 'const':

%endif
%endif

%endfor

