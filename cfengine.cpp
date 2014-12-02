#include "cfengine.h"

CFEngine::CFEngine()
{
	this->globalMean = 0;
	this->itemMeans = new std::map<index_t, float>();
	this->userMeans = new std::map<index_t, float>();
	this->idIndexMap = new std::map<index_t, index_t>();
	this->userItemSparseMatrix = new std::map<index_t, std::map<index_t, float> >();
	this->itemUserSparseMatrix = new std::map<index_t, std::map<index_t, float> >();
	this->itemModels = new std::map<index_t, std::vector< std::pair<index_t, float> >* >();
	this->testData = new std::map<index_t, std::map<index_t, std::pair<float, float> > >();
}

CFEngine::~CFEngine()
{
	this->globalMean = 0;
	delete this->itemMeans;
	this->itemMeans = NULL;
	delete this->userMeans;
	this->userMeans = NULL;
	delete this->idIndexMap;
	this->idIndexMap = NULL;
	delete this->userItemSparseMatrix;
	this->userItemSparseMatrix = NULL;
	delete this->itemUserSparseMatrix;
	this->itemUserSparseMatrix = NULL;
	for (auto itemIter = this->itemModels->begin(); itemIter != this->itemModels->end(); itemIter++)
	{
		delete itemIter->second;
		itemIter->second = NULL;
	}
	delete this->itemModels;
	this->itemModels = NULL;
	delete this->testData;
	this->testData = NULL;
}

void CFEngine::initialize(const char* trainDataPath, const char* testDataPath)
{
	std::cout << "Initialize" << std::endl;
	std::string line;
	std::ifstream ifs;
	index_t uid = 0, iid = 0, timestamp = 0, count = 0;
	float rating = 0.0, total = 0.0;
	ifs.open(trainDataPath);
	while (getline(ifs, line))
	{
		sscanf_s(line.c_str(), "%u,%u,%f,%u", &uid, &iid, &rating, &timestamp);
		(*this->userItemSparseMatrix)[uid][iid] = rating;
		(*this->itemUserSparseMatrix)[iid][uid] = rating;
		total += rating;
		count++;
	}
	std::cout << "User:" << this->userItemSparseMatrix->size() << std::endl;
	std::cout << "Item:" << this->itemUserSparseMatrix->size() << std::endl;
	std::cout << "Total rating:" << count << std::endl;
	for (auto userIter = this->userItemSparseMatrix->cbegin(); userIter != this->userItemSparseMatrix->cend(); userIter++)
	{
		const index_t &visitId = userIter->first;
		(*this->userMeans)[visitId] = 0;
	}
	for (auto itemIter = this->itemUserSparseMatrix->cbegin(); itemIter != this->itemUserSparseMatrix->cend(); itemIter++)
	{
		const index_t &visitId = itemIter->first;
		(*this->itemMeans)[visitId] = 0;
		(*this->itemModels)[visitId] = NULL;
		(*this->idIndexMap)[visitId] = (index_t)this->idIndexMap->size();
	}
	ifs.close();
	ifs.open(testDataPath);
	while (getline(ifs, line))
	{
		sscanf_s(line.c_str(), "%u,%u,%f,%u", &uid, &iid, &rating, &timestamp);
		(*this->testData)[uid][iid] = std::pair<float, float>(rating, (float)0.0);
	}
	ifs.close();
	this->globalMean = total / count;
	std::cout.precision(dbl::digits10);
	std::cout << "Global item rating mean:" << this->globalMean << std::endl;
}

void CFEngine::substractGlobalMeanFromUser() const
{
	std::cout << "Substract global mean from users" << std::endl;
	// For user iterator, the first is the user id and the second is the item-rating map
	for (auto userIter = this->userItemSparseMatrix->begin(); userIter != this->userItemSparseMatrix->end(); userIter++)
	{
		// For item iterator, the first is the item id and the second is the rating
		for (auto itemIter = userIter->second.begin(); itemIter != userIter->second.end(); itemIter++)
		{
			itemIter->second -= this->globalMean;
		}
	}
}

void CFEngine::substractGlobalMeanFromItem() const
{
	std::cout << "Substract global mean from items" << std::endl;
	// For item iterator, the first is the user id and the second is the user-rating map
	for (auto itemIter = this->itemUserSparseMatrix->begin(); itemIter != this->itemUserSparseMatrix->end(); itemIter++)
	{
		// For user iterator, the first is the user id and the second is the rating
		for (auto userIter = itemIter->second.begin(); userIter != itemIter->second.end(); userIter++)
		{
			userIter->second -= this->globalMean;
		}
	}
}

