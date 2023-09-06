//==============================================================================
//
//
//==============================================================================

#ifndef __MessageStack_H__
#define __MessageStack_H__

#include <ansi_c.h>  

typedef struct message_stack
{
	struct message_stack * next;
	char msg[256];
}  *message_stack_t;


extern message_stack_t MS_GlobalMessageStack;
 
message_stack_t msGMS(void);

void msInitStack(message_stack_t *msg_stack);

void msInitGlobalStack(void); 

void msFlushMsgs(message_stack_t msg_stack);

void msAddMsg(message_stack_t msg_stack,char * msg,...);

void msPrintMsgs(message_stack_t msg_stack,FILE * stream);

void msReleaseStack(message_stack_t *msg_stack);

void msReleaseGlobalStack(void);

int msMsgsAvailable(message_stack_t msg_stack);

#endif  /* ndef __MessageStack_H__ */
