from evaluator import evaluate
import hw_config as cfg
import numpy as np

# op_target = ['ee_L2', 'throughput', 'EDP']

bw_ddr = 25.6
# bw_ddr = 100

def find_max_C_W(delay, c_w):
    vol_point_ = np.zeros((7,2)) 
    for i in range(7):
        vol_point_[i,0] = 0 if i == 0 else sum(delay[0:i])
        vol_point_[i,1] = sum(c_w[0:i+1])

    vload = sum(c_w)/sum(delay)
    vin_lpddr = bw_ddr/1e6
    vload = min(vload,vin_lpddr)

    delt = np.zeros(7)
    for i in range(7):
        delt[i] = vol_point_[i,1]/vload - vol_point_[i,0]

    t0 = max(delt)
    voL_L2_ = np.zeros((7,2))
    voL_L2_[:,0] = vol_point_[:,0]+t0
    voL_L2_[:,1] = vol_point_[:,1]-c_w

    L2_Demand = max(voL_L2_[:,0]*vload - voL_L2_[:,1])

    return L2_Demand, vload*1e6, t0




def block_decision(acc0, gli, op_target):
    if gli[0]=='proj':
        metric_rec = [0 for i in range(8)]
        dataflow = ['isap','ispp','wsap','wspp','r_isap','r_ispp','r_wsap','r_wspp']
        metric_rec[0] = evaluate(acc0, gli, 'isap')
        metric_rec[1] = evaluate(acc0, gli, 'ispp')
        metric_rec[2] = evaluate(acc0, gli, 'wsap')
        metric_rec[3] = evaluate(acc0, gli, 'wspp')

        rv_gli = (gli[0],(gli[1][2],gli[1][1],gli[1][0]))

        metric_rec[4] = evaluate(acc0, rv_gli, 'isap')
        metric_rec[5] = evaluate(acc0, rv_gli, 'ispp')
        metric_rec[6] = evaluate(acc0, rv_gli, 'wsap')
        metric_rec[7] = evaluate(acc0, rv_gli, 'wspp')

        cmpme = np.zeros(8)
        for i in range(8):
            cmpme[i] = metric_rec[i][op_target]

        if op_target == 'ee_L2' or op_target == 'throughput':
            return metric_rec[np.argmax(cmpme)], dataflow[np.argmax(cmpme)]
        else:
            return metric_rec[np.argmin(cmpme)], dataflow[np.argmin(cmpme)]

    elif gli[0] == 'a2a':
        return evaluate(acc0, gli, 'lhd'), 'lhd'
        

