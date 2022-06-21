/*
 *	Branch Predictor
 */

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "bp.h"


void Predictor_Init(Predictor type, uint32_t* width)
{
	branch_predictor->predictor_type = type;
	switch (type)
	{
	case bimodal:
	{
		branch_predictor->predictor = (BP_Bimodal *)malloc(sizeof(BP_Bimodal));
		if (branch_predictor->predictor == NULL)
			_error_exit("malloc")
		BPT_Initial((BPT *)branch_predictor->predictor, width[BIMODAL]);
		return;
	}
	case gshare:
	{
		branch_predictor->predictor = (BP_Gshare *)malloc(sizeof(BP_Gshare));
		if (branch_predictor->predictor == NULL)
			_error_exit("malloc")
		BP_Gshare *predictor = branch_predictor->predictor;
		/* initial global history register */
		predictor->global_history_register = (GHR *)malloc(sizeof(GHR));
		if (predictor->global_history_register == NULL)
			_error_exit("malloc")
		GHR_Initial(predictor->global_history_register, width[GHRegister]);
		/* initial branch predition table */
		predictor->branch_prediction_table = (BPT *)malloc(sizeof(BPT));
		if (predictor->branch_prediction_table == NULL)
			_error_exit("malloc")	
		BPT_Initial(predictor->branch_prediction_table, width[GSHARE]);
		return;
	}
	case hybrid:
	{
		branch_predictor->predictor = (BP_Hybrid *)malloc(sizeof(BP_Hybrid));
		if (branch_predictor->predictor == NULL)
			_error_exit("malloc")
		BP_Hybrid *predictor = branch_predictor->predictor;
		/* initial branch chooser table */
		predictor->branch_chooser_table = (BCT *)malloc(sizeof(BCT));
		if (predictor->branch_chooser_table == NULL)
			_error_exit("malloc")
		BCT_Initial(predictor->branch_chooser_table, width[BCTable]);
		/* initial bimodal predictor */
		predictor->bp_bimodal = (BP_Bimodal *)malloc(sizeof(BP_Bimodal));
		if (predictor->bp_bimodal == NULL)
			_error_exit("malloc")
		BPT_Initial((BPT *)predictor->bp_bimodal, width[BIMODAL]);
		/* initial gshare predictor*/
		predictor->bp_gshare = (BP_Gshare *)malloc(sizeof(BP_Gshare));
		predictor->bp_gshare->branch_prediction_table = (BPT *)malloc(sizeof(BPT));
		if (predictor->bp_gshare->branch_prediction_table == NULL)
			_error_exit("malloc")
		predictor->bp_gshare->global_history_register = (GHR *)malloc(sizeof(GHR));
		if (predictor->bp_gshare->global_history_register == NULL)
			_error_exit("malloc")
		BPT_Initial(predictor->bp_gshare->branch_prediction_table, width[GSHARE]);
		GHR_Initial(predictor->bp_gshare->global_history_register, width[GHRegister]);
		return;
	}
	case yeh_patt:
	{
		branch_predictor->predictor = (BP_Yeh_Patt *)malloc(sizeof(BP_Yeh_Patt));
		if (branch_predictor->predictor == NULL)
			_error_exit("malloc")
		BP_Yeh_Patt *predictor = branch_predictor->predictor;
		/* initial branch history table */
		predictor->branch_histroy_table = (BHT *)malloc(sizeof(BHT));
		if (predictor->branch_histroy_table == NULL)
			_error_exit("malloc")
		BHT_Initial(predictor->branch_histroy_table, width[BHTable], width[YEH_PATT]);
		/* initial branch predition table */
		predictor->branch_predition_table = (BPT *)malloc(sizeof(BPT));
		if (predictor->branch_predition_table == NULL)
			_error_exit("malloc")
		BPT_Initial(predictor->branch_predition_table, width[YEH_PATT]);
		return;
	}
	case boom_tage:
	{
		branch_predictor->predictor = (BP_TAGE_B *)malloc(sizeof(BP_TAGE_B));
		if (branch_predictor->predictor == NULL)
			_error_exit("malloc")
		BP_TAGE_B *predictor = branch_predictor->predictor;
		/* initial branch prediction table */
		predictor->alt_bp = (BP_Bimodal *)malloc(sizeof(BP_Bimodal));
		if (predictor->alt_bp == NULL)
			_error_exit("malloc")
		BPT_Initial((BPT *)predictor->alt_bp, width[BIMODAL]);
		/* initial tage*/
		predictor->tage = (BP_TAGE *)malloc(sizeof(BP_TAGE));
		predictor->tage->tage_without_base = (TAGE *)malloc(sizeof(TAGE));
		TAGE_Initial(predictor->tage->tage_without_base);

		/* initial global history register */
		predictor->tage->global_history_register = (GHR *)malloc(sizeof(GHR));
		GHR_Initial(predictor->tage->global_history_register, tageHistLen);  // 先固定history再说
		return;
	}
	case tage_l:
	{
		branch_predictor->predictor = (BP_TAGE_L *)malloc(sizeof(BP_TAGE_L));
		BP_TAGE_L *predictor = branch_predictor->predictor;

		/* initial loop predictor */
		predictor->bp_loop = (BP_LOOP *)malloc(sizeof(BP_LOOP));
		LOOP_Initial(predictor->bp_loop, width[TAGE_L], 10); // 默认是10 tag_width， 一般输入index_width=4

		/* initial tage predictor */
		predictor->bp_tage_b = (BP_TAGE_B *)malloc(sizeof(BP_TAGE_B));
		BP_TAGE_B *bp_tage_b = predictor->bp_tage_b;
		
		/* initial branch prediction table */
		bp_tage_b->alt_bp = (BP_Bimodal *)malloc(sizeof(BP_Bimodal));
		if (bp_tage_b->alt_bp == NULL)
			_error_exit("malloc")
		BPT_Initial((BPT *)bp_tage_b->alt_bp, width[BIMODAL]);
		/* initial tage*/
		bp_tage_b->tage = (BP_TAGE *)malloc(sizeof(BP_TAGE));
		bp_tage_b->tage->tage_without_base = (TAGE *)malloc(sizeof(TAGE));
		TAGE_Initial(bp_tage_b->tage->tage_without_base);

		/* initial global history register */
		bp_tage_b->tage->global_history_register = (GHR *)malloc(sizeof(GHR));
		GHR_Initial(bp_tage_b->tage->global_history_register, tageHistLen);  // 先固定history再说
	}
	}
}

