#include <stdio.h>
#include <stdlib.h>

#define MEMORY_SIZE 30000

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("�÷�: %s <Brainfuck�����ļ�>\n", argv[0]);
        return 1;
    }

    // ��ʼ���ڴ��ָ��
    char memory[MEMORY_SIZE] = {0};
    char *ptr = memory;

    // ��ȡBrainfuck����
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("�޷����ļ�");
        return 1;
    }

    char code[100000];
    int code_len = fread(code, 1, sizeof(code), file);
    fclose(file);

    // ����ִ�д���
    for (int i = 0; i < code_len; i++) {
        switch (code[i]) {
            case '>': ptr++; break;
            case '<': ptr--; break;
            case '+': (*ptr)++; break;
            case '-': (*ptr)--; break;
            case '.': putchar(*ptr); break;
            case ',': *ptr = getchar(); break;
            case '[':
                if (!*ptr) {
                    int loop = 1;
                    while (loop > 0) {
                        i++;
                        if (code[i] == '[') loop++;
                        else if (code[i] == ']') loop--;
                    }
                }
                break;
            case ']':
                if (*ptr) {
                    int loop = 1;
                        while (loop > 0) {
                        i--;
                        if (code[i] == '[') loop--;
                        else if (code[i] == ']') loop++;
                    }
                }
                break;
        }
    }

    return 0;
}