
FROM gcr.io/pixo-bootstrap/docs-docker-base:latest as builder

WORKDIR /build

COPY . .

RUN doxygen -v

# Force fresh git operations to avoid cached states
RUN	git clean -fd && \
	git submodule foreach --recursive 'git clean -fd' && \
	git submodule sync --recursive && \
	git submodule update --init --recursive --force && \
	cd documentation/docs-doxygen && \
	git fetch origin && \
	git reset --hard origin/main

# Force fresh git operations before fetching tags
RUN	git clean -fd && \
	git submodule foreach --recursive 'git clean -fd' && \
	git fetch --tags

RUN ./documentation/build.sh

# Updated to nginx:1.29.0-alpine-slim to address security vulnerabilities
# Alpine-slim variant may avoid vulnerable libxml2, libexpat, libtiff versions
FROM nginx:1.29.0-alpine-slim


RUN rm -v /etc/nginx/conf.d/default.conf

COPY --from=builder /build/documentation/docs-doxygen/nginx/nginx.conf /etc/nginx/nginx.conf
COPY --from=builder /build/documentation/docs-doxygen/nginx/errors /var/www/errors

COPY --from=builder /build/documentation/env.sh ./env.sh
COPY --from=builder /build/documentation/html /tmp/html

RUN eval "$(cat ./env.sh | grep "PROJECT_URL")" && \
        mkdir -p /var/www/html && \
        mv /tmp/html/ /var/www/html/$PROJECT_URL

