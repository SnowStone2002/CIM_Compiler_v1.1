#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hw_config.h"
#include "instruction_count.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

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
void load_is_narrow_bus(int num_rows);
void load_is_wide_bus(int num_rows);
void load_is_block(int num_rows);
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

int main(int argc, char *argv[]) {
    if (argc != 24) {
        printf("error!");
        return 1;
    }

    AL = atoi(argv[1]);
    PC = atoi(argv[2]);
    SCR = atoi(argv[3]);
    ICW = atoi(argv[4]);
    WUW = atoi(argv[5]);

    MACRO_ROW = atoi(argv[6]);
    MACRO_COL = atoi(argv[7]);
    IS_DEPTH = atoi(argv[8]);
    OS_DEPTH = atoi(argv[9]);
    FREQ = atoi(argv[10]);
    BUS_WIDTH = atoi(argv[11]);
    PIPELINE_STAGES = atoi(argv[12]);

    DIRECTION = atoi(argv[13]);
    DATA_TYPE = atoi(argv[14]);
    WEIGHT_WIDTH = atoi(argv[15]);
    WEIGHT_ROW = atoi(argv[16]);
    WEIGHT_COL = atoi(argv[17]);
    INPUT_WIDTH = atoi(argv[18]);
    RESULT_WIDTH = atoi(argv[19]);

    operation = argv[20];
    dim1 = atoi(argv[21]);
    dim2 = atoi(argv[22]);
    dim3 = atoi(argv[23]);
    data_stream = argv[24];


    // 调用初始化函数
    Init_CIM_Macro(&macro, AL, PC, SCR, ICW, WUW, DIRECTION, DATA_TYPE, WEIGHT_WIDTH, WEIGHT_ROW, WEIGHT_COL, INPUT_WIDTH, RESULT_WIDTH);
    Init_Micro_Arch(&arch, &macro, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES);

    // // 打印初始化后的值进行验证
    // Print_CIM_Macro(&macro);
    // Print_Micro_Arch(&arch);

    log_init();

    if (strcmp(operation, "proj") == 0)
        proj_process(dim1,dim2,dim3);
    else if (strcmp(operation, "a2a") == 0)
            a2a_process();

    // #region output
    //***************************************** terminal output ****************************************
    printInstructionCount(&instructionCount);

    // printf("%s", data_stream);

    //******************************************* csv output *******************************************
    // 检查文件是否存在
    FILE *file = fopen("count.csv", "r");
    int needHeader = 0;
    if (file == NULL) {
        needHeader = 1; // 文件不存在，需要写入表头
    } else {
        fclose(file); // 文件已存在，关闭文件
    }

    // 以追加模式打开文件，如果不存在则创建
    file = fopen("count.csv", "a");
    if (file == NULL) {
        perror("Failed to open csv file\n");
        return EXIT_FAILURE;
    }

    // 如果需要，写入表头
    if (needHeader) {
        fprintf(file, "al,pc,scr,icw,wuw,macro_row,macro_col,is_depth,os_depth,freq,bus_width,pipeline_stages,operation,dim1,dim2,dim3,data_stream,\n");
        fprintf(file, "Lin,Linp,Lwt,Lwtp,Cmpfis_aor,Cmpfis_tos,Cmpfis_aos,Cmpfis_ptos,Cmpfis_paos,");
        fprintf(file, "Cmpfgt_aor,Cmpfgt_tos,Cmpfgt_aos,Cmpfgt_ptos,Cmpfgt_paos,Cmpfgtp,Lpenalty,Nop,Nop_w_rd,");
        fprintf(file, "IS_reward,L2_reward,Fussion\n");
    }

    // 写入命令行参数到文件
    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%s\n",
            AL, PC, SCR, ICW, WUW, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES,
            operation, dim1, dim2, dim3, data_stream);

    // 写入InstructionCount到文件
    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            instructionCount.Lin, instructionCount.Linp, instructionCount.Lwt, instructionCount.Lwtp,
            instructionCount.Cmpfis_aor, instructionCount.Cmpfis_tos, instructionCount.Cmpfis_aos,
            instructionCount.Cmpfis_ptos, instructionCount.Cmpfis_paos,
            instructionCount.Cmpfgt_aor, instructionCount.Cmpfgt_tos, instructionCount.Cmpfgt_aos,
            instructionCount.Cmpfgt_ptos, instructionCount.Cmpfgt_paos,
            instructionCount.Cmpfgtp, instructionCount.Lpenalty, instructionCount.Nop, instructionCount.Nop_w_rd, 
            instructionCount.IS_reward,instructionCount.L2_reward,instructionCount.Fussion);

    // 关闭文件
    fclose(file);


    // //*******************************************清理*******************************************
    for(int i = 0; i < para_times; ++i) {
        free(ls_matrix[i]);
        free(atos_matrix[i]);
    }
    free(ls_matrix);
    free(atos_matrix);
    free(weight_update_ls);
    free(IS_load_rows);


    return 0;
}

