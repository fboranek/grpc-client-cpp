# gRPC client

Server and Client to demonstrate `epollex` problem. When asynchronously call 7 servers on one completion queue.
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
fb-server-slave-1:8442
fb-server-slave-2:8442
fb-server-slave-3:8442
fb-server-slave-4:8442
fb-server-slave-5:8442
fb-server-slave-6:8442
fb-server-slave-7:8442
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

## Kubernetes

Deploy the server
```console
kubectl --namespace=<my-namespace> apply -f kubernetes/grpc-server-deployment.yaml
kubectl --namespace=<my-namespace> apply -f kubernetes/grpc-server-service.yaml
```


