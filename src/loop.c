/*
 *  loop branch predictor
 */


#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "loop.h"


void LOOP_Initial(LOOP* loop, uint32_t index_width, uint32_t tag_width)
{
	loop->attributes.index_width = index_width;
	loop->attributes.num_row = (uint64_t)pow_2(index_width);
	loop->attributes.tag_width = tag_width;

	loop->loop_entrys = (Loop_Entry *)malloc(sizeof(Loop_Entry) * loop->attributes.num_row);
	if (loop->loop_entrys == NULL)
		_error_exit("malloc")
	uint64_t i;
	for (i = 0; i < loop->attributes.num_row; i++)
	{
		loop->loop_entrys[i].age = 0;
		loop->loop_entrys[i].s_cnt = 0;
		loop->loop_entrys[i].p_cnt = 0;
		loop->loop_entrys[i].conf = 0;
		loop->loop_entrys[i].tag = 0;
	}
}


uint32_t LOOP_Compute_Tag(LOOP* loop, uint64_t unhashed_idx)
{
	uint32_t tag_width = loop->attributes.tag_width;
    return (unhashed_idx >> tag_width) & ((1 << tag_width)-1);
}

uint32_t LOOP_Compute_Index(LOOP* loop, uint64_t unhashed_idx)
{
    uint32_t index_width = loop->attributes.index_width;
    return (unhashed_idx) & ((1 << index_width)-1);
}

Loop_Meta* LOOP_Predict(LOOP* loop, uint64_t unhashed_idx)
{
	// from the whole index get tag and index
    Loop_Meta* loop_meta = (Loop_Meta *)malloc(sizeof(Loop_Meta));
	uint32_t index = LOOP_Compute_Index(loop, unhashed_idx);
	uint32_t tag = LOOP_Compute_Index(loop, unhashed_idx);

	loop_meta->invert = false;
	loop_meta->s_cnt = loop->loop_entrys[index].s_cnt;
	if(loop->loop_entrys[index].tag == tag)
	{
		if(loop->loop_entrys[index].s_cnt == loop->loop_entrys[index].p_cnt && loop->loop_entrys[index].conf == loopMaxConf)
			loop_meta->invert = true;
		else
			loop_meta->invert = false;

		// 预测结束了，之后立刻更新s_cnt, BOOM中在f4阶段进行的更新

		if(loop->loop_entrys[index].s_cnt == loop->loop_entrys[index].p_cnt && loop->loop_entrys[index].conf == loopMaxConf)
		{
			loop->loop_entrys[index].s_cnt = 0;
			loop->loop_entrys[index].age = loopMaxAge;
		} else
		{
			loop->loop_entrys[index].s_cnt = Saturate_Inc_UCtr(loop->loop_entrys[index].s_cnt, loopMaxCtr, true);
			loop->loop_entrys[index].age = Saturate_Inc_UCtr(loop->loop_entrys[index].age, loopMaxAge, true);
		}
		
	} else
	{
		// do nothing
	}

	return loop_meta;
}

// 非cycle级的LOOP预测器不需要repair机制

void LOOP_Update(LOOP* loop, uint64_t unhashed_idx, Result result)  // 更新策略
{
	bool mispredict = result.actual_taken != result.predict_taken[TAGE_L];
    Loop_Meta* loop_meta = result.meta_info[TAGE_L];

	uint32_t index = LOOP_Compute_Index(loop, unhashed_idx);
	uint32_t tag = LOOP_Compute_Index(loop, unhashed_idx);
	bool tag_match = tag == loop->loop_entrys[index].tag;
	bool ctr_match = loop_meta->s_cnt == loop->loop_entrys[index].p_cnt;
	Loop_Entry* entry = &(loop->loop_entrys[index]);

	if (mispredict)
	{
		// Learned, tag match -> decrement confidence
		if (entry->conf == loopMaxConf && tag_match) {
			entry->s_cnt = 0;
			entry->conf = 0;	

		// Learned, no tag match -> do nothing? Don't evict super-confident entries?
		} else if (entry->conf == loopMaxConf && !tag_match) {
			
			// do nothing

		// Confident, tag match, ctr_match -> increment confidence, reset counter
		} else if (entry->conf != 0 && tag_match && ctr_match) {
			entry->conf = Saturate_Inc_UCtr(entry->conf, loopMaxConf, true);
			entry->s_cnt = 0;

		// Confident, tag match, no ctr match -> zero confidence, reset counter, set previous counter
		} else if (entry->conf != 0 && tag_match && !ctr_match) {
			entry->conf = 0;
			entry->s_cnt = 0;
			entry->p_cnt = loop_meta->s_cnt;

		// Confident, no tag match, age is 0 -> replace this entry with our own, set our age high to avoid ping-pong(where?)
		} else if (entry->conf != 0 && !tag_match && entry->age == 0) {
			entry->tag = tag;
			entry->conf = 1;
			entry->s_cnt = 0;
			entry->p_cnt = loop_meta->s_cnt;

		// Confident, no tag match, age > 0 -> decrement age
		} else if (entry->conf != 0 && !tag_match && entry->age != 0) {
			entry->age = Saturate_Inc_UCtr(entry->age, loopMaxAge, false);

		// Unconfident, tag match, ctr match -> increment confidence
		} else if (entry->conf == 0 && tag_match && ctr_match) {
			entry->conf = 1;
			entry->age = loopMaxAge;
			entry->s_cnt = 0;
		
		// Unconfident, tag match, no ctr match -> set previous counter
		} else if (entry->conf == 0 && tag_match && !ctr_match) {
			entry->p_cnt = loop_meta->s_cnt;
			entry->age = loopMaxAge;
			entry->s_cnt = 0;

		// Unconfident, no tag match -> set previous counter and tag （这里就是初始化）
		} else if (entry->conf == 0 && !tag_match) {
			entry->tag = tag;
			entry->conf = 1;
			entry->age = loopMaxAge;
			entry->s_cnt = 0;
			entry->p_cnt = loop_meta->s_cnt;   // 这个s_cnt有意义吗？有时间考虑一下
      	}

    } else {
		// do nothing
    }

	free(loop_meta);

	return;
}

void LOOP_fprintf(LOOP* loop, FILE *fp)
{
	// do nothing now
}