vcrs
====

A visual c++ program to perform item-based Collaborative Filtering
The program is multiple-thread for windows and the thread amount is defined in utils.h.
The program accept the files of such line format :
uid,iid,rating,timestamp
This format is similar to the original movielens data file where the seperator is '::'.
Therefore, if you just run this code on movielens, you can replace the '::' to ',' or just change the readline function in CFEngine::initialize 
