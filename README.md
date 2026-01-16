An RDMA experiment.

#### 脚本运行

##### 参数调整

`test/2_cli.cpp`和`test/2_srv.cpp`里面开头的一些宏是参数，可以调整

`scripts/list_machines.txt`是实验机器的列表，脚本会读取

`scripts/test.py`是脚本，开头也有一些参数，可以调整，尤其是`path_*`参数们需要调整

暂时想到这些参数

##### 运行指令

```
// 每次新开的机器运行一次这个，配环境等等
// 为了让之后部署的时候远端机器们之间能scp传文件，它们之间需要一对密钥，我是提前在本地生成一对，然后在这一步里面传过去
python scripts/test.py init

// 编译，会把输出读到output/compile.log，每次运行完检查一下编译成功了吗
python scripts/test.py compile

// 部署 + 运行，需要先编译好，再运行这一步
python scripts/test.py deploy; python scripts/test.py run

// 收集结果到output/out_*.log
python scripts/test.py collect

// 中止实验，如果收集时发现不对劲，可以及时中止
python scripts/test.py stop

// perftest测试，结果存在output/perftest_*.log
python scripts/test.py perftest
```