void CFEngine::calculateUserMeans() const
{
	std::cout << "Calculate user means" << std::endl;
	doAtom1(this->userItemSparseMatrix, this->userMeans, atomCalculateMean);
}

void CFEngine::calculateItemMeans() const
{
	std::cout << "Calculate item means" << std::endl;
	doAtom1(this->itemUserSparseMatrix, this->itemMeans, atomCalculateMean);
}

void CFEngine::substractItemMeansFromUser() const
{
	std::cout << "Substract item means from user-item matrix" << std::endl;
	doAtom1(this->userItemSparseMatrix, this->itemMeans, atomSubstract1);
}

void CFEngine::substractItemMeansFromItem() const
{
	std::cout << "Substract item means from item-user matrix" << std::endl;
	doAtom1(this->itemUserSparseMatrix, this->itemMeans, atomSubstract2);
}

void CFEngine::substractUserMeansFromUser() const
{
	std::cout << "Substract user means from user-item matrix" << std::endl;
	doAtom1(this->userItemSparseMatrix, this->userMeans, atomSubstract2);
}

void CFEngine::substractUserMeansFromItem() const
{
	std::cout << "Substract user means from item-user matrix" << std::endl;
	doAtom1(this->itemUserSparseMatrix, this->userMeans, atomSubstract1);
}

void CFEngine::buildItemModels() const
{
	std::cout << "Build item-item model by calculating similaritites" << std::endl;
	size_t index = 0;
	const size_t length = (this->itemModels->size() - 1)*(this->itemModels)->size() / 2;
	float* sims = new float[length];
	memset(sims, 0, sizeof(float)*length);
	std::vector< std::pair<index_t, index_t> > jobLists[MAXTHREADS];
	std::vector<index_t>                      jobLists2[MAXTHREADS];
	threadParam3*                            pDataArray[MAXTHREADS];
	threadParam4*                           pDataArray2[MAXTHREADS];
	DWORD                               dwThreadIdArray[MAXTHREADS];
	HANDLE                                 hThreadArray[MAXTHREADS];
	for (auto iter1 = (this->itemUserSparseMatrix)->begin(); iter1 != (this->itemUserSparseMatrix)->end(); iter1++)
	{
		const index_t &visitId1 = iter1->first;
		auto iter2 = iter1;
		for (iter2++; iter2 != (this->itemUserSparseMatrix)->end(); iter2++)
		{
			const index_t &visitId2 = iter2->first;
			jobLists[index++%MAXTHREADS].push_back(std::pair<index_t, index_t>(visitId1, visitId2));
		}
	}
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		pDataArray[i] = (threadParam3*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(threadParam3));
		if (pDataArray[i] == NULL)
		{
			ExitProcess(2);
		}
		hThreadArray[i] = NULL;
		pDataArray[i]->list   = &jobLists[i];
		pDataArray[i]->input  = this->itemUserSparseMatrix;
		pDataArray[i]->index  = this->idIndexMap;
		pDataArray[i]->output = sims;
		if (hThreadArray[i] == NULL)
		{
			hThreadArray[i] = CreateThread(
				NULL,
				0,
				atomCalculateItemSimilarities,
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
	index = 0;
	for (auto iter1 = this->itemUserSparseMatrix->begin(); iter1 != this->itemUserSparseMatrix->end(); iter1++)
	{
		jobLists2[index++%MAXTHREADS].push_back(iter1->first);
	}
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		pDataArray2[i] = (threadParam4*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(threadParam4));
		if (pDataArray2[i] == NULL)
		{
			ExitProcess(2);
		}
		hThreadArray[i] = NULL;
		pDataArray2[i]->list   = &jobLists2[i];
		pDataArray2[i]->input  = itemUserSparseMatrix;
		pDataArray2[i]->index  = idIndexMap;
		pDataArray2[i]->sims   = sims;
		pDataArray2[i]->output = itemModels;
		if (hThreadArray[i] == NULL)
		{
			hThreadArray[i] = CreateThread(
				NULL,
				0,
				atomGenerateItemBasedModels,
				pDataArray2[i],
				0,
				&dwThreadIdArray[i]);
		}
	}
	WaitForMultipleObjects(MAXTHREADS, hThreadArray, TRUE, INFINITE);
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (pDataArray2[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pDataArray2[i]);
			pDataArray2[i] = NULL;
		}
	}
	delete sims;
}

