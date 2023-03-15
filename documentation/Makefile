
all: build run

build:
	docker build .. -f ./Dockerfile -t ue4-docs

run:
	docker run --rm -it -p 3000:3000 ue4-docs
