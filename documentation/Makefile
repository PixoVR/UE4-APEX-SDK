
all: build run

build:
	docker build .. -f ./Dockerfile -t ue4-docs

run:
	docker run --rm -it -p 5000:80 ue4-docs
