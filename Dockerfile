FROM debian:bookworm
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y build-essential cmake
COPY . /qi
WORKDIR /qi
RUN mkdir src/build && cd src/build && cmake .. && make
