#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H
 
#include <vector>
#include <string>
#include "nodes.h"
 
using std::string;
using std::vector;
 
class Queue
    {
    public:
 
	Queue(Node* parent);
	Queue();
	~Queue();
 
	void add(Node* newNode);	//Adds node to queue with insertion sort in f(n);
	void printQueue();			//Prints all node's in queue for debugging;
	Node* getRoot();			//Returns root of queue nodes for deallocation;
	Node* remove();				//Returns first node no queue (which has lowest f(n);
	int getSize();				//Returns size of queue;
	int getOldStatesSize();		//Returns oldStatesSize;
	bool oldState(Node* testNode);
 
    private:
		Node* root;				//Root of queue nodes;
		vector<Node*> oldStates;	//Holds all states already expanded (not used);
		int oldStatesSize;		//Number of states expanded;
		int size;				//Size of queue;
	};
 
#endif