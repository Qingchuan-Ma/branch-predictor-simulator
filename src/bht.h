/*
 *	branch history table
 */
#pragma once

#ifndef BHT_H_
#define BHT_H_

typedef struct BHT_Attributes
{
	uint32_t history_num;     	// pow_2(index_width)
	uint32_t index_width; 
	uint32_t history_width;      
	uint64_t history_one;		// pow_2(history_width-1)
}BHT_Attributes;

typedef struct BHT
{
	uint64_t* history;
	BHT_Attributes attributes;
}BHT;

/*
 *	Initial the branch history table
 * input	:
 *		index_width		:	the width of index in PC addr to index the branch history table
 *							(yehpatt) h
 *		history_width	:	the width of the history pattern (entry in the branch history table)
 *							(yehpatt) p
 */
void BHT_Initial(BHT *BranchHistoryTable, uint32_t index_width, uint32_t history_width);

/*
 *	Search the BranchPredictionTable for PC "addr" and make prediction
 *	input	:
 *		addr	:	PC
 *	return	:
 *		the history pattern of correspounding entry (at most 64 bits, hence uint64_t)
 */
uint64_t BHT_Search(BHT *BranchHistoryTable, uint64_t addr);

/*
 *	Update the BranchPredictionTable
 *	input	:
 *		addr	:	PC
 *		result	:	struct "Result", the prediction and actual result
 */
void BHT_Update(BHT *BranchHistoryTable, uint64_t addr, Result result);

/*
 * Print the content of BranchHistoryTable to file *fp
 */
void BHT_fprintf(BHT *BranchHistoryTable, FILE *fp);

#endif