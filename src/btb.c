/*
 *	Branch Target Buffer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "btb.h"


void BTB_Init(uint32_t assoc, uint32_t index_width)
{
	/* first, initial the attributes of BTB */
	branch_target_buffer->attributes.assoc = assoc;
	branch_target_buffer->attributes.set_num = (uint32_t)pow_2(index_width);
	
	branch_target_buffer->attributes.index_width = index_width;
	branch_target_buffer->attributes.tag_width = 62 - index_width;

	/* then, allocate space for sets (including blocks and tag array) */
	branch_target_buffer->set = (Set *)malloc(sizeof(Set) * branch_target_buffer->attributes.set_num);
	if (branch_target_buffer->set == NULL)
		_error_exit("malloc")
	uint32_t i;
	for (i = 0; i < branch_target_buffer->attributes.set_num; i++)
	{
		branch_target_buffer->set[i].block = (BTB_Block *)malloc(sizeof(BTB_Block) * branch_target_buffer->attributes.assoc);
		if (branch_target_buffer->set[i].block == NULL)
			_error_exit("malloc")
		memset(branch_target_buffer->set[i].block, 0, sizeof(BTB_Block) * branch_target_buffer->attributes.assoc);
		branch_target_buffer->set[i].rank = (uint64_t *)malloc(sizeof(uint64_t) * branch_target_buffer->attributes.assoc);
		if (branch_target_buffer->set[i].rank == NULL)
			_error_exit("malloc")
		memset(branch_target_buffer->set[i].rank, 0, sizeof(uint64_t) * branch_target_buffer->attributes.assoc);
	}
}

void Interpret_Address(uint64_t addr, uint32_t *tag, uint32_t *index)
{
	uint32_t tag_width = branch_target_buffer->attributes.tag_width;
	*tag = addr >> (64 - tag_width);
	*index = (addr << tag_width) >> (tag_width + 2);
}

uint32_t Rebuild_Address(uint32_t tag, uint32_t index)
{
	uint64_t addr = 0;
	addr |= (tag << (branch_target_buffer->attributes.index_width + 2));
	addr |= (index << 2);
	return addr;
}

uint32_t BTB_Search(uint64_t tag, uint32_t index)
{
	uint32_t i, k = branch_target_buffer->attributes.assoc;
	for (i = 0; i < branch_target_buffer->attributes.assoc; i++)
		if (branch_target_buffer->set[index].block[i].valid_bit == VALID && branch_target_buffer->set[index].block[i].tag == tag)
		{
			k = i;
			break;
		}
	return k;
}


void Rank_Maintain(uint32_t index, uint32_t way_num, uint64_t rank_value)
{
	branch_target_buffer->set[index].rank[way_num] = rank_value;
#ifdef DBG
	{
		fprintf(debug_fp, "Rank: branch_target_buffer Set %u -- ", index);
		uint32_t i;
		for (i = 0; i < branch_target_buffer->attributes.assoc; i++)
			fprintf(debug_fp, "%llu ", rank[i]);
		fprintf(debug_fp, "\n");
	}
#endif
}

uint32_t Rank_Top(uint32_t index)
{
	uint32_t i, assoc = branch_target_buffer->attributes.assoc;
	/* we first use invalid block location */
	for (i = 0; i < assoc; i++)
		if (branch_target_buffer->set[index].block[i].valid_bit == INVALID)
			return i;
	uint64_t *rank = branch_target_buffer->set[index].rank;
	uint32_t k = 0;
	for (i = 0; i < assoc; i++)
		if (rank[i] < rank[k])
			k = i;
	return k;
}

void BTB_Replacement(uint32_t index, uint32_t way_num, uint32_t tag, uint64_t target)
{
	branch_target_buffer->set[index].block[way_num].valid_bit = VALID;
	branch_target_buffer->set[index].block[way_num].tag = tag;
	branch_target_buffer->set[index].block[way_num].target = target;
#ifdef DBG
	fprintf(debug_fp, "Replacement %lx: branch_target_buffer Set %u, Way %u\n", Rebuild_Address(tag, index), index, way_num);
#endif
}



Branch_Result BTB_Predict(uint64_t addr)
{
	uint32_t tag, index;
	Interpret_Address(addr, &tag, &index);
	Branch_Result result;
	result.target = 0;
	uint32_t way_num = BTB_Search(tag, index);
	if (way_num == branch_target_buffer->attributes.assoc)
	{
		result.is_branch = NOT_BRANCH;
		return result;
	} else
	{
		result.is_branch = BRANCH;
		result.target = branch_target_buffer->set[index].block[way_num].target;
	}

	return result;
}

void BTB_Update(uint64_t addr, Result result, uint64_t rank_value)
{
	uint32_t tag, index, way_num;
	/* if predition is correct */
	if (result.actual_branch.is_branch == result.predict_branch.is_branch && result.actual_branch.target == result.predict_branch.target)
	{
		if (result.actual_branch.is_branch == NOT_BRANCH)
			/* if it is not a branch and predition is correct, we do nothing */
			return;
		/* if it is a branch and predition is correct, we update the LRU bit */
		Interpret_Address(addr, &tag, &index);
		way_num = BTB_Search(tag, index);
	} else if (result.actual_branch.is_branch == result.predict_branch.is_branch && result.actual_branch.target != result.predict_branch.target)
	{
		Interpret_Address(addr, &tag, &index);
		way_num = BTB_Search(tag, index);
		BTB_Replacement(index, way_num, tag, result.actual_branch.target);
	}
	/* if predition is not correct */
	else // 预测错误
	{
		/*
		 * Branch Target Branch is empty at first and branch instr is allocated in BTB
		 * only when it is proved to be a branch, so the misprediction can only be the
		 * case where predition is not a branch but it actually is.
		 * Under this situation, we need to replace block and update LRU bit.
		 */
		Interpret_Address(addr, &tag, &index);
		way_num = Rank_Top(index);
		BTB_Replacement(index, way_num, tag, result.actual_branch.target);
	}
	Rank_Maintain(index, way_num, rank_value);  // rank_value是trace_cnt，就是trace的num，选最远的进行替换
}

void BTB_fprintf(FILE *fp)
{
	fprintf(fp, "Final BTB Tag Array Contents {valid, pc}:\n");
	uint32_t i;
	for (i = 0; i < branch_target_buffer->attributes.set_num; i++)
	{
		fprintf(fp, "Set%6u: ", i);
		uint32_t j;
		for (j = 0; j < branch_target_buffer->attributes.assoc; j++)
		{
			fprintf(fp, "  {%u, 0x%8x}", branch_target_buffer->set[i].block[j].valid_bit, Rebuild_Address(branch_target_buffer->set[i].block[j].tag, i));
		}
		fprintf(fp, "\n");
	}
}