void CFEngine::buildItemModelsFromFile(const char* simsFile) const
{
	std::cout << "Build item-item model by loading file" << std::endl;
	size_t index = 0;
	const size_t length = (this->itemModels->size() - 1)*(this->itemModels)->size() / 2;
	float* sims = new float[length];
	memset(sims, 0, sizeof(float)*length);
	std::vector<index_t>                      jobLists2[MAXTHREADS];
	threadParam4*                           pDataArray2[MAXTHREADS];
	DWORD                               dwThreadIdArray[MAXTHREADS];
	HANDLE                                 hThreadArray[MAXTHREADS];
	std::string line;
	std::ifstream ifs;
	index_t iid1 = 0, iid2 = 0;
	float sim = 0;
	ifs.open(simsFile);
	while (getline(ifs, line))
	{
		sscanf_s(line.c_str(), "%u,%u,%f", &iid1, &iid2, &sim);
		auto iter1 = this->idIndexMap->find(iid1);
		auto iter2 = this->idIndexMap->find(iid2);
		if (iter1 != this->idIndexMap->end() && iter2 != this->idIndexMap->end())
		{
			sims[getIndex(iter1->second, iter2->second, (index_t)this->itemMeans->size())] = sim;
		}
	}
	ifs.close();
	
	for (auto iter1 = this->itemUserSparseMatrix->begin(); iter1 != this->itemUserSparseMatrix->end(); iter1++)
	{
		jobLists2[index++%MAXTHREADS].push_back(iter1->first);
	}
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		pDataArray2[i] = (threadParam4*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(threadParam4));
		if (pDataArray2[i] == NULL)
		{
			ExitProcess(2);
		}
		hThreadArray[i] = NULL;
		pDataArray2[i]->list = &jobLists2[i];
		pDataArray2[i]->input = itemUserSparseMatrix;
		pDataArray2[i]->index = idIndexMap;
		pDataArray2[i]->sims = sims;
		pDataArray2[i]->output = itemModels;
		if (hThreadArray[i] == NULL)
		{
			hThreadArray[i] = CreateThread(
				NULL,
				0,
				atomGenerateItemBasedModels,
				pDataArray2[i],
				0,
				&dwThreadIdArray[i]);
		}
	}
	WaitForMultipleObjects(MAXTHREADS, hThreadArray, TRUE, INFINITE);
	for (index_t i = 0; i < MAXTHREADS; i++)
	{
		CloseHandle(hThreadArray[i]);
		if (pDataArray2[i] != NULL)
		{
			HeapFree(GetProcessHeap(), 0, pDataArray2[i]);
			pDataArray2[i] = NULL;
		}
	}
	delete sims;
}

float CFEngine::predictRatings(index_t uid, index_t iid) const
{
	float prediction = this->globalMean;
	auto itemMean    = this->itemMeans->find(iid);
	auto userMean    = this->userMeans->find(uid);
	auto itemModel   = this->itemModels->find(iid);
	auto userHistory = this->userItemSparseMatrix->find(uid);
	if (itemMean != this->itemMeans->end())
	{
		prediction += itemMean->second;
		if (userMean != this->userMeans->end())
		{
			prediction += userMean->second;
			if (userHistory != this->userItemSparseMatrix->end())
			{
				index_t count = 0;
				float bias = 0;
				float absSimSum = 0;
				std::vector< std::pair<index_t, float> >* itemBasedKnn = itemModel->second;
				std::map<index_t, float> &userRatedItems = userHistory->second;
				if (itemBasedKnn != NULL)
				{
					for (auto itemIter = itemBasedKnn->begin(); itemIter != itemBasedKnn->end() && count < NSIZE; itemIter++)
					{
						auto hitItem = userRatedItems.find(itemIter->first);
						if (hitItem != userRatedItems.end())
						{
							bias += itemIter->second * hitItem->second;
							absSimSum += abs(itemIter->second);
							count++;
						}
					}
					if (absSimSum > 0)
					{
						prediction += bias / absSimSum;
					}
				}
			}
		}
	}
	return prediction;
}

void CFEngine::evaluate() const
{
	float totalSE = 0;
	size_t count = 0;
	for (auto userIter = this->testData->begin(); userIter != this->testData->end(); userIter++)
	{
		for (auto itemIter = userIter->second.begin(); itemIter != userIter->second.end(); itemIter++)
		{
			float prediction = predictRatings(userIter->first, itemIter->first);
			if (prediction < 1.0)
			{
				prediction = 1.0;
			}
			if (prediction > 5.0)
			{
				prediction = 5.0;
			}
			totalSE += (itemIter->second.first - prediction)*(itemIter->second.first - prediction);
			count++;
		}
	}
	printf("The RMSE is %f\n", sqrt(totalSE / count));
}