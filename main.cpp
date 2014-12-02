#include "cfengine.h"

int main(int argc, char* argv[])
{
	time_t startTime, endTime;
	startTime = time(NULL);
	CFEngine* cfe = new CFEngine();
	cfe->initialize("train.0.csv", "test.0.csv");
	cfe->substractGlobalMeanFromUser();
	cfe->substractGlobalMeanFromItem();
	cfe->calculateItemMeans();
	cfe->substractItemMeansFromUser();
	cfe->substractItemMeansFromItem();
	cfe->calculateUserMeans();
	cfe->substractUserMeansFromUser();
	cfe->substractUserMeansFromItem();
	cfe->buildItemModels();
	cfe->evaluate();
	endTime = time(NULL);
	std::cout << "The total time cost is " << endTime - startTime << " seconds" << std::endl;
	getchar();
	return 0;
}