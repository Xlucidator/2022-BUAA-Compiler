# 2022-BUAA-Compiler-Course

> 20级信息类-6系 大三上编译技术课程设计

课设即针对SysY文法编写的玩具编译器（SysY为C的子集），具体见文档（doc目录中）

## 一. 结构

项目本身Clion构建，项目编译结果生成在cmake-build-debug目录中

```
- doc/ 堆了资料和引用图片
	- submit-doc/ 提交的文档作业
- src/ 源文件 （整过内部结构，原先就单目录而已）
	- lexer/  词法分析部分
	- parser/ 语法分析部分，同时包含了错误处理、符号表管理、中间代码的制导翻译
		- errorhandler/ 错误处理相关
		- irbuilder/	中间代码相关
	- generator/ 目标代码生成
	- tool/	  工具函数
- test/ 测试数据+简单测评姬
	- 2021-test-1201/ 2021级测试文件库解压而来
	- 2022-test-1105/ 2022级测试文件库解压而来
	（傻瓜处理，上两者均手动将多行数据变为单行（不知官方测评姬怎么处理的））
	- compiler_test.py
	- single_test.py	win下应该就可运行，linux得调整'xcopy'和'\\'
```



## 二. 说明

- 继承了没写完的传统，没有加入优化如SSA、图着色等。要加的话就是再建一个优化类对中间代码/目标代码进行二次处理；本身其实还有好些顺手的优化可作

- 关于自制测评姬：目前需要事先编译项目，在cmake-build-debug中生成编译器的可执行文件。之后就可直接运行测试了（批量测试`python compiler_tester.py`；单文件测试`python compiler_tester.py`）。具体操作逻辑比较傻瓜：遍历两个测试文件库，依次将其复制到执行目录，然后用编译后对拍





