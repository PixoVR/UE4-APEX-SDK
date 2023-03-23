
all: build run

build:
	docker build .. -f ./Dockerfile -t apexsdk-unreal-docs

run:
	docker run --rm -it -p 3000:3000 apexsdk-unreal-docs
