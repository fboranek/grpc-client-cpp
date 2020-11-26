# gRPC client

Server and Client to demonstrate `epollex` problem. The similar issue mentioned [here #22510](https://github.com/grpc/grpc/issues/22510). When asynchronously call 7 servers on one completion queue.
The code can be seen in [GrpcClient::testConnection](src/client/main.cc#L19).

Affected version is grpc in branch v1.26.x. When compiling with 1.16.1-1 it works.

Contains two Debian package `grpc-server` and `grpc-client`.

## Step to reproduce

1) Prepare 7 servers with Debian Buster and install package `grpc-server`
```console
dpkg -i grpc-server_1.0.0_amd64.deb
```

2) Prepare a server with Debian Buster and install package `grpc-client`
3) Prepare file with list of servers
```console
# cat hosts.txt 
fb-server-worker-1:8442
fb-server-worker-2:8442
fb-server-worker-3:8442
fb-server-worker-4:8442
fb-server-worker-5:8442
fb-server-worker-6:8442
fb-server-worker-7:8442
```
3) Run test
```console
/opt/grpc/bin/grpc-client hosts.txt
```

### Current behaviour

Only 2 request from 7 are succesfull. At the begging of 100 loop test, you can see first two are OK, but then it breaks.

```text
signal shutdown (+2 ms).
[4] finished (+8 ms).
[3] finished (+8 ms).
[6] finished (+8 ms).
[5] finished (+9 ms).
[0] finished (+9 ms).
[1] finished (+10 ms).
[2] finished (+10 ms).
[7/7] was ok (+10 ms).
signal shutdown (+0 ms).
[1] finished (+1 ms).
[0] finished (+1 ms).
[3] finished (+1 ms).
[4] finished (+1 ms).
[2] finished (+1 ms).
[5] finished (+1 ms).
[6] finished (+1 ms).
[7/7] was ok (+1 ms).
```

and another 98:
```text
signal shutdown (+0 ms).
[1] finished (+1 ms).
[0] finished (+1 ms).
[2] finished (+100 ms).
[3] finished (+100 ms).
[6] finished (+100 ms).
[4] finished (+100 ms).
[5] finished (+100 ms).
[2/7] was ok (+100 ms).
```

### Workaround

Chose `epoll1` instead of default `epollex` 

```console
export GRPC_POLL_STRATEGY=epoll1
/opt/grpc/bin/grpc-client hosts.txt
```

all 100 test looks like:

```text
signal shutdown (+0 ms).
[3] finished (+0 ms).
[4] finished (+0 ms).
[1] finished (+0 ms).
[6] finished (+0 ms).
[0] finished (+0 ms).
[2] finished (+0 ms).
[5] finished (+0 ms).
[7/7] was ok (+0 ms).
```

### Some other tests

- When address list contains 7x the same server then it's OK.
- When address list contains 2 servers then it's OK.
- When address list contains 3 servers then it started to break.

```text
signal shutdown (+0 ms).
[0] finished (+0 ms).
[1] finished (+1 ms).
[2] finished (+100 ms).
[2/3] was ok (+100 ms).
```
### Test with version gRPC v1.30.2

The result is the same, with disabled keepalive or increased from 150ms to 10s. 

