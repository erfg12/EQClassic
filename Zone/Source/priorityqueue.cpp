#include "priorityqueue.h"
#include "nodes.h"
#include <iostream>
#include <conio.h>
 
bool stateTest(Node* current, Node* goal);
 
using namespace std;
 
/** Default constructor */
Queue::Queue(){
	root = 0;
	size = 0;
	oldStatesSize =0;
}
/** Constructor that sets first queue node */
Queue::Queue(Node* parent){
	root = parent;
	size = 1;
}
/** Clears memory used by the queue */
Queue::~Queue(){
	//root->destroyNode(root);
}
/** Returns root node of queue (not used)*/
Node* Queue::getRoot(){
	return root;
}
/** Returns current size of queue */
int Queue::getSize(){
	return size;
}
/**Insertion method to add new nodes to queue
\  which uses insertion sort if node's state is 
\  not already in the queue*/
void Queue::add(Node* newNode){
	if(oldState(newNode)){
		return;
	}
 
	//if Q is emtpy, place newNode at start of Q;
	if(root == 0 || size == 0){
		root = newNode;
		size = 1;
		return;
	}else{
		Node* currentNode;
		Node* previousNode;
 
		//Test for state already in Q, if it exists return;
		currentNode = root;
		previousNode = root;
		for(int i=0;i<size;i++){
			if(stateTest(newNode, currentNode)){
				//If state exists and is worse, delete it.
				if(newNode->fValue < currentNode->fValue){
					if(i==0){
						root = root->next;
					}else if(i<size-1){
						previousNode->next = currentNode->next;
					}else if(i==size-1){
						previousNode->next = 0;
					}
					size--; 
					break;
				}else{
					return;
				}
			}else{
				previousNode = currentNode;
				currentNode = currentNode->next;
			}
		}
 
		//Compare to first elemenent;
		if(newNode->fValue < root->fValue){
			//Insert node here
			newNode->next = root;
			root = newNode;
			size++;
			return;
		}
 
		currentNode = root;
		previousNode = root;
		//Begin insertion sort on f(n) value;
		for(int k=0;k<size;k++){
			if(newNode->fValue < currentNode->fValue){
				//Insert node here
				previousNode->next = newNode;
				newNode->next = currentNode;
				size++;
				return;
			}else if(currentNode->next == 0){
				//Append to end of Q;
				currentNode->next = newNode;
				size++;
				return;
			}else{
				previousNode = currentNode;
				currentNode = currentNode->next;
			}
		}
	}
}
/** Returns front node in queue (which will be lowest f(n) value) */
Node* Queue::remove(){
	Node* popNode = NULL;
	popNode = root;
	root = root->next;
	size--;
 
	oldStates.push_back(popNode);
	return popNode;
}
 
int Queue::getOldStatesSize(){
	return oldStatesSize;
}
 
/** Returns true if two states are the same */
bool stateTest(Node* current, Node* goal){
	if(current->x == goal->x && current->y == goal->y && current->z == goal->z){
		return true;
	}else{
		return false;
	}
}
/** Prints all nodes in queue for debugging */
void Queue::printQueue(){
	//if(size>0){
	//	Node* tmpRoot = root;
	//	while(tmpRoot->next != 0){
	//		tmpRoot->printState();
	//		cout << "(" << (int)tmpRoot->getfValue() << ")->";
	//		tmpRoot = tmpRoot->next;
	//	}
	//
	//	tmpRoot->printState();
	//	cout << "(" << (int)tmpRoot->getfValue() << ")]";
	//}
	//else{
	//	cout << " --- Queue is Empty ----\n";
	//}
}
 
 
bool Queue::oldState(Node* testNode){
	for(int i=0;i<(int)oldStates.size();i++){
		if(stateTest(oldStates.at(i), testNode)){
			if(testNode->fValue < oldStates.at(i)->fValue){
	
			}else{
				return true;
			}
		}
	}
	
	return false;
}