import hw_config as cfg

#area: 0: fd_is, 1: fd_reg, 2: cims, 3: macros_reg, 4: gd_os, 5: gd_reg, 6: top_reg, 7:total_area;


# for FD.IS( width = 512 ), Area(depth) = 111.288 * depth + 114700.7
# for FD.REG, Area(AL) = 68.05828 * al + 1619.94


def area_modeling(acc0):

    area_sin_bp_macro = 205805.549
    total_layout_width = 844.573
    cell_layout_width_scr16 = 86.24


    area = [0 for i in range(8)]
    area[0] = acc0.MACRO_COL * (111.288*acc0.IS_DEPTH + 114700.7)
    area[1] = acc0.AL * 68.05828 + 1619.94

    area[2] = (acc0.MACRO_COL*acc0.MACRO_ROW) * area_sin_bp_macro/total_layout_width * (cell_layout_width_scr16/16 * acc0.SCR + total_layout_width - cell_layout_width_scr16)
    
    area[3] = acc0.AL*acc0.PC * 1.290714 + 1622.281
    area[4] = acc0.MACRO_ROW * (55.680373377*acc0.OS_DEPTH + 115035.87042)
    area[5] = acc0.PC * 1076.61371 + 1581.7796
    area[6] = acc0.BUS_WIDTH * 5.50720922 + 1334.05853
    area[7] = area[0] + area[1] + area[2] + area[3] + area[4] + area[5] + area[6] + area[7] 
    return area 

if __name__ == "__main__":
    # acc0 = cfg.hwc(config = cfg.Config(bus_width = 128,
    #                                is_depth = 2,
    #                                al = 64,
    #                                pc = 8,
    #                                scr = 16,
    #                                os_depth = 4
    #                                ))
    # acc0 = cfg.hwc(config = cfg.Config(bus_width = 256, is_depth = 1024, al = 128, pc = 32, scr = 32, os_depth = 1024))

    macro = cfg.CIMMacro(
        AL=64,
        PC=8,
        SCR=32,
        ICW=64*8,  # 这个值你可以根据需要调整
        WUW=64*2,  # 这个值你可以根据需要调整
        WEIGHT_COL=2,
        INPUT_WIDTH=8,  # 这个值你可以根据需要调整
        RESULT_WIDTH=32,
        DIRECTION=0,  # 这个值你可以根据需要调整
        DATA_TYPE=1,  # 这个值你可以根据需要调整
        WEIGHT_WIDTH=8,  # 这个值你可以根据需要调整
        WEIGHT_ROW=4
    )

    # 初始化 MicroArch 对象
    acc0 = cfg.MicroArch(
        macro=macro,
        MACRO_ROW=4,
        MACRO_COL=4,
        IS_DEPTH=64,
        OS_DEPTH=128,
        FREQ=500,
        BUS_WIDTH=2048,
        PIPELINE_STAGES=8  # 这个值你可以根据需要调整
    )


    for pri in area_modeling(acc0):
        print(pri)






