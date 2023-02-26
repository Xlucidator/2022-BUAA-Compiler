import os
import subprocess

pwd = os.getcwd()

target_dir = os.path.join(pwd, '..\\cmake-build-debug')


compile_path = os.path.join(target_dir, 'Compiler.exe')
mips_path    = os.path.join(target_dir, 'mips.txt')

testfile_dst = os.path.join(target_dir, "testfile.txt")
input_dst    = os.path.join(target_dir, "input.txt")
output_dst   = os.path.join(target_dir, "output.txt")

test_dir = os.path.join(pwd, "2021-test-1201")  # 2022-test-1105
test_dir = os.path.join(pwd, "2022-test-1105")
tar_no = 25
tar_level = 'C'
root = os.path.join(test_dir, tar_level)

print(test_dir)
print('level:', tar_level)
print('testfile', tar_no)
print('============================')

# 将测试文件拷贝到cmake-build-debug目录下，便于测试
testfile_src = os.path.join(root, 'testfile' + str(tar_no) + '.txt')
input_src = os.path.join(root, 'input' + str(tar_no) + '.txt')
output_src = os.path.join(root, 'output' + str(tar_no) + '.txt')
os.system('echo f | xcopy /y ' + testfile_src + ' ' + testfile_dst + ' > log.txt')
os.system('echo f | xcopy /y ' + input_src    + ' ' + input_dst    + ' > log.txt')
os.system('echo f | xcopy /y ' + output_src   + ' ' + output_dst   + ' > log.txt')

# 在cmake-build-debug目录下运行Compiler.exe，得到mips.txt
subprocess.run(compile_path, cwd=target_dir, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# 利用mars输入测试input.txt并得到结果
fin = open(input_dst, mode='r')
sp = subprocess.Popen("java -jar Mars4Compiler.jar mips.txt", cwd=target_dir, stdin=fin, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
res = bytes.decode(sp.communicate()[0])
print('[self]')
print(res)
print('-------------------')
res_list = res.replace('\r\n', '\n').split('\n')[2:]
sp.kill()
fin.close()

# 与output.txt对比检验
fcheck = open(output_dst, mode='r')
ans = fcheck.read()
print('[ans]')
print(ans)
print('-------------------')
ans_list = ans.replace('\r\n', '\n').split('\n')
fcheck.close()
line_num = min(len(ans_list), len(res_list))
# if len(ans_list) != len(res_list):
#     print('[warning] line num different! ans-res :', str(len(ans_list)) + '-' + str(len(res_list)), end=' ')
wrong_line = []
flag = True
for lno in range(line_num):
    if res_list[lno] != ans_list[lno]:
        wrong_line.append(lno + 1)
        flag = False
        # break
print('pass' if flag else 'wrong at line ' + str(wrong_line))


