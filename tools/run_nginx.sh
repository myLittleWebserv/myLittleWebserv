export PATH=$PATH:/usr/local/bin

dirname "${BASH_SOURCE[0]}"

docker build -t "nginx" "$(dirname ${BASH_SOURCE[0]})/nginx"
docker run -p 80:80 nginx