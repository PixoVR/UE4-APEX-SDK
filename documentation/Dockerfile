
#FROM	docs-docker-base
FROM	gcr.io/pixo-bootstrap/docs-docker-base:latest

WORKDIR	/build

COPY	. .

RUN	doxygen -v	# confirm doxygen version from docs-docker-base

RUN	git submodule set-branch --branch main documentation/docs-doxygen && \
	git submodule update --init documentation/docs-doxygen

RUN	./documentation/build.sh && \
	cp -r ./documentation/html /var/www/

#ENTRYPOINT ["/docker-entrypoint.sh"]
#CMD	["nginx", "-g", "daemon off;"]
