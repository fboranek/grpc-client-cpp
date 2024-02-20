#!/usr/bin/env -S docker build --build-arg VERSION="1.5.0" --tag "fboranek/grpc-server:1.5.0" . -f

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

# to install own libgrpc packages
RUN apt --assume-yes install ./extra_debs/bin/*.deb || true
RUN apt --assume-yes install ./extra_debs/dev/*.deb || true

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
COPY --from=build /tmp/extra_debs/bin /extra_debs/

RUN apt-get update \
&& (apt --assume-yes install /extra_debs/*.deb || true) \
&& apt-get --assume-yes install /deb/grpc-server_${VERSION}_amd64.deb \
&& apt-get --assume-yes install /deb/grpc-client_${VERSION}_amd64.deb \
&& rm -rf /extra_debs \
&& rm -rf /deb

EXPOSE 8442

CMD [ "/opt/grpc/bin/grpc-server" ]
