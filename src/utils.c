#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "btb.h"
#include "bp.h"

void parse_arguments(int argc, char * argv[], Predictor *type, uint32_t* width)
{
	if (argc < 6 || argc > 9 || argc == 8)
		_output_error_exit("wrong number of input parameters")

	if (strcmp(argv[1], "bimodal") == 0)
	{
		*type = bimodal;
		if (argc != 6)
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "gshare") == 0)
	{
		*type = gshare;
		if (argc != 7)
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "hybrid") == 0)
	{
		*type = hybrid;
		if (argc != 9)
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "yehpatt") == 0)
	{
		*type = yeh_patt;
		if (argc != 7)
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "tage") == 0)
	{
		*type = tage_b;
		if (argc != 6) // the same as bimodal, 因为tage的参数都目前都被定死了
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "tage-l") == 0)
	{
		*type = tage_l;
		if (argc != 7) // the same as bimodal+1, 因为tage的参数都目前都被定死了+loop的index_width
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "tage-sc") == 0)
	{
		*type = tage_sc;
		if (argc != 6) // the same as bimodal, 因为tage和sc的参数都目前都被定死了
			_output_error_exit("wrong number of input parameters")
	}
	else if (strcmp(argv[1], "tage-sc-l") == 0)
	{
		*type = tage_sc_l;
		if (argc != 7) // the same as bimodal+1, 因为tage和sc的参数都目前都被定死了+loop的index_width
			_output_error_exit("wrong number of input parameters")
	}
	else
		_output_error_exit("invalid predictor type")

	switch (*type)
	{
	case bimodal:
	{
		width[BIMODAL] = atoi(argv[2]);
		width[BTBuffer] = atoi(argv[3]);
		width[ASSOC] = atoi(argv[4]);
		trace_file = argv[5];
		break;
	}
	case gshare:
	{
		width[GSHARE] = atoi(argv[2]);
		width[GHRegister] = atoi(argv[3]);
		width[BTBuffer] = atoi(argv[4]);
		width[ASSOC] = atoi(argv[5]);
		trace_file = argv[6];
		break;
	}
	case hybrid:
	{
		width[BCTable] = atoi(argv[2]);
		width[GSHARE] = atoi(argv[3]);
		width[GHRegister] = atoi(argv[4]);
		width[BIMODAL] = atoi(argv[5]);
		width[BTBuffer] = atoi(argv[6]);
		width[ASSOC] = atoi(argv[7]);
		trace_file = argv[8];
		break;
	}
	case yeh_patt:
	{
		width[BHTable] = atoi(argv[2]);
		width[YEH_PATT] = atoi(argv[3]);
		width[BTBuffer] = atoi(argv[4]);
		width[ASSOC] = atoi(argv[5]);
		trace_file = argv[6];
		break;
	}
	case tage_b: // TODO: add new parameter
	{
		width[BIMODAL] = atoi(argv[2]);
		width[BTBuffer] = atoi(argv[3]);
		width[ASSOC] = atoi(argv[4]);
		trace_file = argv[5];
		break;
	}
	case tage_l:
	{
		width[BIMODAL] = atoi(argv[2]);
		width[BTBuffer] = atoi(argv[3]);
		width[ASSOC] = atoi(argv[4]);
		width[TAGE_L] = atoi(argv[5]);   // for loop index width
		trace_file = argv[6];
		break;
	}
	case tage_sc:
	{
		width[BIMODAL] = atoi(argv[2]);
		width[BTBuffer] = atoi(argv[3]);
		width[ASSOC] = atoi(argv[4]);
		trace_file = argv[5];
		break;
	}
	case tage_sc_l:
	{
		width[BIMODAL] = atoi(argv[2]);
		width[BTBuffer] = atoi(argv[3]);
		width[ASSOC] = atoi(argv[4]);
		width[TAGE_L] = atoi(argv[5]);   // for loop index width
		trace_file = argv[6];
		break;
	}
	}
}

void Stat_Init()
{
	stat.num_branches = 0;
	stat.num_prediction = 0;
	stat.misprediction_rate = 0;
	stat.num_misprediction[BTBuffer] = 0;
	memset(stat.num_misprediction, 0, sizeof(uint64_t) * (BP_MAX + 1));
}

uint64_t Get_Index(uint64_t addr, uint32_t index_width)
{
	return (addr << (62 - index_width)) >> (64 - index_width);
	// return (addr << (30 - index_width)) >> (32 - index_width);
}

