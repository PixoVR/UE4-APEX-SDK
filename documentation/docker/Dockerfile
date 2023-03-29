
FROM	nginx AS builder

MAINTAINER Chris Healer "chris.healer@pixovr.com"

RUN	apt-get update && \
	apt-get install -y graphviz python3 git

WORKDIR	/build

COPY	. .

RUN	cp documentation/doxygen/doxygen-build-1.9.7/linux/doxygen /usr/local/bin/doxygen && \
	chmod 777 /usr/local/bin/doxygen && \
	doxygen -v

RUN	chmod 755 ./documentation/scripts/* && \
	./documentation/build.sh

FROM	nginx:1.23.3-alpine

COPY	documentation/docker/nginx.conf /etc/nginx/
RUN	rm /etc/nginx/conf.d/default.conf

COPY	--from=builder /build/documentation/html/ /var/www/html


