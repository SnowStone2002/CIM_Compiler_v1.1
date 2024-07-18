// InstructionCount.c
#include <stdio.h>
#include "instruction_count.h"

void printInstructionCount(const InstructionCount* ic) {
    printf("Lin: \t\t%d\n", ic->Lin);
    printf("Linp: \t\t%d\n", ic->Linp);
    printf("Lwt: \t\t%d\n", ic->Lwt);
    printf("Lwtp: \t\t%d\n", ic->Lwtp);
    printf("Cmpfis_aor: \t%d\n", ic->Cmpfis_aor);
    printf("Cmpfis_tos: \t%d\n", ic->Cmpfis_tos);
    printf("Cmpfis_aos: \t%d\n", ic->Cmpfis_aos);
    printf("Cmpfis_ptos: \t%d\n", ic->Cmpfis_ptos);
    printf("Cmpfis_paos: \t%d\n", ic->Cmpfis_paos);
    printf("Cmpfgt_aor: \t%d\n", ic->Cmpfgt_aor);
    printf("Cmpfgt_tos: \t%d\n", ic->Cmpfgt_tos);
    printf("Cmpfgt_aos: \t%d\n", ic->Cmpfgt_aos);
    printf("Cmpfgt_ptos: \t%d\n", ic->Cmpfgt_ptos);
    printf("Cmpfgt_paos: \t%d\n", ic->Cmpfgt_paos);
    printf("Cmpfgtp: \t%d\n", ic->Cmpfgtp);
    printf("Lpenalty: \t%d\n", ic->Lpenalty);
    printf("Nop: \t\t%d\n", ic->Nop);
    printf("Nop_w_rd: \t%d\n", ic->Nop_w_rd);
    printf("IS_reward: \t%d\n", ic->IS_reward);
    printf("L2_reward: \t%d\n", ic->L2_reward);
    printf("Fussion: \t%d\n", ic->Fussion);
}
