### Concurrent

这是一个简单的demo，用于体验使用框架处理并发请求。

#### 编译

1. 首先确认已经安装了cmake 3.13及以上版本
2. 其次确认已经构建了LUTF主程序，若未构建，运行顶层目录的autu-build.sh可自动构建。
3. 运行如下命令编译demo。
```shell script
cmake .
make
```
其中client模拟并发客户端可执行程序；sleep.lf为向lutf提交的函数任务文件；add_func用于提交函数请求。

#### 运行
为方便测试，可直接使用run.sh脚本体验。

脚本会自动启动lutf，提交函数，并模拟一千个并发请求的客户端。客户端的请求内容为睡眠一个随机的时间。并发请求数可通过更改脚本中的COUNT变量修改。
1. 运行脚本
```shell script
chmod +x ./run.sh
./run.sh
```
2. 观察打印信息