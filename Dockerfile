# build cmd: docker build --build-arg VERSION="1.0.0" --tag "fboranek/grpc-server:1.0.0" .

FROM debian:buster

ARG VERSION

LABEL \
	org.label-schema.version="${VERSION}"

COPY grpc-server_${VERSION}_amd64.deb /deb/grpc-server_${VERSION}_amd64.deb

RUN apt-get update \
&& apt-get --assume-yes install /deb/grpc-server_${VERSION}_amd64.deb \
&& rm -rf /deb

EXPOSE 8442

CMD [ "/opt/grpc/bin/grpc-server" ]
