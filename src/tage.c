/*
 *	tage table
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "utils.h"
#include "tage.h"

/* 以下是BOOM的参数输入，目前这些参数不太好传入进去，就先固定下来
                                               nSets, histLen, tagSz
   tageTableInfo Seq[Tuple3[Int, Int, Int]] = Seq((  128,       2,     7),
                                              (  128,       4,     7),
                                              (  256,       8,     8),
                                              (  256,      16,     8),
                                              (  128,      32,     9),
                                              (  128,      64,     9)),
*/

uint32_t tageTableInfo[tageTableNum][3] = {
    // nSets, histLen, tagSz
    {  128,       2,     7},
    {  128,       4,     7},
    {  256,       8,     8},
    {  256,      16,     8},
    {  128,      32,     9},
    {  128,      64,     9}
};

// typedef struct TageTable_Attributes
// {
// 	uint64_t num_row;   // table entry num  = pow(index_width)
// 	uint32_t index_width;
//     uint32_t tag_sz;
//     uint32_t hist_len;
//     uint32_t u_bit_period;      //有两个，每次会
// }TageTable_Attributes;


// typedef struct TageTable
// {
//     uint32_t* hi_us;
//     uint32_t* lo_us;
//     uint32_t clear_u_ctr;  
//     // 这个ctr的长度为 log2(u_bit_period) + log2(num_row) + 1
//     // clear_u_idx =  clear_u_ctr >> log2(u_bit_period)
//     // 在clear_u_ctr(log2(u_bit_period) + log2(num_rows)) == 1时，hi_us对应的clear_u_idx重置为0
//     // 在clear_u_ctr(log2(u_bit_period) + log2(num_rows)) == 0时，lo_us对应的clear_u_idx重置为0；这个细节无法做到cycle级的，认为碰到u_bit_period过期了，就全部给重置为0吧。
// 	TageTable_Entry* entry;
// 	TageTable_Attributes attributes;
// }TageTable;

// typedef struct TageTable_Entry
// {
//     uint32_t valid;
//     uint32_t ctr;  //  3-bit counter
//     uint32_t tag;
// }TageTable_Entry;

// typedef struct Tage_Meta
// {
//     int32_t provider;
//     uint32_t provided;
//     uint32_t provider_pred;
//     uint32_t provider_u; // 用于更新u
//     uint32_t provider_ctr; // 用于更新ctr
// }Tage_Meta;



void TAGE_Initial(TAGE* tage) //, uint32_t index_width)
{

    tage->attributes.tableNum = tageTableNum;
    tage->tage_tables = (TageTable*)malloc(sizeof(TageTable) * tageTableNum);
    
    for (size_t i = 0; i < tageTableNum; i++)
    {
        int num_row = tageTableInfo[i][0];
        tage->tage_tables[i].attributes.index_width = (uint32_t)log2(num_row);
        tage->tage_tables[i].attributes.num_row = tageTableInfo[i][0];
        tage->tage_tables[i].attributes.hist_len = tageTableInfo[i][1];
        tage->tage_tables[i].attributes.tag_sz = tageTableInfo[i][2];
        tage->tage_tables[i].attributes.u_bit_period = tageUPeriod;
        tage->tage_tables[i].hi_us = (uint32_t*)malloc(sizeof(uint32_t) * num_row);
        tage->tage_tables[i].lo_us = (uint32_t*)malloc(sizeof(uint32_t) * num_row);
        tage->tage_tables[i].clear_u_ctr = 0;
        tage->tage_tables[i].entry = (TageTable_Entry *)malloc(sizeof(TageTable_Entry) * num_row);
        
        for (size_t j = 0; j < num_row; j++)
        {
            tage->tage_tables[i].hi_us[j] = 0;
            tage->tage_tables[i].lo_us[j] = 0;
            tage->tage_tables[i].entry[j].valid = false;
            tage->tage_tables[i].entry[j].ctr = 4;
            tage->tage_tables[i].entry[j].tag = 0;
        }   
    }
    return;
}


typedef struct Tage_Tag_Hash
{
    uint32_t tag;
    uint32_t hashed_idx;
}Tage_Tag_Hash;


uint32_t TAGE_Compute_Folded_Hist(uint64_t ghist, uint32_t hist_len, uint32_t folded_len) // folded_len = log2(nrows)
{
    uint32_t nChunks = (hist_len + folded_len - 1) / folded_len;
    uint32_t res = 0;
    for (size_t i = 0; i < nChunks; i++)
    {
        res = res ^ ((ghist >> i*folded_len) & ((1 << folded_len)-1));
    }
    return res;
}

