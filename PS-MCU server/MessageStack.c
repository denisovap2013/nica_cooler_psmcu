//==============================================================================
//
//
//==============================================================================

//==============================================================================
// Include files

//#include "MessageStack.h"

#include "MessageStack.h"
#include "TimeMarkers.h"
#include <stdarg.h>

message_stack_t MS_GlobalMessageStack = 0;

message_stack_t msGMS(void)
{
	return MS_GlobalMessageStack;	
}

void msInitStack(message_stack_t *msg_stack)
{
	if (!msg_stack) return;
	(*msg_stack) = malloc(sizeof(struct message_stack));
	(*msg_stack)->next = 0;
	(*msg_stack)->msg[0] = 0;
}

void msInitGlobalStack(void)
{
	if (MS_GlobalMessageStack) return;
	MS_GlobalMessageStack = malloc(sizeof(struct message_stack));
	MS_GlobalMessageStack->next = 0;
	MS_GlobalMessageStack->msg[0] = 0;
}

void msFlushMsgs(message_stack_t msg_stack)
{
	message_stack_t  pm,  pm2;
	
	if (!msg_stack) return;
	pm = msg_stack->next;
	msg_stack->next = 0;
	while (pm)
	{
		pm2 = pm;
		pm = pm2->next;
		free(pm2);
	}
}

void msAddMsg(message_stack_t msg_stack, char * msg, ...)
{
	message_stack_t  pm;
	char buf[256];
	va_list arglist;
	va_start(arglist,msg);
	
	if (!msg_stack) return;
	pm = msg_stack;
	while (pm->next)
	{
		pm = pm->next;
	}
	if (msg) 
	{
		vsprintf(buf,msg,arglist);
		strcpy(pm->msg,buf);
	}
	else strcpy(pm->msg,"Pity efforts to use the NULL message pointer.");
	
	msInitStack(&pm->next);
	
	va_end(arglist);
}

void msPrintMsgs(message_stack_t msg_stack, FILE *stream)
{
	msPrintMsgsWithPrefix(msg_stack, stream, "");
}

void msPrintMsgsWithPrefix(message_stack_t msg_stack, FILE *stream, char *prefix)
{
	message_stack_t  pm;
	
	// ========
	//if (stream != stdout) return;
	
	if (!msg_stack)
	{
		if (stream) fprintf(stream,"The wrong stack pointer.\n");
		return;
	}
	pm = msg_stack;
	
	if (stream)
		while (pm->next)
		{
			fprintf(stream,"%s%s\n", prefix, pm->msg);
			//printf("%s\n",pm->msg);
			pm = pm->next;
		}
}

void msReleaseStack(message_stack_t *msg_stack)
{
	 if(!msg_stack) return;
	 if(!(*msg_stack)) return;
	 
	 msFlushMsgs(*msg_stack);
	 free(*msg_stack);
	 *msg_stack = 0;
}

void msReleaseGlobalStack(void)
{
	 if(!MS_GlobalMessageStack) return;
	 
	 msFlushMsgs(MS_GlobalMessageStack);
	 free(MS_GlobalMessageStack);
	 MS_GlobalMessageStack = 0;
}

int msMsgsAvailable(message_stack_t msg_stack)
{
	int count=0;
	message_stack_t buf = msg_stack;
	if (msg_stack == 0) return 0;
	while (buf->next != 0) {
		count++;
		buf = buf->next;
	}
	return count;
}

// Wrapper over the global message stack with additionsl timestamp prefix
void logMessage(char *msg, ...) {
	char buf[256];
	va_list arglist;

	va_start(arglist, msg);
	vsprintf(buf, msg, arglist);
	va_end(arglist);

	msAddMsg(msGMS(), "%s %s", TimeStamp(0), buf);
}