- default poll
```text
(buster)6e5192c9d5e7 grpc-client-cpp # obj-x86_64-linux-gnu/src/client/grpc-client conf/host3.txt 
Configured for: 
 - [fb-server-worker-1:8442]
 - [fb-server-worker-2:8442]
 - [fb-server-worker-3:8442]
 - [fb-server-worker-4:8442]
 - [fb-server-worker-5:8442]
 - [fb-server-worker-6:8442]
 - [fb-server-worker-7:8442]


E0731 09:43:50.370872238   13745 chttp2_transport.cc:2881]   ipv4:10.244.8.173:8442: Keepalive watchdog fired. Closing transport.
E0731 09:43:50.371600674   13745 chttp2_transport.cc:2881]   ipv4:10.244.8.168:8442: Keepalive watchdog fired. Closing transport.
E0731 09:43:50.372089307   13745 chttp2_transport.cc:2881]   ipv4:10.244.8.126:8442: Keepalive watchdog fired. Closing transport.
E0731 09:43:50.372590439   13745 chttp2_transport.cc:2881]   ipv4:10.244.8.152:8442: Keepalive watchdog fired. Closing transport.
E0731 09:43:50.373062049   13745 chttp2_transport.cc:2881]   ipv4:10.244.8.149:8442: Keepalive watchdog fired. Closing transport.
[fb-server-worker-3:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {31 count, 15.5 req/s, 0 ms per request}>
[fb-server-worker-4:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {31 count, 15.5 req/s, 0 ms per request}>
[fb-server-worker-5:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {31 count, 15.5 req/s, 0 ms per request}>
[fb-server-worker-6:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {31 count, 15.5 req/s, 0 ms per request}>
[fb-server-worker-7:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {31 count, 15.5 req/s, 0 ms per request}>
[ipv4:10.244.8.126:8442]  ok: {2 count, 1 req/s, 13 ms per request} bad: {17 count, 8.5 req/s, 97 ms per request}>
[ipv4:10.244.8.149:8442]  ok: {2 count, 1 req/s, 19 ms per request} bad: {17 count, 8.5 req/s, 97 ms per request}>
[ipv4:10.244.8.152:8442]  ok: {2 count, 1 req/s, 7 ms per request} bad: {17 count, 8.5 req/s, 97 ms per request}>
[ipv4:10.244.8.153:8442]  ok: {50 count, 25 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.160:8442]  ok: {50 count, 25 req/s, 1 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.168:8442]  ok: {2 count, 1 req/s, 10 ms per request} bad: {17 count, 8.5 req/s, 97 ms per request}>
[ipv4:10.244.8.173:8442]  ok: {2 count, 1 req/s, 7 ms per request} bad: {17 count, 8.5 req/s, 97 ms per request}>

[fb-server-worker-3:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {332 count, 166 req/s, 0 ms per request}>
[fb-server-worker-4:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {332 count, 166 req/s, 0 ms per request}>
[fb-server-worker-5:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {332 count, 166 req/s, 0 ms per request}>
[fb-server-worker-6:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {332 count, 166 req/s, 0 ms per request}>
[fb-server-worker-7:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {332 count, 166 req/s, 0 ms per request}>
[ipv4:10.244.8.153:8442]  ok: {332 count, 166 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.160:8442]  ok: {332 count, 166 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>

[fb-server-worker-3:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {163 count, 81.5 req/s, 0 ms per request}>
[fb-server-worker-4:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {163 count, 81.5 req/s, 0 ms per request}>
[fb-server-worker-5:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {163 count, 81.5 req/s, 0 ms per request}>
[fb-server-worker-6:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {163 count, 81.5 req/s, 0 ms per request}>
[fb-server-worker-7:8442]  ok: {0 count, 0 req/s, 0 ms per request} bad: {163 count, 81.5 req/s, 0 ms per request}>
[ipv4:10.244.8.126:8442]  ok: {1 count, 0.5 req/s, 0 ms per request} bad: {9 count, 4.5 req/s, 100 ms per request}>
[ipv4:10.244.8.149:8442]  ok: {1 count, 0.5 req/s, 0 ms per request} bad: {9 count, 4.5 req/s, 100 ms per request}>
[ipv4:10.244.8.152:8442]  ok: {1 count, 0.5 req/s, 1 ms per request} bad: {9 count, 4.5 req/s, 100 ms per request}>
[ipv4:10.244.8.153:8442]  ok: {174 count, 87 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.160:8442]  ok: {174 count, 87 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.168:8442]  ok: {1 count, 0.5 req/s, 0 ms per request} bad: {9 count, 4.5 req/s, 100 ms per request}>
[ipv4:10.244.8.173:8442]  ok: {1 count, 0.5 req/s, 0 ms per request} bad: {9 count, 4.5 req/s, 100 ms per request}>
```

- epoll1
```text
(buster)6e5192c9d5e7 grpc-client-cpp # obj-x86_64-linux-gnu/src/client/grpc-client conf/host3.txt 
Configured for: 
 - [fb-server-worker-1:8442]
 - [fb-server-worker-2:8442]
 - [fb-server-worker-3:8442]
 - [fb-server-worker-4:8442]
 - [fb-server-worker-5:8442]
 - [fb-server-worker-6:8442]
 - [fb-server-worker-7:8442]


[ipv4:10.244.8.126:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.149:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.152:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.153:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.160:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.168:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.173:8442]  ok: {320 count, 160 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>

[ipv4:10.244.8.126:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.149:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.152:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.153:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.160:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.168:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.173:8442]  ok: {317 count, 158.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>

[ipv4:10.244.8.126:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.149:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.152:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.153:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.160:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.168:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
[ipv4:10.244.8.173:8442]  ok: {329 count, 164.5 req/s, 0 ms per request} bad: {0 count, 0 req/s, 0 ms per request}>
```

## How to build

On Debian Buster:

1) install dependencies
- for 1.16.1
```console
# apt install -f libgrpc++-dev=1.16.1-1 libgrpc-dev=1.16.1-1 libgrpc++1=1.16.1-1 protobuf-compiler-grpc=1.16.1-1 libprotobuf-dev=3.6.1.3-2 protobuf-compiler=3.6.1.3-2
```
- for 1.26.0 you cannot simply install dependencies due to bug [21213](https://github.com/grpc/grpc/issues/21213) which will be fixed in 1.26.1. You have to compile from source from branch 1.26.x.
```console
# apt install -f libgrpc++-dev=1.26.0-2~bpo10+1 libgrpc-dev=1.26.0-2~bpo10+1 libgrpc++1=1.26.0-2~bpo10+1 protobuf-compiler-grpc=1.26.0-2~bpo10+1 libprotobuf-dev=3.11.4-3~bpo10+1 protobuf-compiler=3.11.4-3~bpo10+1
```

2) run command
```console
# dpkg-buildpackage 
```

## How to run
Using prebuild docker images.

- server: `docker run -it --rm fboranek/grpc-server:1.1.0`
- client: `docker run -v `pwd`/hosts.conf:/hosts.conf -it --rm fboranek/grpc-server:1.1.0 /opt/grpc/bin/grpc-client /hosts.conf`


## Kubernetes

Deploy the server
```console
kubectl --namespace=<my-namespace> apply -f kubernetes/grpc-server-deployment.yaml
kubectl --namespace=<my-namespace> apply -f kubernetes/grpc-server-service.yaml
```


