
#FROM	docs-docker-base
FROM	gcr.io/pixo-bootstrap/docs-docker-base:latest

ARG	CACHEBREAKER=https://www.random.org/cgi-bin/randbyte?nbytes=10&format=h

WORKDIR	/build

COPY	. .

RUN	doxygen -v	# confirm doxygen version from docs-docker-base

ADD	$CACHEBREAKER skipcache

RUN	git submodule set-branch --branch main documentation/docs-doxygen && \
	cd documentation/docs-doxygen && git pull origin main && cd ../../ && \
	git submodule update --init documentation/docs-doxygen

#ADD	$CACHEBREAKER skipcache

RUN	./documentation/build.sh && \
	eval "$(cat ./documentation/env.sh | grep "PROJECT_URL")" && \
	mkdir -p /var/www/html && \
	mv ./documentation/html /var/www/html/$PROJECT_URL
	#mkdir -p /var/www/html/$PROJECT_URL && \
	#cp -r ./documentation/html/* /var/www/html/$PROJECT_URL

#ENTRYPOINT ["/docker-entrypoint.sh"]
#CMD	["nginx", "-g", "daemon off;"]



