#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hw_config.h"
#include "inst_stack.h"
#include "tensor_stack.h"
#include "instruction_count.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define WRITE_INST 0
#define VERIFY 0

int bus_width, al, pc, scr, is_depth, os_depth, freq;
char* operation;
int dim1, dim2, dim3;
char* data_stream;

hwc acc0;
Config config;
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

InstStack inst_stack;

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

int main(int argc, char *argv[]){
    if (argc != 13) {
        printf("error!");
        return 1;
    }

    // printf("Number of command line arguments: %d\n", argc);

    bus_width = atoi(argv[1]);
    al = atoi(argv[2]);
    pc = atoi(argv[3]);
    scr = atoi(argv[4]);
    is_depth = atoi(argv[5]);
    os_depth = atoi(argv[6]);
    freq = atoi(argv[7]);

    operation = argv[8];

    dim1 = atoi(argv[9]);
    dim2 = atoi(argv[10]);
    dim3 = atoi(argv[11]); 

    data_stream = argv[12];

    InitConfig(&config, bus_width, al, pc, scr, is_depth, os_depth, freq);
    // PrintConfig(&config);

    Inithwc(&acc0, config);
    // Printhwc(&acc0);

    InitInstStack(&inst_stack, 10, "inst.txt");

    log_init();

    if (strcmp(operation, "proj") == 0)
        proj_process(dim1,dim2,dim3);
    else if (strcmp(operation, "a2a") == 0)
            a2a_process();
    
    for (int i = 0; i < inst_stack.len; i++){
        PushInstStack(&inst_stack,"",0,0);
    }

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
        fprintf(file, "bus_width,al,pc,scr,is_depth,os_depth,freq,operation,dim1,dim2,dim3,data_stream,");
        fprintf(file, "Lin,Linp,Lwt,Lwtp,Cmpfis_aor,Cmpfis_tos,Cmpfis_aos,Cmpfis_ptos,Cmpfis_paos,");
        fprintf(file, "Cmpfgt_aor,Cmpfgt_tos,Cmpfgt_aos,Cmpfgt_ptos,Cmpfgt_paos,Cmpfgtp,Lpenalty,Nop,Nop_w_rd,");
        fprintf(file, "IS_reward,L2_reward,Fussion\n");
    }

    // 写入命令行参数到文件
    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%s,",
            bus_width, al, pc, scr, is_depth, os_depth, freq, operation,
            atoi(argv[9]), atoi(argv[10]), atoi(argv[11]), argv[12]);

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
    sprintf(filepath, "inst.txt");

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
    PushInstStack(&inst_stack, "starting compiler:\n", 0, 0);
}

void idle(){
    instructionCount.Nop++; 
    if (WRITE_INST == 1){
        PushInstStack(&inst_stack, "nop\n", 0, 0);
    }

}

