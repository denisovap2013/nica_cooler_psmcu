//==============================================================================
//
// Title:       commands_queue.c
// Purpose:     A short description of the implementation.
//
// Created on:  30.11.2021 at 11:26:25 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <ansi_c.h>
#include "commands_queue.h"


void initQueue(commands_queue_t *queue) {
	memset(queue->commands, 0, sizeof(queue->commands));
	memset(queue->awaitAnswer, 0, sizeof(queue->awaitAnswer));
	queue->start = 0;
	queue->end = 0;
	queue->commandsNumber = 0;
}


int addCommandToQueue(commands_queue_t *queue, char * command, int awaitAnswer) {
	int i;
	
	if (isQueueFull(queue)) return 0;
	
	strcpy(queue->commands[queue->end], command);
	queue->awaitAnswer[queue->end] = awaitAnswer;
	queue->end = (queue->end + 1) % COMMANDS_QUEUE_CAPACITY;
	queue->commandsNumber++;
	
	return 1;
}


int popCommandFromQueue(commands_queue_t *queue, char *tgtString) {
	int awaitAnswer;
	if (isQueueEmpty(queue)) return NULL;
	
	
	strcpy(tgtString, queue->commands[queue->start]);
	awaitAnswer = queue->awaitAnswer[queue->start];
	
	queue->start = (queue->start + 1) % COMMANDS_QUEUE_CAPACITY;
	queue->commandsNumber--;
	
	if (queue->commandsNumber == 0) {
		queue->start = 0;
		queue->end = 0;
	}
	
	return awaitAnswer;
}


int isQueueEmpty(commands_queue_t *queue) {
	if (queue->commandsNumber == 0) return 1;
	return 0;	
}


int isQueueFull(commands_queue_t *queue) {
	if (queue->commandsNumber == COMMANDS_QUEUE_CAPACITY) return 1;
	return 0;	
}


int getQueueElementsNumber(commands_queue_t *queue) {
	return queue->commandsNumber;	
}
