关于常量定义

```c
const int const1 = 1;
const int _my_const = 3*1-1, const2 = -2;
const int _my_const_arr[10/5] = {4,1}, const3_arr[2+3] = {-3, 5, 7};
const int _my_const_arr2[3*2-10/2][5-4%3] = {{4,3,2,1}};
const int const4 = const2;
const int const5_arr[_my_const][const2+4] = {{const3_arr[2],const3_arr[const2+2]},{0,1}};
```

关于变量定义

```c
int var1 = 3;
int var2 = 4-2%1, var3[3]={1,2,3}, var[2][1]={{3%2+0}, {10}};
int var3;
int var4, var5[2], var6[3][5];
int var7[const1] = {const1};
```

关于函数定义

```c
int func_int1() {}
int func_int2(int a1) {}
int func_int3(int a2[], int num) {}
int func_int4(int a3[][3], int a4[], int share_n) {}
void func_void1() {}
int func_void2(int a1) {}
int func_void3(int a2[], int num) {}
int func_void4(int a3[][3], int a4[], int share_n) {}
// 带return和不带return
```

关于表达式

```c
a = +3;
a1[2] = -+-6;
a2[1][1] = !2;
a = 1*2 + -4/3 - +5%+6;
a = a - a1[6/3] + func_int2(a2[1][1]);
a = a + func_int4(a2, a1, 3);

if (a == 3 && b != 4) {
	;  
} else if (c < 3 || c <= 3 && d >5)
    ;
else 
    3;
while (a1[1] >= 4/2) {
    if (!a) break;
    if (b == 3) continue;
    a = a + 3;
}


```





```c
编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef // 1.是否存在Decl 2.是否存在FuncDef
声明 Decl → ConstDecl | VarDecl // 覆盖两种声明
    
常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';' 
基本类型 BType → 'int' // 存在即可
常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal 
常量初值 ConstInitVal → ConstExp| '{' [ ConstInitVal { ',' ConstInitVal } ] '}' 
    
变量声明 VarDecl → BType VarDef { ',' VarDef } ';' // 1.花括号内重复0次 2.花括号内重复多次
变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal 
变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
    
函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block // 1.无形参 2.有形参 
函数类型 FuncType → 'void' | 'int' // 覆盖两种类型的函数
函数形参表 FuncFParams → FuncFParam { ',' FuncFParam } // 1.花括号内重复0次 2.花括号内重复多次
函数形参 FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }] 
    
主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block // 存在main函数
语句块 Block → '{' { BlockItem } '}' // 1.花括号内重复0次 2.花括号内重复多次
语句块项 BlockItem → Decl | Stmt // 覆盖两种语句块项
语句 Stmt → LVal '=' Exp ';' // 每种类型的语句都要覆盖
			| [Exp] ';' //有无Exp两种情况
			| Block
			| 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // 1.有else 2.无else
			| 'while' '(' Cond ')' Stmt
			| 'break' ';' | 'continue' ';'
			| 'return' [Exp] ';' // 1.有Exp 2.无Exp
			| LVal '=' 'getint''('')'';'
			| 'printf''('FormatString{','Exp}')'';' // 1.有Exp 2.无Exp
```

```c
/* Exp消除左递归 */
表达式 Exp → AddExp 注：SysY 表达式是int 型表达式 // 存在即可
条件表达式 Cond → LOrExp // 存在即可
常量表达式 ConstExp → AddExp 注：使用的Ident 必须是常量 // 存在即可
    
数值 Number → IntConst // 存在即可
单目运算符 UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 // 三种均需覆盖
    
左值表达式 LVal → Ident {'[' Exp ']'} //1.普通变量 2.一维数组 3.二维数组
基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number // 三种情况均需覆盖


函数实参表 FuncRParams → Exp { ',' Exp } // 1.花括号内重复0次 2.花括号内重复多次 3.Exp需要覆盖数组传参和部分数组传参

一元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp // 存在即可
    	 // 3种情况均需覆盖,函数调用也需要覆盖FuncRParams的不同情况
乘除模表达式 MulExp → UnaryExp { ε | ('*' | '/' | '%') UnaryExp } 
加减表达式 AddExp → MulExp { ε | ('+' | '−') MulExp } // 1.MulExp 2.+ 需覆盖 3.-需覆盖
关系表达式 RelExp → AddExp { ε | ('<' | '>' | '<=' | '>=') AddExp } 
相等性表达式 EqExp → RelExp { ε | ('==' | '!=') RelExp } // 1.RelExp 2.== 3.!= 均需覆盖
逻辑与表达式 LAndExp → EqExp { ε | '&&' EqExp } // 1.EqExp 2.&& 均需覆盖
逻辑或表达式 LOrExp → LAndExp { ε | '||' LAndExp } // 1.LAndExp 2.|| 均需覆盖

```



