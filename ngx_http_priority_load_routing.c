#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_http_variable_t *ngx_http_add_variable(ngx_conf_t *cf, ngx_str_t *name, ngx_uint_t flags);

static ngx_int_t ngx_priority_init(ngx_conf_t *cf);
static ngx_int_t ngx_priority_get_variable(ngx_http_request_t *request, ngx_http_variable_value_t *value, uintptr_t data);

// Module context struct
// defines callbacks for different stages of Nginx’s configuration lifecycle.
// In NGINX, every HTTP module must provide an ngx_http_module_t context object like this. 
// It’s how the module tells NGINX which functions to call at various lifecycle stages. 
// By setting most fields to NULL, only hooking into the postconfiguration phase with ngx_http_priority_init.
// [Action]: We only use ngx_priority_init to set up the variable when Nginx starts.
static ngx_http_module_t ngx_http_priority_load_module_ctx = {
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
// The module’s entry point. Tells Nginx this is an HTTP module and links to the context.
ngx_module_t ngx_http_priority_load_module = {
    NGX_MODULE_V1,
    &ngx_http_priority_load_module_ctx,
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

    ngx_str_t allow = ngx_string("allow");  // No restrictions
    ngx_str_t limit_premium = ngx_string("limit_premium");  // Apply rate limiting
    ngx_str_t reject = ngx_string("reject");

    ngx_str_t *stream_priority_value = &allow;
    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Default Priority set %V", stream_priority_value);

    // Check for premium user via header
    ngx_uint_t is_premium = 0;
    // Access the headers list (ngx_list_t)
    ngx_list_part_t *part = &request->headers_in.headers.part; // First part of the list
    ngx_table_elt_t *h = part->elts;                           // Array of headers in this part
    ngx_uint_t i;

    // Iterate over all parts and headers
    for (i = 0; /* void */; i++) {
        // If we've processed all headers in this part, move to the next part
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break; // No more parts to process
            }
            part = part->next; // Move to the next part
            h = part->elts;    // Get the headers array for the new part
            i = 0;             // Reset index
        }

        // Check for header "X-Premium-User"
        if (ngx_strcasecmp(h[i].key.data, (u_char *)"X-Premium-User") == 0) {
            if (ngx_strcasecmp(h[i].value.data, (u_char *)"true") == 0) {
                is_premium = 1;
            }
            break;
        }
    }
    
    // Nginx tracks the total number of active connections across all worker processes at any moment
    // ngx_cycle is a global pointer (ngx_cycle_t *) to Nginx’s runtime cycle structure
    ngx_uint_t total_connections_slots =  ngx_cycle->connection_n;   // total slots allocated across all workers 
    ngx_uint_t free_connections_slots = ngx_cycle->free_connection_n; // free unused connections slots
    ngx_uint_t active_connections = total_connections_slots - free_connections_slots;

    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "Active Connections %ui , priority is %V", active_connections, stream_priority_value);
    
    // set the threshold
    ngx_uint_t threshold = 30;

    if (active_connections <= threshold) {
        stream_priority_value = &allow; // Normal load: no rate limiting for anyone
    } else {
        if (is_premium) {
            stream_priority_value = &limit_premium; // High load: rate limit premium users
        } else {
            stream_priority_value = &reject;    // High load: rate limit free users
        }
    }

    // Log the decision
    ngx_log_error(NGX_LOG_INFO, request->connection->log, 0, "User: %s, Active Connections: %ui, Priority: %V", is_premium ? "premium" : "free", active_connections, stream_priority_value);


    // set the variable value for nginx to use
    nginx_variable->data = stream_priority_value->data;
    nginx_variable->len = stream_priority_value->len;
    nginx_variable->valid = 1;
    nginx_variable->no_cacheable = 0;
    nginx_variable->not_found = 0;
    
    return NGX_OK;
}

// Initialization: Register the variable
static ngx_int_t ngx_priority_init(ngx_conf_t *cf) {
    ngx_str_t var_name = ngx_string("my_priority_stream");
    ngx_http_variable_t *var;

    // Registers $my_priority_stream as a dynamic variable.
    // NGX_HTTP_VAR_CHANGEABLE: Allows it to be set per request.
    var = ngx_http_add_variable(cf, &var_name, NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return NGX_ERROR;
    }
    // Links the getter to the variable.
    var->get_handler = ngx_priority_get_variable;

    return NGX_OK;
}
