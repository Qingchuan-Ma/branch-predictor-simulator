#pragma once

#ifndef UTILS_H_
#define UTILS_H_

#define UINT32_MAX 4294967295

#include <stdbool.h>

//  this is for width index
#define BIMODAL 0
#define GSHARE 1
#define HYBRID 2
#define YEH_PATT 3
#define BOOM_TAGE 4
#define TAGE_L 5
#define BP_MAX 6
// 以上是分支预测器种类，还可以继续加；以下是width index


#define BTBuffer 6
#define GHRegister 7
#define BCTable 8
#define BHTable 9
#define ASSOC 10

// 一共有多少个width
#define WIDTH_MAX 11


#define NOT_BRANCH 0
#define BRANCH 1

#define NOT_TAKEN 0
#define TAKEN 1

#define VALID 1
#define INVALID 0

#define CORRECT 0
#define WRONG 1

#define _error_exit(fun) { perror(fun); exit(EXIT_FAILURE); }

#define _output_error_exit(msg) { printf("error: %s", msg); exit(EXIT_FAILURE);}

#define pow_2(num) (1 << ((int)(num)))

typedef char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef struct Branch_Result
{ 
	bool is_branch;
	uint64_t target;
} Branch_Result;

// typedef enum Taken_Result
// { 
// 	not_taken = NOT_TAKEN,
// 	taken = TAKEN 
// } Taken_Result;

typedef bool Taken_Result;

typedef enum Predictor
{
	bimodal = BIMODAL,
	gshare = GSHARE,
	hybrid = HYBRID,
	yeh_patt = YEH_PATT,
	boom_tage = BOOM_TAGE,
	tage_l = TAGE_L
}Predictor;

typedef struct Result
{
	Predictor predict_predictor;
	Branch_Result predict_branch;
	Taken_Result predict_taken[BP_MAX];
	Branch_Result actual_branch;  // 带地址
	Taken_Result actual_taken;
	void* meta_info[BP_MAX];
}Result;

typedef struct Stat
{
	uint64_t num_branches;
	uint64_t num_prediction;
	uint64_t num_misprediction[BP_MAX+2];  // 所有BP的情况，在加上一个BTB，在加上一个总的
	double misprediction_rate;
}Stat;

extern Stat stat;
extern char *trace_file;

/*
 *	Parse the arguments to Predictor "type" and array "width"
 *	input(output)	:
 *		type	:	the type of predictor
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
void parse_arguments(int argc, char * argv[], Predictor *type, uint32_t* width);

/*
 *	Initial the stat (global statistic data)
 */
void Stat_Init();

/*
 *	saturate increase the unsigned counter
 *  taken: increase, non-taken: decrease
 */
uint32_t Saturate_Inc_UCtr(uint32_t ctr, uint32_t saturate, bool taken);


/*
 *	get index from "addr"
 */
uint64_t Get_Index(uint64_t addr, uint32_t index_width);

/*
 *	Update the stat according to result
 */
void Update_Stat(Result result, bool BTBisExist);

/*
 *	Print the result to file *fp
 */
void Result_fprintf(FILE *fp, int argc, char* argv[]);

#endif