```c
为方便对照, 下文给出了文法符号与可能存在的错误的对应关系:
编译单元    CompUnit → {Decl} {FuncDef} MainFuncDef  
声明  	 Decl → ConstDecl | VarDecl
常量声明    ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';' // i
基本类型    BType → 'int'
常数定义    ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal  // b k
常量初值    ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}' 
变量声明    VarDecl → BType VarDef { ',' VarDef } ';' // i
变量定义    VarDef → Ident { '[' ConstExp ']' } // b
    			  | Ident { '[' ConstExp ']' } '=' InitVal // k
变量初值    InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
函数定义    FuncDef → FuncType Ident '(' [FuncFParams] ')' Block // b g j
主函数定义   MainFuncDef → 'int' 'main' '(' ')' Block // b g j
函数类型    FuncType → 'void' | 'int' 
函数形参表   FuncFParams → FuncFParam { ',' FuncFParam } 
函数形参    FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }]  //   b k
语句块     Block → '{' { BlockItem } '}' 
语句块项    BlockItem → Decl | Stmt 
语句  Stmt → LVal '=' Exp ';' | [Exp] ';' | Block // h i
    | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // j
    | 'while' '(' Cond ')' Stmt // j
    | 'break' ';' | 'continue' ';' // i m
    | 'return' [Exp] ';' // f i
    | LVal '=' 'getint''('')'';' // h i j
    | 'printf''('FormatString{,Exp}')'';' // i j l
表达式 	 Exp → AddExp 注：SysY 表达式是int 型表达式 
条件表达式   Cond → LOrExp 
左值表达式   LVal → Ident {'[' Exp ']'} // c k
基本表达式   PrimaryExp → '(' Exp ')' | LVal | Number 
数值  	  Number → IntConst 
一元表达式   UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' // c d e j
        			  | UnaryOp UnaryExp 
单目运算符   UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 
函数实参表   FuncRParams → Exp { ',' Exp } 
乘除模表达式  MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp 
加减表达式   AddExp → MulExp | AddExp ('+' | '−') MulExp 
关系表达式   RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
相等性表达式  EqExp → RelExp | EqExp ('==' | '!=') RelExp
逻辑与表达式  LAndExp → EqExp | LAndExp '&&' EqExp
逻辑或表达式  LOrExp → LAndExp | LOrExp '||' LAndExp 
常量表达式   ConstExp → AddExp 注：使用的Ident 必须是常量
格式字符串:
<FormatString> → '"'{<Char>}'"' // a
```



语法制导翻译

```c
为方便对照, 下文给出了文法符号与可能存在的错误的对应关系:
编译单元    CompUnit → {Decl} {FuncDef} MainFuncDef  
声明  	 Decl → ConstDecl | VarDecl
常量声明    ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';' // i
基本类型    BType → 'int'
常数定义    ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal  // b k
常量初值    ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}' 
变量声明    VarDecl → BType VarDef { ',' VarDef } ';' // i
变量定义    VarDef → Ident { '[' ConstExp ']' } // b
    			  | Ident { '[' ConstExp ']' } '=' InitVal // k
变量初值    InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
函数定义    FuncDef → FuncType Ident '(' [FuncFParams] ')' Block // b g j
主函数定义   MainFuncDef → 'int' 'main' '(' ')' Block // b g j
函数类型    FuncType → 'void' | 'int' 
函数形参表   FuncFParams → FuncFParam { ',' FuncFParam } 
函数形参    FuncFParam → BType Ident ['[' ']' { '[' ConstExp ']' }]  //   b k
语句块     Block → '{' { BlockItem } '}' 
语句块项    BlockItem → Decl | Stmt 
语句  Stmt → LVal '=' Exp ';' | [Exp] ';' | Block // h i
    | 'if' '(' Cond ')' Stmt [ 'else' Stmt ] // j
    | 'while' '(' Cond ')' Stmt // j
    | 'break' ';' | 'continue' ';' // i m
    | 'return' [Exp] ';' // f i
    | LVal '=' 'getint''('')'';' // h i j
    | 'printf''('FormatString{,Exp}')'';' // i j l
表达式 	 Exp → AddExp 注：SysY 表达式是int 型表达式 
条件表达式   Cond → LOrExp 
左值表达式   LVal → Ident {'[' Exp ']'} // c k
基本表达式   PrimaryExp → '(' Exp ')' | LVal | Number 
数值  	  Number → IntConst 
一元表达式   UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' // c d e j
        			  | UnaryOp UnaryExp 
单目运算符   UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中 
函数实参表   FuncRParams → Exp { ',' Exp } 
乘除模表达式  MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp 
加减表达式   AddExp → MulExp | AddExp ('+' | '−') MulExp 
关系表达式   RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
相等性表达式  EqExp → RelExp | EqExp ('==' | '!=') RelExp
逻辑与表达式  LAndExp → EqExp | LAndExp '&&' EqExp
逻辑或表达式  LOrExp → LAndExp | LOrExp '||' LAndExp 
常量表达式   ConstExp → AddExp 注：使用的Ident 必须是常量
格式字符串:
<FormatString> → '"'{<Char>}'"' // a
```

