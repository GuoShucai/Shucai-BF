#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <vector>
#include <cctype>
#include <string>

using namespace std;
int ignored_number = 0;
// Brainfuck指令到FASM的映射
const struct {
    char bf_char;
    const char* asm_code;
} INSTRUCTION_MAP[] = {
    {'>', "add esi, 1"},         // 指针右移
    {'<', "sub esi, 1"},         // 指针左移
    {'+', "inc byte [esi]"},     // 增加当前内存值
    {'-', "dec byte [esi]"},     // 减少当前内存值
    {'.', "push eax\n"
          "mov al, [esi]\n"
          "mov [lpBuffer], al\n"
          "invoke WriteFile, [hStdOut], lpBuffer, 1, bytesWritten, 0\n"
          "pop eax"},            // 输出当前字符
    {',', "push eax\n"
          "invoke ReadFile, [hStdIn], lpBuffer, 1, bytesRead, 0\n"
          "mov al, [lpBuffer]\n"
          "mov [esi], al\n"
          "pop eax"}             // 输入字符到当前位置
};

// FASM模板
const char* FASM_TEMPLATE = 
"; Brainfuck to FASM编译器\n"
"format PE console\n"
"entry start\n"
"\n"
"include 'win32a.inc'\n"
"\n"
"section '.data' data readable writeable\n"
"    tape        db 30000 dup(0)   ; Brainfuck内存带\n"
"    hStdIn      dd ?             ; 标准输入句柄\n"
"    hStdOut     dd ?             ; 标准输出句柄\n"
"    bytesRead   dd ?             ; 读取字节计数\n"
"    bytesWritten dd ?            ; 写入字节计数\n"
"    lpBuffer    rb 1             ; 单字符缓冲区\n"
"\n"
"section '.code' code readable executable\n"
"start:\n"
"    ; 获取标准输入/输出句柄\n"
"    invoke  GetStdHandle, STD_INPUT_HANDLE\n"
"    mov     [hStdIn], eax\n"
"    invoke  GetStdHandle, STD_OUTPUT_HANDLE\n"
"    mov     [hStdOut], eax\n"
"\n"
"    mov     esi, tape           ; ESI = 当前内存指针\n"
"\n"
"    ; 生成的Brainfuck代码开始\n"
"    ; [BRAINFUCK_CODE_START]\n"
"%s"  // 这里将插入生成的Brainfuck代码
"\n"
"    ; [BRAINFUCK_CODE_END]\n"
"\n"
"    ; 退出程序\n"
"    invoke  ExitProcess, 0\n"
"\n"
"section '.idata' import data readable\n"
"library kernel32, 'kernel32.dll'\n"
"import kernel32,\\\n"
"    GetStdHandle, 'GetStdHandle',\\\n"
"    ReadFile, 'ReadFile',\\\n"
"    WriteFile, 'WriteFile',\\\n"
"    ExitProcess, 'ExitProcess'\n";

// 编译Brainfuck代码为FASM汇编
string compile_brainfuck(const string& bf_code) {
    ostringstream output;
    stack<int> loop_stack;  // 用于处理循环的栈
    int loop_count = 0;     // 循环计数器，用于生成唯一标签
    
    // 遍历Brainfuck代码中的每个字符
    for (char c : bf_code) {
        // 检查是否为有效Brainfuck指令
        bool valid_instruction = false;
        
        // 查找指令映射
        for (const auto& inst : INSTRUCTION_MAP) {
            if (c == inst.bf_char) {
                // 添加注释说明当前指令
                switch (c) {
                    case '.': output << "; . (输出)\n"; break;
                    case ',': output << "; , (输入)\n"; break;
                    case '[': output << "; [ (循环开始)\n"; break;
                    case ']': output << "; ] (循环结束)\n"; break;
                }
                
                // 添加对应的汇编代码
                output << inst.asm_code << "\n";
                valid_instruction = true;
                break;
            }
        }
        
        // 处理循环开始 '['
        if (c == '[') {
            output << "; [ (循环开始)\n";
            output << "loop_" << loop_count << "_start:\n";
            output << "cmp byte [esi], 0\n";
            output << "je loop_" << loop_count << "_end\n";
            
            // 将当前循环计数压入栈
            loop_stack.push(loop_count);
            loop_count++;
            valid_instruction = true;
        }
        
        // 处理循环结束 ']'
        if (c == ']') {
            if (!loop_stack.empty()) {
                int label_num = loop_stack.top();
                loop_stack.pop();
                
                output << "; ] (循环结束)\n";
                output << "jmp loop_" << label_num << "_start\n";
                output << "loop_" << label_num << "_end:\n";
                valid_instruction = true;
            } else {
                cerr << "警告: 未匹配的 ']' 指令\n";
            }
        }
        
        // 忽略非Brainfuck指令字符（如空格、换行等）
        if (!valid_instruction && !isspace(static_cast<unsigned char>(c))) {
            //cerr << "警告: 忽略未知指令 '" << c << "'\n";
            ignored_number = ignored_number + 1;
        }
    }
    
    // 检查未闭合的循环
    if (!loop_stack.empty()) {
        cerr << "错误: " << loop_stack.size() << " 个未闭合的 '[' 指令\n";
    }
    
    return output.str();
}

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc != 3) {
        cerr << "用法: " << argv[0] << " <输入文件.bf> <输出文件>\n";
        cerr << "示例: " << argv[0] << " hello.bf output\n";
        return 1;
    }
    
    const char* input_filename = argv[1];
    const char* output_onlyname = argv[2];
    std::string output_middle = std::string(output_onlyname) + ".asm";
    const char* output_filename = output_middle.c_str();
    
    // 读取Brainfuck源代码
    ifstream input_file(input_filename);
    if (!input_file) {
        cerr << "错误: 无法打开输入文件 " << input_filename << "\n";
        return 1;
    }
    
    // 将整个文件读入字符串
    string bf_code((istreambuf_iterator<char>(input_file)), 
                  istreambuf_iterator<char>());
    input_file.close();
    
    // 编译Brainfuck代码
    string compiled_code = compile_brainfuck(bf_code);
    
    // 生成完整的FASM程序
    char final_asm[40960];  // 足够大的缓冲区
    snprintf(final_asm, sizeof(final_asm), FASM_TEMPLATE, compiled_code.c_str());
    
    // 写入输出文件
    ofstream output_file(output_filename);
    if (!output_file) {
        cerr << "错误: 无法创建输出文件 " << output_filename << "\n";
        return 1;
    }
    
    output_file << final_asm;
    output_file.close();
    
    std::string result = "fasm " + std::string(output_filename);
    const char* rst = result.c_str();
    std::system(rst);
    //调用FASM编译成exe可执行文件

    std::remove(output_filename);
    //删除中间代码

    cout << "编译成功: " << output_onlyname << ".exe" <<"\n";
    cout << "忽略" << ignored_number << "个未知指令" <<"\n";
    //cout << "使用Flat Assembler编译: fasm " << output_filename << "\n";
    
    return 0;
}