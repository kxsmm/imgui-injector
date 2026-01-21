
# imgui-injector

[![GitHub Stars](https://img.shields.io/github/stars/kxsmm/imgui-injector?style=social)](https://github.com/kxsmm/imgui-injector/stargazers)
[![GitHub Forks](https://img.shields.io/github/forks/kxsmm/imgui-injector?style=social)](https://github.com/kxsmm/imgui-injector/network/members)
[![GitHub Issues](https://img.shields.io/github/issues/kxsmm/imgui-injector)](https://github.com/kxsmm/imgui-injector/issues)
[![GitHub Last Commit](https://img.shields.io/github/last-commit/kxsmm/imgui-injector)](https://github.com/kxsmm/imgui-injector/commits/main)
[![GitHub Language Count](https://img.shields.io/github/languages/count/kxsmm/imgui-injector)](https://github.com/kxsmm/imgui-injector)
[![GitHub top language](https://img.shields.io/github/languages/top/kxsmm/imgui-injector)](https://github.com/kxsmm/imgui-injector)
[![GitHub License](https://img.shields.io/github/license/kxsmm/imgui-injector)](https://github.com/kxsmm/imgui-injector)

## 项目简介

imgui-injector 是一个用 C 语言编写的 ImGui 动态注入适配多平台，适合用于原生程序的 UI 插件开发、逆向调试等场景。

## 项目特性

- 纯 C 实现，轻量高效
- 支持 ImGui 注入本地程序
- 推荐 NDK 编译，兼容性广
- 适用于动态 UI 插件开发和逆向调试

## 快速开始

1. 获取源码

```bash
git clone https://github.com/kxsmm/imgui-injector.git
```

2. 下载并安装 NDK（推荐已打包好的 Termux NDK 分发包）：
   - [termux-ndk releases v1](https://github.com/jzinferno2/termux-ndk/releases/tag/v1)

3. 按照 NDK 指引进行编译和集成，即可在目标进程中注入 ImGui。

## 贡献

欢迎 Issue 或 PR 参与项目完善！
