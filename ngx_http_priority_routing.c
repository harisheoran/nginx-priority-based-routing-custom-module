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



// runs for every request in the rewrite phase.
static ngx_int_t ngx_priority_get_variable(ngx_http_request_t *request, ngx_http_variable_value_t *v, uintptr_t data) {
    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Priority handler started");
    
    // first,we check the priority header
    // get the headers from the request, it is ngx_list_t (Nginx dynamic list)
    // .part gives us the first chunk of the headers
    ngx_list_part_t *part = &request->headers_in.headers.part;
    // get the actual array of elements in a part
    ngx_table_elt_t *header;

    // this ngx_str_t is custom nginx string and this type have two things -
    // - length of the string and actual string 
    //ngx_str_t stream = ngx_string("my_priority_stream");
    // possible value
    // 1. high
    // 2. low
    ngx_str_t high_stream_val = ngx_string("high_priority_stream");
    ngx_str_t low_stream_val = ngx_string("low_priority_stream");

    // default value set is to low
    ngx_str_t *priority_value_set = &low_stream_val;
    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Set my_priority_stream to %V", priority_value_set); 

    while (part) {
        header = part->elts;
        ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Processing part with %d elements", part->nelts);
        if (header == NULL) {
            ngx_log_error(NGX_LOG_ERR, request->connection->log, 0, "Header elements are NULL");
            break;
        }
        for (ngx_uint_t i = 0; i < part->nelts; i++) {
            ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Checking header %V", &header[i].key);
            if (header[i].key.len == 10 && 
                ngx_strncasecmp(header[i].key.data, (u_char *)"X-Priority", 10) == 0) {
                ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Found X-Priority: %V", &header[i].value);
                if (header[i].value.len == 4 && 
                    ngx_strncasecmp(header[i].value.data, (u_char *)"high", 4) == 0) {
                    priority_value_set = &high_stream_val;
                }
                break;
            }
        }
        part = part->next;
    }

  // Set the variable value
  v->data = priority_value_set->data;
  v->len = priority_value_set->len;
  v->valid = 1;
  v->no_cacheable = 0;
  v->not_found = 0;

  ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Set my_priority_stream to %V", priority_value_set);

  return NGX_OK;
}

// intialization
// Registers ngx_priority_handler in the rewrite phase:
// Initialization: Register the variable
static ngx_int_t ngx_priority_init(ngx_conf_t *cf) {
    ngx_str_t var_name = ngx_string("my_priority_stream");
    ngx_http_variable_t *var;

    var = ngx_http_add_variable(cf, &var_name, NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return NGX_ERROR;
    }
    var->get_handler = ngx_priority_get_variable;

    return NGX_OK;
}