Taken_Result Bimodal_Predict(BP_Bimodal *predictor, uint64_t addr)
{
	uint32_t index = Get_Index(addr, ((BPT *)predictor)->attributes.index_width);
	return BPT_Predict((BPT *)predictor, index);
}

Taken_Result Gshare_Predict(BP_Gshare *predictor, uint64_t addr)
{
	uint32_t h = predictor->global_history_register->attributes.history_width;
	uint32_t i = predictor->branch_prediction_table->attributes.index_width;
	uint32_t index = Get_Index(addr, i);
	uint32_t history = predictor->global_history_register->history;
	// TODO: 需要修改index的生成，因为改了方法
	uint32_t index_tail = (index << (32 - (i - h))) >> (32 - (i - h));
	uint32_t index_head = (index >> (i - h)) ^ history;
	index = ((index_head) << (i - h)) | index_tail;
	return BPT_Predict(predictor->branch_prediction_table, index);
}

Tage_Meta* Tage_Predict(BP_TAGE *predictor, uint64_t addr)
{
	uint64_t ghist = predictor->global_history_register->history;
	uint64_t unhashed_idx = Get_Index(addr, 62);  // 把pc[64:2]给传进去了
	return TAGE_Predict(predictor->tage_without_base, unhashed_idx, ghist);
}

Loop_Meta* Loop_Predict(BP_LOOP *predictor, uint64_t addr)
{
	uint64_t unhashed_idx = Get_Index(addr, 62);  // 把pc[64:2]给传进去了
	return LOOP_Predict(predictor, unhashed_idx);
}

