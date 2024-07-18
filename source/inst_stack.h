#ifndef INST_STACK_H
#define INST_STACK_H

#define STACK_LEN 10
#define FILENAME_SIZE 20

// 定义指令堆栈结构
typedef struct {
    int len;
    char inst_fifo[STACK_LEN][256]; // 假设指令的最大长度为256
    int req_queue[STACK_LEN];
    int addr_queue[STACK_LEN];
    int preq_queue[STACK_LEN];
    int paddr_queue[STACK_LEN];
    char filename[FILENAME_SIZE];
} InstStack;

int Verify(const char *inst, int new_addr);
void InitInstStack(InstStack *stack, int len, const char *filename);
void PushInstStack(InstStack *stack, const char *inst, int rd_req, int rd_addr);
void CheckInstStack(const InstStack *stack);

#endif // INST_STACK_H
