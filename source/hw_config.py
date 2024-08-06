import math

class CIMMacro:
    def __init__(self, AL, PC, SCR, ICW, WUW, WEIGHT_COL, INPUT_WIDTH, RESULT_WIDTH, DIRECTION, DATA_TYPE, WEIGHT_WIDTH, WEIGHT_ROW):
        self.AL = AL
        self.PC = PC
        self.SCR = SCR
        self.ICW = ICW
        self.WUW = WUW
        self.WEIGHT_COL = WEIGHT_COL
        self.INPUT_WIDTH = INPUT_WIDTH
        self.RESULT_WIDTH = RESULT_WIDTH
        self.DIRECTION = DIRECTION
        self.DATA_TYPE = DATA_TYPE
        self.WEIGHT_WIDTH = WEIGHT_WIDTH
        self.WEIGHT_ROW = WEIGHT_ROW

class MicroArch:
    def __init__(self, macro, MACRO_ROW, MACRO_COL, IS_DEPTH, OS_DEPTH, FREQ, BUS_WIDTH, PIPELINE_STAGES):
        self.MACRO_ROW = MACRO_ROW
        self.MACRO_COL = MACRO_COL
        self.AL = MACRO_COL * macro.AL
        self.PC = MACRO_ROW * macro.PC
        self.SCR = macro.SCR
        self.ICW = MACRO_COL * macro.ICW
        self.WUW = MACRO_COL * macro.WUW
        self.DIRECTION = macro.DIRECTION
        self.DATA_TYPE = macro.DATA_TYPE
        self.WEIGHT_WIDTH = macro.WEIGHT_WIDTH
        self.WEIGHT_ROW = macro.WEIGHT_ROW
        self.WEIGHT_COL = macro.WEIGHT_COL
        self.INPUT_WIDTH = macro.INPUT_WIDTH
        self.RESULT_WIDTH = macro.RESULT_WIDTH
        
        if macro.DIRECTION == 0:
            self.CIM_DEPTH = macro.WEIGHT_ROW * macro.SCR * self.PC
        else:
            self.CIM_WIDTH = macro.WEIGHT_ROW * self.PC
        
        self.CIM_WIDTH = self.WUW
        self.CIM_SIZE = self.CIM_WIDTH * self.CIM_DEPTH

        self.IS_WIDTH = self.ICW
        self.IS_DEPTH = IS_DEPTH
        self.OS_WIDTH = (macro.RESULT_WIDTH + int(math.log2(self.MACRO_COL))) * self.PC
        self.OS_DEPTH = OS_DEPTH
        self.IS_SIZE = self.IS_WIDTH * self.IS_DEPTH
        self.OS_SIZE = self.OS_WIDTH * self.OS_DEPTH

        self.FREQ = FREQ
        self.BUS_WIDTH = BUS_WIDTH
        self.PIPELINE_STAGES = PIPELINE_STAGES

    def print_macro(self, macro):
        print("CIMMacro:")
        print("AL:", macro.AL)
        print("PC:", macro.PC)
        print("SCR:", macro.SCR)
        print("ICW:", macro.ICW)
        print("WUW:", macro.WUW)
        print("WEIGHT_COL:", macro.WEIGHT_COL)
        print("INPUT_WIDTH:", macro.INPUT_WIDTH)
        print("RESULT_WIDTH:", macro.RESULT_WIDTH)
        print("DIRECTION:", macro.DIRECTION)
        print("DATA_TYPE:", macro.DATA_TYPE)
        print("WEIGHT_WIDTH:", macro.WEIGHT_WIDTH)
        print("WEIGHT_ROW:", macro.WEIGHT_ROW)
        print()

    def print_arch(self):
        print("MicroArch:")
        print("MACRO_ROW:", self.MACRO_ROW)
        print("MACRO_COL:", self.MACRO_COL)
        print("AL:", self.AL)
        print("PC:", self.PC)
        print("SCR:", self.SCR)
        print("ICW:", self.ICW)
        print("WUW:", self.WUW)
        print("DIRECTION:", self.DIRECTION)
        print("DATA_TYPE:", self.DATA_TYPE)
        print("WEIGHT_WIDTH:", self.WEIGHT_WIDTH)
        print("WEIGHT_ROW:", self.WEIGHT_ROW)
        print("WEIGHT_COL:", self.WEIGHT_COL)
        print("INPUT_WIDTH:", self.INPUT_WIDTH)
        print("RESULT_WIDTH:", self.RESULT_WIDTH)
        print("CIM_WIDTH:", self.CIM_WIDTH)
        print("CIM_DEPTH:", self.CIM_DEPTH)
        print("CIM_SIZE:", self.CIM_SIZE)
        print("IS_WIDTH:", self.IS_WIDTH)
        print("IS_DEPTH:", self.IS_DEPTH)
        print("OS_WIDTH:", self.OS_WIDTH)
        print("OS_DEPTH:", self.OS_DEPTH)
        print("IS_SIZE:", self.IS_SIZE)
        print("OS_SIZE:", self.OS_SIZE)
        print("FREQ:", self.FREQ)
        print("BUS_WIDTH:", self.BUS_WIDTH)
        print("PIPELINE_STAGES:", self.PIPELINE_STAGES)
        print()