int load_is_narrow_bus(int num_rows, int input_map_position) {
    for (int i_rows = 0; i_rows < num_rows; ++i_rows) {
        input_map_position += (config.BUS_WIDTH / config.DATA_WIDTH) * (acc0.InputSRAMWidth / acc0.BusWidth - 1);
        for (int j_reg = acc0.InputSRAMWidth / acc0.BusWidth - 1; j_reg >= 0; --j_reg) {
            if (j_reg != 0) {
                instructionCount.Linp++;
                if (WRITE_INST){
                    if (VERIFY) {
                        sprintf(item, "Linp\t <pos> %d\t <is_addr> %d\t <input_map> %d\n", j_reg, i_rows, input_map_position);
                        PushInstStack(&inst_stack, item, 0, 0);
                    } else {
                        sprintf(item, "Linp\t <pos> %d\n", j_reg);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                }
            } else {
                instructionCount.Lin++;
                if (WRITE_INST){
                    if (VERIFY) {
                        sprintf(item, "Lin\t\t <pos> %d\t <is_addr> %d\t <input_map> %d\n", j_reg, i_rows, input_map_position);
                        PushInstStack(&inst_stack, item, 0, 0);
                    } else {
                        sprintf(item, "Lin\t\t <pos> %d\t <is_addr> %d\n", j_reg, i_rows);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                }
            }
            input_map_position -= (config.BUS_WIDTH / config.DATA_WIDTH);
            // 注意: 每个channel的最后一行可能需要特殊处理
        }
        input_map_position += (acc0.InputSRAMWidth / config.DATA_WIDTH) + (config.BUS_WIDTH / config.DATA_WIDTH);
    }
    return input_map_position;
}

int load_is_wide_bus(int num_rows, int input_map_position) {
    for (int i_rows = 0; i_rows < num_rows; ++i_rows) {
        input_map_position += acc0.InputSRAMWidth / config.DATA_WIDTH;
        instructionCount.Lin++;
        if (i_rows % bus_is_width_ratio != 0) {
            instructionCount.L2_reward++;
        }
        if (WRITE_INST){
            if (VERIFY) {
                sprintf(item, "Lin\t\t <pos> %d\t <is_addr> %d\t <input_map> %d\n", 0, i_rows, input_map_position);
                PushInstStack(&inst_stack, item, 0, 0);
            } else {
                sprintf(item, "Lin\t\t <pos> %d\t <is_addr> %d\n", 0, i_rows);
                PushInstStack(&inst_stack, item, 0, 0);
            }
        }
    }
    return input_map_position;
}

int load_is_block(int num_rows, int input_map_position){
    bus_is_width_ratio = config.BUS_WIDTH / acc0.InputSRAMWidth;
    if (bus_is_width_ratio == 0) {
        return load_is_narrow_bus(num_rows, input_map_position);
    } else {
        return load_is_wide_bus(num_rows, input_map_position);
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
            int i_weight_channel = i_pt * config.PC + j_channel;
            int j_data_in_channel = i_at * config.AL;
            int weight_map_position = i_weight_channel * weight_map_length + j_data_in_channel;
            
            for (int k_reg = acc0.CIMsWriteWidth / acc0.BusWidth * config.WEIGHT_ROW - 1; k_reg >= 0; k_reg--) {
                weight_map_position = i_weight_channel * weight_map_length + j_data_in_channel + k_reg * acc0.BusWidth / config.DATA_WIDTH;
                int row_reg = (acc0.CIMsWriteWidth / acc0.BusWidth * config.WEIGHT_ROW - 1 - k_reg) / (acc0.CIMsWriteWidth / acc0.BusWidth);
                int pause_reg = k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth);

                if (pause_reg == 0) {
                    instructionCount.Lwt++;
                    if (WRITE_INST) {
                        if (VERIFY) {
                            sprintf(item, "Lwt\t\t <pos> %d\t <cm_addr> %d\t <weight_map> %d\n", k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth), j_channel * config.SCR * config.WEIGHT_ROW + row_reg * config.SCR + i_ls, weight_map_position);
                            PushInstStack(&inst_stack, item, 0, 0);
                        } else {
                            sprintf(item, "Lwt\t\t <pos> %d\t <cm_addr> %d\n", k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth), j_channel * config.SCR * config.WEIGHT_ROW + row_reg * config.SCR + i_ls);
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
                    }
                } else {
                    instructionCount.Lwtp++;
                    if (WRITE_INST) {
                        if (VERIFY) {
                            sprintf(item, "Lwtp\t <pos> %d\t <cm_addr> %d\t <weight_map> %d\n", k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth), j_channel * config.SCR * config.WEIGHT_ROW + row_reg * config.SCR + i_ls, weight_map_position);
                            PushInstStack(&inst_stack, item, 0, 0);
                        } else {
                            sprintf(item, "Lwtp\t <pos> %d\n", k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth));
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
                    }
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
            int i_weight_channel = i_pt * config.PC + j_channel;
            int j_data_in_channel = i_at * config.AL;
            int weight_map_position = i_weight_channel * weight_map_length + j_data_in_channel;
            
            for (int k_reg = config.WEIGHT_ROW - 1; k_reg >= 0; k_reg--) {
                // weight_map_position = i_weight_channel * weight_map_length + j_data_in_channel + k_reg * acc0.BusWidth / config.DATA_WIDTH;
                // 完全裂开了，验证不了一点
                // int row_reg = (acc0.CIMsWriteWidth / acc0.BusWidth * config.WEIGHT_ROW - 1 - k_reg) / (acc0.CIMsWriteWidth / acc0.BusWidth);
                instructionCount.Lwt++;
                if (k_reg >= config.WEIGHT_ROW / bus_wu_width_ratio) {
                    instructionCount.L2_reward++;
                }

                // if (WRITE_INST) {
                //     if (VERIFY) {
                //         sprintf(item, "Lwt\t\t <pos> %d\t <cm_addr> %d\t <weight_map> %d\n", k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth), j_channel * config.SCR * config.WEIGHT_ROW + row_reg * config.SCR + i_ls, weight_map_position);
                //         PushInstStack(&inst_stack, item, 0, 0);
                //     } else {
                //         sprintf(item, "Lwt\t\t <pos> %d\t <cm_addr> %d\n", k_reg % (acc0.CIMsWriteWidth / acc0.BusWidth), j_channel * config.SCR * config.WEIGHT_ROW + row_reg * config.SCR + i_ls);
                //         PushInstStack(&inst_stack, item, 0, 0);
                //     }
                // }
            }
        }
        i_block += 1;
    }
    return i_block;
}

