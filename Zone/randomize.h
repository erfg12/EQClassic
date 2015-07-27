#include <time.h>
#include <iostream>
#include <math.h>

/** Returns a random integer between 0 and parameter. */
int iRandomNum(int max){
	int seed = rand()%max + 0;
	srand((unsigned int)(time(0)* rand()));
	return (seed);
}
float fRandomNum(float max){
	float seed = (((float)rand()/(float)RAND_MAX)*(max));
	srand((unsigned int)(time(0) * rand()));
	return (seed);
}
/** Returns a random number between 0 and parameter. */
float fRandomNum(float min, float max){
	float seed = (((float)rand()/(float)RAND_MAX)*(max-min))+ min;
	srand((unsigned int)(time(0) * seed));
	return (seed);
}
 
/** Returns a number from a random normal distribution. */
float Normal(float mean, float std){
  	float sum = 0;
 
	for(float i=0;i<12;i++){
		float ran_num = fRandomNum(1);
		sum = sum + ran_num;
	}
 
	std = std * std;//square(std);
 
	float gaussian = mean + std * (sum - (float)6);
	return gaussian;
}