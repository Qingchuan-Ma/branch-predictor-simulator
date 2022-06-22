/*
 *	tage table
 */
#pragma once

#ifndef TAGE_H_
#define TAGE_H_

#include <stdbool.h>

#define tageHistLen 64
#define tageTableNum 6
#define tageUPeriod 1024
#define tageCtrBits 3
#define tageMaxCtr ((1 << tageCtrBits) -1)
#define tageMaxU 3

// 目前假定tage的参数固定

typedef struct TageTable_Entry
{
    bool valid;
    uint32_t ctr;  //  3-bit counter
    uint32_t tag;
}TageTable_Entry;


typedef struct TageTable_Attributes
{
	uint64_t num_row;   // table entry num  = pow(index_width)
	uint32_t index_width;
    uint32_t tag_sz;
    uint32_t hist_len;
    uint32_t u_bit_period;      //有两个，每次会
}TageTable_Attributes;

typedef struct TageTable
{
    //uint32_t* us;
    uint32_t* hi_us;
    uint32_t* lo_us; // 这两个不能合并成us，不好clear
    uint32_t clear_u_ctr;  
    // 这个ctr的长度为 log2(u_bit_period) + log2(num_row) + 1
    // clear_u_idx =  clear_u_ctr >> log2(u_bit_period)
    // 在clear_u_ctr(log2(u_bit_period) + log2(num_rows)) == 1时，hi_us对应的clear_u_idx重置为0
    // 在clear_u_ctr(log2(u_bit_period) + log2(num_rows)) == 0时，lo_us对应的clear_u_idx重置为0；这个细节无法做到cycle级的，认为碰到u_bit_period过期了，就全部给重置为0吧。
	TageTable_Entry* entry;
	TageTable_Attributes attributes;
}TageTable;

typedef struct Tage_Meta
{
    uint32_t provider;
    bool provided;
    bool alt_pred;
    bool provider_pred;
    uint32_t provider_u; // 用于更新u
    uint32_t provider_ctr; // 用于更新ctr
    bool allocatable;
    bool alt_differs;
    uint32_t alloc_entry;
}Tage_Meta;



typedef struct Tage_Attributes
{
    uint32_t tableNum;
}Tage_Attributes;


typedef struct TAGE
{
    TageTable* tage_tables;
    Tage_Attributes attributes;
}TAGE;

/*
 *	Initial the branch prediction table
 *	input	:
 *		index_width	:	the width of index
 *						bimodal:	i_B		gshare:	i_G
 *						hybrid:		(bimodal) i_B	(gshare) i_G
 *						yehpatt:	p
 */
void TAGE_Initial(TAGE* tage); //, uint32_t index_width); 所有参数目前都是固定的，所以没有进行传参

/*
 *	Search the TageTables for entry of "index" and tag match to make prediction
 *	input	:
 *		index	:	index of counter
 *      tag     :   tag of content
 *	return	:
 *		the tage prediction meta info
 */
Tage_Meta* TAGE_Predict(TAGE* tage, uint64_t unhashed_idx, uint64_t ghist);

/*
 *	Update the BranchPredictionTable
 *	input	:
 *		index	:	index of counter
 *		result	:	struct "Result", the prediction and actual result
 */
void TAGE_Update(TAGE* tage, uint64_t unhashed_idx, uint64_t ghist, Result result);



/*
 * Print the content of BranchPredictionTable to file *fp
 */
void TAGE_fprintf(TAGE* tage, FILE *fp);


void TAGE_Clear(TAGE* tage);


#endif