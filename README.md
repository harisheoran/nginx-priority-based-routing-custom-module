## Setup and run
- Configure and build the Nginx with our custom module
```
./configure --add-module=../ngx_priority_routing --with-http_stub_status_module --with-debug
make -j$(nproc)
sudo make install
```
- Start the Nginx
```
sudo /usr/local/nginx/sbin/nginx 
```
- Run two dummy servers on port 8080 and port 8081
- Create Nginx conf file (check below) and test
```
sudo /usr/local/nginx/sbin/nginx -t     
sudo /usr/local/nginx/sbin/nginx -s reload
```
- Send request to Nginx
```
curl http://127.0.0.1:8082
```

- Check logs
```
sudo tail -f /usr/local/nginx/logs/error.log /usr/local/nginx/logs/proxy.log /usr/local/nginx/logs/access.log
```

## Nginx Custom Plugin
- Priority based Routing

## Workflow
### Overview
- Client Request -> 
- Nginx Process request (through its phases) -> 
- when $my_priority_stream hits, Nginx needs the value of it ->
- our custom  module has a getter function which runs and compute value of it based on active connection load. -> 
- Request goes to the backend server accordingly -> 
- Response

### Technical overview
- Module
    - It is the entry point. Tells Nginx this is an HTTP module and links to the context.

- Define the Context: 
    - It defines callbacks for different stages of Nginx’s configuration lifecycle.
    - for this we are only using init function to tell nginx when it starts about our variable(this variable holds the value of kind of priority)

- Init function
    - Registers $my_priority_stream as a dynamic variable.
    - Links the getter function to compute its value.

- Getter function
    - Runs every time $my_priority_stream is accessed (by proxy_pass).
    - Check the active connection from Nginx Cycle and calculate the value of $my_priority_stream
    - 


### Routing based on Active Connections Load
- Active Connection: total number of open connections Nginx is handling right now.
    - Active Connection = Reading + Writing + Waiting

- the total number of open connections (including active requests, keep-alive connections, and connections to upstream servers) across all Nginx worker processes

### Check Cuncurrent Requests
- Visit ``` http://127.0.0.1:8083/status ```
- Concurrent Request = Reading + Writing
    - Reading: Number of connections where Nginx is currently reading the request from the client.
    - Writing: Number of connections where Nginx is sending a response back to the client.
    - Waiting: Number of idle, keep-alive connections where the client has an open connection but isn’t sending a request yet.


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
error_log  logs/error.log info;

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
    server {
        listen 8083;
        location /status {
        stub_status;
    }
}
}
```

## ISSUE
rewrite phase didn't work - crashing with signal 11 (core dumped) 


## Some Learnings
- ```./configure``` is a script in the Nginx source directory that prepares the build system by checking depenedencies, setting options etc. before compiling Nginx.

- ``` make -j$(nproc) ``` compiles the source code into an executable binary
    - ```-j``` flag tell make to run multiple jobs in parallel.
    - ```$(nproc)``` return available cors
    - so let say you have 4 cors, it'll run 4 tasks simultaneously

- ```sudo make install``` This command installs the compiled NGINX binary and its associated files (e.g., configuration files, modules) into the appropriate system directories (often /usr/local/nginx by default). 