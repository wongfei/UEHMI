#pragma once

#define HMI_GUARD_PARAM_NOT_EMPTY(Val) if (Val.IsEmpty()) { ProcError(TEXT("Invalid param "#Val)); return false; }
#define HMI_BREAK_ON_CANCEL() if (CancelFlag) { break; }

#define HMI_PROC_DECLARE_STATS(ProcName)\
	DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT(#ProcName), STAT_HMI_##ProcName##_Elapsed, STATGROUP_HMI)

#define HMI_PROC_PRE_WORK_STATS(ProcName)

#define HMI_PROC_POST_WORK_STATS(ProcName)\
	SET_FLOAT_STAT(STAT_HMI_##ProcName##_Elapsed, GetLastOperationDuration())
