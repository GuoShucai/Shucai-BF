#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <vector>
#include <cctype>
#include <string>

using namespace std;
int ignored_number = 0;
// Brainfuckָ�FASM��ӳ��
const struct {
    char bf_char;
    const char* asm_code;
} INSTRUCTION_MAP[] = {
    {'>', "add esi, 1"},         // ָ������
    {'<', "sub esi, 1"},         // ָ������
    {'+', "inc byte [esi]"},     // ���ӵ�ǰ�ڴ�ֵ
    {'-', "dec byte [esi]"},     // ���ٵ�ǰ�ڴ�ֵ
    {'.', "push eax\n"
          "mov al, [esi]\n"
          "mov [lpBuffer], al\n"
          "invoke WriteFile, [hStdOut], lpBuffer, 1, bytesWritten, 0\n"
          "pop eax"},            // �����ǰ�ַ�
    {',', "push eax\n"
          "invoke ReadFile, [hStdIn], lpBuffer, 1, bytesRead, 0\n"
          "mov al, [lpBuffer]\n"
          "mov [esi], al\n"
          "pop eax"}             // �����ַ�����ǰλ��
};

// FASMģ��
const char* FASM_TEMPLATE = 
"; Brainfuck to FASM������\n"
"format PE console\n"
"entry start\n"
"\n"
"include 'win32a.inc'\n"
"\n"
"section '.data' data readable writeable\n"
"    tape        db 30000 dup(0)   ; Brainfuck�ڴ��\n"
"    hStdIn      dd ?             ; ��׼������\n"
"    hStdOut     dd ?             ; ��׼������\n"
"    bytesRead   dd ?             ; ��ȡ�ֽڼ���\n"
"    bytesWritten dd ?            ; д���ֽڼ���\n"
"    lpBuffer    rb 1             ; ���ַ�������\n"
"\n"
"section '.code' code readable executable\n"
"start:\n"
"    ; ��ȡ��׼����/������\n"
"    invoke  GetStdHandle, STD_INPUT_HANDLE\n"
"    mov     [hStdIn], eax\n"
"    invoke  GetStdHandle, STD_OUTPUT_HANDLE\n"
"    mov     [hStdOut], eax\n"
"\n"
"    mov     esi, tape           ; ESI = ��ǰ�ڴ�ָ��\n"
"\n"
"    ; ���ɵ�Brainfuck���뿪ʼ\n"
"    ; [BRAINFUCK_CODE_START]\n"
"%s"  // ���ｫ�������ɵ�Brainfuck����
"\n"
"    ; [BRAINFUCK_CODE_END]\n"
"\n"
"    ; �˳�����\n"
"    invoke  ExitProcess, 0\n"
"\n"
"section '.idata' import data readable\n"
"library kernel32, 'kernel32.dll'\n"
"import kernel32,\\\n"
"    GetStdHandle, 'GetStdHandle',\\\n"
"    ReadFile, 'ReadFile',\\\n"
"    WriteFile, 'WriteFile',\\\n"
"    ExitProcess, 'ExitProcess'\n";

// ����Brainfuck����ΪFASM���
string compile_brainfuck(const string& bf_code) {
    ostringstream output;
    stack<int> loop_stack;  // ���ڴ���ѭ����ջ
    int loop_count = 0;     // ѭ������������������Ψһ��ǩ
    
    // ����Brainfuck�����е�ÿ���ַ�
    for (char c : bf_code) {
        // ����Ƿ�Ϊ��ЧBrainfuckָ��
        bool valid_instruction = false;
        
        // ����ָ��ӳ��
        for (const auto& inst : INSTRUCTION_MAP) {
            if (c == inst.bf_char) {
                // ���ע��˵����ǰָ��
                switch (c) {
                    case '.': output << "; . (���)\n"; break;
                    case ',': output << "; , (����)\n"; break;
                    case '[': output << "; [ (ѭ����ʼ)\n"; break;
                    case ']': output << "; ] (ѭ������)\n"; break;
                }
                
                // ��Ӷ�Ӧ�Ļ�����
                output << inst.asm_code << "\n";
                valid_instruction = true;
                break;
            }
        }
        
        // ����ѭ����ʼ '['
        if (c == '[') {
            output << "; [ (ѭ����ʼ)\n";
            output << "loop_" << loop_count << "_start:\n";
            output << "cmp byte [esi], 0\n";
            output << "je loop_" << loop_count << "_end\n";
            
            // ����ǰѭ������ѹ��ջ
            loop_stack.push(loop_count);
            loop_count++;
            valid_instruction = true;
        }
        
        // ����ѭ������ ']'
        if (c == ']') {
            if (!loop_stack.empty()) {
                int label_num = loop_stack.top();
                loop_stack.pop();
                
                output << "; ] (ѭ������)\n";
                output << "jmp loop_" << label_num << "_start\n";
                output << "loop_" << label_num << "_end:\n";
                valid_instruction = true;
            } else {
                cerr << "����: δƥ��� ']' ָ��\n";
            }
        }
        
        // ���Է�Brainfuckָ���ַ�����ո񡢻��еȣ�
        if (!valid_instruction && !isspace(static_cast<unsigned char>(c))) {
            //cerr << "����: ����δָ֪�� '" << c << "'\n";
            ignored_number = ignored_number + 1;
        }
    }
    
    // ���δ�պϵ�ѭ��
    if (!loop_stack.empty()) {
        cerr << "����: " << loop_stack.size() << " ��δ�պϵ� '[' ָ��\n";
    }
    
    return output.str();
}

int main(int argc, char* argv[]) {
    // ��������в���
    if (argc != 3) {
        cerr << "�÷�: " << argv[0] << " <�����ļ�.bf> <����ļ�>\n";
        cerr << "ʾ��: " << argv[0] << " hello.bf output\n";
        return 1;
    }
    
    const char* input_filename = argv[1];
    const char* output_onlyname = argv[2];
    std::string output_middle = std::string(output_onlyname) + ".asm";
    const char* output_filename = output_middle.c_str();
    
    // ��ȡBrainfuckԴ����
    ifstream input_file(input_filename);
    if (!input_file) {
        cerr << "����: �޷��������ļ� " << input_filename << "\n";
        return 1;
    }
    
    // �������ļ������ַ���
    string bf_code((istreambuf_iterator<char>(input_file)), 
                  istreambuf_iterator<char>());
    input_file.close();
    
    // ����Brainfuck����
    string compiled_code = compile_brainfuck(bf_code);
    
    // ����������FASM����
    char final_asm[40960];  // �㹻��Ļ�����
    snprintf(final_asm, sizeof(final_asm), FASM_TEMPLATE, compiled_code.c_str());
    
    // д������ļ�
    ofstream output_file(output_filename);
    if (!output_file) {
        cerr << "����: �޷���������ļ� " << output_filename << "\n";
        return 1;
    }
    
    output_file << final_asm;
    output_file.close();
    
    std::string result = "fasm " + std::string(output_filename);
    const char* rst = result.c_str();
    std::system(rst);
    //����FASM�����exe��ִ���ļ�

    std::remove(output_filename);
    //ɾ���м����

    cout << "����ɹ�: " << output_onlyname << ".exe" <<"\n";
    cout << "����" << ignored_number << "��δָ֪��" <<"\n";
    //cout << "ʹ��Flat Assembler����: fasm " << output_filename << "\n";
    
    return 0;
}