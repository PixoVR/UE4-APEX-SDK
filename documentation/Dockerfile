
#FROM	docs-docker-base
FROM	gcr.io/pixo-bootstrap/docs-docker-base:latest

WORKDIR	/build

COPY	. .

RUN	doxygen -v	# confirm doxygen version from docs-docker-base

RUN	git submodule set-branch --branch main documentation/docs-doxygen && \
	git submodule update --init documentation/docs-doxygen

RUN	./documentation/build.sh && \
	cat ./documentation/env.sh | sh && \
	mkdir -p /var/www/html/ && \
	cp -r ./documentation/html /var/www/html/$PROJECT_URL
	#ln -s ./documentation/html /var/www/$PROJECT_URL

#ENTRYPOINT ["/docker-entrypoint.sh"]
#CMD	["nginx", "-g", "daemon off;"]
