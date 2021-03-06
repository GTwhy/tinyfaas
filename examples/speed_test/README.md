# 各种方式运行函数任务耗时
## 实验背景
根据任炬老师的建议，在框架实现前补充进程与轻量级上下文耗时对比实验，初步验证想法。
从已有开源商业框架来看，以容器为单位启动函数的时延在秒到百毫秒水平。
发表于ATC18的论文《SAND...》为每个函数任务fork一个进程运行，启动耗时毫秒级。
下面比较轻量级上下文与进程的耗时对比。
## 实验内容
任务函数是计算输出100-999区间的水仙花数。
实验环境为鹏城实验室提供的鲲鹏920 8核16g ubuntu18.04虚拟机
函数被编译为二进制文件，得到的数据为执行一万次耗时。
## 实验数据
- 函数自循环1w次：2449ms
- fork执行1w次：8660ms
- lutf执行1w次：2579ms
## 数据说明
将fork、lutf的耗时分别减去纯函数运行耗时即可得到两者的耗时。
分别是:
- fork：6441ms
- lutf：130ms

由此可以得到轻量级上下文相较于进程快51倍。
当然，本次实验只是为了对比进程和轻量级上下文模型的耗时。
所得耗时数据不能代表在框架中某函数被调用一万次的实际耗时，框架内部、外部通信耗时有待合入。