void Update_Stat(Result result, bool BTBisExist)  // 这个函数需要大改
{
	// 关于BTB的几种情况
	// BTB认为不是branch，但是真实情况是branch且跳转，认为BTB预测错误, s1
	// BTB认为不是branch，但是真实情况是branch但是不跳转，可以认为BTB预测正确: s2
	// BTB认为是branch，且taken但是target不对，认为BTB预测错误: s3
	// BTB认为是branch，且target对，认为BTB预测正确: s4
	bool s1 = result.predict_branch.is_branch == NOT_BRANCH && result.actual_branch.is_branch == BRANCH && result.actual_taken == TAKEN;
	bool s3 = result.predict_branch.is_branch == BRANCH && (result.predict_branch.target != result.actual_branch.target) && result.actual_taken == TAKEN;
	if (result.actual_branch.is_branch == BRANCH)
		stat.num_branches++;  // num_branch表明branch总数
	if ((s1 || s3) && BTBisExist)
		stat.num_misprediction[BTBuffer]++;

	if ((s1 || s3) && BTBisExist)  // 如果BTB预测错误，则认为直接错误，不需要考虑后边的预测器是否预测准确，如果没有BTB，不能进该分支
	{
		stat.num_misprediction[BP_MAX+1] = stat.num_misprediction[branch_predictor->predictor_type] + stat.num_misprediction[BTBuffer];
		// 最后一个位置的错误预测值表示 该预测器预测错误+BTB预测错误；相当于总预测错误率；没有BTB的情况下就等于该预测器预测错误值
		stat.misprediction_rate = (double)stat.num_misprediction[BP_MAX+1] / (double)stat.num_prediction * 100.0;
		return;
	}
	stat.num_prediction++; // 总预测次数
	if (result.actual_taken != result.predict_taken[branch_predictor->predictor_type])
	{
		stat.num_misprediction[branch_predictor->predictor_type]++;
		if (branch_predictor->predictor_type == hybrid)
			stat.num_misprediction[result.predict_predictor]++;
	}
	
#ifdef MyDBG1
	printf("BTBuffer misprediction number: %d\n", stat.num_misprediction[BTBuffer]);
#endif
	stat.num_misprediction[BP_MAX+1] = stat.num_misprediction[branch_predictor->predictor_type] + stat.num_misprediction[BTBuffer];
	stat.misprediction_rate = (double)stat.num_misprediction[BP_MAX+1] / (double)stat.num_branches * 100.0;
}

void Result_fprintf(FILE *fp, int argc, char* argv[])
{
	fprintf(fp, "Command Line:\n");
	int i;
	for (i = 0; i < argc; i++)
		fprintf(fp, "%s ", argv[i]);
	fprintf(fp, "\n\n");
	// if (branch_target_buffer != NULL)
	// {
	// 	BTB_fprintf(fp);
	// 	fprintf(fp, "\n\n");
	// }
	BP_fprintf(fp);
	fprintf(fp, "\n");
	fprintf(fp, "Final Branch Predictor Statistics:\n");
	fprintf(fp, "a. Number of branches: %llu\n", stat.num_branches);
	fprintf(fp, "b. Number of predictions from the branch predictor: %llu\n", stat.num_prediction);
	fprintf(fp, "c. Number of mispredictions from the branch predictor: %llu\n", stat.num_misprediction[branch_predictor->predictor_type]);
	fprintf(fp, "d. Number of mispredictions from the BTB: %llu\n", stat.num_misprediction[BTBuffer]);
	fprintf(fp, "e. Misprediction Rate: %4.2f percent\n", stat.misprediction_rate);
}


uint32_t Saturate_Inc_UCtr(uint32_t ctr, uint32_t ctr_bits, bool taken)
{
	bool old_sat_taken = ctr == (1 << ctr_bits) - 1;
	bool old_sat_non_taken = ctr == 0;
	
	if(taken && old_sat_taken)
		return (1 << ctr_bits)-1;
	else if (!taken && old_sat_non_taken)
		return 0;
	else
		return taken ? ctr + 1: ctr -1;
}

uint32_t Saturate_Inc_SCtr(int32_t ctr, uint32_t ctr_bits, bool taken)
{
	bool old_sat_taken = ctr == (1 << (ctr_bits-1)) - 1;
	bool old_sat_non_taken = ctr == -(1 << (ctr_bits-1));


	if(taken && old_sat_taken)
		return (1 << (ctr_bits-1)) - 1;
	else if (!taken && old_sat_non_taken)
		return -(1 << (ctr_bits-1));
	else
		return taken ? ctr + 1: ctr -1;
}