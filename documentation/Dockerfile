FROM	nginx

MAINTAINER Chris Healer "chris.healer@pixovr.com"

WORKDIR	/server

COPY	Source		./
COPY	Resources	./
COPY	documentation	./

RUN	apt-get update && \
	apt-get install doxygen -y	# may want to add a version when installing doxygen

RUN	./build.sh

RUN	cp -r ./html/ /usr/share/nginx/html

