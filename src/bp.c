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
	case tage_b:
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
		/* initial tage */
		predictor->tage = (BP_TAGE *)malloc(sizeof(BP_TAGE));
		predictor->tage->tage_without_base = (TAGE *)malloc(sizeof(TAGE));
		TAGE_Initial(predictor->tage->tage_without_base);

		/* initial global history register */
		predictor->tage->global_history_register = (GHR *)malloc(sizeof(GHR));
		GHR_Initial(predictor->tage->global_history_register, tageHistLen);  // 先固定history再说

		predictor->tage->sc = NULL; // predict和update时不使用sc

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
		/* initial tage */
		bp_tage_b->tage = (BP_TAGE *)malloc(sizeof(BP_TAGE));
		bp_tage_b->tage->tage_without_base = (TAGE *)malloc(sizeof(TAGE));
		TAGE_Initial(bp_tage_b->tage->tage_without_base);

		/* initial global history register */
		bp_tage_b->tage->global_history_register = (GHR *)malloc(sizeof(GHR));
		GHR_Initial(bp_tage_b->tage->global_history_register, tageHistLen);  // 固定history

		bp_tage_b->tage->sc = NULL; // predict和update时不使用sc

		return;
	}
	case tage_sc:  
	{
		branch_predictor->predictor = (BP_TAGE_SC *)malloc(sizeof(BP_TAGE_SC));
		if (branch_predictor->predictor == NULL)
			_error_exit("malloc")
		BP_TAGE_SC *predictor = branch_predictor->predictor;
		/* initial branch prediction table */
		predictor->alt_bp = (BP_Bimodal *)malloc(sizeof(BP_Bimodal));
		if (predictor->alt_bp == NULL)
			_error_exit("malloc")
		BPT_Initial((BPT *)predictor->alt_bp, width[BIMODAL]);
		/* initial tage */
		predictor->tage = (BP_TAGE *)malloc(sizeof(BP_TAGE));
		predictor->tage->tage_without_base = (TAGE *)malloc(sizeof(TAGE));
		TAGE_Initial(predictor->tage->tage_without_base);

		/* initial global history register */
		predictor->tage->global_history_register = (GHR *)malloc(sizeof(GHR));
		GHR_Initial(predictor->tage->global_history_register, tageHistLen);  // 固定history

		/* initial sc */
		predictor->tage->sc = (SC*)malloc(sizeof(SC));
		SC_Initial(predictor->tage->sc); // 没有参数传递
		return;
	}
	case tage_sc_l: // need to be modified
	{
		
		branch_predictor->predictor = (BP_TAGE_SC_L *)malloc(sizeof(BP_TAGE_SC_L));
		BP_TAGE_SC_L *predictor = branch_predictor->predictor;

		/* initial loop predictor */
		predictor->bp_loop = (BP_LOOP *)malloc(sizeof(BP_LOOP));
		LOOP_Initial(predictor->bp_loop, width[TAGE_L], 10); // 默认是10 tag_width， 一般输入index_width=4

		/* initial tage predictor */
		predictor->bp_tage_sc = (BP_TAGE_B *)malloc(sizeof(BP_TAGE_B));
		BP_TAGE_B *bp_tage_sc = predictor->bp_tage_sc;
		
		/* initial branch prediction table */
		bp_tage_sc->alt_bp = (BP_Bimodal *)malloc(sizeof(BP_Bimodal));
		if (bp_tage_sc->alt_bp == NULL)
			_error_exit("malloc")
		BPT_Initial((BPT *)bp_tage_sc->alt_bp, width[BIMODAL]);
		/* initial tage */
		bp_tage_sc->tage = (BP_TAGE *)malloc(sizeof(BP_TAGE));
		bp_tage_sc->tage->tage_without_base = (TAGE *)malloc(sizeof(TAGE));
		TAGE_Initial(bp_tage_sc->tage->tage_without_base);

		/* initial sc */
		bp_tage_sc->tage->sc = (SC*)malloc(sizeof(SC));
		SC_Initial(bp_tage_sc->tage->sc); // 没有参数传递

		/* initial global history register */
		bp_tage_sc->tage->global_history_register = (GHR *)malloc(sizeof(GHR));
		GHR_Initial(bp_tage_sc->tage->global_history_register, tageHistLen);  // 固定history
		return;
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

void Tage_Predict(BP_TAGE *predictor, uint64_t addr, Tage_Meta* tage_meta, SC_Meta* sc_meta, bool alt_pred)
{
	uint64_t ghist = predictor->global_history_register->history;
	uint64_t unhashed_idx = Get_Index(addr, 62);  // 把pc[64:2]给传进去了
	
	TAGE_Predict(predictor->tage_without_base, unhashed_idx, ghist, tage_meta);

	tage_meta->alt_pred = alt_pred;
	tage_meta->alt_differs = tage_meta->provided ? tage_meta->provider_pred != alt_pred : 1;
	
	if (predictor->sc != NULL)
	{
		SC_Predict(predictor->sc, unhashed_idx, ghist, tage_meta, sc_meta);
		#ifdef MyDBG1
			if(sc_meta->sc_pred != sc_meta->tage_taken)
			printf("sc is working; sc_pred: %d, sc_used: %d, tage_taken:%d\n", sc_meta->sc_pred, sc_meta->sc_used, sc_meta->tage_taken);
		#endif
	}
	return;
}


void Loop_Predict(BP_LOOP *predictor, uint64_t addr, Loop_Meta* loop_meta)
{
	uint64_t unhashed_idx = Get_Index(addr, 62);  // 把pc[64:2]给传进去了
	LOOP_Predict(predictor, unhashed_idx, loop_meta);
	return;
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
	case tage_b: // 确实tage+其他组件有很多重复代码，闲时可以优化
	{
		BP_TAGE_B *predictor = branch_predictor->predictor;

		
		Tage_Meta* tage_meta = (Tage_Meta *)malloc(sizeof(Tage_Meta));
		SC_Meta* sc_meta = NULL; // (SC_Meta*) malloc(sizeof(SC_Meta));

		result.predict_taken[BIMODAL] = Bimodal_Predict(predictor->alt_bp, addr);

		Tage_Predict(predictor->tage, addr, tage_meta, sc_meta, result.predict_taken[BIMODAL]);

#ifdef MyDBG1
		printf("%d\n", tage_meta->provided);
		printf("-------------get passed------------alt_pred: %d, pvdr_pred: %d\n", tage_meta->alt_pred, tage_meta->provider_pred);
#endif
		result.predict_taken[TAGE_B] = tage_meta->provided ? tage_meta->provider_pred : result.predict_taken[BIMODAL];

		result.meta_info[TAGE_B] = (Tage_Meta *)tage_meta;
		if(sc_meta != NULL)
			result.meta_info[TAGE_SC] = (SC_Meta *)sc_meta;
		
		return result;
	}
	case tage_l:
	{
		BP_TAGE_L *predictor = branch_predictor->predictor;

		BP_TAGE_B *bp_tage_b = predictor->bp_tage_b;
		BP_LOOP *bp_loop = predictor->bp_loop;

		result.predict_taken[BIMODAL] = Bimodal_Predict(bp_tage_b->alt_bp, addr);
		
		Tage_Meta* tage_meta = (Tage_Meta *)malloc(sizeof(Tage_Meta));
		SC_Meta* sc_meta = NULL; // (SC_Meta*) malloc(sizeof(SC_Meta));
		Loop_Meta *loop_meta = (Loop_Meta *)malloc(sizeof(Loop_Meta));

		Tage_Predict(bp_tage_b->tage, addr, tage_meta, sc_meta, result.predict_taken[BIMODAL]);

		result.predict_taken[TAGE_B] = tage_meta->provided ? tage_meta->provider_pred : result.predict_taken[BIMODAL];

		result.meta_info[TAGE_B] = (Tage_Meta *)tage_meta;
		result.meta_info[TAGE_SC] = (SC_Meta *)sc_meta; // 这个无效


		Loop_Predict(bp_loop, addr, loop_meta);

		result.predict_taken[TAGE_L] = loop_meta->invert ? !result.predict_taken[TAGE_B] : result.predict_taken[TAGE_B];
		result.meta_info[TAGE_L] = (Loop_Meta *)loop_meta;

		// 不包括bimodal，2个predict_taken, 2个meta

		return result;
	}
	case tage_sc:
	{
		BP_TAGE_SC *predictor = branch_predictor->predictor;

		result.predict_taken[BIMODAL] = Bimodal_Predict(predictor->alt_bp, addr);
		
		Tage_Meta* tage_meta = (Tage_Meta *)malloc(sizeof(Tage_Meta));
		SC_Meta* sc_meta = (SC_Meta*) malloc(sizeof(SC_Meta));

		Tage_Predict(predictor->tage, addr, tage_meta, sc_meta, result.predict_taken[BIMODAL]);
		
		result.predict_taken[TAGE_B] = tage_meta->provided ? tage_meta->provider_pred : result.predict_taken[BIMODAL];
		result.predict_taken[TAGE_SC] = sc_meta->sc_pred;
		
		result.meta_info[TAGE_B] = (Tage_Meta *)tage_meta;
		result.meta_info[TAGE_SC] = (SC_Meta *)sc_meta;;
		// 不包括bimodal，2个predict_taken, 2个meta
		return result;
	}
	case tage_sc_l:
	{
		BP_TAGE_SC_L *predictor = branch_predictor->predictor;

		BP_TAGE_SC *bp_tage_sc = predictor->bp_tage_sc;
		BP_LOOP *bp_loop = predictor->bp_loop;

		result.predict_taken[BIMODAL] = Bimodal_Predict(bp_tage_sc->alt_bp, addr);

		Tage_Meta* tage_meta = (Tage_Meta *)malloc(sizeof(Tage_Meta));
		SC_Meta* sc_meta = (SC_Meta*) malloc(sizeof(SC_Meta));
		Loop_Meta *loop_meta = (Loop_Meta *)malloc(sizeof(Loop_Meta));

		Tage_Predict(bp_tage_sc->tage, addr, tage_meta, sc_meta, result.predict_taken[BIMODAL]);

		result.predict_taken[TAGE_B] = tage_meta->provided ? tage_meta->provider_pred : result.predict_taken[BIMODAL];

		result.meta_info[TAGE_B] = (Tage_Meta *)tage_meta;
		result.meta_info[TAGE_SC] = (SC_Meta *)sc_meta;
		// result.meta_info[TAGE_SC_L] = NULL;

		result.predict_taken[TAGE_SC] = sc_meta->sc_pred;

		Loop_Predict(bp_loop, addr, loop_meta);

		result.predict_taken[TAGE_L] = loop_meta->invert ? !result.predict_taken[TAGE_SC] : result.predict_taken[TAGE_SC];
		result.meta_info[TAGE_L] = (Loop_Meta *)loop_meta;

		result.predict_taken[TAGE_SC_L] = result.predict_taken[TAGE_L];
		// 不包括bimodal，3个predict_taken 3个meta

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
	if(predictor->sc != NULL && result.meta_info[TAGE_SC] != NULL)
		SC_Update(predictor->sc, unhashed_idx, ghist, result);
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
	case tage_b:
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
		Loop_Update(predictor->bp_loop, addr, result);	// loop_update内部只有在mispredict的时候才会加项
		Tage_Update(predictor->bp_tage_b->tage, addr, result);

		return;
	}
	case tage_sc:
	{
		BP_TAGE_SC *predictor = branch_predictor->predictor;
		Bimodal_Update(predictor->alt_bp, addr, result);
		Tage_Update(predictor->tage, addr, result);

		return;

	}
	case tage_sc_l:
	{
		BP_TAGE_SC_L *predictor = branch_predictor->predictor;
		Bimodal_Update(predictor->bp_tage_sc->alt_bp, addr, result);
		Loop_Update(predictor->bp_loop, addr, result);	// loop_update内部只有在mispredict的时候才会加项， 需要先update_loop，free的问题，可以修改，之后有空改
		Tage_Update(predictor->bp_tage_sc->tage, addr, result);

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
	case tage_b:
	{
		return;
	}
	case tage_l:
	{
		return;
	}
	case tage_sc:
	{
		return;
	}
	case tage_sc_l:
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
	case tage_b:
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