def Encoder_decision(acc0, seq_len, hidden_size, head_num, num_layers, op_target, check = 0):
    
    m0 = block_decision(acc0, ('proj',(seq_len, hidden_size, hidden_size)), op_target)

    lhd_nfg = evaluate(acc0, ('a2a', (seq_len, hidden_size, head_num)), 'lhd') == {}  
    if lhd_nfg:
        m10 = block_decision(acc0, ('proj',(seq_len, hidden_size/head_num, seq_len)), op_target)
        m11 = block_decision(acc0, ('proj',(hidden_size/head_num, seq_len, seq_len)), op_target)
    else:
        m1 = block_decision(acc0, ('a2a',(seq_len, hidden_size, head_num)), op_target)
    
    # m2 = m0 # concat, m0 * 4
    
    m3 = block_decision(acc0, ('proj',(seq_len, hidden_size, hidden_size*4)), op_target)

    m4 = block_decision(acc0, ('proj',(seq_len, hidden_size*4, hidden_size)), op_target)

    # m0*4, m10,m11*12 / m1*1, m3*1, m4*1
    if lhd_nfg:
        total_energy = num_layers*(m0[0]['energy_L2']*4 + (m10[0]['energy_L2'] + m11[0]['energy_L2'])*head_num + m3[0]['energy_L2'] + m4[0]['energy_L2'])
        total_op = num_layers*(m0[0]['op']*4 + (m10[0]['op'] + m11[0]['op'])*head_num + m3[0]['op'] + m4[0]['op'])
        total_delay = num_layers*(m0[0]['delay']*4 + (m10[0]['delay'] + m11[0]['delay'])*head_num + m3[0]['delay'] + m4[0]['delay'])
    else:
        total_energy = num_layers*(m0[0]['energy_L2']*4 + m1[0]['energy_L2'] + m3[0]['energy_L2'] + m4[0]['energy_L2'])
        total_op = num_layers*(m0[0]['op']*4 + m1[0]['op'] + m3[0]['op'] + m4[0]['op'])
        total_delay = num_layers*(m0[0]['delay']*4 + m1[0]['delay'] + m3[0]['delay'] + m4[0]['delay'])
    
    total_ee_L2 = total_op/total_energy
    total_throughput = total_op/total_delay
    total_EDP = total_energy * total_delay /1e12
    total_area = m0[0]['area'][7]

    if check:
        # print('te:',total_energy/1e6/512)
        # print('delay:',total_delay/1e6)
        if lhd_nfg:
            energy_L1 = num_layers*(m0[0]['energy'][18,7]*4 + (m10[0]['energy'][18,7] + m11[0]['energy'][18,7])*head_num + m3[0]['energy'][18,7] + m4[0]['energy'][18,7])
        else:
            energy_L1 = num_layers*(m0[0]['energy'][18,7]*4 + m1[0]['energy'][18,7] + m3[0]['energy'][18,7] + m4[0]['energy'][18,7])

        print('ee_L2:', total_ee_L2)
        print('ee_L1:', total_op/energy_L1)
        print('throughput density:',total_throughput/total_area*1e6)

        # 
        C_gen_W = (hidden_size*hidden_size)/1024/1024
        C_gen_A = (seq_len*hidden_size + seq_len*hidden_size)/1024/1024
        delay_gen = m0[0]['delay']
        
        # x3 times

        C_dm_W = 0
        C_dm_A = (seq_len*hidden_size*4)/1024/1024
        delay_dm = (m10[0]['delay']+m11[0]['delay'])*head_num if lhd_nfg else m1[0]['delay']

        C_concat_W = (hidden_size*hidden_size)/1024/1024
        C_concat_A = (seq_len*hidden_size + seq_len*hidden_size)/1024/1024
        delay_concat = delay_gen

        C_ffn0_W = (hidden_size*hidden_size*4)/1024/1024
        C_ffn0_A = (seq_len*hidden_size + seq_len*hidden_size*4)/1024/1024
        delay_ffn0 = m3[0]['delay']

        C_ffn1_W = (hidden_size*hidden_size*4)/1024/1024
        C_ffn1_A = (seq_len*hidden_size*4 + seq_len*hidden_size)/1024/1024
        delay_ffn1 = m4[0]['delay']

        C_Layer = (hidden_size*hidden_size*4*3)/1024/1024

        print(delay_gen, delay_dm, delay_concat, delay_ffn0, delay_ffn1)
        print(C_gen_W, C_dm_W, C_concat_W, C_ffn0_W, C_ffn1_W)
        delay = np.array([delay_gen, delay_gen, delay_gen, delay_dm, delay_concat, delay_ffn0, delay_ffn1])
        c_w = np.array([C_gen_W, C_gen_W, C_gen_W, C_dm_W, C_concat_W, C_ffn0_W, C_ffn1_W])

        L2_W, v_ult, t0 = find_max_C_W(delay,c_w)
        L2_A = max(C_gen_A, C_dm_A, C_concat_A, C_ffn0_A, C_ffn1_A)

        print('L2_Demand', L2_A + L2_W)
        print('v_DRAM_ult (GBs)', v_ult)
        Npara = C_Layer*num_layers
        cdelay = C_Layer*num_layers/bw_ddr
        print('Network Parameters(MB)', Npara)
        print('Constrained Delay(ms)', cdelay)
        print('op:',total_op)
        print('Constrained Throughput(GOPS):',total_op/cdelay/1e6)

        print('uarch paras(paper format):' ,(acc0.BUS_WIDTH, acc0.AL, acc0.PC, acc0.IS_DEPTH, acc0.OS_DEPTH, acc0.SCR))

        print('3D bw (Gbs):', acc0.BUS_WIDTH/2 + v_ult*8 )
        print('delay', total_delay/1e6)


    return_metric = 0
    if op_target == 'ee_L2': return_metric = total_ee_L2
    elif op_target == 'throughput': return_metric = total_throughput
    elif op_target == 'EDP': return_metric = total_EDP
    else: print("?")
    
    if lhd_nfg:
        decision = [m0[1],m10[1],m11[1],m0[1],m3[1],m4[1]]
    else:
        decision = [m0[1],m1[1],m0[1],m3[1],m4[1]]
    
    return return_metric, decision, total_area

