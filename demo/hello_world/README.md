### Hello World!

这是一个简单的demo，以hello world程序作为任务，演示了框架的使用流程。

#### 编译

1. 首先确认已经安装了cmake 3.13及以上版本
2. 其次确认已经构建了LUTF主程序，若未构建，运行顶层目录的autu-build.sh可自动构建。
3. 运行如下命令编译demo。
```shell script
mkdir build
cd build
cmake ..
make
```
其中hello_client为演示所用的客户端可执行程序，hello_world.lf为向lutf提交的hello_world函数任务文件。

#### 运行

1. 启动lutf主程序（server端）
```shell script
cd ../../../bin
chmod + ./*
./start_worker.sh
```
2. 在新窗口中运行demo，即client端(或将lutf运行在后台)。
```shell script
cd demo/hello_world/build
./hello_client
```
3. 根据提示信息体验整个工作流程。
4. 体验结束后，停止lutf。
```shell script
../../../bin/stop_worker.sh
```