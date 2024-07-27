#ifndef HW_CONFIG_H
#define HW_CONFIG_H

typedef struct {
//necessary
    int AL, PC, SCR, ICW, WUW;
    int WEIGHT_WIDTH;
    int INPUT_WIDTH;
    int RESULT_WIDTH;
//optional
    int DIRECTION;
    int DATA_TYPE;
    int WEIGHT_ROW;
    int WEIGHT_COL;
} CIM_Macro;

typedef struct {
    // CIM Macros
    int MACRO_ROW, MACRO_COL;
    int AL, PC, SCR, ICW, WUW; //all in total
    int DIRECTION;
    int DATA_TYPE;
    int WEIGHT_WIDTH, WEIGHT_ROW, WEIGHT_COL;
    int INPUT_WIDTH, RESULT_WIDTH;
    int CIM_WIDTH, CIM_DEPTH, CIM_SIZE;

    // Cache
    int IS_WIDTH, IS_DEPTH;
    int OS_WIDTH, OS_DEPTH;
    int IS_SIZE, OS_SIZE;

    // Others
    int FREQ;
    int BUS_WIDTH;
    int PIPELINE_STAGES;
} Micro_Arch;

void Init_CIM_Macro(CIM_Macro *macro, int AL, int PC, int SCR, int ICW, int WUW, int WEIGHT_COL, int INPUT_WIDTH, int RESULT_WIDTH, int DIRECTION, int DATA_TYPE, int WEIGHT_WIDTH, int WEIGHT_ROW);
void Init_Micro_Arch(Micro_Arch *arch, CIM_Macro *macro, int MACRO_ROW, int MACRO_COL, int IS_DEPTH, int OS_DEPTH, int FREQ, int BUS_WIDTH, int PIPELINE_STAGES);

void Print_CIM_Macro(const CIM_Macro *CIM_Macro);
void Print_Micro_Arch(const Micro_Arch *Micro_Arch);

#endif // CONFIG_H
