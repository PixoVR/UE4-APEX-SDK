FROM	nginx:alpine

MAINTAINER Chris Healer "chris.healer@pixovr.com"

COPY	Source		/usr/share/apex
COPY	Resources	/usr/share/apex
COPY	documentation	/usr/share/apex

RUN	apt-get update && \
	apt-get install doxygen		# may want to add a version when installing doxygen

RUN	/usr/share/apex/documentation/build.sh

RUN	mv /usr/share/nginx/html /usr/share/nginx/html.original && \
	ln -s /usr/share/apex/documentation/html /usr/share/nginx/html

