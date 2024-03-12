Stanford CS 144 Networking Lab
==============================

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`

# 实现

- lab0: `apps\webget.cc`、`src\byte_stream.cc`
```
实现字节流中的 Writer class 和 Reader class，即从 buffer 中写入或读取内容
```



- lab1: `src\reassembler.cc`、`src\reassembler.hh`

![](https://file.fbichao.top/2024/03/c63a8aa9ce9f424f91de1f3c0359b4c4.png)

```
对可能出现乱序、重复的字节流进行重排
```

- lab2: `src\tcp_receiver.hh`、`src\tcp_receiver.cc`

![](https://file.fbichao.top/2024/03/057e29abc6889c738aedbb5f9b650ced.png)

```
实现 seqno 和 absolute_seqno 之间的转换

实现 TCPReceiver 的 receive 和 send
- receive: 接受 peer(对等端)发送的信息并通过 reassemble 重整，经过 Writer 写入 stream
- send: 回送 ack 和 window size
```


- lab3: `src\tcp_sender.hh`、`src\tcp_sender.cc`

```
实现发送端的发送数据、超时重传、计时器、流量控制
```

