namespace adam {
typedef void (*OnJsTaskCallback)(double task_id, double error_code, const char* resp_body);

typedef void (*OnJsEventCallback)(const char* event, const char* event_body);

void js_invoke_function(double task_id, const char* func_name, const char* request_body);

void js_regist_task_callback(OnJsTaskCallback callback);

void js_regist_event_callback(OnJsEventCallback callback);

void js_unregist_task_callback();

void js_unregist_event_callback();
}