def Decoder_decision_w_kvcache(acc0, seq_len, hidden_size, head_num, num_layers, op_target, check=0):

    total_energy = 0
    total_energy_L1 = 0
    total_op = 0
    total_delay = 0

    for i in range(seq_len):
        kv_cache = i
        m0 = block_decision(acc0, ('proj',(1, hidden_size, hidden_size)), op_target)
    
        m10 = block_decision(acc0, ('proj',(1, hidden_size/head_num, kv_cache+1)), op_target)
        m11 = block_decision(acc0, ('proj',(1, kv_cache+1, hidden_size/head_num)), op_target)

        # m2 = m0

        m3 = block_decision(acc0, ('proj',(1, hidden_size, hidden_size*4)), op_target)
    
        m4 = block_decision(acc0, ('proj',(1, hidden_size*4, hidden_size)), op_target)

        total_energy += num_layers*(m0[0]['energy_L2']*4 + (m10[0]['energy_L2'] + m11[0]['energy_L2'])*head_num + m3[0]['energy_L2'] + m4[0]['energy_L2'])
        total_op += num_layers*(m0[0]['op']*4 + (m10[0]['op'] + m11[0]['op'])*head_num + m3[0]['op'] + m4[0]['op'])
        total_delay += num_layers*(m0[0]['delay']*4 + (m10[0]['delay'] + m11[0]['delay'])*head_num + m3[0]['delay'] + m4[0]['delay'])
        
        #print(total_delay/1e6)

        if check:
            # print('te:',total_energy/1e6/512)
            # print('delay:',total_delay/1e6)
            energy_L1 = num_layers*(m0[0]['energy'][18,7]*4 + (m10[0]['energy'][18,7] + m11[0]['energy'][18,7])*head_num + m3[0]['energy'][18,7] + m4[0]['energy'][18,7])
            
            total_energy_L1 += energy_L1
            if i == 0:
                # 
                C_gen_W = (hidden_size*hidden_size)/1024/1024
                C_gen_A = (1*hidden_size + 1*hidden_size)/1024/1024
                delay_gen = m0[0]['delay']

                # x3 times

                C_dm_W = 0
                C_dm_A = (1*hidden_size*4)/1024/1024
                delay_dm = (m10[0]['delay']+m11[0]['delay'])*head_num

                C_concat_W = (hidden_size*hidden_size)/1024/1024
                C_concat_A = (1*hidden_size + 1*hidden_size)/1024/1024
                delay_concat = delay_gen

                C_ffn0_W = (hidden_size*hidden_size*4)/1024/1024
                C_ffn0_A = (1*hidden_size + 1*hidden_size*4)/1024/1024
                delay_ffn0 = m3[0]['delay']

                C_ffn1_W = (hidden_size*hidden_size*4)/1024/1024
                C_ffn1_A = (1*hidden_size*4 + 1*hidden_size)/1024/1024
                delay_ffn1 = m4[0]['delay']

                C_Layer = (hidden_size*hidden_size*4*3)/1024/1024

                print(delay_gen, delay_dm, delay_concat, delay_ffn0, delay_ffn1)
                print(C_gen_W, C_dm_W, C_concat_W, C_ffn0_W, C_ffn1_W)
                delay = np.array([delay_gen, delay_gen, delay_gen, delay_dm, delay_concat, delay_ffn0, delay_ffn1])
                c_w = np.array([C_gen_W, C_gen_W, C_gen_W, C_dm_W, C_concat_W, C_ffn0_W, C_ffn1_W])

                L2_W, v_ult, t0 = find_max_C_W(delay,c_w)
                L2_A = max(C_gen_A, C_dm_A, C_concat_A, C_ffn0_A, C_ffn1_A)
                L2_KV_Cache = seq_len*hidden_size*num_layers/1024/1024 *2

                print('L2_Demand', L2_A + L2_W + L2_KV_Cache)
                print('v_DRAM_ult', v_ult)
                Npara = C_Layer*num_layers
                cdelay = C_Layer*num_layers/bw_ddr
                print('Network Parameters(MB)', Npara)
                print('Constrained Delay(ms)', cdelay, sum(delay)/1e6)
                print('op:',total_op)
                print('Constrained Throughput(GOPS):',total_op/cdelay/1e6)
            
            else:
                print(i)


    total_area = m0[0]['area'][7]
    
    total_ee_L2 = total_op/total_energy
    total_throughput = total_op/total_delay
    total_EDP = total_energy * total_delay /1e12
    
    #print('ee_L2:', total_ee_L2)
    #print('ee_L1:', total_op/total_energy_L1)
    #print('throughput density:',total_throughput/total_area*1e6)

    #print('3D bw (Gbs):', acc0.BusWidth/2 + v_ult*8 )
    #print('delay', total_delay/1e6)



    return_metric = 0
    if op_target == 'ee_L2': return_metric = total_ee_L2
    elif op_target == 'throughput': return_metric = total_throughput
    elif op_target == 'EDP': return_metric = total_EDP
    else: print("?")

    return return_metric, total_area