int wu_ls_bank(int num_ls, int num_channel, int i_block){
    bus_wu_width_ratio = config.BUS_WIDTH / config.CIMsWriteWidth;
    if (bus_wu_width_ratio == 0) {
        return wu_ls_narrow_bus(num_ls, num_channel, i_block);
    } else {
        return wu_ls_wide_bus(num_ls, num_channel, i_block);
    }
}

void compute(int i_input_channel, int computing_block,int fussion_flag) {
    int i_ls = computing_block % config.SCR;
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
            input_map_position = i_input_channel * input_map_length + i_at * config.AL;
            if(input_map_position / bus_is_width_ratio == input_map_position_record / bus_is_width_ratio && !fussion_flag) instructionCount.L2_reward++;
            input_map_position_record = input_map_position;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (atos_flag == 2) {
                if (os_addr >= os_virtual_depth) { // aos, os overflow
                    if (i_at == acc_times - 1) {
                        os_virtual_depth += 1;
                    }
                    instructionCount.Cmpfgt_paos++;
                    instructionCount.Lpenalty+=ceil((float)acc0.OutputSRAMWidth / (config.RESULT_WIDTH / config.DATA_WIDTH) / config.BUS_WIDTH);
                    if (WRITE_INST){
                        if (VERIFY) {
                            sprintf(item, "Lpenalty\t <os_addr_rd>\t %d\n", os_addr);
                            PushInstStack(&inst_stack, item, 0, 0);
                            sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                            PushInstStack(&inst_stack, item, 0, 0);
                        } else {
                            for (int i = 0; i < ceil((float)acc0.OutputSRAMWidth / (config.RESULT_WIDTH / config.DATA_WIDTH) / config.BUS_WIDTH); i++) {
                                sprintf(item, "Lpenalty\t\n");
                                PushInstStack(&inst_stack, item, 0, 0);
                            }
                            sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <paos>\n", j_reg, i_ls);
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
                    }
                } else { // aos, os not overflow
                    if (i_at == acc_times - 1) {
                        os_virtual_depth += 1;
                        instructionCount.Cmpfgt_paos++;
                        // if (fussion_flag){
                        //     instructionCount.Fussion++;
                        //     if (WRITE_INST){
                        //         if (VERIFY) {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                        //             PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                        //         } else {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <paos>\n", i_input_channel, i_ls);
                        //             PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                        //         }
                        //     }
                        //     return;
                        // }
                        if (WRITE_INST){
                            if (VERIFY) {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                            } else {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <paos>\n", j_reg, i_ls);
                                PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                            }
                        }
                    }
                    else {
                        instructionCount.Cmpfgt_aos++;
                        // if (fussion_flag){
                        //     instructionCount.Fussion++;
                        //     if (WRITE_INST){
                        //         if (VERIFY) {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                        //             PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                        //         } else {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <aos>\n", i_input_channel, i_ls);
                        //             PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                        //         }
                        //     }
                        //     return;
                        // }
                        if (WRITE_INST){
                            if (VERIFY) {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 1, os_addr); 
                            } else {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", j_reg, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 1, os_addr); 
                            }
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
                    // if (fussion_flag){
                    //     instructionCount.Fussion++;
                    //     if (WRITE_INST){
                    //         if (VERIFY) {
                    //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                    //             PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                    //         } else {
                    //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\n", i_input_channel, i_ls);
                    //             PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                    //         }
                    //     }
                    //     return;
                    // }
                    if (WRITE_INST){
                        if (VERIFY) {
                            sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                            PushInstStack(&inst_stack, item, 0, 0);
                        } else {
                            sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <ptos>\n", j_reg, i_ls);
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
                    }
                }
                else{    // tos, os not overflow
                    if (i_at == acc_times-1){ //gen rd request
                        os_virtual_depth += 1;
                        instructionCount.Cmpfgt_ptos++;
                        // if (fussion_flag){
                        //     instructionCount.Fussion++;
                        //     if (WRITE_INST){
                        //         if (VERIFY) {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                        //             PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                        //         } else {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\n", i_input_channel, i_ls);
                        //             PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                        //         }
                        //     }
                        //     return;
                        // }
                        if (WRITE_INST){
                            if (VERIFY) {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 0, 0);
                            } else {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <ptos>\n", j_reg, i_ls);
                                PushInstStack(&inst_stack, item, 0, 0);
                            }
                        }
                    }
                    else{
                        instructionCount.Cmpfgt_tos++;
                        // if (fussion_flag){
                        //     instructionCount.Fussion++;
                        //     if (WRITE_INST){
                        //         if (VERIFY) {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <tos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                        //             PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                        //         } else {
                        //             sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <tos>\n", i_input_channel, i_ls);
                        //             PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                        //         }
                        //     }
                        //     return;
                        // }
                        if (WRITE_INST){
                            if (VERIFY) {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <%d>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, atos_flag, os_addr);
                                PushInstStack(&inst_stack, item, 0, 0);
                            } else {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <tos>\t <os_addr_wt> %d\n", j_reg, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 0, 0);
                            }
                        }
                    }
                }
            }

            if (atos_flag == 0){
                instructionCount.Cmpfgt_aor++;
                // if (fussion_flag){
                //     instructionCount.Fussion++;
                //     if (WRITE_INST){
                //         sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <aor>\n", i_input_channel, i_ls);
                //         PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                //     }
                //     return;
                // }
                if (WRITE_INST){
                    if (VERIFY) {
                        sprintf(item, "Cmpfgt\t <pos> \t%d\t<ca> %d\t <aor>\t <input_map_position>\t%d\n", j_reg, i_ls, input_map_position);
                        PushInstStack(&inst_stack, item, 0, 0);
                    } else {
                        sprintf(item, "Cmpfgt\t <pos> \t%d\t<ca> %d\t <aor>\n", j_reg, i_ls);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                }
            }
            ////////////////////////////////////////////////////////////////////////


            return;
        }

    //  这里是bus narrow的情况
        input_map_position = i_input_channel * input_map_length + i_at * config.AL + (config.BUS_WIDTH / config.DATA_WIDTH) * (acc0.InputSRAMWidth / acc0.BusWidth - 1);
        
        // if (is_addr == is_addr_record && !fussion_flag) instructionCount.L2_reward++;
        // is_addr_record = is_addr;

        for (j_reg = acc0.InputSRAMWidth / acc0.BusWidth - 1; j_reg >= 0; j_reg--) {
            
            if ((gt_in_map_record / (acc0.InputSRAMWidth / config.DATA_WIDTH)) == (input_map_position / (acc0.InputSRAMWidth / config.DATA_WIDTH)) && (i_ls != 0) && j_reg != 0) {
                // 当前位置跟上次算的位置一样 && 不是第一组(第一组需要数据进来) && 不是最后一个（最后一个需要计算）
                input_map_position -= config.BUS_WIDTH / config.DATA_WIDTH;
                continue;
            }

            if (j_reg != 0) {   // Cmpfgtp
                instructionCount.Cmpfgtp++;
                if (VERIFY) {
                    if (WRITE_INST) {
                        sprintf(item, "Cmpfgtp\t <pos> \t%d\t<input_map_position>\t%d\n", j_reg, input_map_position);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                } else {
                    if (WRITE_INST) {
                        sprintf(item, "Cmpfgtp\t <pos> \t%d\n", j_reg);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                }
                input_map_position_record = input_map_position;///???
                input_map_position -= config.BUS_WIDTH / config.DATA_WIDTH;
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
                        if (WRITE_INST){
                            if (VERIFY) {
                                sprintf(item, "Lpenalty\t <os_addr_rd>\t %d\n", os_addr);
                                PushInstStack(&inst_stack, item, 0, 0);
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 0, 0);
                            } else {
                                for (int i = 0; i < ceil((float)acc0.OutputSRAMWidth / (config.RESULT_WIDTH / config.DATA_WIDTH) / config.BUS_WIDTH); i++) {
                                    sprintf(item, "Lpenalty\t\n");
                                    PushInstStack(&inst_stack, item, 0, 0);
                                }
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <paos>\n", j_reg, i_ls);
                                PushInstStack(&inst_stack, item, 0, 0);
                            }
                        }
                    } else { // aos, os not overflow
                        if (i_at == acc_times - 1) {
                            os_virtual_depth += 1;
                            instructionCount.Cmpfgt_paos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                if (WRITE_INST){
                                    if (VERIFY) {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                                        PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                                    } else {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <paos>\n", i_input_channel, i_ls);
                                        PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                                    }
                                }
                                return;
                            }
                            if (WRITE_INST){
                                if (VERIFY) {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                    PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                                } else {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <paos>\n", j_reg, i_ls);
                                    PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                                }
                            }
                        }
                        else {
                            instructionCount.Cmpfgt_aos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                if (WRITE_INST){
                                    if (VERIFY) {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                                        PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                                    } else {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <aos>\n", i_input_channel, i_ls);
                                        PushInstStack(&inst_stack, item, 1, os_addr); // virtual -> practical?
                                    }
                                }
                                return;
                            }
                            if (WRITE_INST){
                                if (VERIFY) {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                    PushInstStack(&inst_stack, item, 1, os_addr); 
                                } else {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", j_reg, i_ls, os_addr);
                                    PushInstStack(&inst_stack, item, 1, os_addr); 
                                }
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
                            if (WRITE_INST){
                                if (VERIFY) {
                                    sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                                    PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                                } else {
                                    sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\n", i_input_channel, i_ls);
                                    PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                                }
                            }
                            return;
                        }
                        if (WRITE_INST){
                            if (VERIFY) {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                PushInstStack(&inst_stack, item, 0, 0);
                            } else {
                                sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <ptos>\n", j_reg, i_ls);
                                PushInstStack(&inst_stack, item, 0, 0);
                            }
                        }
                    }
                    else{    // tos, os not overflow
                        if (i_at == acc_times-1){ //gen rd request
                            os_virtual_depth += 1;
                            instructionCount.Cmpfgt_ptos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                if (WRITE_INST){
                                    if (VERIFY) {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                                        PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                                    } else {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <ptos>\n", i_input_channel, i_ls);
                                        PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                                    }
                                }
                                return;
                            }
                            if (WRITE_INST){
                                if (VERIFY) {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, os_addr);
                                    PushInstStack(&inst_stack, item, 0, 0);
                                } else {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <ptos>\n", j_reg, i_ls);
                                    PushInstStack(&inst_stack, item, 0, 0);
                                }
                            }
                        }
                        else{
                            instructionCount.Cmpfgt_tos++;
                            if (fussion_flag){
                                instructionCount.Fussion++;
                                if (WRITE_INST){
                                    if (VERIFY) {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <tos>\t <os_addr_wt> %d\n", i_input_channel, i_ls, os_addr);
                                        PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                                    } else {
                                        sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <tos>\n", i_input_channel, i_ls);
                                        PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                                    }
                                }
                                return;
                            }
                            if (WRITE_INST){
                                if (VERIFY) {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t<input_map_position>\t%d\t <ca> %d\t <%d>\t <os_addr_wt> %d\n", j_reg, input_map_position, i_ls, atos_flag, os_addr);
                                    PushInstStack(&inst_stack, item, 0, 0);
                                } else {
                                    sprintf(item, "Cmpfgt\t <pos> \t%d\t <ca> %d\t <tos>\t <os_addr_wt> %d\n", j_reg, i_ls, os_addr);
                                    PushInstStack(&inst_stack, item, 0, 0);
                                }
                            }
                        }
                    }
                }

                if (atos_flag == 0){
                    instructionCount.Cmpfgt_aor++;
                    if (fussion_flag){
                        instructionCount.Fussion++;
                        if (WRITE_INST){
                            sprintf(item, "Cmpfgt\t <fus> \t%d\t <ca> %d\t <aor>\n", i_input_channel, i_ls);
                            PushInstStack(&inst_stack, item, 0, 0); // virtual -> practical?
                        }
                        return;
                    }
                    if (WRITE_INST){
                        if (VERIFY) {
                            sprintf(item, "Cmpfgt\t <pos> \t%d\t<ca> %d\t <aor>\t <input_map_position>\t%d\n", j_reg, i_ls, input_map_position);
                            PushInstStack(&inst_stack, item, 0, 0);
                        } else {
                            sprintf(item, "Cmpfgt\t <pos> \t%d\t<ca> %d\t <aor>\n", j_reg, i_ls);
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
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
                instructionCount.Lpenalty+=ceil((float)acc0.OutputSRAMWidth / (config.RESULT_WIDTH / config.DATA_WIDTH) / config.BUS_WIDTH);
                if (WRITE_INST){
                    if (VERIFY) {
                        sprintf(item, "Lpenalty\t <os_addr_rd>\t %d\n", os_addr);
                        PushInstStack(&inst_stack, item, 0, 0);
                        sprintf(item, "Cmpfis\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", i_ls, os_addr);
                        PushInstStack(&inst_stack, item, 0, 0);
                    } else {
                        for (int i = 0; i < ceil((float)acc0.OutputSRAMWidth / (config.RESULT_WIDTH / config.DATA_WIDTH) / config.BUS_WIDTH); i++) {
                            sprintf(item, "Lpenalty\t\n");
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
                        sprintf(item, "Cmpfis\t <ca> %d\t <paos>\n", i_ls);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                }
            } else {                             // aos, os not overflow
                if (i_at == acc_times - 1) {
                    os_virtual_depth += 1;
                    instructionCount.Cmpfis_paos++;
                    if (WRITE_INST){
                        if (VERIFY) {
                            sprintf(item, "Cmpfis\t <ca> %d\t <paos>\t <os_addr_wt> %d\n", i_ls, os_addr);
                            PushInstStack(&inst_stack, item, 1, os_addr);
                        } else {
                            sprintf(item, "Cmpfis\t <ca> %d\t <paos>\n", i_ls);
                            PushInstStack(&inst_stack, item, 1, os_addr);
                        }
                    }
                }
                else {
                    instructionCount.Cmpfis_aos++;
                    if (WRITE_INST){
                        sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <aos>\t <os_addr_wt> %d\n", is_addr, i_ls, os_addr);
                        PushInstStack(&inst_stack, item, 1, os_addr);
                    }
                }
                instructionCount.Nop_w_rd++;
            }
        }

        if (atos_flag == 1){
            if (os_addr >= os_virtual_depth){    // tos, os overflow
                if (i_at == acc_times-1)    // complete one output, gen rd request, aos: paos(go through)
                    os_virtual_depth += 1;
                instructionCount.Cmpfis_ptos++;
                if (WRITE_INST){
                    if (VERIFY) {
                        sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", is_addr, i_ls, os_addr);
                        PushInstStack(&inst_stack, item, 0, 0);
                    } else {
                        sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <ptos>\n", is_addr, i_ls);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }
                }
            }
            else{    // tos, os not overflow
                if (i_at == acc_times-1){ //gen rd request
                    os_virtual_depth += 1;
                    instructionCount.Cmpfis_ptos++;
                    if (WRITE_INST){
                        if (VERIFY) {
                            sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <ptos>\t <os_addr_wt> %d\n", is_addr, i_ls, os_addr);
                            PushInstStack(&inst_stack, item, 0, 0);
                        } else {
                            sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <ptos>\n", is_addr, i_ls);
                            PushInstStack(&inst_stack, item, 0, 0);
                        }
                    }
                }
                else{
                    instructionCount.Cmpfis_tos++;
                    if (WRITE_INST){
                        sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <tos>\t <os_addr_wt> %d\n", is_addr, i_ls, os_addr);
                        PushInstStack(&inst_stack, item, 0, 0);
                    }   
                }
            }
        }

        if (atos_flag == 0){
            instructionCount.Cmpfis_aor++;
            if (WRITE_INST){
                sprintf(item, "Cmpfis\t <is_addr> %d\t <ca> %d\t <aor>\n", is_addr, i_ls);
                PushInstStack(&inst_stack, item, 0, 0);
            }
        }
    }
}

void is_process(void) {
    int input_map_position = 0;
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
        input_map_position = load_is_block(IS_load_rows[i_IS_load], input_map_position);

        i_block = 0;
        for (int j_weight_load = 0; j_weight_load < weight_update_times_per_inst; j_weight_load++) {
            //i_block = 
            wu_ls_bank(weight_update_ls[j_weight_load], config.PC, i_block);

            for (int i = 0; i < acc0.CIMsWriteWidth / acc0.BusWidth * config.WEIGHT_ROW; i++) {
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
    load_is_block(rows_per_input_channel * input_channels_per_ISload, 0);

    for (int i_weight_update = 0; i_weight_update < weight_update_times_per_inst; i_weight_update++) {
        wu_ls_bank(weight_update_ls[i_weight_update], config.PC, i_block);

        for (int i = 0; i < acc0.CIMsWriteWidth / acc0.BusWidth * config.WEIGHT_ROW; i++) {
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

    input_data_per_row = floor(acc0.InputSRAMWidth / config.DATA_WIDTH);
    rows_per_input_channel = (int)ceil((float)input_map_length / input_data_per_row);
    input_channels_per_ISload = acc0.InputSRAMDepth / rows_per_input_channel;
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

    weight_block_row = (int)ceil((float)weight_map_channel / config.PC);
    weight_block_col = (int)ceil((float)weight_map_length / config.AL);
    weight_block_num = weight_block_col * weight_block_row;
    weight_update_times_per_inst = (int)ceil((float)weight_block_num / config.SCR);

    weight_update_ls = (int*)malloc(weight_update_times_per_inst * sizeof(int));
    for (int i = 0; i < weight_update_times_per_inst; ++i) {
        weight_update_ls[i] = config.SCR;
    }
    if (weight_block_num % config.SCR != 0) {
        weight_update_ls[weight_update_times_per_inst - 1] = weight_block_num % config.SCR;
    }

    para_times = weight_block_row;
    acc_times = weight_block_col;

    weight_map_channel = para_times * config.PC;
    weight_map_length = acc_times * config.AL;

    input_map_length  = acc_times * config.AL;
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
                ls_fg = (ls_fg + 1) % config.SCR;
            }
        }
        
        for (int i_pt = 0; i_pt < para_times; ++i_pt) {
            int row_st_fg = 0; // row start flag
            for (int i_at = 0; i_at < acc_times; ++i_at) {    
                if (i_at == acc_times-1 || ls_matrix[i_pt][i_at] == acc0.LocalSwitchrows - 1) {
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
                ls_fg = (ls_fg + 1) % config.SCR;
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
    os_virtual_depth = acc0.OutputSRAMDepth;

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
    int K_para_times = ceil((float)K_map_channel / config.PC);
    int K_acc_times = ceil((float)K_map_length / config.AL);
    int K_map_block = K_para_times * K_acc_times;
    int Sqk = K_map_block;

    int V_map_channel = dim2 / dim3;
    int V_map_length = dim1;
    int V_para_times = ceil((float)V_map_channel / config.PC);
    int V_acc_times = ceil((float)V_map_length / config.AL);
    int V_map_block = V_para_times * V_acc_times;
    int Spv = V_map_block;

    int using_scr = K_map_block + V_map_block;

    if (using_scr > config.SCR){
        printf("ERROR\n");
        return;
    }

    int Q_serial_times = ceil((float)(Sqk + config.PIPELINE_STAGES) / Sqk);

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
        wu_ls_bank(para_times, config.PC, 0);
        for (int i = 0; i < acc0.CIMsWriteWidth / acc0.BusWidth * config.WEIGHT_ROW; i++) {
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


/*
来起名
gli:
    "a2a", seq_len, hid_size, head_num

    hid_size = head_num * embedding_length

    e.g.    "a2a", N, 768, 12

data stream:    
    1. 2ph(two phase)
        分为QK/PV两个phase的计算，将QK和PV看成两个独立的矩阵乘操作
    2. lhd(Low-Hierarchy-Dive)
        每次只送入一个q向量，得到一个xo，
*/
