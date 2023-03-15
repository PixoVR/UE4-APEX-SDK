
FROM nginx AS builder

MAINTAINER Chris Healer "chris.healer@pixovr.com"

WORKDIR	/build

RUN	apt-get update && \
	apt-get install python3 doxygen -y	# may want to add a version when installing doxygen

COPY	Source		./
COPY	Resources	./
COPY	documentation	./

RUN	./documentation/build.sh

FROM nginx:1.23.3-alpine

COPY documentation/nginx.conf /etc/nginx/
RUN rm /etc/nginx/conf.d/default.conf

COPY --from=builder /build/documentation/html/ /var/www/html