void log_init() {
    // Log initialization logic here
    char filepath[256];
    sprintf(filepath, "./build/inst.txt");

    if (access(filepath, F_OK) != -1) {
        // file exists
        if (remove(filepath) == 0) {
            //printf("Deleted successfully\n");
        } else {
            //printf("Unable to delete the file\n");
        }
    } else {
        // file doesn't exist
        // printf("File doesn't exist\n");
        FILE *file = fopen(filepath, "w");
        if (file != NULL) {
            fclose(file);
        }
    }
}

void idle(){
    instructionCount.Nop++; 
}

void load_is_block(int num_rows){
    bus_is_width_ratio = arch.BUS_WIDTH / arch.IS_WIDTH;
    if (bus_is_width_ratio == 0) {
        load_is_narrow_bus(num_rows);
    } else {
        load_is_wide_bus(num_rows);
    }
}

void load_is_narrow_bus(int num_rows) {
    for (int i_rows = 0; i_rows < num_rows; ++i_rows) {
        for (int j_reg = arch.IS_WIDTH / arch.BUS_WIDTH - 1; j_reg >= 0; --j_reg) {
            if (j_reg != 0) {
                instructionCount.Linp++;
            } else {
                instructionCount.Lin++;
            }
        }
    }
}

void load_is_wide_bus(int num_rows) {
    for (int i_rows = 0; i_rows < num_rows; ++i_rows) {
        instructionCount.Lin++;
        if (i_rows % bus_is_width_ratio != 0) {
            instructionCount.L2_reward++;
        }
    }
}

int wu_ls_narrow_bus(int num_ls, int num_channel, int i_block) {
    for (int i_ls = 0; i_ls < num_ls; ++i_ls) {
        int i_pt, i_at;
        if (strcmp(data_stream, "isap") == 0 || strcmp(data_stream, "wsap") == 0) {
            i_pt = i_block / weight_block_col;
            i_at = i_block % weight_block_col;
        } else if (strcmp(data_stream, "ispp") == 0 || strcmp(data_stream, "wspp") == 0) {
            i_pt = i_block % weight_block_row;
            i_at = i_block / weight_block_row;
        }
        for (int j_channel = 0; j_channel < num_channel; j_channel++) {        
            for (int k_reg = arch.CIM_WIDTH / arch.BUS_WIDTH * config.WEIGHT_ROW - 1; k_reg >= 0; k_reg--) {
                int row_reg = (arch.CIM_WIDTH / arch.BUS_WIDTH * config.WEIGHT_ROW - 1 - k_reg) / (acc0.CIMsWriteWidth / acc0.BusWidth);
                int pause_reg = k_reg % (arch.CIM_WIDTH / arch.BUS_WIDTH);

                if (pause_reg == 0) {
                    instructionCount.Lwt++;
                } else {
                    instructionCount.Lwtp++;
                }
            }
        }
        i_block += 1;
    }
    return i_block;
}

int wu_ls_wide_bus(int num_ls, int num_channel, int i_block) {
    for (int i_ls = 0; i_ls < num_ls; ++i_ls) {
        int i_pt, i_at;
        if (strcmp(data_stream, "isap") == 0 || strcmp(data_stream, "wsap") == 0) {
            i_pt = i_block / weight_block_col;
            i_at = i_block % weight_block_col;
        } else if (strcmp(data_stream, "ispp") == 0 || strcmp(data_stream, "wspp") == 0) {
            i_pt = i_block % weight_block_row;
            i_at = i_block / weight_block_row;
        }
        for (int j_channel = 0; j_channel < num_channel; j_channel++) {
            for (int k_reg = arch.WEIGHT_ROW - 1; k_reg >= 0; k_reg--) {
                instructionCount.Lwt++;
                if (k_reg >= arch.WEIGHT_ROW / bus_wu_width_ratio) {
                    instructionCount.L2_reward++;
                }
            }
        }
        i_block += 1;
    }
    return i_block;
}

