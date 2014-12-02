#include "utils.h"

index_t getIndex(index_t a, index_t b, index_t size)
{
	index_t nums = (a < b) ? a : b;
	index_t numb = (a > b) ? a : b;
	return (nums * (size * 2 - nums - 1)) / 2 + numb - nums - 1;
}

bool sort_condition(std::pair<index_t, float> a, std::pair<index_t, float> b)
{
	return (a.second > b.second);
}

float calculateCosineSimiliarity(const std::map<index_t, float> &vector1, const std::map<index_t, float> &vector2)
{
	float dividend = 0, divisor = 0, divisor1 = 0, divisor2 = 0, similarity = 0;
	for (auto iter1 = vector1.begin(); iter1 != vector1.end(); iter1++)
	{
		const float &var1 = iter1->second;
		divisor1 += var1 * var1;

		const index_t &visitId = iter1->first;
		auto iter2 = vector2.find(visitId);
		if (iter2 != vector2.end())
		{
			const float &var1 = iter1->second;
			const float &var2 = iter2->second;
			dividend += var1 * var2;
		}
	}
	for (auto iter1 = vector2.begin(); iter1 != vector2.end(); iter1++)
	{
		const float &var1 = iter1->second;
		divisor2 += var1 * var1;
	}
	divisor = sqrt(divisor1) * sqrt(divisor2) + DAMPING;
	if (divisor > 0)
	{
		similarity = dividend / divisor;
	}
	return similarity;
}
//A thread fucntion whihc caculates the damping means for the selected rows of the matrix
DWORD WINAPI atomCalculateMean(LPVOID param)
{
	threadParam1* pparam = (threadParam1*)param;
	std::vector<index_t>* jobList = pparam->jobList;
	std::map<index_t, std::map<index_t, float> >* matrix = pparam->matrix;
	std::map<index_t, float>* means = pparam->means;
	for (auto iter1 = jobList->cbegin(); iter1 != jobList->cend(); iter1++)
	{
		const index_t &visitId = *iter1;
		auto iter2 = matrix->find(visitId);
		if (iter2 != matrix->end())
		{
			auto &ratings = iter2->second;
			for (auto iter = ratings.cbegin(); iter != ratings.cend(); iter++)
			{
				const float &operateVal = iter->second;
				(*means)[visitId] += operateVal;
			}
			(*means)[visitId] /= (ratings.size() + DAMPING);
		}
	}
	return 0;
}

DWORD WINAPI atomSubstract1(LPVOID param)
{
	threadParam1* pparam = (threadParam1*)param;
	std::vector<index_t> &list = *pparam->jobList;
	std::map<index_t, std::map<index_t, float> > &matrix = *pparam->matrix;
	std::map<index_t, float> &means = *pparam->means;
	for (index_t index = 0; index < list.size(); index++)
	{
		index_t &visitId = list[index];
		std::map<index_t, float> &ratings = matrix[visitId];
		for (auto iter = ratings.begin(); iter != ratings.end(); iter++)
		{
			iter->second -= means[iter->first];
		}
	}
	return 0;
}

DWORD WINAPI atomSubstract2(LPVOID param)
{
	threadParam1* pparam = (threadParam1*)param;
	std::vector<index_t> &list = *pparam->jobList;
	std::map<index_t, std::map<index_t, float> > &matrix = *pparam->matrix;
	std::map<index_t, float> &means = *pparam->means;
	for (index_t index = 0; index < list.size(); index++)
	{
		index_t &visitId = list[index];
		std::map<index_t, float> &ratings = matrix[visitId];
		for (auto iter = ratings.begin(); iter != ratings.end(); iter++)
		{
			iter->second -= means[visitId];
		}
	}
	return 0;
}

void doAtom1(std::map<index_t, std::map<index_t, float> > *matrix, std::map<index_t, float>* means, func call_back)
{
	index_t index = 0;
	std::vector<index_t> jobLists[MAXTHREADS];
	threadParam1*     pDataArray[MAXTHREADS];
	DWORD        dwThreadIdArray[MAXTHREADS];
	HANDLE          hThreadArray[MAXTHREADS];
	for (auto keyIter = matrix->begin(); keyIter != matrix->end(); keyIter++)
	{
		jobLists[index++%MAXTHREADS].push_back(keyIter->first);
	}
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		pDataArray[i] = (threadParam1*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(threadParam1));
		if (pDataArray[i] == NULL)
		{
			ExitProcess(2);
		}
		hThreadArray[i] = NULL;
		pDataArray[i]->jobList = &jobLists[i];
		pDataArray[i]->matrix = matrix;
		pDataArray[i]->means = means;
		if (hThreadArray[i] == NULL)
		{
			hThreadArray[i] = CreateThread(
				NULL,
				0,
				call_back,
				pDataArray[i],
				0,
				&dwThreadIdArray[i]);
		}
	}
	WaitForMultipleObjects(MAXTHREADS, hThreadArray, TRUE, INFINITE);
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (pDataArray[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pDataArray[i]);
			pDataArray[i] = NULL;
		}
	}
}

DWORD WINAPI atomCalculateItemSimilarities(LPVOID param)
{
	threadParam3* pparam = (threadParam3*)param;
	std::vector< std::pair<index_t, index_t> > &list = *pparam->list;
	std::map<index_t, std::map<index_t, float> > &input = *pparam->input;
	std::map<index_t, index_t> &idIndexMap = *pparam->index;
	float* output = pparam->output;
	for (auto iter = list.begin(); iter != list.end(); iter++)
	{
		index_t &visitId1 = iter->first;
		index_t &visitId2 = iter->second;
		std::map<index_t, float> &vector1 = input[visitId1];
		std::map<index_t, float> &vector2 = input[visitId2];
		index_t index = getIndex(idIndexMap[visitId1], idIndexMap[visitId2], (index_t)idIndexMap.size());
		output[index] = (float)max(0, calculateCosineSimiliarity(vector1, vector2));
	}
	return 0;
}

DWORD WINAPI atomGenerateItemBasedModels(LPVOID param)
{
	threadParam4* pparam = (threadParam4*)param;
	std::vector<index_t> &list = *pparam->list;
	std::map<index_t, index_t> &idIndexMap = *pparam->index;
	std::map<index_t, std::map<index_t, float> > &input = *pparam->input;
	std::map<index_t, std::vector< std::pair<index_t, float> >* > &output = *pparam->output;
	float* sims = pparam->sims;

	for (auto iter1 = list.cbegin(); iter1 != list.cend(); iter1++)
	{
		const index_t &visitId1 = *iter1;
		std::vector< std::pair<index_t, float> >* itemSimilarites = new std::vector< std::pair<index_t, float> >();
		for (auto iter2 = input.begin(); iter2 != input.end(); iter2++)
		{
			const index_t &visitId2 = iter2->first;
			if (visitId1 != visitId2)
			{
				const index_t index = getIndex(idIndexMap[visitId1], idIndexMap[visitId2], (index_t)idIndexMap.size());
				itemSimilarites->push_back(std::pair<index_t, float>(visitId2, sims[index]));
			}
		}
		sort(itemSimilarites->begin(), itemSimilarites->end(), sort_condition);
		itemSimilarites->erase(itemSimilarites->begin() + MSIZE, itemSimilarites->end());
		output[visitId1] = itemSimilarites;
	}
	return 0;
}