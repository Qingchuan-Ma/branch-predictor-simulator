/*
 *	globle branch history register
 */

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "ghr.h"


void GHR_Initial(GHR *GlobalBranchHistoryRegister, uint32_t history_width)
{
	GlobalBranchHistoryRegister->attributes.history_width = history_width;
	GlobalBranchHistoryRegister->history = 0;
}

void GHR_Update(GHR *GlobalBranchHistoryRegister, Result result)
{
	uint32_t old_history = GlobalBranchHistoryRegister->history;
	old_history = old_history << 1;
	if (result.actual_taken == TAKEN)
		old_history = old_history | 0x01;
	GlobalBranchHistoryRegister->history = old_history;
}

void GHR_fprintf(GHR *GlobalBranchHistoryRegister, FILE *fp)
{
	fprintf(fp, "0x\t\t%llx\n", GlobalBranchHistoryRegister->history);
}

uint64_t GHR_Get_Len(GHR *GlobalBranchHistoryRegister,  uint32_t len)
{
	return GlobalBranchHistoryRegister->history & ((1 << len) - 1);
}