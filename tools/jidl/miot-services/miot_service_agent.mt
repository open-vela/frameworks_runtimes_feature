// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module_name = utils.toIdName(doc['name'])
%>

<%
  feature_name = utils.getFeatureName(doc['name'])
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
  __${p_name}__ = ${prefix}_${p_name};
  %endif
%else:
  sub_arg.${key_name} = ${len(to_key) > 0 and '%s(%s%s)' % (to_key, prefix, p_name) or '%s%s'%(prefix, p_name)};
%endif
</%def>

<%def name='gen_method(inf, method)'>
<%
  meta = None
  func_name = None
  json_method = None
  if 'meta' in method:
    meta = method['meta']
    msg_arg_id = 'MIOTMSG__TEST_ONEOF__NOT_SET'
    msg_sub_type = None
    msg_sub_type_id = None
    cb_data_parser = 'NULL'
    if 'msg_sub_type' in meta:
      msg_sub_type = meta['msg_sub_type']
    if 'msg_arg_id' in meta:
      msg_arg_id = meta['msg_arg_id']
    if 'cb_data_parser' in meta:
      cb_data_parser = meta['cb_data_parser']
    if 'json_method' in meta:
      json_method = meta['json_method']
    if 'msg_sub_id' in meta:
      msg_sub_type_id = meta['msg_sub_id']

  if json_method:
    if msg_arg_id == 'MIOTMSG__TEST_ONEOF__NOT_SET':
      msg_arg_id = 'MIOTMSG__TEST_ONEOF_JSONARG'
    if not msg_sub_type:
      msg_sub_type = "Jsonmsg"
    if not msg_sub_id:
      msg_sub_type_id = "MIOTMSG__MSGSUBTYPE__JSONDATA"

  ret_type = method['return_type']
  is_promise = utils.isPromiseType(ret_type)

  return_type = utils.cppType(method['return_type'])

  params = utils.genParamsList(method)
  param_out = utils.genParams(method)
%>

%if meta:
void ${feature_name}_wrap_${method['identifier']}(FeatureInstanceHandle feature, AppendData data ${len(params) > 0 and ',' or ''} ${','.join(['%s %s'%(p[0], p[1]) for p in params])}) {
  FEATURE_LOG_DEBUG("%s start ${feature_name} ${method['identifier']}", TAG);
  MiotConnect* conn = MiotConnect::From(feature);

  if (!conn) {
    FEATURE_LOG_ERROR("%s ${feature_name}_wrap_${method['identifier']} has a invalid connection.", TAG);
    return;
  }

  FtCallbackId __success__ = 0;
  FtCallbackId __fail__ = 0;
  FtCallbackId __complete__ = 0;
  Miotmsg msg;
  %if msg_sub_type:
  ${msg_sub_type} sub_arg;
  %endif
  miotmsg__init(&msg);
  msg.type = MIOTMSG__MSGTYPE__INVOKE;
  msg.subtype = ${msg_sub_type_id};
  msg.test_oneof_case = ${msg_arg_id};

  %if msg_sub_type:
  ${msg_sub_type.lower()}__init(&sub_arg);
  msg.subarg = (Submsg*)&sub_arg;
  %endif
  %if json_method:
  char szbuf[JSON_DATA_LEN];
  // add a space to avoid a null params
  int buf_len = snprintf(szbuf, sizeof(szbuf), "{ ");
    %for k, v in param_out.items():
      %if k != 'callbacks':
  buf_len += snprintf(szbuf + buf_len, sizeof(szbuf) - buf_len, "\"%s\":\"%s\",", "${k}", ${v});
      %endif
    %endfor
  // -1 to remove the last ','
  snprintf(szbuf + buf_len - 1, sizeof(szbuf) - buf_len, "}");
  char method_name[] = "${json_method}";
  sub_arg.method_name = method_name;
  sub_arg.json = szbuf;
  %else:
    %for k, v in param_out.items():
      %if k != 'callbacks':
  sub_arg.${k} = ${v};
      %endif
    %endfor
  %endif
  %if 'callbacks' in param_out:
  %for cb_key, cb_value in param_out['callbacks'].items():
  __${cb_key}__ = ${cb_value};
  %endfor
  %endif
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
void ${feature_name}_set_${prop_name}(FeatureInstanceHandle feature, AppendData data, ${prop_native_type} ${prop_name}) {
  FEATURE_LOG_DEBUG("%s start ${feature_name}_${prop_name}", TAG);
  MiotConnect* conn = MiotConnect::From(feature);

  if (!conn) {
    FEATURE_LOG_ERROR("%s ${feature_name}_set_${prop_name} has a invalid connection.", TAG);
  }

  Miotmsg msg;
  ${msg_sub_type} sub_arg;
  miotmsg__init(&msg);
  msg.type = MIOTMSG__MSGTYPE__INVOKE;
  msg.subtype = ${msg_sub_id};
  msg.test_oneof_case = ${msg_arg_id};
  msg.subarg = (Submsg*)(&sub_arg);

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

#include "${feature_name}.h"

static const char* TAG = "[jidl feature] ${feature_name}";

void ${feature_name}_onRegister(const char* feature_name) {
  // TODO implement the service agent initialize
  FEATURE_LOG_DEBUG("%s ${feature_name} onRegister", TAG);
}

void ${feature_name}_onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  // TODO create prototype info
  FEATURE_LOG_DEBUG("%s ${feature_name} onCreate", TAG);
}

void ${feature_name}_onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  // TODO onrequired
  FEATURE_LOG_DEBUG("%s ${feature_name} onRequired", TAG);
  MiotConnect* conn = MiotConnect::Create(ctx);
  FeatureSetObjectData(handle, conn);
}

void ${feature_name}_onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle) {
  FEATURE_LOG_DEBUG("%s ${feature_name} onDetached", TAG);
  MiotConnect* conn = MiotConnect::From(handle);
  conn->Destroy();
}

void ${feature_name}_onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle) {
  // TODO
  FEATURE_LOG_DEBUG("%s ${feature_name} onDestroy", TAG);
}

void ${feature_name}_onUnregister(const char* feature_name) {
  // TODO
  FEATURE_LOG_DEBUG("%s ${feature_name} onUnregister", TAG);
}

%for m in doc['members']:

%if utils.needGenerator(m):
%if m['type'] == 'property':
${gen_property(doc, m)}
%elif m['type'] == 'function':
${gen_method(doc, m)}
%elif m['type'] == 'enum':

%elif m['type'] == 'const':

%endif
%endif

%endfor

