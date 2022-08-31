FROM alpine:3.16
RUN apk add nginx && mkdir -p /var/www/html && mkdir -p /var/www/html/autoindex
COPY ./index.html /var/www/html/
COPY ./nginx.conf /etc/nginx
EXPOSE 80/tcp
ENTRYPOINT /usr/sbin/nginx -g "daemon off;";