Result Predictor_Predict(uint64_t addr)
{
	Result result;
	result.predict_predictor = branch_predictor->predictor_type;
	switch (branch_predictor->predictor_type)
	{
	case bimodal:
	{
		result.predict_taken[BIMODAL] = Bimodal_Predict((BP_Bimodal *)branch_predictor->predictor, addr);
		return result;
	}
	case gshare:
	{
		result.predict_taken[GSHARE] = Gshare_Predict((BP_Gshare *)branch_predictor->predictor, addr);
		return result;
	}
	case hybrid:
	{
		BP_Hybrid *predictor = branch_predictor->predictor;
		result.predict_taken[BIMODAL] = Bimodal_Predict(predictor->bp_bimodal, addr);
		result.predict_taken[GSHARE] = Gshare_Predict(predictor->bp_gshare, addr);
		result.predict_predictor = BCT_Predict(predictor->branch_chooser_table, addr);
		if (result.predict_predictor == bimodal)
			result.predict_taken[HYBRID] = result.predict_taken[BIMODAL];
		else
			result.predict_taken[HYBRID] = result.predict_taken[GSHARE];
		return result;
	}
	case yeh_patt:
	{
		BP_Yeh_Patt *predictor = branch_predictor->predictor;
		uint64_t history = BHT_Search(predictor->branch_histroy_table, addr);
		result.predict_taken[YEH_PATT] = BPT_Predict(predictor->branch_predition_table, history);
		return result;
	}
	case boom_tage:
	{
		BP_TAGE_B *predictor = branch_predictor->predictor;

		result.predict_taken[BIMODAL] = Bimodal_Predict(predictor->alt_bp, addr);
		
		Tage_Meta *tage_meta = Tage_Predict(predictor->tage, addr);

		result.predict_taken[BOOM_TAGE] = tage_meta->provided ? tage_meta->provider_pred : result.predict_taken[BIMODAL];
		tage_meta->alt_pred = result.predict_taken[BIMODAL];
		tage_meta->alt_differs = tage_meta->provided ? tage_meta->provider_pred != tage_meta->alt_pred : 1;
		#ifdef MyDBG1
		if(tage_meta->alt_differs==1)
			printf("addr: %lx, alt_differs: 0\n", addr, tage_meta->alt_differs);
		#endif
		result.meta_info[BOOM_TAGE] = (Tage_Meta *)tage_meta;
		
		return result;
	}
	case tage_l:
	{
		BP_TAGE_L *predictor = branch_predictor->predictor;

		BP_TAGE_B *bp_tage_b = predictor->bp_tage_b;
		BP_LOOP *bp_loop = predictor->bp_loop;

		result.predict_taken[BIMODAL] = Bimodal_Predict(bp_tage_b->alt_bp, addr);
		
		Tage_Meta *tage_meta = Tage_Predict(bp_tage_b->tage, addr);

		result.predict_taken[BOOM_TAGE] = tage_meta->provided ? tage_meta->provider_pred : result.predict_taken[BIMODAL];
		tage_meta->alt_pred = result.predict_taken[BIMODAL];
		tage_meta->alt_differs = tage_meta->provided ? tage_meta->provider_pred != tage_meta->alt_pred : 1;
		result.meta_info[BOOM_TAGE] = (Tage_Meta *)tage_meta;


		Loop_Meta *loop_meta = Loop_Predict(bp_loop, addr);

		result.predict_taken[TAGE_L] = loop_meta->invert ? !result.predict_taken[BOOM_TAGE] : result.predict_taken[BOOM_TAGE];
		result.meta_info[TAGE_L] = (Loop_Meta *)loop_meta;

		return result;
	}
	default:
		return result;
	}
}

void Bimodal_Update(BP_Bimodal *predictor, uint64_t addr, Result result)
{
	uint32_t index = Get_Index(addr, ((BPT *)predictor)->attributes.index_width);
	BPT_Update(predictor, index, result);
}

void Gshare_Update(BP_Gshare *predictor, uint64_t addr, Result result)
{
	uint32_t h = predictor->global_history_register->attributes.history_width;
	uint32_t i = predictor->branch_prediction_table->attributes.index_width;
	uint32_t index = Get_Index(addr, i);
	uint32_t history = predictor->global_history_register->history;
	uint32_t index_tail = (index << (32 - (i - h))) >> (32 - (i - h));
	uint32_t index_head = (index >> (i - h)) ^ history;
	index = ((index_head) << (i - h)) | index_tail;
	BPT_Update(predictor->branch_prediction_table, index, result);
}

void Tage_Update(BP_TAGE* predictor, uint64_t addr, Result result)
{
	uint64_t ghist = predictor->global_history_register->history;
	uint64_t unhashed_idx = Get_Index(addr, 62);  // 把pc[64:2]给传进去了
	TAGE_Update(predictor->tage_without_base, unhashed_idx, ghist, result);
	return GHR_Update(predictor->global_history_register, result);
}

void Loop_Update(BP_LOOP* predictor, uint64_t addr, Result result)
{
	uint64_t unhashed_idx = Get_Index(addr, 62);  // 把pc[64:2]给传进去了
	return LOOP_Update(predictor, unhashed_idx, result);
}

