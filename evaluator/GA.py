import random
import numpy as np
from sw_decima import Encoder_decision, Decoder_decision_w_kvcache
import hw_config as cfg
import numpy as np
import pickle as pk
import time
from evaluator import evaluate

# 个体表示和初始化
def initialize_population(pop_size, spaces):
    population = []
    for _ in range(pop_size):
        individual = {
            'bus_width': random.choice(spaces['bus_width_space']),
            'is_depth': random.choice(spaces['is_depth_space']),
            'al': random.choice(spaces['al_space']),
            'pc': random.choice(spaces['pc_space']),
            'scr': random.choice(spaces['scr_space']),
            'os_depth': random.choice(spaces['os_depth_space'])
        }
        population.append(individual)
    return population

# 个体生成函数
def generate_individual(individual):
    macro = cfg.CIMMacro(
        AL=64,
        PC=8,
        SCR=individual['scr'],
        ICW=64 * 8,
        WUW=64 * 2,
        WEIGHT_COL=2,
        INPUT_WIDTH=8,
        RESULT_WIDTH=32,
        DIRECTION=0,
        DATA_TYPE=1,
        WEIGHT_WIDTH=8,
        WEIGHT_ROW=4
    )
    acc0 = cfg.MicroArch(
        macro=macro,
        MACRO_ROW=individual['pc']/8,
        MACRO_COL=individual['al']/64,
        IS_DEPTH=individual['is_depth'],
        OS_DEPTH=individual['os_depth'],
        FREQ=500,
        BUS_WIDTH=individual['bus_width'],
        PIPELINE_STAGES=8
    )
    return acc0

# 适应度函数
def fitness_function(individual, seq_len, hidden_size, head_num, num_layers, op_target):
    acc0 = generate_individual(individual)
    return_metric, decision, total_area = Encoder_decision(
        acc0, 
        seq_len=seq_len, 
        hidden_size=hidden_size, 
        head_num=head_num, 
        num_layers=num_layers, 
        op_target=op_target
    )
    if total_area > 7000000:
        return float('0')  # 面积超过7mm^2的个体被判为无穷大
    
    # if return_metric == float('nan'):
    #     return float('-1')  # 处理无效的能量指标

    return return_metric

# 选择
# 选择
def selection(population, fitnesses, num_parents):
    weights = []
    for f in fitnesses:
        # if f == float('inf') or f < 0:
        #     weights.append(0)
        # else:
        #     weights.append(f)
        weights.append(f)

    if sum(weights) == 0:
        # 如果所有权重为零，则随机选择个体
        selected = random.sample(population, num_parents)
    else:
        selected = random.choices(population, weights=weights, k=num_parents)
    
    return selected


# 交叉
def crossover(parent1, parent2):
    child = {}
    for key in parent1.keys():
        child[key] = random.choice([parent1[key], parent2[key]])
    return child

# 变异
def mutate(individual, spaces, mutation_rate):
    for key in individual.keys():
        if random.random() < mutation_rate:
            individual[key] = random.choice(spaces[f'{key}_space'])
    return individual

# 遗传算法主函数
def genetic_algorithm(pop_size, spaces, num_generations, mutation_rate, seq_len, hidden_size, head_num, num_layers, op_target):
    population = initialize_population(pop_size, spaces)
    best_individual = None
    best_fitness = float('0')

    for generation in range(num_generations):
        fitnesses = [fitness_function(individual, seq_len, hidden_size, head_num, num_layers, op_target) for individual in population]

        # 更新最佳个体
        for i, fitness in enumerate(fitnesses):
            if fitness > best_fitness:
                best_fitness = fitness
                best_individual = population[i]

        # 选择父母
        parents = selection(population, fitnesses, pop_size // 2)

        # 生成下一代
        next_population = []
        while len(next_population) < pop_size:
            parent1, parent2 = random.sample(parents, 2)
            child = crossover(parent1, parent2)
            next_population.append(mutate(child, spaces, mutation_rate))

        population = next_population

        print(f'Generation {generation}, Best Fitness: {best_fitness}')

    return best_individual, best_fitness

# 参数设置
spaces = {
    'bus_width_space': [128, 256, 512, 1024, 2048, 4096],
    'is_depth_space': [32, 64, 128, 256, 512, 1024, 2048],
    'al_space': [64, 128, 192, 256, 320, 384, 448, 512],
    'pc_space': [8, 16, 24, 32, 40, 48, 56, 64],
    'scr_space': [4, 8, 16, 32, 64],
    'os_depth_space': [32, 64, 128, 256, 512, 1024, 2048]
}

spaces = {
    'bus_width_space': [2048],
    'is_depth_space': [64],
    'al_space': [256],
    'pc_space': [32],
    'scr_space': [32],
    'os_depth_space': [128]
}

spaces = {
    'bus_width_space': [2048],
    'is_depth_space': [256],
    'al_space': [256],
    'pc_space': [32],
    'scr_space': [4],
    'os_depth_space': [128]
}

pop_size = 10
num_generations = 2
mutation_rate = 0

name = 'bert_large'
seq_len = 512
hidden_size = 1024
head_num = 16
num_layers = 24

for op_target in ['ee_L2','throughput']: 
    # 运行遗传算法
    best_solution, best_solution_fitness = genetic_algorithm(pop_size, spaces, num_generations, mutation_rate, seq_len, hidden_size, head_num, num_layers, op_target)
    print(f'Best Solution: {best_solution}, Best Solution Fitness: {best_solution_fitness}')
    print()
