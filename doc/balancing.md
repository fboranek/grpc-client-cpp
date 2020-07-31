# Useful sources

- [grpc-loadbalancing-kubernetes-examples](https://github.com/jtattermusch/grpc-loadbalancing-kubernetes-examples)
- [xDS for gRPC clients](https://github.com/grpc/proposal/blob/master/A27-xds-global-load-balancing.md#grpc-client-architecture)

Workaround for gRPC's built-in loadbalancing policy when `GRPC_MAX_CONNECTION_AGE` has downside. During the test when
server ends connection the request fails. So when set to max 2 minutes it means also failed request every 2 minute for
each upstream.