# Handler

## 项目定位

基于C++11的通用业务框架

支持网络,串口

支持服务端，客户端

支持TCP,UDP,HTTP,WebSocket,MQTT,SSL

支持linux,windows和嵌入式linux

由于要支持资源受限的嵌入式linux,SSL除了openssl之外，也要支持wolfssl等嵌入式ssl开源库



## 项目源由

基于corelooper在项目中的应用，发现有些地方有改进空间，打算重做一版

## 注意事项

这次要注意如下事项

- 不要再采用windows特有的BYTE,DWORD类型，全部采用c++11标准的uint8_t,int32_t之类
- 采用cmake来管理项目,能在cmake中启用/禁用某个功能
- 所有接口都采用unit test
- 









