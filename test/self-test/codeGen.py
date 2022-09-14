from distutils.ccompiler import gen_lib_options
from mimetypes import init
import random

BType = 'int'
IntConst = [0, 9, 10, 53, 99]
Mul_pool = ['*', '/', '%']
Add_pool = ['+', '-']
Uar_pool = ['+', '-', '!']
Rel_pool = ['<', '>', '<=', '>=']
Eql_pool = ['==', '!=']

icnt = 0
Int_pool = ['-10', '+5', '0', '100']
Ident_pool = []

p_same_line = 0.3
p_dim_up = 0.2
p_has_ivalue = 0.2
p_more_param = 0.3

# self 
def gen_constnum_serial(num):
    init_numserial = '0'
    for i in range(1, num):
        init_numserial += ', ' + str(i)
    return init_numserial

def gen_varnum_serial(num): # TODO
    return gen_constnum_serial(num)

def gen_Ident():
    global icnt
    icnt += 1
    return 'indent' + str(icnt)

############################ 常量 ###############################
# 常量声明
def gen_ConstDecl():
    init_ConstDecl = 'const' + ' ' + BType + ' ' + gen_ConstDef() + ' '
    while random.random() < p_same_line:
        init_ConstDecl += ', ' + gen_ConstDef() + ' '
    init_ConstDecl += ';'
    return init_ConstDecl

# 常数定义
def gen_ConstDef():
    init_ConstDef = gen_Ident()
    d_cnt = 0
    d_exp = []
    while d_cnt < 2 and random.random() < p_dim_up:
        d_exp.append(gen_ConstExp())
        init_ConstDef +=  '[' + d_exp[d_cnt] + ']'
        d_cnt += 1
    init_ConstDef += ' = ' + gen_ConstInitVal(d_cnt, d_exp)
    return init_ConstDef

# 常量初值
def gen_ConstInitVal(dim, l_exp = []):
    init_ConstInitVal = ''
    if dim == 0:
        init_ConstInitVal += gen_ConstExp()
    elif dim == 1:
        init_ConstInitVal += '{' + gen_constnum_serial(eval(l_exp[0])) + '}'
    else:
        init_ConstInitVal += '{'
        for i in range(eval(l_exp[0])):
            init_ConstInitVal += '{' + gen_constnum_serial(eval(l_exp[1])) + '},'
        init_ConstInitVal[-1] = '}'
    return init_ConstInitVal

# 常量表达式（需要变种，使用的Ident必须是常量）
def gen_ConstExp():
    return gen_AddExp()

############################ 变量 ###############################
# 变量声明
def gen_VarDecl():
    init_VarDecl = BType + ' ' + gen_VarDef() + ' '
    while random.random() < p_same_line:
        init_VarDecl += ', ' + gen_VarDef() + ' '
    init_VarDecl += ';'
    return init_VarDecl

# 变量定义
def gen_VarDef():
    init_VarDef = gen_Ident()
    d_cnt = 0
    d_exp = []
    while d_cnt < 2 and random.random() < p_dim_up:
        d_exp.append(gen_ConstExp())
        init_VarDef +=  '[' + d_exp[d_cnt] + ']'
        d_cnt += 1
    if random.random() < p_has_ivalue:
        init_VarDef += ' = ' + gen_InitVal(d_cnt, d_exp)
    return init_VarDef

# 变量初值
def gen_InitVal(dim, l_exp = []):
    init_InitVal = ''
    if dim == 0:
        init_InitVal += gen_Exp()
    elif dim == 1:
        init_ConstInitVal += '{' + gen_varnum_serial(eval(l_exp[0])) + '}'
    else:
        init_ConstInitVal += '{'
        for i in range(eval(l_exp[0])):
            init_ConstInitVal += '{' + gen_varnum_serial(eval(l_exp[1])) + '},'
        init_ConstInitVal[-1] = '}'
    return init_ConstInitVal

############################ 函数 ###############################
def gen_FuncType():
    return 'void' if random.random() < 0.5 else 'int'

# 函数定义
def gen_FuncDef():
    init_FuncDef = gen_FuncType() + ' ' + gen_Ident() + ' ' + '(' + gen_FuncFParams() + ')'
    init_FuncDef += ' ' + gen_Block()
    return init_FuncDef

# 主函数定义
def gen_MainFuncDef():
    return 'int main()' + gen_Block()

# 函数形参表
def gen_FuncFParams():
    init_FuncFParams = gen_FuncFParam()
    pnum = 0
    while pnum < 2 and random.random() < p_more_param:
        init_FuncFParams += ', ' + gen_FuncFParam()
    return init_FuncFParams

# 函数形参
def gen_FuncFParam():
    init_FuncFParam = BType + ' ' + gen_Ident()
    d_cnt = 0
    d_exp = []
    while d_cnt < 2 and random.random() < p_dim_up:
        if (d_cnt == 0):
            init_FuncFParam += '[]'
        else:
            init_FuncFParam += '[' + gen_ConstExp() + ']'

############################ 表达式 ###############################
# 表达式（这不是和常量表达式一样了嘛）
def gen_Exp():
    return gen_AddExp()

# 条件表达式
def gen_Cond():
    return gen_LOrExp()

# 左值表达式（普通变量；一维数组；二维数组）
def gen_LVal(): # TODO 需要记录，变量池；先直接乱生成了
    init_LVal = gen_Ident()
    d_cnt = 0
    d_exp = []
    while d_cnt < 2 and random.random() < p_dim_up:
        d_exp.append(gen_ConstExp())
        init_LVal +=  '[' + d_exp[d_cnt] + ']'
        d_cnt += 1
    return init_LVal

# 基本表达式
p_primary_select = [0.33, 0.66]
def gen_PrimaryExp():
    init_PrimaryExp = ''
    slt = random.random()
    if slt < p_primary_select[0]:
        init_PrimaryExp += '(' + gen_Exp() + ')'
    elif slt < p_primary_select[1]:
        init_PrimaryExp += gen_LVal()
    else:
        init_PrimaryExp += gen_Number()

# 数值（缩水了，就搞了几个代表性的数值放在池子里随机选择）
def gen_Number():
    return random.choice(IntConst)

