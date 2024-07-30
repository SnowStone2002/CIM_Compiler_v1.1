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
    if (argc != 25) {
        printf("error!");
        return 1;
    }

    // Parsing command line arguments
    int AL = atoi(argv[1]);
    int PC = atoi(argv[2]);
    int SCR = atoi(argv[3]);
    int ICW = atoi(argv[4]);
    int WUW = atoi(argv[5]);

    int MACRO_ROW = atoi(argv[6]);
    int MACRO_COL = atoi(argv[7]);
    int IS_DEPTH = atoi(argv[8]);
    int OS_DEPTH = atoi(argv[9]);
    int FREQ = atoi(argv[10]);
    int BUS_WIDTH = atoi(argv[11]);
    int PIPELINE_STAGES = atoi(argv[12]);

    int DIRECTION = atoi(argv[13]);
    int DATA_TYPE = atoi(argv[14]);
    int WEIGHT_WIDTH = atoi(argv[15]);
    int WEIGHT_ROW = atoi(argv[16]);
    int WEIGHT_COL = atoi(argv[17]);
    int INPUT_WIDTH = atoi(argv[18]);
    int RESULT_WIDTH = atoi(argv[19]);

    char* operation = argv[20];
    int dim1 = atoi(argv[21]);
    int dim2 = atoi(argv[22]);
    int dim3 = atoi(argv[23]);
    char* data_stream = argv[24];

    // Call initialization functions
    Init_CIM_Macro(&macro, AL, PC, SCR, ICW, WUW, DIRECTION, DATA_TYPE, WEIGHT_WIDTH, WEIGHT_ROW, WEIGHT_COL, INPUT_WIDTH, RESULT_WIDTH);
    Init_Micro_Arch(&arch, &macro, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES);
    // Print_CIM_Macro(&macro);
    // Print_Micro_Arch(&arch);

    //***************************************** main process ****************************************
    if (strcmp(operation, "proj") == 0)
        proj_process(dim1, dim2, dim3);
    else if (strcmp(operation, "a2a") == 0)
        a2a_process();

    //***************************************** terminal output ****************************************
    printInstructionCount(&instructionCount);

    //******************************************* csv output *******************************************
    // Check if file exists
    FILE *file = fopen("count.csv", "r");
    int needHeader = 0;
    if (file == NULL) {
        needHeader = 1; // File does not exist, need to write header
    } else {
        fclose(file); // File exists, close file
    }

    // Open file in append mode, create if it doesn't exist
    file = fopen("count.csv", "a");
    if (file == NULL) {
        perror("Failed to open csv file\n");
        return EXIT_FAILURE;
    }

    // Write header if needed
    if (needHeader) {
        fprintf(file, "al,pc,scr,icw,wuw,macro_row,macro_col,is_depth,os_depth,freq,bus_width,pipeline_stages,operation,dim1,dim2,dim3,data_stream,\n");
        fprintf(file, "Lin,Linp,Lwt,Lwtp,Cmpfis_aor,Cmpfis_tos,Cmpfis_aos,Cmpfis_ptos,Cmpfis_paos,");
        fprintf(file, "Cmpfgt_aor,Cmpfgt_tos,Cmpfgt_aos,Cmpfgt_ptos,Cmpfgt_paos,Cmpfgtp,Lpenalty,Nop,Nop_w_rd,");
        fprintf(file, "IS_reward,L2_reward,Fussion\n");
    }

    // Write command line arguments to file
    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%s\n",
            AL, PC, SCR, ICW, WUW, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES,
            operation, dim1, dim2, dim3, data_stream);

    // Write InstructionCount to file
    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            instructionCount.Lin, instructionCount.Linp, instructionCount.Lwt, instructionCount.Lwtp,
            instructionCount.Cmpfis_aor, instructionCount.Cmpfis_tos, instructionCount.Cmpfis_aos,
            instructionCount.Cmpfis_ptos, instructionCount.Cmpfis_paos,
            instructionCount.Cmpfgt_aor, instructionCount.Cmpfgt_tos, instructionCount.Cmpfgt_aos,
            instructionCount.Cmpfgt_ptos, instructionCount.Cmpfgt_paos,
            instructionCount.Cmpfgtp, instructionCount.Lpenalty, instructionCount.Nop, instructionCount.Nop_w_rd, 
            instructionCount.IS_reward, instructionCount.L2_reward, instructionCount.Fussion);

    // Close file
    fclose(file);

    //******************************************* clear *******************************************
    for (int i = 0; i < para_times; ++i) {
        free(ls_matrix[i]);
        free(atos_matrix[i]);
    }
    free(ls_matrix);
    free(atos_matrix);
    free(weight_update_ls);
    free(IS_load_rows);

    return 0;
}

