/*
 *	tage table
 */
#pragma once

#ifndef SC_H_
#define SC_H_

#include <stdbool.h>
#include "tage.h"

#define scHistLen 64
#define scTableNum 4

#define scMaxThres 31 // 8bit
#define scMinThres 6
#define scInitThres 6

#define scThresCtrBits 6 // 6bit
#define scMaxThresCtr ((1 << scThresCtrBits) - 1)   // 32bit
#define scInitThresCtr (1 << (scThresCtrBits - 1))   // 16

// 目前假定tage的参数固定

typedef struct SCTable_Entry
{
    //bool valid;
    int32_t taken_ctr;  //  6-bit signed counter for tage-taken branch
    int32_t nontaken_ctr;  // 6-bit signed counter for tage-nontaken branch
    // uint32_t tag;  // 没有tag
}SCTable_Entry;


typedef struct SCTable_Attributes
{
	uint64_t num_row;   // table entry num  = pow(index_width)
	uint32_t index_width;
    uint32_t hist_len;  // 该table需要的hist_len
    uint32_t ctr_bits;  // 默认是6
}SCTable_Attributes;


typedef struct SCThreshold
{
    uint32_t ctr; // 用于动态更新threshold的5bit ctr
    uint32_t thres; // 8-bit
}SCThreshold;

typedef struct SCTable
{
    // uint32_t* us;
    SCTable_Entry* entry;
	SCTable_Attributes attributes;
}SCTable;

typedef struct SC_Meta
{
    bool tage_taken;
    bool sc_used;
    bool sc_pred;
    int32_t ctrs[scTableNum];
}SC_Meta;



typedef struct SC_Attributes
{
    uint32_t tableNum;
}SC_Attributes;


typedef struct SC
{
    SCThreshold scThreshold;
    SCTable* sc_tables;
    SC_Attributes attributes;
}SC;

/*
 *	Initial the branch prediction table
 *	input	:
 *		index_width	:	the width of index
 *						bimodal:	i_B		gshare:	i_G
 *						hybrid:		(bimodal) i_B	(gshare) i_G
 *						yehpatt:	p
 */
void SC_Initial(SC* sc); // , uint32_t index_width); // 可以指定SC的row_num, 现在假定所有table的index_width都相同

/*
 *	Search the TageTables for entry of "index" and tag match to make prediction
 *	input	:
 *		index	:	index of counter
 *      tag     :   tag of content
 *	return	:
 *		the tage prediction meta info
 */
SC_Meta* SC_Predict(SC* sc, uint64_t unhashed_idx, uint64_t ghist, Tage_Meta* tage_meta);

/*
 *	Update the BranchPredictionTable
 *	input	:
 *		index	:	index of counter
 *		result	:	struct "Result", the prediction and actual result
 */

void SC_Update(SC* sc, uint64_t unhashed_idx, uint64_t ghist, Result result);



/*
 * Print the content of BranchPredictionTable to file *fp
 */
void TAGE_fprintf(TAGE* tage, FILE *fp);


void TAGE_Clear(TAGE* tage);


#endif