# SkyNE Key Remapper
&emsp;国服PC光遇无法改键位的临时解决方案。

## 使用方式
&emsp;**本软件涉及到DLL注入与Inline Hook技术，可能引起游戏运行不稳定、掉帧等情况，请谨慎使用。**

&emsp;下载[release/latest](https://github.com/HTMonkeyG/SkyNEKeyRemapper/releases/latest)中最新版本dll，并使用任一dll注入工具将其注入进光遇进程内。
也可下载压缩包（不是source code）并解压后**管理员权限运行**``start.bat``。

## 配置文件格式
&emsp;下为示例配置文件：
```
# 配置文件示例
#
# 冒号前十六进制数表示要被映射的按键的扫描码，
# 冒号后十六进制数表示要把按键映射到哪个按键。
#
# 也就是说，当你按下前者对应的按键时，游戏将
# 接收到后者对应按键被按下。
#
# 该按键映射对于游戏内全局有效，请谨慎使用。

# Shift映射至Tab
0x002A:0x000F

# Tab映射至Z
0x000F:0x002C

# T映射至Shift
0x0014:0x002A
```
&emsp;在DLL同目录下新建``skykey-config.txt``（注意后缀名），并将上述内容复制进文件内保存。

&emsp;配置文件中``#``开头的行为注释，不会被处理。若多行的被映射按键相同，仅最靠后的行生效。
扫描码十六进制可在[MSDN](https://learn.microsoft.com/zh-cn/windows/win32/inputdev/about-keyboard-input#scan-codes)中表内``Scan 1 Make``列查询到。
扫描码必须要保留十六进制前缀``0x``，x必须为小写；冒号前后、行首行末均不可出现空格。若违反格式约定则该行配置失效。

## 编译方式
&emsp;本项目使用MinGW-x64编译。将本仓库clone至本地，切换进仓库根目录并执行mingw32-make即可。
