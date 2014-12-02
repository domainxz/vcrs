#ifndef CFENGINE_H
#define CFENGINE_H

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "utils.h"

class CFEngine
{
	public:
		CFEngine();
		~CFEngine();
		void initialize(const char*, const char*);
		void substractGlobalMeanFromUser() const;
		void substractGlobalMeanFromItem() const;
		void calculateItemMeans() const;
		void substractItemMeansFromUser() const;
		void substractItemMeansFromItem() const;
		void calculateUserMeans() const;
		void substractUserMeansFromUser() const;
		void substractUserMeansFromItem() const;
		void buildItemModels() const;
		void buildItemModelsFromFile(const char*) const;
		float predictRatings(index_t uid, index_t iid) const;
		void evaluate() const;
		void writeRatings(const char* writePath) const;
	private:
		float globalMean = 0;
		std::map<index_t, float>* itemMeans;
		std::map<index_t, float>* userMeans;
		std::map<index_t, index_t>* idIndexMap;
		std::map<index_t, std::map<index_t, float> >* userItemSparseMatrix;
		std::map<index_t, std::map<index_t, float> >* itemUserSparseMatrix;
		std::map<index_t, std::vector< std::pair<index_t, float> >* >* itemModels;
		std::map<index_t, std::map<index_t, std::pair<float, float> > >* testData;
};
#endif