int wu_ls_bank(int num_ls, int num_channel, int i_block){
    bus_wu_width_ratio = arch.BUS_WIDTH / arch.CIMsWriteWidth;
    if (bus_wu_width_ratio == 0) {
        return wu_ls_narrow_bus(num_ls, num_channel, i_block);
    } else {
        return wu_ls_wide_bus(num_ls, num_channel, i_block);
    }
}

void compute(int i_input_channel, int computing_block,int fussion_flag) {
    int i_ls = computing_block % arch.SCR;
    int i_pt, i_at, is_addr, os_addr, input_map_position, j_reg;
    
    if (strcmp(data_stream, "isap") == 0 || strcmp(data_stream, "wsap") == 0) {
        i_pt = computing_block / weight_block_col;
        i_at = computing_block % weight_block_col;
    } else if (strcmp(data_stream, "ispp") == 0 || strcmp(data_stream, "wspp") == 0) {
        i_pt = computing_block % weight_block_row;
        i_at = computing_block / weight_block_row;
    }
    
    int atos_flag = atos_matrix[i_pt][i_at];

    os_addr = i_input_channel * para_times + i_pt;
    // if (fussion_flag) os_addr = i_input_channel * Spv + (i_pt - Sqk) % Spv + ;

    if (strcmp(data_stream, "isap") == 0 || strcmp(data_stream, "ispp") == 0) {
        is_addr = (i_input_channel % input_channels_per_ISload) * rows_per_input_channel + i_at;
    } else if (strcmp(data_stream, "wsap") == 0 || strcmp(data_stream, "wspp") == 0) {
        is_addr = i_input_channel * rows_per_input_channel + i_at;
    }

    if ((strcmp(data_stream, "wsap") == 0 || strcmp(data_stream, "wspp") == 0) && (i_input_channel >= input_channels_per_ISload)) { // Cmpfgt & Cmpfgtp
    //  这里是bus wide的情况
        if (bus_is_width_ratio >= 1){
            input_map_position = i_input_channel * input_map_length + i_at * arch.AL;
            if(input_map_position / bus_is_width_ratio == input_map_position_record / bus_is_width_ratio && !fussion_flag) instructionCount.L2_reward++;
            input_map_position_record = input_map_position;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (atos_flag == 2) {
                if (os_addr >= os_virtual_depth) { // aos, os overflow
                    if (i_at == acc_times - 1) {
                        os_virtual_depth += 1;
                    }
                    instructionCount.Cmpfgt_paos++;
                    instructionCount.Lpenalty+=ceil((float)arch.OS_WIDTH / (arch.RESULT_WIDTH / arch.INPUT_WIDTH) / arch.BUS_WIDTH);
                } else { // aos, os not overflow
                    if (i_at == acc_times - 1) {
                        os_virtual_depth += 1;
                        instructionCount.Cmpfgt_paos++;
                    }
                    else {
                        instructionCount.Cmpfgt_aos++;
                    }
                    instructionCount.Nop_w_rd++;
                }
                // Additional conditions and corresponding fprintf statements should be added here as per the Python logic
            }
            
            if (atos_flag == 1){
                if (os_addr >= os_virtual_depth){    // tos, os overflow
                    if (i_at == acc_times-1)    // complete one output, gen rd request, aos: paos(go through)
                        os_virtual_depth += 1;
                    instructionCount.Cmpfgt_ptos++;
                }
                else{    // tos, os not overflow
                    if (i_at == acc_times-1){ //gen rd request
                        os_virtual_depth += 1;
                        instructionCount.Cmpfgt_ptos++;
                    }
                    else{
                        instructionCount.Cmpfgt_tos++;
                    }
                }
            }

            if (atos_flag == 0){
                instructionCount.Cmpfgt_aor++;
            }
            ////////////////////////////////////////////////////////////////////////


            return;
        }

    //  这里是bus narrow的情况
        input_map_position = i_input_channel * input_map_length + i_at * arch.AL + (arch.BUS_WIDTH / arch.INPUT_WIDTH) * (arch.IS_WIDTH / arch.BUS_WIDTH - 1);
        
        // if (is_addr == is_addr_record && !fussion_flag) instructionCount.L2_reward++;
        // is_addr_record = is_addr;

        for (j_reg = arch.IS_WIDTH / arch.BUS_WIDTH - 1; j_reg >= 0; j_reg--) {
            
            if ((gt_in_map_record / (arch.IS_WIDTH / arch.IS_WIDTH)) == (input_map_position / (arch.IS_WIDTH / arch.IS_WIDTH)) && (i_ls != 0) && j_reg != 0) {
                // 当前位置跟上次算的位置一样 && 不是第一组(第一组需要数据进来) && 不是最后一个（最后一个需要计算）
                input_map_position -= arch.BUS_WIDTH / arch.INPUT_WIDTH;
                continue;
            }

            if (j_reg != 0) {   // Cmpfgtp
                instructionCount.Cmpfgtp++;
                input_map_position_record = input_map_position;///???
                input_map_position -= arch.BUS_WIDTH / arch.INPUT_WIDTH;
            } else {            // Cmpfgt
                if (input_map_position == input_map_position_record && !fussion_flag) instructionCount.L2_reward++;
                input_map_position_record = input_map_position;///???
                if (atos_flag == 2) {
                    if (os_addr >= os_virtual_depth) { // aos, os overflow
                        if (i_at == acc_times - 1) {
                            os_virtual_depth += 1;
                        }
                        instructionCount.Cmpfgt_paos++;
                        instructionCount.Lpenalty+=ceil((float)acc0.OutputSRAMWidth / (config.RESULT_WIDTH / config.DATA_WIDTH) / config.BUS_WIDTH);
                    } else { // aos, os not overflow
                        if (i_at == acc_times - 1) {
                            os_virtual_depth += 1;
                            instructionCount.Cmpfgt_paos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                return;
                            }
                        }
                        else {
                            instructionCount.Cmpfgt_aos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                return;
                            }
                        }
                        instructionCount.Nop_w_rd++;
                    }
                    // Additional conditions and corresponding fprintf statements should be added here as per the Python logic
                }
                
                if (atos_flag == 1){
                    if (os_addr >= os_virtual_depth){    // tos, os overflow
                        if (i_at == acc_times-1)    // complete one output, gen rd request, aos: paos(go through)
                            os_virtual_depth += 1;
                        instructionCount.Cmpfgt_ptos++;
                        if (fussion_flag){
                            instructionCount.Fussion++;
                            return;
                        }
                    }
                    else{    // tos, os not overflow
                        if (i_at == acc_times-1){ //gen rd request
                            os_virtual_depth += 1;
                            instructionCount.Cmpfgt_ptos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                return;
                            }
                        }
                        else{
                            instructionCount.Cmpfgt_tos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                return;
                            }
                        }
                    }
                }

                if (atos_flag == 0){
                    instructionCount.Cmpfgt_aor++;
                    if (fussion_flag){
                        instructionCount.Fussion++;
                        return;
                    }
                }
            }
        }
        gt_in_map_record = input_map_position;
    }

    else{   // Cmpfis
        if (is_addr == is_addr_record) instructionCount.IS_reward++;
            is_addr_record = is_addr;

        if (atos_flag == 2) { //aos
            if (os_addr >= os_virtual_depth) {   // aos, os overflow
                if (i_at == acc_times-1) {
                    os_virtual_depth += 1;
                }
                instructionCount.Cmpfis_paos++;
                instructionCount.Lpenalty+=ceil((float)arch.OS_WIDTH / (arch.RESULT_WIDTH / arch.INPUT_WIDTH) / arch.BUS_WIDTH);
            } else {                             // aos, os not overflow
                if (i_at == acc_times - 1) {
                    os_virtual_depth += 1;
                    instructionCount.Cmpfis_paos++;
                }
                else {
                    instructionCount.Cmpfis_aos++;
                }
                instructionCount.Nop_w_rd++;
            }
        }

        if (atos_flag == 1){
            if (os_addr >= os_virtual_depth){    // tos, os overflow
                if (i_at == acc_times-1)    // complete one output, gen rd request, aos: paos(go through)
                    os_virtual_depth += 1;
                instructionCount.Cmpfis_ptos++;
            }
            else{    // tos, os not overflow
                if (i_at == acc_times-1){ //gen rd request
                    os_virtual_depth += 1;
                    instructionCount.Cmpfis_ptos++;
                }
                else{
                    instructionCount.Cmpfis_tos++;
                }
            }
        }

        if (atos_flag == 0){
            instructionCount.Cmpfis_aor++;
        }
    }
}