def Decoder_decision_wo_kvcache(acc0, seq_len, hidden_size, head_num, num_layers, op_target):

    total_energy = 0
    total_op = 0
    total_delay = 0

    for i in range(seq_len):
        m0 = block_decision(acc0, ('proj',(i+1, hidden_size, hidden_size)), op_target)
    
        m10 = block_decision(acc0, ('proj',(i+1, hidden_size/head_num, i+1)), op_target)
        m11 = block_decision(acc0, ('proj',(i+1, i+1, hidden_size/head_num)), op_target)

        # m2 = m0

        m3 = block_decision(acc0, ('proj',(i+1, hidden_size, hidden_size*4)), op_target)
    
        m4 = block_decision(acc0, ('proj',(i+1, hidden_size*4, hidden_size)), op_target)

        total_energy += num_layers*(m0[0]['energy_L2']*4 + (m10[0]['energy_L2'] + m11[0]['energy_L2'])*head_num + m3[0]['energy_L2'] + m4[0]['energy_L2'])
        total_op += num_layers*(m0[0]['op']*4 + (m10[0]['op'] + m11[0]['op'])*head_num + m3[0]['op'] + m4[0]['op'])
        total_delay += num_layers*(m0[0]['delay']*4 + (m10[0]['delay'] + m11[0]['delay'])*head_num + m3[0]['delay'] + m4[0]['delay'])
        
        #print(total_delay/1e6)
    
    total_area = m0[0]['area'][7]

    total_ee_L2 = total_op/total_energy
    total_throughput = total_op/total_delay
    total_EDP = total_energy * total_delay /1e12
    
    return_metric = 0
    if op_target == 'ee_L2': return_metric = total_ee_L2
    elif op_target == 'throughput': return_metric = total_throughput
    elif op_target == 'EDP': return_metric = total_EDP
    else: print("?")

    return return_metric, total_area


if __name__ == "__main__":
    # gli = ('proj',(512,1024,1024))

    
    macro = cfg.CIMMacro(
        AL=128,
        PC=8,
        SCR=8,
        ICW=16,  # 这个值你可以根据需要调整
        WUW=32,  # 这个值你可以根据需要调整
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
        MACRO_ROW=8,
        MACRO_COL=16,
        IS_DEPTH=1024,
        OS_DEPTH=2048,
        FREQ=500,
        BUS_WIDTH=256,
        PIPELINE_STAGES=4  # 这个值你可以根据需要调整
    )

    
    # acc0 = cfg.hwc(config = cfg.Config(bus_width = 128, is_depth = 128, al = 64, pc = 8, scr = 8, os_depth = 256))

    # meme = block_decision(acc0, gli, 'ee_L2')
    # for content in meme[0]:
    #     print(content,":",meme[0][content])
    # print(meme[1])

    print(Encoder_decision(acc0, seq_len=128, hidden_size=1024, head_num=16, num_layers=24, op_target='throughput', check=1))

    # print(Decoder_decision_w_kvcache(acc0, seq_len=64, hidden_size=1600, head_num=25, num_layers=48, op_target='throughput', check=1))







