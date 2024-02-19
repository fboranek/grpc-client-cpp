# build cmd: docker build --build-arg VERSION="1.1.0" --tag "fboranek/grpc-server:1.1.0" .

#########################################################
# Build deb stage
#########################################################
FROM debian:bookworm AS build

RUN apt-get update \
  && apt-get --assume-yes install \
  build-essential cmake pkg-config debhelper libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
  libboost-program-options-dev

WORKDIR /tmp/
COPY . /tmp/

RUN dpkg-buildpackage -j4

# output:
# /grpc-server_1.1.0_amd64.deb
# /grpc-client_1.1.0_amd64.deb


#########################################################
# Build docker stage
#########################################################
FROM debian:bookworm

ARG VERSION

LABEL \
	org.label-schema.version="${VERSION}"

COPY --from=build /grpc-server_${VERSION}_amd64.deb /deb/
COPY --from=build /grpc-client_${VERSION}_amd64.deb /deb/

RUN apt-get update \
&& apt-get --assume-yes install /deb/grpc-server_${VERSION}_amd64.deb \
&& apt-get --assume-yes install /deb/grpc-client_${VERSION}_amd64.deb \
&& rm -rf /deb

EXPOSE 8442

CMD [ "/opt/grpc/bin/grpc-server" ]