uint32_t TAGE_Compute_Tag(uint64_t unhashed_idx, uint64_t ghist, uint32_t table_index)
{
    uint32_t hist_len = tageTableInfo[table_index][1];
    uint32_t folded_len = tageTableInfo[table_index][2];
    return ((unhashed_idx >> folded_len) ^ TAGE_Compute_Folded_Hist(ghist, hist_len, folded_len)) & ((1 << folded_len)-1);
}

uint32_t TAGE_Compute_Index(uint64_t unhashed_idx, uint64_t ghist, uint32_t table_index)
{
    uint32_t hist_len = tageTableInfo[table_index][1];
    uint32_t folded_len = (uint32_t) log2(tageTableInfo[table_index][0]);
    return (unhashed_idx ^ TAGE_Compute_Folded_Hist(ghist, hist_len, folded_len)) & ((1 << folded_len)-1);
}

Tage_Meta* TAGE_Predict(TAGE* tage, uint64_t unhashed_idx, uint64_t ghist)
{
    int32_t provider = -1;
    bool provided = false;
    uint32_t provider_pred = 0;
    uint32_t provider_u = 0;
    uint32_t provider_ctr = 0;
    Tage_Meta* tage_meta = (Tage_Meta *)malloc(sizeof(Tage_Meta));
    bool allocatable_slots[tageTableNum];


    for (size_t i = 0; i < tageTableNum; i++)
    {
        uint32_t index = TAGE_Compute_Index(unhashed_idx, ghist, i);
        uint32_t tag = TAGE_Compute_Tag(unhashed_idx, ghist, i);

		#ifdef MyDBG2
		printf("is here stuck? tableidx: %d, index: %d\n", i, index);
		#endif

        bool hit = tage->tage_tables[i].entry[index].tag == tag;
        
        

        if(tage->tage_tables[i].entry[index].valid && hit)  // 不用管是否useful
        {
            provided = provided | true;
            provider = i;
            provider_pred = tage->tage_tables[i].entry[index].ctr >= 4;
            provider_u = 2 * tage->tage_tables[i].hi_us[index] + tage->tage_tables[i].lo_us[index];
            provider_ctr = tage->tage_tables[i].entry[index].ctr;
        }

        
        if(!tage->tage_tables[i].entry[index].valid && tage->tage_tables[i].hi_us[index] == 0 && tage->tage_tables[i].lo_us[index] == 0)
        {
            allocatable_slots[i] = true; //不可能是provider
            #ifdef MyDBG1
            printf("tage can allocate a entry\n");
            #endif
        }
        else
        {
            allocatable_slots[i] = false;
        }
    }
    
    bool mask_slots[tageTableNum];
    bool allocatable = false;
    uint32_t first_entry = 0;
    for (size_t i = 0; i < tageTableNum; i++)
    {
        if(provided && i < provider)
        {
            mask_slots[i] = 1;
        }
        else
        {
            mask_slots[i] = 0;
        }
        allocatable_slots[i] = !mask_slots[i] && allocatable_slots[i];
        if(allocatable_slots[i])
        {
            allocatable = true;
            first_entry = (uint32_t) i;
        }
    }

    uint32_t lfsr_entry = rand() % tageTableNum;

    uint32_t alloc_entry = allocatable_slots[lfsr_entry] ? lfsr_entry : first_entry;


    tage_meta->provider_pred = provider_pred;
    tage_meta->provided = provided;
    tage_meta->provider = provider;
    tage_meta->provider_u = provider_u;
    tage_meta->provider_ctr = provider_ctr;
    // alt_differs这里不可知
    tage_meta->allocatable = allocatable;
    tage_meta->alloc_entry = alloc_entry;

		
		#ifdef MyDBG1
		// typedef struct Tage_Meta
		// {
		// 	int32_t provider;
		// 	uint32_t provided;
		// 	uint32_t provider_pred;
		// 	uint32_t provider_u; // 用于更新u
		// 	uint32_t provider_ctr; // 用于更新ctr
		// 	bool allocatable;
		// 	bool alt_differs;
		// 	uint32_t alloc_entry;
		// }Tage_Meta;

		printf("tage_meta: provided: %d, provider: %d, provider_pred: %d, provider_u: %d, provider_ctr: %d, allocatable: %d, alloc_entry: %d, first_entry: %d\n", 
				tage_meta->provided,
				tage_meta->provider,
				tage_meta->provider_pred,
				tage_meta->provider_u,
				tage_meta->provider_ctr,
				tage_meta->allocatable,
				tage_meta->alloc_entry,
                first_entry);
		#endif

    // 硬件的wrbypass不需要实现，因为非cycle级没有延迟更新
    return tage_meta;
}


