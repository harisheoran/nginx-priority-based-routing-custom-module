#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_http_variable_t *ngx_http_add_variable(ngx_conf_t *cf, ngx_str_t *name, ngx_uint_t flags);

// Function prototypes
static ngx_int_t ngx_priority_init(ngx_conf_t *cf);
static ngx_int_t ngx_priority_get_variable(ngx_http_request_t *request, ngx_http_variable_value_t *value, uintptr_t data);


// Module context struct
// defines callbacks for different stages of Nginx’s configuration lifecycle.
// In NGINX, every HTTP module must provide an ngx_http_module_t context object like this. 
// It’s how the module tells NGINX which functions to call at various lifecycle stages. 
// By setting most fields to NULL, only hooking into the postconfiguration phase with ngx_http_priority_init.
static ngx_http_module_t ngx_http_priority_module_ctx = {
    NULL,
    ngx_priority_init,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

// Module definition
ngx_module_t ngx_http_priority_module = {
    NGX_MODULE_V1,
    &ngx_http_priority_module_ctx,
    NULL,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

// Goal: Priority based on load
// for simplicity taking load as active connection number

// runs for every request in the rewrite phase.
static ngx_int_t ngx_priority_get_variable(ngx_http_request_t *request, ngx_http_variable_value_t *nginx_variable, uintptr_t data) {
    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Priority handler started here man !!");

    ngx_str_t high_stream = ngx_string("high_priority_stream");
    ngx_str_t low_stream = ngx_string("low_priority_stream");

    ngx_str_t *stream_priority_value = &high_stream;
    
    // Nginx tracks the total number of active connections across all worker processes at any moment
    // ngx_cycle is a global pointer (ngx_cycle_t *) to Nginx’s runtime cycle structure
    ngx_uint_t total_connections_slots =  ngx_cycle->connection_n;   // total slots allocated across all workers 
    ngx_uint_t free_connections_slots = ngx_cycle->free_connection_n; // free unused connections slots
    ngx_uint_t active_connections = total_connections_slots - free_connections_slots;

    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Active Connections %V", active_connections, stream_priority_value); 

    // set the threshold
    ngx_uint_t threshold = 60;

    // check threshold and change the stream
    if (active_connections > threshold){
        stream_priority_value = &low_stream;
    }

    // set the variable value for nginx to use
    nginx_variable->data = stream_priority_value->data;
    nginx_variable->len = stream_priority_value->len;
    nginx_variable->valid = 1;
    nginx_variable->no_cacheable = 0;
    nginx_variable->not_found = 0;
    
    return NGX_OK;
}

// intialization
// Initialization: Register the variable
static ngx_int_t ngx_priority_init(ngx_conf_t *cf) {
    ngx_str_t var_name = ngx_string("my_priority_stream");
    ngx_http_variable_t *var;

    var = ngx_http_add_variable(cf, &var_name, NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return NGX_ERROR;
    }
    // Links the getter to the variable.
    var->get_handler = ngx_priority_get_variable;

    return NGX_OK;
}