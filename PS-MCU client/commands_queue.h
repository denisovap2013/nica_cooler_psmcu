//==============================================================================
//
// Title:       commands_queue.h
// Purpose:     A short description of the interface.
//
// Created on:  30.11.2021 at 11:26:25 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __commands_queue_H__
#define __commands_queue_H__

#define COMMANDS_QUEUE_CAPACITY 20

typedef struct
{
	char commands[COMMANDS_QUEUE_CAPACITY][256];			// commands
	int start, end;
	int commandsNumber;
	int awaitAnswer[COMMANDS_QUEUE_CAPACITY];
} commands_queue_t;

void initQueue(commands_queue_t *queue);
int addCommandToQueue(commands_queue_t *queue, char * command, int awaitAnswer);
int popCommandFromQueue(commands_queue_t *queue, char* tgtString);
int isQueueEmpty(commands_queue_t *queue);
int isQueueFull(commands_queue_t *queue);
int getQueueElementsNumber(commands_queue_t *queue); 

#endif  /* ndef __commands_queue_H__ */
