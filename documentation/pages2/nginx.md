
\page nginx Nginx Proxy Configuration

Depending on your use case, Nginx will need to be configured to proxy connections to underlying services.

# http and websocket proxy without ssl using paths

\code

# File: /etc/nginx/conf.d/vfs.conf

map $http_upgrade $connection_upgrade {
    default upgrade;
    '' close;
}

# Set the address:port of the listening webserver
upstream http {
    server 127.0.0.1:80;
}

# Set the address:port of the listening websocket server
upstream wsvfs {
    server 127.0.0.1:3350;
}

# Open the outward facing port
server {
    listen       80 default_server;
    listen       [::]:80 default_server;
    server_name  _;
    root         /var/www/html;

    # Load configuration files for the default server block.
    include /etc/nginx/default.d/*.conf;

    # Proxy all requests below '/' to the webserver
    location / {
	proxy_pass http://http;
    }

    # Except if a request is made on path '/wsvfs/', proxy it to the websocket server
    location /wsvfs/ {
        proxy_pass http://wsvfs;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection $connection_upgrade;
    }

    error_page 404 /404.html;
        location = /40x.html {
    }

    error_page 500 502 503 504 /50x.html;
        location = /50x.html {
    }
}
\endcode

##http and websocket proxy with ssl using paths

\code

# File: /etc/nginx/conf.d/vfs_ssl.conf

map $http_upgrade $connection_upgrade {
    default upgrade;
    '' close;
}

# Set the address:port of the listening webserver
upstream https {
    server 127.0.0.1:443;
}

# Set the address:port of the listening websocket server
upstream wssvfs {
    server 127.0.0.1:3350;
}

# Open the outward facing port
server {
    listen       443 ssl;
    listen       [::]:443 ssl;
    server_name  _;
    root         /var/www/html;

    ssl                  on;
    #path to cert files
    ssl_certificate      /etc/ssl/your_domain_name.pem;
    ssl_certificate_key  /etc/ssl/your_domain_name.key;

    # Load configuration files for the default server block.
    include /etc/nginx/default.d/*.conf;

    # Proxy all requests below '/' to the webserver
    location / {
	proxy_pass https://https;
    }

    # Except if a request is made on path '/wssvfs/', proxy it to the websocket server
    location /wssvfs/ {
        proxy_pass https://wssvfs;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection $connection_upgrade;
    }

    error_page 404 /404.html;
        location = /40x.html {
    }

    error_page 500 502 503 504 /50x.html;
        location = /50x.html {
    }
}
\endcode

