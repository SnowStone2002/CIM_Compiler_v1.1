#include "hw_config.h"
#include <stdio.h>

int main() {
    CIM_Macro macro;
    Micro_Arch arch;

    // 参数输入
    int AL = 64;
    int PC = 32;
    int SCR = 8;
    int ICW = 128;
    int WUW = 128;
    int DIRECTION = 0;
    int DATA_TYPE = 1; // Assuming INT8 is represented by 1
    int WEIGHT_WIDTH = 8;
    int WEIGHT_ROW = 1;
    int WEIGHT_COL = 8;
    int INPUT_WIDTH = 16;
    int RESULT_WIDTH = 32;

    int MACRO_ROW = 2;
    int MACRO_COL = 2;
    int IS_DEPTH = 1024;
    int OS_DEPTH = 1024;
    int FREQ = 500;
    int BUS_WIDTH = 128;
    int PIPELINE_STAGES = 8;

    // 调用初始化函数
    Init_CIM_Macro(&macro, AL, PC, SCR, ICW, WUW, DIRECTION, DATA_TYPE, WEIGHT_WIDTH, WEIGHT_ROW, WEIGHT_COL, INPUT_WIDTH, RESULT_WIDTH);
    Init_Micro_Arch(&arch, &macro, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES);

    // 打印初始化后的值进行验证
    Print_CIM_Macro(&macro);
    Print_Micro_Arch(&arch);

    return 0;
}
