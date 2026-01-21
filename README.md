# imgui-injector 项目介绍

项目地址：https://github.com/kxsmm/imgui-injector
![release](https://img.shields.io/github/v/release/kxsmm/imgui-injector) 
## 项目简介

imgui-injector 是一个用 C 语言编写的 ImGui 动态注入工具，可通过 Android NDK 交叉编译适配多平台，适合用于原生程序的 UI 插件开发、逆向调试等场景。

## 项目特性

- 纯 C 实现，轻量高效
- 支持 ImGui 注入各类本地程序
- 推荐 NDK 编译，兼容性好
- 适用于动态 UI 插件开发或逆向调试需求

## 快速开始

1. 获取源码

```bash
git clone https://github.com/kxsmm/imgui-injector.git
```

2. 下载并安装 NDK（推荐已打包好的 Termux NDK 分发包）：

- NDK 下载地址：[termux-ndk releases v1](https://github.com/jzinferno2/termux-ndk/releases/tag/v1)

3. 按照 NDK 指引进行编译和集成，即可在目标进程中注入 ImGui。

