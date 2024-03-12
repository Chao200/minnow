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
实现 Reassembler class，即接收从网络中来的数据，并重新排序
通过 Writer 写入到 buffer 中，buffer 的容量是 capacity（`lab0 src\byte_stream.cc`）

1. data 在 capcity 之外，包括前面和后面，直接舍弃，节省内存，提高利用率
2. 此时 data 必有一部分或全部在 capacity 之内，要么头部或尾部或者都超过了 capacity，进行截取
3. 截取得到数据后需要和之前存在还没 push 的数据（等待前面数据的到来才 push）拼接
4. 之前的数据存放在一个数据结构 buffer_（list 实现） 中，将新数据和旧数据一一比对后按照 data 的 first_index 排序
5. 如果 data 的 first_index 与 capacity 的第一个索引（min_index）一样，则 push
6. 比对过程可能会出现 data 在旧数据的前面或者后面，还有重叠，需要截取
```

- data 在 capacity 之外
![](https://file.fbichao.top/2024/03/8b0e69763a9405ea92db38574c933e9e.png)

- data 在 capacity 之内有内容，但是边界可能会超
![](https://file.fbichao.top/2024/03/576de62f0bd37d51a79ab4f340f08179.png)

- data 有一部分位于 seg，但是可能两侧有越界
  - data 完全覆盖 seg 不需要操作，data 不变，只剩下两种情况
  - data 左侧越界，但是不可以用 seg.first_ > first_index 作为判断，因为完全覆盖也满足该条件，所以应该使用 seg.last_ > last_index，此时要更新 last_index = seg.last_; is_last_substring = seg.eof_;
  - 同理，右侧越界，seg.first_ < first_index，此时要更新 first_index = seg.first_;
![](https://file.fbichao.top/2024/03/f7b79c713971c0a5597bb8bebd975dc5.png)


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

