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

#### （1）基本约定

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

return值的约定：

#### ConstInitVal相关理解与递归设计（放语法分析）

ConstInitVal 的文法设计很怪，加上语义限制的话，完全可以改成更好的递归表述，改为:

```
ConstInitVal → ConstExp | '{' ConstInitVal { ',' ConstInitVal } '}'
```

所有值均仅来自parseConstInitVal的else部分（）

#### （2）循环/分支语句翻译设计

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

#### （3）break和continue

在parseStmt中设计传入相关while出入口label参数；只传一个出口参数，入口的话就把字符串中end替换成begin。

遇到break，跳到while_end；遇到continue，跳到while_begin

#### （4）Cond逻辑判断与短路逻辑设计

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

#### （5）数组值/地址的抉择

借用错误处理中符号表对数组变量的记录，当函数调用的语法分析发现，并未用全相应数组的所有维数时，将本该生成的中间代码的`IROp::LOAD_ARR`改成`IROp::LOAD_ADDR`，借此改变目标代码生成的方式



### 2. 生成目标代码

生成mips

- 临时变量消除
- 变量名变成唯一标识： 通过加上#layerNo#来区分
- t寄存器也需要保存



- DEF_END 和 DEF_FUN_END 可能还是得区分一下（但好像不影响正确性）



#### 关于地址

发现mips的la可以较好的处理数组地址问题；数组的赋值/取值，和数组地址作为函数参数，似乎可以合并到一起

```assembly
.data
a: .word 1 2 3 4

.text
la $t1 a+3	# 就是a标签的地址加上3
```



#### 加载函数值`IROp::LOAD_ARR`的生成方式

对于中间代码 LOAD_ARR tar a[t] 而言，`a`的地址可能表示为 某个标签 或 某个寄存器+偏移的形式；`t`可能为 数字 或 符号 的形式；因此共有4种情况，对应4种生成方式



### 3. 测试记录

**testfile17**：发现全局变量只load未存而随分析到函数定义时，因重置寄存器失去初始化值

**testfile29**：RET特殊符号也不要加#tableNo标识；计算模除时，使用div会出现数值两操作数皆为$t9存数而冲突覆盖

------

2022-C

**testfile13**：发现parseUnaryExp中`! + number`情况下，`GET_symbol`误写成`OUT_symbol`; 同时发现SET、AND、OR的中间代码忘翻译

**testfile16**：对于`const int con3=con1+con2/39;`，我之前的中间代码会生成临时变量赋给con3；然而在parseConstExp中，我却默认所有已知const都会得到数字，从而必定得到int型。这个问题从语法分析到代码生成连带很大很大（参数传递时都设置成int了，还有代码生成时的`.word`），受到了很大的惊吓；冷静下来发现默认的情况可以实现，于是在符号表中的identItem添加values的值数据存储，在parseConstInitVal中给常量赋值，然后再parseLVal中特判（当为常量时，就返回存储的value值），没想到那么顺利（自己先前写的代码用充足的信息量和较易拓展结构，有点小惊喜），也算是代码优化了。

**testfile18**：忘记给RET中间代码翻译时添加`jr $ra`，（一直靠着genFuncDef中自己加的`jr $ra`，竟一直没查出）；但若如此统一添加，其他函数结尾多余一个`jr $ra`无所谓，**但在MARS中，main函数翻译后本就不该有jr $ra**。修改了RET相关翻译；同时增加了结尾的`$$main$_$end$$:`标签，为main函数中途出现`return 0` 做拓展准备（虽然我觉得未必）

**testfile21**：没有给return；添加中间代码RET的翻译。这样会导致中途return未发生

<u>注意：希望变量名没有“RET”</u>

**testfile23**：全局变量在函数调用前需要存回其相应的$gp指向的内存，因为若靠$sp存，则函数调用过程中使用该全局变量将得不到一致的值，（仅函数调用结束后才能得到）；output疑似有问题，`cnt % Mod == 0`应该永远满足不了，不会有前两句的输出 -> 没问题，原因在下一个testfile发现，全局的cnt值没存回

**testfile4**： 全局变量在函数返回前也同样需要存回其相应的$gp指向的内存；**那么应该好好想想**，离开当前scope都得存回

----

2021-C

**testfile1** ：genGlobalVar的时候，由于中间代码有运算导致和预想的“只有DEF_INIT中间代码”不符合；增加了相应的判断，以防万一；同时**全局变量的初始化必然用常量表达式能算出值**，所以原先的考虑其实没问题，修改了parseExp相关处理，当类似-5时不需要去符号；**四种变量的目标代码生成函数需要注意**

**testfile12**：又又又是全局变量的问题，这次是因为全局跳转前没有将全局变量存回，而重入的代码块有lw全局变量，便覆盖率最新值；醒悟到，**所有跳转之前必须存全局变量，其实类似基本块内DAG公共子表达式化简的注意事项/活跃变量分析（？），要保证代码块的可重入性**

**testfile25**：进一步理解！ **基本块**！！！我们写编译器的思路和代码运行思路不同，我们无法动态的掌握当前regFile的存储情况（因为跳转）；在顺序执行的基本块内我们能局部的存储情况，一旦跳出便不可知。所以在每个基本块结束的时候，进行寄存器状态的清空重置（该存回内存的存回），**保证基本块的可重入**。希望是最后一次。本次揭示问题的地方在于

```c
if () {
    cnt
} else {
    cnt
}
// 其实程序直接跳转进入了else基本块，此时cnt并没有加载到相应的寄存器中；但是编译器在生成代码时，在if块中将cnt加载到寄存器中，跳出此基本块后没有清空，导致在生成else基本块代码时，编译器以为cnt在寄存器中，状态错位
// 以上不是当时错误，但是本质一样
```

----

2022-B

**testfile27**：在useNamefromBaseOff和useNameFromAddr中，对数字的使用可能导致`$t9`的不够用（当初图省事，直接分了t9给数字）；重载了相关函数，数组处理时的上述两个函数，使用$t7装载数字，应该不会重了吧

**testfile30**：搞不懂



## 七. 代码优化设计 

> 编码前的设计、编码完成之后的修改，未选择MIPS代码生成的同学无需完成此项内容

#### 无效运算删去

```
MIN @t4 b#2 9
MUL @t4 @t4 3
ADD @t4 0 @t4
ADD @t4 @t4 0	// 来自某段数组offset计算
```







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