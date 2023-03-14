FROM nginx AS builder

MAINTAINER Chris Healer "chris.healer@pixovr.com"

WORKDIR /build

COPY Source		     ./Source
COPY Resources	   ./Resources
COPY documentation ./documentation

RUN	apt-get update && \
    apt-get install doxygen -y		# may want to add a version when installing doxygen

RUN	./documentation/build.sh
RUN ls


FROM nginx:alpine-slim
COPY --from=builder /build/documentation/html/ /var/www/html
RUN ls /var/www/html

