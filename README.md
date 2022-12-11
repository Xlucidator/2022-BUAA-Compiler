# SysY-MIPS编译器

> BUAA 编译原理与技术课程设计

SysY文法为C文法的真子集

## 一. 参考编译器介绍

> 总结所阅读的编译器的总体结构、接口设计、文件组织等内容

尚未阅读别人的编译器，接口均按课设要求

## 二. 编译器总体设计

> 介绍自己的将要实现的编译器的总体结构、接口设计、文件组织等内容

目前采用传统7模块编译器实现结构，多遍读取翻译。

## 三. 词法分析设计

> 编码前的设计、编码完成之后的修改    

## 四. 语法分析设计

> 编码前的设计、编码完成之后的修改

## 五. 错误处理设计

> 编码前的设计、编码完成之后的修改

## 六. 代码生成设计

> 编码前的设计、编码完成之后的修改

### 1. 生成中间代码

#### 基本约定

> 大概还是L-ATG的思路，命名有种HTTP协议的感觉

关于各parse函数间消息传递的参数名，我们约定：

- OUT_xxx 表示通过函数参数，将数据xxx传出函数
- IN_xxx 表示通过函数参数，从函数外部传入的数据xxx
- GET_xxx 表示在函数内通过调用其他函数获取的数据xxx
- PUT_xxx 表示在函数内向其他函数传递数据xxx
- xxx 函数声明的形参用名，表名数据实体xxx

> 错误处理时对各parse函数的调整（参数和返回值）设计的并不是很好；这也会与代码生成时的调整相互错杂糅合，很难搞，先不管它最后再合并吧

@前缀：中间临时变量

#后缀：标识变量的所在符号表层次

$前缀：标识label

#### 循环/分支语句翻译设计

跳转label的设计：`'$' + ('if' | 'while') + <含义描述词> + '_' + lno + '_' + label_no`；例如第10行出现的第3个while语句（从0开始计数）的开头标签为`$while_begin_10_3`

while语句： while (Cond) Stmt

```assembly
while_begin:
...		# Cond逻辑判断相关计算
beq Cond, 0, while_end:		# 在parseCond种完成
	...
	<Stmt content>
	...
jump while_begin:
while_end:
```

if语句：if (Cond) Stmt [else Stmt]

```assembly
if_begin:
...		# Cond逻辑判断相关计算
beq Cond, 0, if_else:		# 在parseCond中完成
	...
	<if Stmt content>
	...
jump if_end:				# [可选] 若没有else 则这删去
if_else:
	...						# [可选]
	<else Stmt content>		# [可选]
	...						# [可选]
if_end:
```

#### break和continue

在parseStmt中设计传入相关while出入口label参数；只传一个出口参数，入口的话就把字符串中end替换成begin。

遇到break，跳到while_end；遇到continue，跳到while_begin

#### Cond逻辑判断与短路逻辑设计

对于逻辑判断层，从RelExp、EqExp、LAndExp由下到上至LOrExp，下层都给上层反馈**逻辑值标识符**（"0","1", identSymbol；这点在RelExp层接收AddExp时做到）,如此每层仅需根据这些判断其逻辑。而短路逻辑从LAndExp开始，翻译设计如下，这部分中间代码和目标汇编基本类似，混着写了。

##### LAndExp中的短路逻辑

样例： `B && C && D` ； 其中B、C、D为下层返回的逻辑值标识符，可直接使用

```assembly
set @t0, 1				# @t0临时变量，用来存储该层表达式的逻辑值，对and来说初始应为1
...
beq B, 0, and_false:	# 对于每个子表达式，但凡为0，则短路，调至and_false
...
beq C, 0, and_false:
...
and @t0, @t0, D			# 优化：其实和上面两行一样也可，但最后一个表达式正确与否与短路逻辑无关，不妨从跳转指令换成计算指令。
jump and_end:
and_false:
and @t0, @t0, 0
and_end:
```

##### LOrExp中的短路逻辑

样例： `A || F || E` ； 其中A、E、F为下层返回的逻辑值标识符（其中`F`可当作 `A || B && C && D || E` 中 `B && C && D` 的逻辑值标识符），可直接使用

```assembly
set @t0, 0				# @t0临时变量，用来存储该层表达式的逻辑值，对or来说初始应为0
...
bne A, 0, or_true:		# 对于每个子表达式，但凡不为0，则短路，调至or_true
...
bne F, 0, or_true:
...
or @t0, @t0, E			# 优化：其实和上面两行一样也可，但最后一个表达式正确与否与短路逻辑无关，不妨从跳转指令换成计算指令。
jump or_end:
or_false:
or @t0, @t0, 1
or_end:
```

##### 边界情况

- 当只有一个子表达式时，将该子表达式返回给上一层即可；

- 当某行的逻辑值标识符可直接静态判断真假
  - 未导致短路逻辑：相关beq跳转的代码可以不生成
  - 导致了短路逻辑：不优化（则之后的表达式将不再生成中间代码，直接返回分析得来的逻辑值标识符 -->  好像没啥区别）
  - 导致短路逻辑处**在第一行**：不优化（直接返回"0"即可就可以返回了 --> 不行，之后的表达式都得分析，**副作用会照常生成代码**，需要jump跳过）

样例：`C`

```assembly
...
C # 前面的set也不要了
```

样例：`1 && 1 && 1`

```assembly
set @t0, 1
...
(beq 1, 0, and_false:)	# 可以删去
...
(beq 1, 0, and_false:)	# 可以删去
...
(and @t0, @t0, 1)		# 可以删去
jump and_end:
and_false:
and @t0, @t0, 0
and_end:
```



### 2. 生成目标汇编

生成mips

- 临时变量消除
- 变量名变成唯一标识： 通过加上#layerNo#来区分
- t寄存器也需要保存



- DEF_END 和 DEF_FUN_END 可能还是得区分一下（但好像不影响正确性）





### 3. 测试记录

testfile17：发现全局变量只load未存而随分析到函数定义时，因重置寄存器失去初始化值

testfile29：RET特殊符号也不要加#tableNo标识；计算模除时，使用div会出现数值两操作数皆为$t9存数而冲突覆盖

## 七. 代码优化设计 

> 编码前的设计、编码完成之后的修改，未选择MIPS代码生成的同学无需完成此项内容





关于编译器代码本身的优化：

- 看看有没有可以改为emplace_back()的vector容器操作
- 视情况，为每个类补充“拷贝构造函数”和”移动构造函数“
- 将.h的具体函数代码移至.cpp中
- 梳理头文件引用，现在是随便引



除法（尤其是%，纯数（const value）的话可以优化）

关于`!`，连续多个的情况，会出现冗余，可以在中间代码处优化

例子：`!-a+1 || b`





ConstInitVal 的文法设计很怪，加上语义限制的话，完全可以改成更好的递归表述。改为:

```
ConstInitVal → ConstExp | '{' ConstInitVal { ',' ConstInitVal } '}'
```

#### 