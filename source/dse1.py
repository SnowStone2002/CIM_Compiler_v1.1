# from sw_decima import Encoder_decision, Decoder_decision_w_kvcache
import hw_config as cfg
import numpy as np
import pickle as pk
import time
from evaluator import evaluate

bus_width_space = [128, 256, 512, 1024, 2048, 4096]
is_depth_space = [64, 512, 1024]
al_space = [64, 128, 256, 512]
pc_space = [8, 16, 32, 64]
scr_space = [4, 8, 16, 32, 64]
os_depth_space = [128, 1024, 4096]

# 定义要保存文件的路径和文件名
file_path = 'metrics_cycle.log'

# num_point = len(bus_width_space)*len(is_depth_space)*len(al_space)*len(pc_space)*len(scr_space)*len(os_depth_space)
# print(num_point)


# bus_width_space = [1024]
# is_depth_space = [256]
# al_space = [128]
# pc_space = [8]
# scr_space = [32]
# os_depth_space = [256]


# NN paras:
# name = 'bert_large'
name = 'vit_huge'
seq_len = 197
hidden_size = 1280
head_num = 16
num_layers = 32

# filename = "./dse_pks/"+name+"_N"+str(seq_len)+"_dse_strong_"
filename = "./dse_pks/"+name+"_N"+str(seq_len)+"_dse_0415_"

start = time.time()
with open(name+"_N"+str(seq_len)+".log","w") as dlog:    
    dlog.write('dse_start!\n')

for op_target in ['ee_L2','throughput']: 
    pi = 0
    dse_res = []
    for al in al_space:
        for is_depth in is_depth_space:
            for bus_width in bus_width_space:
                if bus_width > al *8:
                    continue
                else:
                    for pc in pc_space:
                        for scr in scr_space:
                            for os_depth in os_depth_space:

                                # 初始化 CIMMacro 对象
                                macro = cfg.CIMMacro(
                                    AL=al,
                                    PC=pc,
                                    SCR=scr,
                                    ICW=al*8,  # 这个值你可以根据需要调整
                                    WUW=al*2,  # 这个值你可以根据需要调整
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
                                    MACRO_ROW=1,
                                    MACRO_COL=1,
                                    IS_DEPTH=is_depth,
                                    OS_DEPTH=os_depth,
                                    FREQ=500,
                                    BUS_WIDTH=bus_width,
                                    PIPELINE_STAGES=8  # 这个值你可以根据需要调整
                                )

                                # return_metric, decision, total_area = Encoder_decision(
                                #                                                 acc0, 
                                #                                                 seq_len = seq_len, 
                                #                                                 hidden_size = hidden_size, 
                                #                                                 head_num = head_num, 
                                #                                                 num_layers = num_layers, 
                                #                                                 op_target = op_target
                                #                                             )
                                gli = ("a2a",(128,768,12))

                                for dataflow in ['ph2']:
                                    metrics = evaluate(acc0, gli, dataflow)

                                    # 使用 open 函数以写模式打开文件，并将数据写入文件
                                    with open(file_path, 'a') as file:
                                        file.write(str(metrics['cycle']))

                                    
                                # dse_res.append([total_area, return_metric, decision, [bus_width, is_depth, al, pc, scr, os_depth]])
                                pi += 1
                                print(op_target+".point:", pi)


    with open(filename + op_target + ".pk", "wb") as f:
        pk.dump(dse_res, f)

    with open(name+"_N"+str(seq_len)+".log","a") as dlog:    
        dlog.write(str(time.time()-start)+'\n')

# FOR DATAFLOW DISSCUSION, BERT_decision -> BLOCK_decision

