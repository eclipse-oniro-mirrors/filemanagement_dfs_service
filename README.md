# 分布式文件管理

[TOC]

## 简介

分布式文件管理提供跨设备的、符合POSIX规范的文件访问能力。其在分布式软总线动态组网的基础上，为网络上各个设备结点提供一个统一的、逻辑的、树形的文件系统层次结构。

分布式文件管理的架构如下图所示：

![架构图](https://images.gitee.com/uploads/images/2021/1102/094937_01125861_9249473.png "io.drawio.png")

其包括如下几个核心模块：

- distributedfiledaemon：分布式文件管理常驻用户态服务，负责接入设备组网、数据传输能力，并负责挂载hmdfs
- distributedfileservice：分布式文件管理服务进程，对应用提供分布式扩展能力
- hmdfs(Harmony Distributed File System)：分布式文件管理核心模块，是一种面向移动分布式场景的、高性能的、基于内核实现的、堆叠式文件系统。

## 目录

```raw
//foundation/storage/distributed_file_manager
├── frameworks                  // 接口实现
│   └── native                      // c/c++ 接口实现
├── interfaces                  // 接口声明
│   └── innerkits                   // 对内接口声明
├── services                    // 服务实现
│   └── distributedfiledaemon       // 常驻服务实现
|       └── include
|       └── src
            └── device            // 设备上下线管理
            └── ipc               // daemon进程拉起退出流程以及ipc接口实现
            └── mountpoint        // hmdfs挂载管理
            └── network           // 软总线和内核会话session交互相关
|       └── test
|       └── BUILD.gn
|   └── distributedfileservice     // 三方应用调用流程服务实现
└── utils                       // 公共组件
    ├── log                         // 日志组件
    └── system                      // 平台相关组件
```

## 约束

### 接口支持情况

分布式文件管理当前不支持或有限支持如下 VFS 系统调用：

- symlink：不支持
- mmap：仅支持读
- rename：仅支持同目录操作

### 目录项规格

- 最大目录层级

    与被堆叠文件系统，即data分区所用文件系统，如 ext4，f2fs等，保持一致
- 最大文件名长度
    取决于680字节与被堆叠文件支持长度的最小值。f2fs和ext4均为255字节。
- 单文件最大大小
    取决于2^64字节与被堆叠文件系统的最小值。ext4单文件最大为16TB, f2fs单文件最大为3.94T。
- 单目录下最大目录项数
    取决于被堆叠文件系统单文件大小。当堆叠f2fs时，假设平均目录长度放大系数为3，则为`3.94TB/4KB*85/3=29,966,344,738`个。

## 说明

### 使用说明

可以使用终端调试分布式文件管理能力。

在多台设备已经组网（同一局域网）的情况下, 并且在经过hichian模块可信认证的设备间，可以建立设备间分布式文件互相访问的能力。
可以在应用内通过```Context.distributedFileDir()```接口获取本应用沙箱内的分布式路径，然后在此目录下可以进行创建、删除、读写文件和目录。
在此目录下有```device_view```和```merge_view```两个目录。

device_view 的含义是分设备视图，其下的 local 对应被堆叠的本地文件系统，`79xxx0c8d` 对应相应设备上的本地文件系统。如果在 local 中进行文件系统操作，其影响也将体现在 hmdfs 的源目录，即 `/data/misc_ce/0/hmdfs/storage` 中。如果在 `79xxx0c8d` 中进行文件系统操作，其影响也将体现在对应设备的 hmdfs 的源目录中。

merge_view 的含义是融合视图，它由分设备视图中各目录融合而成。在其中进行文件系统操作，将影响所有设备 hmdfs 的源目录。特别地，当将会产生新文件/目录时，总会产生在本地 hmdfs 的源目录中。

## 相关

- [分布式软总线-SoftBus](https://gitee.com/openharmony/communication_dsoftbus)
- [分布式硬件-设备管理](https://gitee.com/openharmony/device_manager)
- [用户程序框架](https://gitee.com/openharmony/appexecfwk_standard)
