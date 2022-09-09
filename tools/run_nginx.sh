export PATH=$PATH:/usr/local/bin


docker ps -aq | xargs  docker rm -
docker build -t "nginx" "$(dirname ${BASH_SOURCE[0]})/nginx"
docker run --name nginx -p 80:80 -v "$(pwd)"/nginx/html:/var/www/html nginx
