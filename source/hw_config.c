#include "hw_config.h"
#include <stdio.h>

void Init_CIM_Macro(CIM_Macro *macro, int AL, int PC, int SCR, int ICW, int WUW, int DIRECTION, int DATA_TYPE, int WEIGHT_WIDTH, int WEIGHT_ROW, int WEIGHT_COL) {
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
    if (macro->DIRECTION == 0){
        arch->CIM_WIDTH = macro->WEIGHT_COL * arch->AL;
        arch->CIM_DEPTH = macro->WEIGHT_ROW * macro->SCR * arch->PC;
    }
    else {
        arch->CIM_DEPTH = macro->WEIGHT_COL * arch->AL;
        arch->CIM_WIDTH = macro->WEIGHT_ROW * macro->SCR * arch->PC;
    }
    arch->CIM_SIZE = arch->CIM_WIDTH * arch->CIM_DEPTH;

    // 初始化 Cache
    arch->IS_WIDTH = arch->ICW;
    arch->IS_DEPTH = IS_DEPTH;
    arch->OS_WIDTH = arch->ICW;
    arch->OS_DEPTH = OS_DEPTH;
    arch->IS_SIZE = arch->IS_WIDTH * arch->IS_DEPTH;
    arch->OS_SIZE = arch->OS_WIDTH * arch->OS_DEPTH;

    // 初始化其他参数
    arch->FREQ = FREQ;
    arch->BUS_WIDTH = BUS_WIDTH;
    arch->PIPELINE_STAGES = PIPELINE_STAGES;
}