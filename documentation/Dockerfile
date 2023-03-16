
FROM	nginx AS builder

MAINTAINER Chris Healer "chris.healer@pixovr.com"

#RUN	apt-get update && \
#	apt-get install -y graphviz python3 doxygen	# may want to add a version when installing doxygen

RUN	apt-get update && \
	apt-get install -y wget tar graphviz python3

WORKDIR	/build

RUN	wget -nv https://www.doxygen.nl/files/doxygen-1.9.6.linux.bin.tar.gz && \
	tar -zxf doxygen-1.9.6.linux.bin.tar.gz && \
	ln -s /build/doxygen-1.9.6/bin/doxygen /usr/local/bin/doxygen && \
	doxygen -v

COPY	Source		./Source
COPY	Resources	./Resources
COPY	documentation	./documentation

RUN	chmod 755 ./documentation/doxygen/scripts/* && \
	./documentation/build.sh

FROM	nginx:1.23.3-alpine

COPY	documentation/nginx.conf /etc/nginx/
RUN	rm /etc/nginx/conf.d/default.conf

COPY	--from=builder /build/documentation/html/ /var/www/html


