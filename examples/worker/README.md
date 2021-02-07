
coroutine 基于[云风的协程库](https://github.com/cloudwu/coroutine)实现，对其进行了进一步的修改和封装。

实现了非严格的FIFO的协程调度。

```
# 编译与运行
make
./main
```
注册两个协程，分别从0和100进行打印，并打印一次就 yield 一次，编译运行后结果输出：
```
main start
coroutine 0 : 0
coroutine 1 : 100
coroutine 0 : 1
coroutine 1 : 101
coroutine 0 : 2
coroutine 1 : 102
coroutine 0 : 3
coroutine 1 : 103
coroutine 0 : 4
coroutine 1 : 104
no coroutine can be run, finished
```

to do: 加入信号的控制，在信号中断中注册一个协程后继续正常执行。（已经实现）


加入信号中断机制，可正常运行，结果如下。第 6 行和第 9 行两次 ctr+c 信号分别开启了两个协程，id 分别为2、3，各个协程正常执行后退出。

```
$ ./main 
main start
coroutine 0 : 0
coroutine 1 : 100
coroutine 0 : 1
^Ccoroutine 1 : 101
coroutine 2 : 200
^Ccoroutine 3 : 200
coroutine 0 : 2
coroutine 1 : 102
coroutine 2 : 201
...
coroutine 2 : 204
coroutine 3 : 204
no coroutine can be run, finished
```