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

