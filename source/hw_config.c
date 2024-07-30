#include "hw_config.h"
#include <stdio.h>
#include <math.h>

void Init_CIM_Macro(CIM_Macro *macro, int AL, int PC, int SCR, int ICW, int WUW, int WEIGHT_COL, int INPUT_WIDTH, int RESULT_WIDTH, int DIRECTION, int DATA_TYPE, int WEIGHT_WIDTH, int WEIGHT_ROW) {
    macro->AL = AL;
    macro->PC = PC;
    macro->SCR = SCR;
    macro->ICW = ICW;
    macro->WUW = WUW;
    macro->DIRECTION = DIRECTION;
    macro->DATA_TYPE = DATA_TYPE;
    macro->WEIGHT_WIDTH = WEIGHT_WIDTH;
    macro->WEIGHT_ROW = WEIGHT_ROW;
    macro->WEIGHT_COL = WEIGHT_COL;
    macro->INPUT_WIDTH = INPUT_WIDTH;
    macro->RESULT_WIDTH = RESULT_WIDTH;
}

void Init_Micro_Arch(Micro_Arch *arch, CIM_Macro *macro, int MACRO_ROW, int MACRO_COL, int IS_DEPTH, int OS_DEPTH, int FREQ, int BUS_WIDTH, int PIPELINE_STAGES) {
    arch->MACRO_ROW = MACRO_ROW;
    arch->MACRO_COL = MACRO_COL;
    arch->AL = MACRO_COL * macro->AL;
    arch->PC = MACRO_ROW * macro->PC;
    arch->SCR = macro->SCR;
    arch->ICW = MACRO_COL * macro->ICW;
    arch->WUW = MACRO_COL * macro->WUW;

    arch->DIRECTION = macro->DIRECTION;
    arch->DATA_TYPE = macro->DATA_TYPE;
    arch->WEIGHT_WIDTH = macro->WEIGHT_WIDTH;
    arch->WEIGHT_ROW = macro->WEIGHT_ROW;
    arch->WEIGHT_COL = macro->WEIGHT_COL;
    arch->INPUT_WIDTH = macro->INPUT_WIDTH;
    arch->RESULT_WIDTH = macro->RESULT_WIDTH;
    if (macro->DIRECTION == 0){
        // arch->CIM_WIDTH = macro->WEIGHT_COL * arch->AL;
        arch->CIM_DEPTH = macro->WEIGHT_ROW * macro->SCR * arch->PC;
    }
    else {
        // arch->CIM_DEPTH = macro->WEIGHT_COL * arch->AL * macro->SCR;
        arch->CIM_WIDTH = macro->WEIGHT_ROW * arch->PC;
    }
    arch->CIM_WIDTH = arch->WUW;
    arch->CIM_SIZE = arch->CIM_WIDTH * arch->CIM_DEPTH;

    // 初始化 Cache
    arch->IS_WIDTH = arch->ICW;
    arch->IS_DEPTH = IS_DEPTH;
    arch->OS_WIDTH = (macro->RESULT_WIDTH+(int)(log2(arch->MACRO_COL)))*arch->PC;
    arch->OS_DEPTH = OS_DEPTH;
    arch->IS_SIZE = arch->IS_WIDTH * arch->IS_DEPTH;
    arch->OS_SIZE = arch->OS_WIDTH * arch->OS_DEPTH;

    // 初始化其他参数
    arch->FREQ = FREQ;
    arch->BUS_WIDTH = BUS_WIDTH;
    arch->PIPELINE_STAGES = PIPELINE_STAGES;
}

void Print_CIM_Macro(const CIM_Macro *macro) {
    printf("CIM_Macro:\n");
    printf("AL: %d\n", macro->AL);
    printf("PC: %d\n", macro->PC);
    printf("SCR: %d\n", macro->SCR);
    printf("ICW: %d\n", macro->ICW);
    printf("WUW: %d\n", macro->WUW);
    printf("WEIGHT_COL: %d\n", macro->WEIGHT_COL);
    printf("INPUT_WIDTH: %d\n", macro->INPUT_WIDTH);
    printf("RESULT_WIDTH: %d\n", macro->RESULT_WIDTH);

    printf("DIRECTION: %d\n", macro->DIRECTION);
    printf("DATA_TYPE: %d\n", macro->DATA_TYPE);
    printf("WEIGHT_WIDTH: %d\n", macro->WEIGHT_WIDTH);
    printf("WEIGHT_ROW: %d\n", macro->WEIGHT_ROW);
    printf("\n");
}

void Print_Micro_Arch(const Micro_Arch *arch) {
    printf("Micro_Arch:\n");
    printf("MACRO_ROW: %d\n", arch->MACRO_ROW);
    printf("MACRO_COL: %d\n", arch->MACRO_COL);
    printf("AL: %d\n", arch->AL);
    printf("PC: %d\n", arch->PC);
    printf("SCR: %d\n", arch->SCR);
    printf("ICW: %d\n", arch->ICW);
    printf("WUW: %d\n", arch->WUW);
    printf("DIRECTION: %d\n", arch->DIRECTION);
    printf("DATA_TYPE: %d\n", arch->DATA_TYPE);
    printf("WEIGHT_WIDTH: %d\n", arch->WEIGHT_WIDTH);
    printf("WEIGHT_ROW: %d\n", arch->WEIGHT_ROW);
    printf("WEIGHT_COL: %d\n", arch->WEIGHT_COL);
    printf("INPUT_WIDTH: %d\n", arch->INPUT_WIDTH);
    printf("RESULT_WIDTH: %d\n", arch->RESULT_WIDTH);
    printf("CIM_WIDTH: %d\n", arch->CIM_WIDTH);
    printf("CIM_DEPTH: %d\n", arch->CIM_DEPTH);
    printf("CIM_SIZE: %d\n", arch->CIM_SIZE);
    printf("IS_WIDTH: %d\n", arch->IS_WIDTH);
    printf("IS_DEPTH: %d\n", arch->IS_DEPTH);
    printf("OS_WIDTH: %d\n", arch->OS_WIDTH);
    printf("OS_DEPTH: %d\n", arch->OS_DEPTH);
    printf("IS_SIZE: %d\n", arch->IS_SIZE);
    printf("OS_SIZE: %d\n", arch->OS_SIZE);
    printf("FREQ: %d\n", arch->FREQ);
    printf("BUS_WIDTH: %d\n", arch->BUS_WIDTH);
    printf("PIPELINE_STAGES: %d\n", arch->PIPELINE_STAGES);
    printf("\n");
}
