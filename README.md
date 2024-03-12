Stanford CS 144 Networking Lab
==============================

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`

# 实现

- lab0 可靠字节流
- lab1~3 实现 TCP
- lab4 实现 IP 和链路层接口
- lab5 实现 IP 路由

## lab0
> `apps\webget.cc`、`src\byte_stream.cc`
```
实现字节流中的 Writer class 和 Reader class，即从 buffer 中写入或读取内容
```



## lab1
> `src\reassembler.cc`、`src\reassembler.hh`

![](https://file.fbichao.top/2024/03/c63a8aa9ce9f424f91de1f3c0359b4c4.png)

```
对可能出现乱序、重复的字节流进行重排
```

## lab2
> `src\tcp_receiver.hh`、`src\tcp_receiver.cc`

![](https://file.fbichao.top/2024/03/057e29abc6889c738aedbb5f9b650ced.png)

```
实现 seqno 和 absolute_seqno 之间的转换

实现 TCPReceiver 的 receive 和 send
- receive: 接受 peer(对等端)发送的信息并通过 reassemble 重整，经过 Writer 写入 stream
- send: 回送 ack 和 window size
```


## lab3
> `src\tcp_sender.hh`、`src\tcp_sender.cc`

```
实现发送端的发送数据、超时重传、计时器、流量控制
```


## lab4
> `src\network_interface.hh`、`src\network_interface.cc`

```
有一个从 IP 地址到以太网地址的映射缓存

实现将数据装帧发出到下一跳，如果缓存中找不到下一跳，就发送 ARP 请求，等收到 ARP 响应，解析后得到下一跳，传输帧

接受网络中到来的其它帧（以广播地址或该链路层地址为目的地址）
① 如果是 IPv4（非 ARP 类型），装帧传输；
如果是 ARP 类型的帧，就需要缓存，
② 如果是 ARP 请求，并且以本地 IP 地址为目的地，需要装帧回送 ARP 响应；
③ 如果是 ARP 响应，则传输之前要发送到该目的地的帧
```


## lab5
> `src\router.hh`、`src\router.cc`

```
实现最长前缀匹配查找路由表
```

