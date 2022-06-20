/*
 *	Branch Predictor
 */
#pragma once
#ifndef BP_H_
#define BP_H_

#include "bpt.h"
#include "bht.h"
#include "ghr.h"
#include "bct.h"
#include "tage.h"
#include "loop.h"

typedef BPT BP_Bimodal;
typedef LOOP BP_LOOP;

typedef struct BP_Gshare
{
	GHR* global_history_register;
	BPT* branch_prediction_table;
}BP_Gshare;

typedef struct BP_Hybrid
{
	BCT* branch_chooser_table;
	BP_Bimodal* bp_bimodal;
	BP_Gshare* bp_gshare;
}BP_Hybrid;

typedef struct BP_Yeh_Patt
{
	BHT* branch_histroy_table;  	//正常不需要branch histroy table：这个结构是用于记录该branch的历史的跳转情况的，除了该预测器其他预测器并未使用
	BPT* branch_predition_table;
}BP_Yeh_Patt;

typedef struct BP_TAGE
{
	TAGE* tage_without_base;
	GHR* global_history_register;
}BP_TAGE;

typedef struct BP_TAGE_B  // tage with base
{
	BP_Bimodal* alt_bp;
	BP_TAGE* tage;
}BP_TAGE_B;

typedef struct BP_TAGE_L
{
	BP_TAGE_B* bp_tage_b;
	BP_LOOP* bp_loop;
}BP_TAGE_L;

typedef struct BP
{
	Predictor predictor_type;
	void *predictor;
}BP;

extern BP* branch_predictor;

/*  参数太多了
 *	Initial the branch_predictor
 *	input	:
 *		type	:	the type of the predictor
 *		width	:	the array of width
 *					width[BIMODAL]		:	bimodal, hybrid		i_B
 *					width[GSHARE]		:	gshare, hybrid		i_G
 *					width[GHRegister]	:	gshare, hybrid		h
 *					width[BCTable]		:	hybrid				i_C
 *					width[BHTable]		:	yet_patt			h
 *					width[YEH_PATT]		:	yet_patt			p
 *					width[BTBuffer]		:	all					i_BTB
 *					width[ASSOC]		:	all					assoc
 */
void Predictor_Init(Predictor name, uint32_t* width);

/*
 *	Prediction on taken_or_not of bimodal predictor
 */
Taken_Result Bimodal_Predict(BP_Bimodal *predictor, uint64_t addr);

/*
 *	Prediction on taken_or_not of gshare predictor
 */
Taken_Result Gshare_Predict(BP_Gshare *predictor, uint64_t addr);

/*
 *	Prediction of bimodal predictor
 */
Result Predictor_Predict(uint64_t addr);

/*
 *	Update bimodal prediction table
 */
void Bimodal_Update(BP_Bimodal *predictor, uint64_t addr, Result result);

/*
 *	Update gshare prediction table
 */
void Gshare_Update(BP_Gshare *predictor, uint64_t addr, Result result);

/*
 *	Update predictior
 */
void Predictor_Update(uint64_t addr, Result result);

/*
 *	Print the content of branch_predictor to file *fp
 */
void BP_fprintf(FILE *fp);


void Predictor_Clear(uint64_t addr, uint64_t old_target);

#endif