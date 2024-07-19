#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hw_config.h"
#include "inst_stack.h"
#include "instruction_count.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define WRITE_INST 0
#define VERIFY 0

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

char* operation;
int dim1, dim2, dim3;
char* data_stream;

CIM_Macro   macro;
Micro_Arch  arch;
InstStack   inst_stack;
InstructionCount instructionCount;

int weight_map_channel, weight_map_length;
int input_map_length, input_map_channel;

int weight_block_col, weight_block_row, weight_block_num, weight_update_times_per_inst;
int input_data_per_row, rows_per_input_channel, input_channels_per_ISload, IS_load_times_per_inst;
int para_times, acc_times;
int is_addr_record = 114514; //随便给了个不可能的初值input_map_position
int input_map_position_record = 114514;
int bus_is_width_ratio;
int bus_wu_width_ratio;

int gt_in_map_record;
int os_virtual_depth;

int* IS_load_rows;
int* weight_update_ls;

int** ls_matrix;
int** atos_matrix;

char item[100];

void log_init(void);
void idle(void);
int load_is_narrow_bus(int num_rows, int input_map_position);
int load_is_wide_bus(int num_rows, int input_map_position);
int load_is_block(int num_rows, int input_map_position);
int wu_ls_narrow_bus(int num_ls, int num_channel, int i_block);
int wu_ls_wide_bus(int num_ls, int num_channel, int i_block);
int wu_ls_bank(int num_ls, int num_channel, int i_block);
void compute(int i_input_channel, int computing_block, int fussion_flag);
void is_process(void);
void ws_process(void);
void proj_process(int a, int b, int c);
void ph2_process(void);
void lhd_process(void);
void a2a_process(void);

int main() {

    // 调用初始化函数
    Init_CIM_Macro(&macro, AL, PC, SCR, ICW, WUW, DIRECTION, DATA_TYPE, WEIGHT_WIDTH, WEIGHT_ROW, WEIGHT_COL, INPUT_WIDTH, RESULT_WIDTH);
    Init_Micro_Arch(&arch, &macro, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES);

    // 打印初始化后的值进行验证
    Print_CIM_Macro(&macro);
    Print_Micro_Arch(&arch);

    return 0;
}