void idle(){
    instructionCount.Nop++; 
}

void load_is_block(int num_rows){// num_rows is for the IS
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

int wu_ls_bank(int num_ls, int num_channel, int i_block){
    bus_wu_width_ratio = arch.BUS_WIDTH / arch.WUW;
    if (bus_wu_width_ratio == 0) {
        return wu_ls_narrow_bus(num_ls, num_channel, i_block);
    } else {
        return wu_ls_wide_bus(num_ls, num_channel, i_block);
    }
}

int wu_ls_narrow_bus(int num_ls, int num_channel, int i_block) {
    for (int i_ls = 0; i_ls < num_ls; ++i_ls) {
        for (int j_channel = 0; j_channel < num_channel; j_channel++) {        
            for (int k_reg = arch.CIM_WIDTH / arch.BUS_WIDTH * arch.WEIGHT_ROW - 1; k_reg >= 0; k_reg--) {
                //int row_reg = (arch.CIM_WIDTH / arch.BUS_WIDTH * arch.WEIGHT_ROW - 1 - k_reg) / (arch.WUW / arch.BUS_WIDTH);
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
            
            if ((gt_in_map_record / (arch.IS_WIDTH / arch.INPUT_WIDTH)) == (input_map_position / (arch.IS_WIDTH / arch.INPUT_WIDTH)) && (i_ls != 0) && j_reg != 0) {
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
                        instructionCount.Lpenalty+=ceil((float)arch.OS_WIDTH / (arch.RESULT_WIDTH / arch.INPUT_WIDTH) / arch.BUS_WIDTH);
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

void is_process(void) {
    int i_block = 0;

    if (IS_load_times_per_inst == 0){
        if (strcmp(data_stream, "isap") == 0)   strcpy(data_stream, "wsap");
        if (strcmp(data_stream, "ispp") == 0)   strcpy(data_stream, "wspp");

        for(int i = 0; i < para_times; ++i) {
            free(ls_matrix[i]);
            free(atos_matrix[i]);
        }
        free(ls_matrix);
        free(atos_matrix);
        free(weight_update_ls);
        free(IS_load_rows);

        proj_process(dim1,dim2,dim3);

        return;
    }

    for (int i_IS_load = 0; i_IS_load < IS_load_times_per_inst; i_IS_load++) {
        load_is_block(IS_load_rows[i_IS_load]);

        i_block = 0;
        for (int j_weight_load = 0; j_weight_load < weight_update_times_per_inst; j_weight_load++) {
            //i_block = 
            wu_ls_bank(weight_update_ls[j_weight_load], arch.PC, i_block);

            for (int i = 0; i < arch.WUW / arch.BUS_WIDTH * arch.WEIGHT_ROW; i++) {
                idle();
            }

            for (int i_input_channel = i_IS_load * input_channels_per_ISload; 
                 i_input_channel < i_IS_load * input_channels_per_ISload + 
                 IS_load_rows[i_IS_load] / rows_per_input_channel; 
                 i_input_channel++) {

                for (int j_ls = 0; j_ls < weight_update_ls[j_weight_load]; j_ls++) {
                    int j_compute_block = i_block + j_ls;
                    compute(i_input_channel, j_compute_block, 0);
                }
            }

            i_block += weight_update_ls[j_weight_load];
        }
    }
}

void ws_process(void) {
    int i_block = 0;

    // 假设rows_per_input_channel和input_channels_per_ISload已经定义
    load_is_block(rows_per_input_channel * input_channels_per_ISload);

    for (int i_weight_update = 0; i_weight_update < weight_update_times_per_inst; i_weight_update++) {
        wu_ls_bank(weight_update_ls[i_weight_update], arch.PC, i_block);

        for (int i = 0; i < arch.WUW / arch.BUS_WIDTH * arch.WEIGHT_ROW; i++) {
            idle();
        }

        for (int i_input_channel = 0; i_input_channel < input_map_channel; i_input_channel++) {
            for (int j_ls = 0; j_ls < weight_update_ls[i_weight_update]; j_ls++) {
                int j_compute_block = i_block + j_ls;
                compute(i_input_channel, j_compute_block, 0);
            }
        }

        i_block += weight_update_ls[i_weight_update];
    }
}

void proj_process(int a, int b, int c){ //实际上的输入参数是input和weight map的长宽
    weight_map_channel = a;
    weight_map_length = b;

    input_map_length = b;
    input_map_channel = c;

    input_data_per_row = floor(arch.IS_WIDTH / arch.INPUT_WIDTH);
    rows_per_input_channel = (int)ceil((float)input_map_length / input_data_per_row);
    input_channels_per_ISload = arch.IS_DEPTH / rows_per_input_channel;
    if (input_channels_per_ISload > input_map_channel)
        input_channels_per_ISload = input_map_channel;
    if (input_channels_per_ISload == 0)
        IS_load_times_per_inst = 0;
    else{
        IS_load_times_per_inst = (int)ceil((float)input_map_channel / input_channels_per_ISload);
        IS_load_rows = (int*)malloc(IS_load_times_per_inst * sizeof(int));
        for (int i = 0; i < IS_load_times_per_inst; ++i) {
            IS_load_rows[i] = input_channels_per_ISload * rows_per_input_channel;
        }
        if (input_map_channel % input_channels_per_ISload != 0) {
            IS_load_rows[IS_load_times_per_inst - 1] = (input_map_channel % input_channels_per_ISload) * rows_per_input_channel;
        }
    }

    // IS_load_rows = (int*)malloc(IS_load_times_per_inst * sizeof(int));
    // for (int i = 0; i < IS_load_times_per_inst; ++i) {
    //     IS_load_rows[i] = input_channels_per_ISload * rows_per_input_channel;
    // }
    // if (input_map_channel % input_channels_per_ISload != 0) {
    //     IS_load_rows[IS_load_times_per_inst - 1] = (input_map_channel % input_channels_per_ISload) * rows_per_input_channel;
    // }

    weight_block_row = (int)ceil((float)weight_map_channel / arch.PC);
    weight_block_col = (int)ceil((float)weight_map_length / arch.AL);
    weight_block_num = weight_block_col * weight_block_row;
    weight_update_times_per_inst = (int)ceil((float)weight_block_num / arch.SCR);

    weight_update_ls = (int*)malloc(weight_update_times_per_inst * sizeof(int));
    for (int i = 0; i < weight_update_times_per_inst; ++i) {
        weight_update_ls[i] = arch.SCR;
    }
    if (weight_block_num % arch.SCR != 0) {
        weight_update_ls[weight_update_times_per_inst - 1] = weight_block_num % arch.SCR;
    }

    para_times = weight_block_row;
    acc_times = weight_block_col;

    weight_map_channel = para_times * arch.PC;
    weight_map_length = acc_times * arch.AL;

    input_map_length  = acc_times * arch.AL;
    input_map_channel = input_map_channel;

    // 分配ls_matrix并初始化为0
    ls_matrix = (int**)malloc(para_times * sizeof(int*));
    for(int i = 0; i < para_times; ++i) {
        ls_matrix[i] = (int*)malloc(acc_times * sizeof(int));
        for(int j = 0; j < acc_times; ++j) {
            ls_matrix[i][j] = 0;
        }
    }

    // 分配atos_matrix并初始化为0
    atos_matrix = (int**)malloc(para_times * sizeof(int*));
    for(int i = 0; i < para_times; ++i) {
        atos_matrix[i] = (int*)malloc(acc_times * sizeof(int));
        for(int j = 0; j < acc_times; ++j) {
            atos_matrix[i][j] = 0;
        }
    }

    int ls_fg = 0;

    if (strcmp(data_stream, "isap") == 0 || strcmp(data_stream, "wsap") == 0) {
        // ap 优先acc
        for (int i_pt = 0; i_pt < para_times; ++i_pt) {
            for (int i_at = 0; i_at < acc_times; ++i_at) {
                ls_matrix[i_pt][i_at] = ls_fg;
                ls_fg = (ls_fg + 1) % arch.SCR;
            }
        }
        
        for (int i_pt = 0; i_pt < para_times; ++i_pt) {
            int row_st_fg = 0; // row start flag
            for (int i_at = 0; i_at < acc_times; ++i_at) {    
                if (i_at == acc_times-1 || ls_matrix[i_pt][i_at] == arch.SCR - 1) {
                    atos_matrix[i_pt][i_at] = row_st_fg == 0 ? 1 : 2; // 1 for TOS, 2 for AOS
                    row_st_fg = 1;
                }
            }
        }
    } else if (strcmp(data_stream, "ispp") == 0 || strcmp(data_stream, "wspp") == 0) {
        // pp 优先para
        for (int i_at = 0; i_at < acc_times; ++i_at) {
            for (int i_pt = 0; i_pt < para_times; ++i_pt) {
                ls_matrix[i_pt][i_at] = ls_fg;
                ls_fg = (ls_fg + 1) % arch.SCR;
            }
        }

        for (int i_pt = 0; i_pt < para_times; ++i_pt) {
            for (int i_at = 0; i_at < acc_times; ++i_at) {    
                atos_matrix[i_pt][i_at] = i_at == 0 ? 1 : 2; // 1 for TOS, 2 for AOS
            }
        }
    }

    // // 打印基本计算结果
    // printf("input_data_per_row = %d\n", input_data_per_row);
    // printf("rows_per_input_channel = %d\n", rows_per_input_channel);
    // printf("input_channels_per_ISload = %d\n", input_channels_per_ISload);
    // printf("IS_load_times_per_inst = %d\n", IS_load_times_per_inst);

    // // 打印IS_load_rows数组
    // printf("IS_load_rows:\n");
    // for (int i = 0; i < IS_load_times_per_inst; ++i) {
    //     printf("%d ", IS_load_rows[i]);
    // }
    // printf("\n");

    // // 打印权重更新信息
    // printf("weight_block_row = %d\n", weight_block_row);
    // printf("weight_block_col = %d\n", weight_block_col);
    // printf("weight_block_num = %d\n", weight_block_num);
    // printf("weight_update_times_per_inst = %d\n", weight_update_times_per_inst);

    // // 打印weight_update_ls数组
    // printf("weight_update_ls:\n");
    // for (int i = 0; i < weight_update_times_per_inst; ++i) {
    //     printf("%d ", weight_update_ls[i]);
    // }
    // printf("\n");

    // // 打印ls_matrix
    // printf("ls_matrix:\n");
    // for (int i = 0; i < para_times; ++i) {
    //     for (int j = 0; j < acc_times; ++j) {
    //         printf("%d ", ls_matrix[i][j]);
    //     }
    //     printf("\n");
    // }

    // // 打印atos_matrix
    // printf("atos_matrix:\n");
    // for (int i = 0; i < para_times; ++i) {
    //     for (int j = 0; j < acc_times; ++j) {
    //         printf("%d ", atos_matrix[i][j]);
    //     }
    //     printf("\n");
    // }


    gt_in_map_record = 0;
    os_virtual_depth = arch.OS_DEPTH;

    if (strcmp(data_stream, "ispp") == 0 || strcmp(data_stream, "isap") == 0){
        is_process();
    }
    else
        ws_process();

}

void ph2_process(void){
    strcpy(data_stream, "wspp");
    for(int i=0; i<dim3; i++){
        proj_process(dim1,dim2/dim3,dim1);
        proj_process(dim2/dim3,dim1,dim1);
    }//    e.g.    "a2a", N, 768, 12
}

void lhd_process(void){
    int K_map_channel = dim1;
    int K_map_length = dim2 / dim3;
    int K_para_times = ceil((float)K_map_channel / arch.PC);
    int K_acc_times = ceil((float)K_map_length / arch.AL);
    int K_map_block = K_para_times * K_acc_times;
    int Sqk = K_map_block;

    int V_map_channel = dim2 / dim3;
    int V_map_length = dim1;
    int V_para_times = ceil((float)V_map_channel / arch.PC);
    int V_acc_times = ceil((float)V_map_length / arch.AL);
    int V_map_block = V_para_times * V_acc_times;
    int Spv = V_map_block;

    int using_scr = K_map_block + V_map_block;

    if (using_scr > arch.SCR){
        printf("ERROR\n");
        return;
    }

    int Q_serial_times = ceil((float)(Sqk + arch.PIPELINE_STAGES) / Sqk);

    // printf("K_map_channel: %d\n", K_map_channel);
    // printf("K_map_length: %d\n", K_map_length);
    // printf("K_para_times: %d\n", K_para_times);
    // printf("K_acc_times: %d\n", K_acc_times);
    // printf("K_map_block: %d\n", K_map_block);
    // printf("Sqk: %d\n", Sqk);

    // printf("V_map_channel: %d\n", V_map_channel);
    // printf("V_map_length: %d\n", V_map_length);
    // printf("V_para_times: %d\n", V_para_times);
    // printf("V_acc_times: %d\n", V_acc_times);
    // printf("V_map_block: %d\n", V_map_block);
    // printf("Spv: %d\n", Spv);

    // printf("using_scr: %d\n", using_scr);
    // printf("Q_serial_times: %d\n", Q_serial_times);

    // we are using wspp here
    weight_block_row = using_scr;
    para_times = using_scr;
    acc_times = 1;

    input_channels_per_ISload = 0;
    IS_load_times_per_inst = 0;

    // 分配ls_matrix
    int ls_fg = 0;
    ls_matrix = (int**)malloc(para_times * sizeof(int*));
    for(int i = 0; i < para_times; ++i) {
        ls_matrix[i] = (int*)malloc(acc_times * sizeof(int));
        for(int j = 0; j < acc_times; ++j) {
            ls_matrix[i][j] = ls_fg;
            ls_fg++;
        }
    }

    // 分配atos_matrix
    atos_matrix = (int**)malloc(para_times * sizeof(int*));
    for(int i = 0; i < para_times; ++i) {
        atos_matrix[i] = (int*)malloc(acc_times * sizeof(int));
        for(int j = 0; j < acc_times; ++j) {
            atos_matrix[i][j] = 0;
        }
    }

    int i_block = 0;
    for(int i = 0; i < K_para_times; i++){
        for (int j = 0; j < K_acc_times; j++){
            if (j == K_acc_times - 1)
                atos_matrix[i_block][0] = 1;
            else 
                atos_matrix[i_block][0] = 0;
            i_block+=1;
        }
    }

    for(int i = 0; i < V_para_times; i++){
        for (int j = 0; j < V_acc_times; j++){
            if (j == V_acc_times - 1)
                atos_matrix[i_block][0] = 1;
            else 
                atos_matrix[i_block][0] = 0;
            i_block+=1;
        }
    }

    // // 打印 ls_matrix
    // printf("ls_matrix:\n");
    // for(int i = 0; i < para_times; ++i) {
    //     for(int j = 0; j < acc_times; ++j) {
    //         printf("%d ", ls_matrix[i][j]);
    //     }
    //     printf("\n");
    // }

    // // 打印 atos_matrix
    // printf("atos_matrix:\n");
    // for(int i = 0; i < para_times; ++i) {
    //     for(int j = 0; j < acc_times; ++j) {
    //         printf("%d ", atos_matrix[i][j]);
    //     }
    //     printf("\n");
    // }

    for(int i_head=0; i_head<dim3; i_head++){
        // 我们不用更新IS，which is great
        // 首先更新K和V矩阵到CIM中
        strcpy(data_stream, "wspp");//新增的
        wu_ls_bank(para_times, arch.PC, 0);
        for (int i = 0; i < arch.WUW / arch.BUS_WIDTH * arch.WEIGHT_ROW; i++) {
            idle();
        }
        for (int j = 0; j < ceil((float)dim1 / Q_serial_times); j++){
            int current_serial_times = Q_serial_times;
            if (j == ceil((float)dim1 / Q_serial_times) - 1) 
                current_serial_times = min(dim1 - j * Q_serial_times, Q_serial_times);

            strcpy(data_stream, "wspp");
            for (int k = 0; k < current_serial_times; k++){
                for(int m_count=0; m_count<Sqk; m_count++){
                    compute(j * Q_serial_times + k, m_count, 0);
                    //compute(int i_input_channel, int computing_block,int fussion_flag = 0)
                }
            }
            strcpy(data_stream, "wspp");
            for (int k = 0; k < current_serial_times; k++){
                for(int m_count=0; m_count<Spv; m_count++){
                    compute(k, m_count + Sqk, 1);
                    //compute(int fussion_count, int computing_block,int fussion_flag = 0)
                }
            }
        }

    }
}

void a2a_process(void){
    if (strcmp(data_stream, "ph2") == 0){
        ph2_process();
    }
    else if (strcmp(data_stream, "lhd") == 0)
        lhd_process();
}

