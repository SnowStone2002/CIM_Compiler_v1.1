#include "inst_stack.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h> // Add this line to include the declaration for the atoi function

int Verify(const char *inst, int new_addr) {
    if (strstr(inst, "tos") == NULL && strstr(inst, "aos") == NULL) {
        return 1; // True
    } else if (strstr(inst, "paos") == NULL || strstr(inst, "ptos") == NULL) {
        return 1; // True
    } else {
        char *ptr = strstr(inst, "<os_addr_wt>");
        if (ptr != NULL) {
            ptr += strlen("<os_addr_wt>");
            while (*ptr != '\0' && !isdigit(*ptr)) {
                ptr++;
            }
            int os_addr = atoi(ptr);
            if ((os_addr + new_addr) % 2 == 1) {
                return 1; // True
            }
        }
        return 0; // False
    }
}

void InitInstStack(InstStack *stack, int len, const char *filename) {
    stack->len = len;
    memset(stack->inst_fifo, '\n', sizeof(stack->inst_fifo));
    memset(stack->req_queue, 0, sizeof(stack->req_queue));
    memset(stack->addr_queue, 0, sizeof(stack->addr_queue));
    memset(stack->preq_queue, 0, sizeof(stack->preq_queue));
    memset(stack->paddr_queue, 0, sizeof(stack->paddr_queue));
    strncpy(stack->filename, filename, FILENAME_SIZE - 1);
    stack->filename[FILENAME_SIZE-1] = '\0'; // Ensure null-termination
}

void PushInstStack(InstStack *stack, const char *inst, int rd_req, int rd_addr) {
    FILE *file;
    if (rd_req == 1) { // 新的读请求
        for (int i = STACK_LEN - 1; i >= 0; i--) {
            if (stack->req_queue[i] == 0 && Verify(stack->inst_fifo[i], rd_addr)) {
                stack->req_queue[i] = 1;
                stack->addr_queue[i] = rd_addr;
                rd_req = 0; // 读请求已解决
                break;
            } else {
                if (i == 0) { // 读请求未解决
                    printf("@@@@@@@@@@@@@@@@@@@@@!Warning! Unsolved Read Request!!@@@@@@@@@@@@@@@@@@@@@\n");
                }
            }
        }
    }

    // 打开文件追加指令
    file = fopen(stack->filename, "a");
    if (file == NULL) {
        printf("Failed to open file\n");
        return;
    }
    if (stack->inst_fifo[0][0] != '\n') {
        if (stack->preq_queue[0] == 1) {
            fprintf(file, "nop\t <os_addr_rd> %d\n", stack->paddr_queue[0]);
        }
        if (stack->req_queue[0] == 0) {
            fprintf(file, "%s", stack->inst_fifo[0]);
        } else if (stack->req_queue[0] == 1) {
            // 将字符串末尾的换行符替换为地址信息
            fprintf(file, "%.*s\t <os_addr_rd> %d\n", (int)strlen(stack->inst_fifo[0])-1, stack->inst_fifo[0], stack->addr_queue[0]);
        }
    }
    fclose(file);

    // 更新队列
    for (int i = 0; i < stack->len - 1; i++) {
        memmove(stack->inst_fifo[i], stack->inst_fifo[i+1], sizeof(stack->inst_fifo[i]));
        stack->req_queue[i] = stack->req_queue[i+1];
        stack->addr_queue[i] = stack->addr_queue[i+1];
        stack->preq_queue[i] = stack->preq_queue[i+1];
        stack->paddr_queue[i] = stack->paddr_queue[i+1];
    }
    strncpy(stack->inst_fifo[stack->len - 1], inst, sizeof(stack->inst_fifo[stack->len - 1]) - 1);
    stack->inst_fifo[stack->len - 1][sizeof(stack->inst_fifo[stack->len - 1]) - 1] = '\0'; // Ensure null-termination
    stack->req_queue[stack->len - 1] = 0;
    stack->addr_queue[stack->len - 1] = rd_addr;
    stack->preq_queue[stack->len - 1] = rd_req;
    stack->paddr_queue[stack->len - 1] = rd_addr;
}

void CheckInstStack(const InstStack *stack) {
    printf("inst:\n");
    for (int i = 0; i < stack->len; i++) {
        printf("\t%s\n", stack->inst_fifo[i]);
    }
    printf("req_queue:\n\t");
    for (int i = 0; i < stack->len; i++) {
        printf("%d ", stack->req_queue[i]);
    }
    printf("\naddr_queue:\n\t");
    for (int i = 0; i < stack->len; i++) {
        printf("%d ", stack->addr_queue[i]);
    }
    printf("\n");
}
