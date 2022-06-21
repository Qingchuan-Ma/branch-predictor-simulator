
/*
 *	sc table
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "utils.h"
#include "sc.h"

uint32_t scTableInfo[scTableNum][3] = {
    // nSets, ctrbit,  hislen
    {  512,       6,     0},
    {  512,       6,     4},
    {  512,       6,     10},
    {  512,       6,     16}
};


uint32_t SC_Compute_Folded_Hist(uint64_t ghist, uint32_t hist_len, uint32_t folded_len) // folded_len = log2(nrows)
{
    uint32_t nChunks = (hist_len + folded_len - 1) / folded_len;
    uint32_t res = 0;
    for (size_t i = 0; i < nChunks; i++)
    {
        res = res ^ ((ghist >> i*folded_len) & ((1 << folded_len)-1));
    }
    return res;
}

uint32_t SC_Compute_Index(uint64_t unhashed_idx, uint64_t ghist, uint32_t table_index)
{
    uint32_t hist_len = scTableInfo[table_index][2];
    uint32_t folded_len = (uint32_t) log2(scTableInfo[table_index][0]);
    if(hist_len == 0)
        return unhashed_idx & ((1 << folded_len)-1);
    return (unhashed_idx ^ SC_Compute_Folded_Hist(ghist, hist_len, folded_len)) & ((1 << folded_len)-1);
}


void SC_Initial(SC* sc)
{
    sc->attributes.tableNum = scTableNum;
    sc->sc_tables = (SCTable* )malloc(sizeof(SCTable) * scTableNum);

    for (size_t i = 0; i < scTableNum; i++)
    {
        int num_row = scTableInfo[i][0];
        sc->sc_tables[i].attributes.index_width = (uint32_t)log2(num_row);
        sc->sc_tables[i].attributes.num_row = scTableInfo[i][0];
        sc->sc_tables[i].attributes.ctr_bits = scTableInfo[i][1];
        sc->sc_tables[i].attributes.hist_len = scTableInfo[i][2];

        sc->sc_tables[i].entry = (SCTable_Entry *)malloc(sizeof(SCTable_Entry) * num_row);
        
        for (size_t j = 0; j < num_row; j++)
        {
            sc->sc_tables[i].entry[j].taken_ctr = 0;
            sc->sc_tables[i].entry[j].nontaken_ctr = 0;
        }   
    }

    sc->scThreshold.ctr = scInitThresCtr;
    sc->scThreshold.thres = scInitThres;
    return;
}

int32_t Get_Centered(int32_t ctr) // 这两个函数可能需要更改
{
    return 2*ctr + 1;
}

int32_t Get_Pvdr_Centered(uint32_t ctr)  // 该函数默认3-bit的tage ctr输出
{
    return (2*((int32_t)ctr-4)+1)*8;
}

bool Sum_Above_Threshold(SC* sc, int32_t totalSum) // TODO
{
    return true;
}

SC_Meta* SC_Predict(SC* sc, uint64_t unhashed_idx, uint64_t ghist, Tage_Meta* tage_meta)
{
    bool tage_provided = tage_meta->provided;
    bool taken = tage_provided ? tage_meta->provider_pred : tage_meta->alt_pred;

    SC_Meta* sc_meta = (SC_Meta*) malloc(sizeof(SC_Meta));

    int32_t scTableSum = 0;

    for (size_t i = 0; i < scTableNum; i++)
    {
        uint32_t index = SC_Compute_Index(unhashed_idx, ghist, i);
        int32_t ctr = taken ? Get_Centered(sc->sc_tables->entry[index].taken_ctr) : Get_Centered(sc->sc_tables->entry[index].nontaken_ctr);
        scTableSum += ctr;
        sc_meta->ctrs[i] = ctr;
    }

    int32_t tagePrvdCtrCentered = Get_Pvdr_Centered(tage_meta->provider_ctr);

    int32_t totalSum = scTableSum + tagePrvdCtrCentered;

    sc_meta->sc_used = tage_provided; // 只有使用过sc才会进行sc的更新
    sc_meta->tage_taken = taken;

    if(tage_provided && Sum_Above_Threshold(sc, totalSum))
        sc_meta->sc_pred = totalSum >= 0;
    else
        sc_meta->sc_pred = taken;   //sc_pred永远是正确的，不管用没用sc，最后预测结果直接从sc_pred获得
    

    return sc_meta;
}
