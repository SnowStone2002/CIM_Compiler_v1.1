from subprocess import run
import hw_config as cfg
import numpy as np
from power_modeling import power_modeling
from area_modeling import area_modeling


# # 初始化 CIMMacro 对象
# macro = cfg.CIMMacro(
#     AL=128,
#     PC=8,
#     SCR=8,
#     ICW=16,  # 这个值你可以根据需要调整
#     WUW=32,  # 这个值你可以根据需要调整
#     WEIGHT_COL=2,
#     INPUT_WIDTH=8,  # 这个值你可以根据需要调整
#     RESULT_WIDTH=32,
#     DIRECTION=0,  # 这个值你可以根据需要调整
#     DATA_TYPE=1,  # 这个值你可以根据需要调整
#     WEIGHT_WIDTH=8,  # 这个值你可以根据需要调整
#     WEIGHT_ROW=4
# )

# # 初始化 MicroArch 对象
# acc0 = cfg.MicroArch(
#     macro=macro,
#     MACRO_ROW=8,
#     MACRO_COL=16,
#     IS_DEPTH=1024,
#     OS_DEPTH=2048,
#     FREQ=500,
#     BUS_WIDTH=256,
#     PIPELINE_STAGES=4  # 这个值你可以根据需要调整
# )

# gli = ("mvm",(64,1024,1024))

def evaluate(acc0, gli, dataflow, bit_energy_L2 = 2):
   types = [
           "Lin", # 0
           "Linp", # 1
           "Lwt", # 2
           "Lwtp", # 3
           "Cmpfis_aor", # 4
           "Cmpfis_tos", # 5
           "Cmpfis_aos", # 6
           "Cmpfis_ptos", # 7
           "Cmpfis_paos", # 8 
           "Cmpfgt_aor", # 9   
           "Cmpfgt_tos", # 10
           "Cmpfgt_aos", # 11
           "Cmpfgt_ptos", # 12
           "Cmpfgt_paos", # 13
           "Cmpfgtp", # 14
           "Lpenalty", # 15
           "Nop", # 16
           "Nop_w_rd", # 17
           "IS_reward", # 18
           "L2_reward", # 19
           "Fussion" # 20
           ]

   metrics = {}

   cmd = (
    "./build/compiler_count.exe "
    + str(acc0.AL) + " "
    + str(acc0.PC) + " "
    + str(acc0.SCR) + " "
    + str(acc0.ICW) + " "
    + str(acc0.WUW) + " "
    + str(acc0.MACRO_ROW) + " "
    + str(acc0.MACRO_COL) + " "
    + str(acc0.IS_DEPTH) + " "
    + str(acc0.OS_DEPTH) + " "
    + str(acc0.FREQ) + " "
    + str(acc0.BUS_WIDTH) + " "
    + str(acc0.PIPELINE_STAGES) + " "
    + str(acc0.DIRECTION) + " "
    + str(acc0.DATA_TYPE) + " "
    + str(acc0.WEIGHT_WIDTH) + " "
    + str(acc0.WEIGHT_ROW) + " "
    + str(acc0.WEIGHT_COL) + " "
    + str(acc0.INPUT_WIDTH) + " "
    + str(acc0.RESULT_WIDTH) + " "
    + "\"" + gli[0] + "\" "
    + str(gli[1][0]) + " "
    + str(gli[1][1]) + " "
    + str(gli[1][2]) + " "
    + "\"" + dataflow + "\""
)

   # print(cmd)
   cres = run(cmd, capture_output=True, text=True).stdout.split() # attention the print format in .c
   # print(cres)
   icnt = np.zeros(21)
   if cres[0] != "ERROR":
      for i in range(21):
         icnt[i] = cres[2*i+1]

      metrics['icnt'] = icnt

      metrics['area'] = area_modeling(acc0) # um2

      ##### energy #####
      power = power_modeling(acc0)
      energy = np.zeros((19,8)) # energy on types[0:17], sum on the last row
      for i in range(18):
         energy[i] = power[i] * icnt[i] * 2 # Unit: pJ
      energy[18] = np.sum(energy[0:19],axis=0) # sum
      same_is_addr_saving = power[4][0]*icnt[18]*2  # power{"Cmpfis_aor","fd_is"} * IS_reward
      energy[18,0] = energy[18,0] - same_is_addr_saving
      energy[18,7] = energy[18,7] - same_is_addr_saving
      metrics['energy'] = energy # pJ
      # print('ebd:',energy[18,:])

      metrics['cycle'] = sum(icnt[0:17])
      if gli[0] == 'proj':
         metrics['op'] = sum(icnt[4:14])*min(acc0.AL, gli[1][1])*min(acc0.PC, gli[1][0])*2
      else:
         #QK phase & PV phase
         metrics['op'] = (sum(icnt[4:14])-icnt[20])*min(acc0.AL, gli[1][1]/gli[1][2])*min(acc0.PC, gli[1][0])*2
         metrics['op'] += (icnt[20])*min(acc0.AL, gli[1][0])*min(acc0.PC, gli[1][1]/gli[1][2])*2
      #metrics['op'] = sum(icnt[4:14])*acc0.AL*acc0.PC*2

      metrics['delay'] = float(metrics['cycle']) * (1000.0/float(acc0.FREQ)) # Unit: ns
      
      metrics['ee_L1'] = float(metrics['op']) / metrics['energy'][18,7] # TOPS/W
      metrics['throughput'] = float(metrics['op']) / metrics['delay'] # GOPS
      metrics['ae_L1'] = float(metrics['throughput']) / metrics['area'][7] *1000 # TOPS/mm2

      # bit_energy_L2 = 1 #pJ/bit
      metrics['read_bits_L2'] = (sum(icnt[0:4]) + sum(icnt[9:16]) - icnt[19] - icnt[20])*acc0.BUS_WIDTH
      metrics['write_bits_L2'] = (sum(icnt[7:9]) + sum(icnt[12:14]) - icnt[20])*acc0.BUS_WIDTH
      energy_L2 = (metrics['read_bits_L2'] + metrics["write_bits_L2"])*bit_energy_L2
      # energy_L2 = metrics['read_bits_L2']*4.3875 + metrics["write_bits_L2"]*1.584375
      metrics['ee_L2'] = metrics['op'] / (metrics['energy'][18,7] + energy_L2)
      metrics['EDP'] = (metrics['energy'][18,7] + energy_L2) * metrics['delay'] / 1e12 # Unit: nJs
      metrics['energy_on_L2'] = energy_L2
      metrics['energy_L2'] = metrics['op']/metrics['ee_L2']

   else:
      pass

   return metrics


if __name__ == "__main__":
   # acc0 = cfg.hwc(config = cfg.Config(bus_width = 256, is_depth = 1024, al = 128, pc = 16, scr = 16, os_depth = 2048))
   # gli = ("proj",(512,1024,1024))

      # 初始化 CIMMacro 对象
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

   gli = ("proj",(64,1024,1024))

   for dataflow in ["isap","ispp","wsap","wspp"]:
      metrics = evaluate(acc0, gli, dataflow)

      print(metrics['ee_L2'])
      # print(metrics['delay'])
      # print(metrics['read_bits_L2'])
      # print(metrics['write_bits_L2'])
   print(metrics['area'])

   mld = evaluate(acc0, ("a2a",(128,768,12)), 'lhd')
   print(mld['ee_L2'])
   print(mld['ee_L1'])
   print(mld['write_bits_L2'])
   print(mld['read_bits_L2'])



# check methods:
# 1. same ops for dataflows
# 2. enough sram: check read/write L2 bits


