#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAXTHREADS 40
#define NSIZE 30
#define MSIZE 500
#define DAMPING 25
#define THRESHHOLD 0.0

typedef DWORD(WINAPI *func)(LPVOID);
typedef unsigned int index_t;
typedef std::numeric_limits< float > dbl;

struct threadParam1
{
	std::vector<index_t>* jobList;
	std::map<index_t, std::map<index_t, float> >* matrix;
	std::map<index_t, float>* means;
};

struct threadParam3
{
	std::vector< std::pair<index_t, index_t> >* list;
	std::map<index_t, std::map<index_t, float> >* input;
	std::map<index_t, index_t>* index;
	float* output;
};

struct threadParam4
{
	std::vector<index_t>* list;
	std::map<index_t, std::map<index_t, float> >* input;
	std::map<index_t, index_t>* index;
	float* sims;
	std::map<index_t, std::vector< std::pair<index_t, float> >* >* output;
};

extern index_t getIndex(index_t, index_t, index_t);

extern bool sort_condition(std::pair<index_t, float>, std::pair<index_t, float>);

extern float calculateCosineSimiliarity(const std::map<index_t, float> &, const std::map<index_t, float> &);

extern DWORD WINAPI atomCalculateMean(LPVOID);

extern DWORD WINAPI atomSubstract1(LPVOID);

extern DWORD WINAPI atomSubstract2(LPVOID);

extern void doAtom1(std::map<index_t, std::map<index_t, float> >*, std::map<index_t, float>*, func);

extern DWORD WINAPI atomCalculateItemSimilarities(LPVOID);

extern DWORD WINAPI atomGenerateItemBasedModels(LPVOID);
#endif