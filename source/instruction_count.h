// InstructionCount.h

#ifndef INSTRUCTION_COUNT_H
#define INSTRUCTION_COUNT_H

typedef struct {
    int Lin;
    int Linp;
    int Lwt;
    int Lwtp;
    int Cmpfis_aor;
    int Cmpfis_tos;
    int Cmpfis_aos;
    int Cmpfis_ptos;
    int Cmpfis_paos;
    int Cmpfgt_aor;
    int Cmpfgt_tos;
    int Cmpfgt_aos;
    int Cmpfgt_ptos;
    int Cmpfgt_paos;
    int Cmpfgtp;
    int Lpenalty;
    int Nop;
    int Nop_w_rd;
    int IS_reward;
    int L2_reward;
    int Fussion;
} InstructionCount;

void printInstructionCount(const InstructionCount* ic);

#endif // INSTRUCTION_COUNT_H
