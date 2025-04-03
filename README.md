## Nginx Custom Plugin
- Priority based Routing

## Workflow
- ngx_priority_init registers $my_priority_stream variable.

A variable getter function (ngx_priority_get_variable) that computes $my_priority_stream on demand when Nginx needs

- When a directive like proxy_pass references $my_priority_stream, Nginx calls the getter to get its value.

- Check header

- routing according to logic defined



### Priority Based Routing
1. We need to define a special structure ```ngc_module_t``` which tell Nginx about my module and what it does.

- ```ngx_priority_routing/ngx_priority_routing.c``` contains the main logic
- ```config``` A script file that tells Nginx’s build system how to include our module.
    - tells the Nginx that it is an HTTP module.
    - Specify the C file(main logic file) and how it should be compiled.
    - Compile our main logic file and plug it into the HTTP part of Nginx.
    - ngx_addon_name: Names the module.
    - HTTP_MODULES: Registers it as an HTTP module.
    - NGX_ADDON_SRCS: Points to the C file.
    - . auto/module: Includes Nginx’s standard module-building logic.

- Setup nginx and build with module
```
./configure --add-module=../ngx_priority_routing
make
sudo make install
```

- Nginx Conf file 
```
worker_processes  1;
error_log  logs/error.log debug;

events {
    worker_connections  1024;
}

http {
    upstream high_priority_stream {
        server 127.0.0.1:8081;
    }
    upstream low_priority_stream {
        server 127.0.0.1:8080;
    }

    server {
        listen 8082;
        location / {
            proxy_pass http://$my_priority_stream;
            proxy_set_header Host "localhost";
            proxy_set_header Connection "";
            proxy_http_version 1.1;
            proxy_read_timeout 10s;
            proxy_connect_timeout 10s;
            error_log logs/proxy.log debug;
        }
    }
}


```

## ISSUE
rewrite phase didn't work - crashing with signal 11 (core dumped) 
