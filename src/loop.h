/*
 * loop branch predictor
 */
#pragma once

#ifndef LOOP_H_
#define LOOP_H_

#include <stdbool.h>

#define loopCtrBits 10
#define loopMaxCtr ((1 << loopCtrBits) - 1)
#define loopAgeBits 3
#define loopMaxAge ((1 << loopAgeBits) - 1)
#define loopConfBits 3
#define loopMaxConf ((1 << loopConfBits) - 1)

// 16-set

typedef struct Loop_Entry
{
	uint32_t age;    // 3-bit
	uint32_t conf; 	 // 3-bit
    uint32_t p_cnt;  // 10-bit counter, past iter cnt
	uint32_t s_cnt;  // 10-bit counter
    uint32_t tag;    // 10-bit tag
}Loop_Entry;

typedef struct Loop_Meta
{
    uint32_t s_cnt;
	// bool hit;  do not need
	bool invert; // 表示是否反转结果
}Loop_Meta;


typedef struct Loop_Attributes
{
	uint64_t num_row;   // 16项
	uint32_t index_width;   // 4-bit的索引
	uint32_t tag_width;
}Loop_Attributes;

typedef struct LOOP
{
	Loop_Entry* loop_entrys;
	Loop_Attributes attributes;
}LOOP;



/*
 *	Initial the branch prediction table
 *	input	:
 *		index_width	:	the width of index
 *						bimodal:	i_B		gshare:	i_G
 *						hybrid:		(bimodal) i_B	(gshare) i_G
 *						yehpatt:	p
 */
void LOOP_Initial(LOOP* loop, uint32_t index_width, uint32_t tag_width);  // 默认tag_width=10

/*
 *	Search the BranchPredictionTable for entry of "index" and make prediction
 *	input	:
 *		index	:	index of counter
 *	return	:
 *		the prediction on whether branch is taken -- taken or not_taken
 */
Loop_Meta* LOOP_Predict(LOOP* loop_bp, uint64_t unhashed_idx);

/*
 *	Update the BranchPredictionTable
 *	input	:
 *		index	:	index of counter
 *		result	:	struct "Result", the prediction and actual result
 */
void LOOP_Update(LOOP* loop_bp, uint64_t index, Result result);

/*
 * Print the content of BranchPredictionTable to file *fp
 */
void LOOP_fprintf(LOOP* loop, FILE *fp);

#endif