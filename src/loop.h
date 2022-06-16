/*
 * loop branch predictor
 */
#pragma once

#ifndef LOOP_H_
#define LOOP_H_

#include <stdbool.h>


typedef struct LOOP_Attributes
{
	uint64_t counter_num;   // table entry num  = pow(index_width)
	uint32_t index_width;   
}LOOP_Attributes;

typedef struct LOOP
{
	__uint32_t* counter;
	LOOP_Attributes attributes;
}LOOP;

/*
 *	Initial the branch prediction table
 *	input	:
 *		index_width	:	the width of index
 *						bimodal:	i_B		gshare:	i_G
 *						hybrid:		(bimodal) i_B	(gshare) i_G
 *						yehpatt:	p
 */
void LOOP_Initial(LOOP* loop_bp, uint32_t index_width);

/*
 *	Search the BranchPredictionTable for entry of "index" and make prediction
 *	input	:
 *		index	:	index of counter
 *	return	:
 *		the prediction on whether branch is taken -- taken or not_taken
 */
Taken_Result LOOP_Predict(LOOP* loop_bp, uint64_t index);

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
void LOOP_fprintf(LOOP* loop_bp, FILE *fp);

#endif