void Predictor_Update(uint64_t addr, Result result)
{
	switch (branch_predictor->predictor_type)
	{
	case bimodal:
	{
		Bimodal_Update((BP_Bimodal *)branch_predictor->predictor, addr, result);
		return;
	}
	case gshare:
	{
		BP_Gshare *predictor = branch_predictor->predictor;
		Gshare_Update(predictor, addr, result);
		GHR_Update(predictor->global_history_register, result);
		return;
	}
	case hybrid:
	{
		BP_Hybrid *predictor = branch_predictor->predictor;
		if (result.predict_predictor == bimodal)
			Bimodal_Update(predictor->bp_bimodal, addr, result);
		else
			Gshare_Update(predictor->bp_gshare, addr, result);
		GHR_Update(predictor->bp_gshare->global_history_register, result);
		BCT_Update(predictor->branch_chooser_table, addr, result);
		return;
	}
	case yeh_patt:
	{
		BP_Yeh_Patt *predictor = branch_predictor->predictor;
		uint64_t history = BHT_Search(predictor->branch_histroy_table, addr);
		BPT_Update(predictor->branch_predition_table, history, result);
		BHT_Update(predictor->branch_histroy_table, addr, result);
		return;
	}
	case boom_tage:
	{
		BP_TAGE_B *predictor = branch_predictor->predictor;
		Bimodal_Update(predictor->alt_bp, addr, result);
		Tage_Update(predictor->tage, addr, result);

		return;
	}
	case tage_l:
	{
		BP_TAGE_L *predictor = branch_predictor->predictor;
		Bimodal_Update(predictor->bp_tage_b->alt_bp, addr, result);
		Tage_Update(predictor->bp_tage_b->tage, addr, result);
		Loop_Update(predictor->bp_loop, addr, result);	// loop_update内部只有在mispredict的时候才会加项

		return;
	}
	}
}

void BP_fprintf(FILE *fp)  // 打印最后的final table
{
	switch (branch_predictor->predictor_type)
	{
	case bimodal:
	{
		fprintf(fp, "Final Bimodal Table Contents: \n");
		BPT_fprintf((BPT *)(branch_predictor->predictor), fp);
		return;
	}
	case gshare:
	{
		BP_Gshare *predictor = branch_predictor->predictor;
		fprintf(fp, "Final GShare Table Contents: \n");
		BPT_fprintf(predictor->branch_prediction_table, fp);
		fprintf(fp, "\n");
		fprintf(fp, "Final GHR Contents: ");
		GHR_fprintf(predictor->global_history_register, fp);
		return;
	}
	case hybrid:
	{
		BP_Hybrid *predictor = branch_predictor->predictor;
		fprintf(fp, "Final Bimodal Table Contents: \n");
		BPT_fprintf((BPT *)predictor->bp_bimodal, fp);
		fprintf(fp, "\n");
		fprintf(fp, "Final GShare Table Contents: \n");
		BPT_fprintf(predictor->bp_gshare->branch_prediction_table, fp);
		fprintf(fp, "\n");
		fprintf(fp, "Final GHR Contents: ");
		GHR_fprintf(predictor->bp_gshare->global_history_register, fp);
		fprintf(fp, "\n");
		fprintf(fp, "Final Chooser Table Contents : \n");
		BCT_fprintf(predictor->branch_chooser_table, fp);
		return;
	}
	case yeh_patt:
	{
		BP_Yeh_Patt *predictor = branch_predictor->predictor;
		fprintf(fp, "Final History Table Contents: \n");
		BHT_fprintf(predictor->branch_histroy_table, fp);
		fprintf(fp, "\n");
		fprintf(fp, "Final Prediction Table Contents: \n");
		BPT_fprintf(predictor->branch_predition_table, fp);
		return;
	}
	case boom_tage:
	{
		return;
	}
	case tage_l:
	{
		return;
	}
	}
}


void Predictor_Clear(uint64_t addr, uint64_t old_target)
{
	uint32_t cycle = addr - old_target;
	
	switch (branch_predictor->predictor_type)
	{
	case boom_tage:
	{
		BP_TAGE_B *predictor = branch_predictor->predictor;
		if(cycle > tageUPeriod)
		{
			#ifdef MyDBG1
			printf("do clear\n"); 
			#endif
			TAGE_Clear(predictor->tage->tage_without_base);
		}
		return;
	}
	default:
		return;
	}
}