void TAGE_Clear(TAGE* tage)
{
    // true is hi, false is low
    static bool clear_u_hi = true;
    for (size_t i = 0; i < tageTableNum; i++)
    {
        int num_row = tageTableInfo[i][0];
        if(clear_u_hi)
        {
            memset(tage->tage_tables[i].hi_us, 0, sizeof(uint32_t) * num_row);
        } else 
        {
            memset(tage->tage_tables[i].lo_us, 0, sizeof(uint32_t) * num_row);
        }
    }

    clear_u_hi = !clear_u_hi;
    return;
}

static uint32_t TAGE_U_Update(bool alt_differ, bool mispredict, uint32_t old_u)
{
    return !alt_differ ? old_u : mispredict ? (old_u == 0 ? 0 : old_u - 1) :
                                     old_u == tageMaxU ? tageMaxU : old_u + 1;
}

// static uint32_t TAGE_Ctr_Update(bool is_taken, uint32_t old_ctr)
// {
//     return !is_taken ? (old_ctr == 0 ? 0 : old_ctr - 1) : (old_ctr == tageMaxCtr ? tageMaxCtr : old_ctr + 1);
// }



void TAGE_Update(TAGE* tage, uint64_t unhashed_idx, uint64_t ghist, Result result)
{
    bool mispredict = result.actual_taken != result.predict_taken[BOOM_TAGE];
    Tage_Meta* tage_meta = result.meta_info[BOOM_TAGE];

    if(tage_meta->provided) // 这里是更新provider
    {
        uint32_t provider = tage_meta->provider;
        uint32_t index = TAGE_Compute_Index(unhashed_idx, ghist, provider);
        uint32_t provider_u = tage_meta->provider_u;
        uint32_t provider_new_u = TAGE_U_Update(tage_meta->alt_differs, mispredict, provider_u);
        uint32_t provider_ctr = tage_meta->provider_ctr;
        uint32_t provider_new_ctr = Saturate_Inc_UCtr(provider_ctr, tageMaxCtr, result.actual_taken == TAKEN);

        tage->tage_tables[provider].hi_us[index] = (provider_new_u & ~0x02) >> 1;
        tage->tage_tables[provider].lo_us[index] = provider_new_u & ~0x01;
        tage->tage_tables[provider].entry[index].ctr = provider_new_ctr;
    }

    if(mispredict && tage_meta->allocatable) // 这里是分配新项，provider和alloc_idx这两个不可能是同一个
    {
        uint32_t alloc_idx = tage_meta->alloc_entry;
        uint32_t alloc_new_u = 0;
        uint32_t alloc_new_ctr = result.actual_taken == TAKEN ? (tageMaxCtr+1)/2 : (tageMaxCtr+1)/2-1;
        uint32_t index = TAGE_Compute_Index(unhashed_idx, ghist, alloc_idx);
        uint32_t tag = TAGE_Compute_Tag(unhashed_idx, ghist, alloc_idx);

        #ifdef MyDBG1
        printf("there exists a misprediction, allocate a new entry, idx: %d, u: %d, ctr: %d, index: %d\n", alloc_idx, alloc_new_u, alloc_new_ctr, index);
        #endif


        tage->tage_tables[alloc_idx].hi_us[index] = (alloc_new_u & ~0x02) >> 1;
        tage->tage_tables[alloc_idx].lo_us[index] = alloc_new_u & ~0x01;
        tage->tage_tables[alloc_idx].entry[index].ctr = alloc_new_ctr;
        tage->tage_tables[alloc_idx].entry[index].tag = tag;
        tage->tage_tables[alloc_idx].entry[index].valid = true;
    }

    free(tage_meta);
    return;
}

void TAGE_fprintf(TAGE* tage, FILE *fp)
{
	// uint64_t i;
	// for (i = 0; i < BranchPredictionTable->attributes.counter_num; i++)
	// 	fprintf(fp, "table[%llu]: %u\n", i, BranchPredictionTable->counter[i]);
}