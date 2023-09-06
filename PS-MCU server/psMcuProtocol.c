//==============================================================================
//   The description of functions is in "psMcuProtocol.h"
//
// Purpose:     Functions for forming the data package acceptable by CANBUS protocol.   
//==============================================================================

//==============================================================================
// Include files

#include "psMcuProtocol.h"
#include <ansi_c.h>
#include "MessageStack.h"
#include "TimeMarkers.h" 

// ID generator
unsigned long psMcuProtocol_IDgen(unsigned char priority, unsigned char address, unsigned char reserved)
{
	unsigned long _id = 0;
	int errors = 0;
	
	if (priority < 5 || priority > 7)
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_IDGen /-/: Wrong priority code. Acceptable values are 5 (addressless), 6 (normal), 7 (replied).");
		errors = 1;
	}
	if (address > 0x3F)
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_IDGen /-/: Wrong address. It should be less or equal to 0x3F (0-63).");
		errors = 1;
	}
	
	if (errors) return 0;
	
	_id = ( ((priority << 6) | address) << 2) | (0x3 & reserved);	

	return _id;
}

// ID unpacking
unsigned char psMcuProtocol_PriorityFromID(unsigned long ID)
{
	if (ID > 0x7FF)
		msAddMsg(msGMS(),"/-/ psMcuProtocol_PriorityFromID /-/: Warning! ID contains non zero additional bits (ID11,ID12..). They will be ignored.");
	return (0x7 & (ID >> 8));
}

unsigned char psMcuProtocol_NumberFromID(unsigned long ID)
{
	if (ID > 0x7FF)
		msAddMsg(msGMS(),"/-/ psMcuProtocol_NumberFromID /-/: Warning! ID contains non zero additional bits (ID11,ID12..). They will be ignored.");
	return (0x3F & (ID >> 2));
}

unsigned char psMcuProtocol_FlagsFromID(unsigned long ID)
{
	if (ID > 0x7FF)
		msAddMsg(msGMS(),"/-/ psMcuProtocol_FlagsFromID /-/: Warning! ID contains non zero additional bits (ID11,ID12..). They will be ignored.");
	return (0x3 & ID);
}

// Message 00
unsigned int psMcuProtocol_Stop(unsigned char * data)				 
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_Stop /-/: The data buffer is not indicated.");
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	return 1;			// the first byte 00 is the descriptor of the command "stop"
}

// Message 01  
unsigned int psMcuProtocol_MultiChannelConfig(unsigned char * data, 
	unsigned char ChBeg, unsigned char ChEnd, psMcuProtocol_time_t Time, psMcuProtocol_mode_t Mode, unsigned char Label)	
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_MultiChannelConfig /-/: The data buffer is not indicated."); 
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0x01;
	
	// Channels range  
	if (ChBeg >= PSMCU_ADC_CHANNELS_NUM)
	{
		data[1] = PSMCU_ADC_CHANNELS_NUM - 1;
		msAddMsg(msGMS(),"/-/ psMcuProtocol_MultiChannelConfig /-/: ChBeg > %d (Automaticly set to 19).", PSMCU_ADC_CHANNELS_NUM - 1); 
	}
	else data[1] = ChBeg;
	
	if (ChEnd < data[1])
	{
		data[2] = data[1]; 
		msAddMsg(msGMS(),"/-/ psMcuProtocol_MultiChannelConfig /-/: ChEnd < ChBeg (Automaticly corrected)."); 
	}
		else 
			if(ChEnd >19)
			{
				data[2] = 19;
				msAddMsg(msGMS(),"/-/ psMcuProtocol_MultiChannelConfig /-/: ChEnd > 19 (Automaticly set to 7).");  
			}
			else data[2] = ChEnd;
	// Time of measurements
	data[3] = Time;
	// Measurements mode
	data[4] = Mode & (0x3 << 4);
	// Label
	if  (Label) data[5]=1; else data[5] = 0;
	
	return 6;
}

// Message 02
unsigned int psMcuProtocol_SingleAdcChannelRequest(unsigned char * data, unsigned char Channel, psMcuProtocol_time_t Time, psMcuProtocol_mode_t Mode)
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_SingleAdcChannelRequest /-/: The data buffer is not indicated."); 
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero 
	
	// descriptor 
	data[0] = 0x02;
	
	// Channels range  
	if (Channel > 19)
	{
		data[1] = 19;
		msAddMsg(msGMS(),"/-/ psMcuProtocol_SingleAdcChannelRequest /-/: Channel > 19 (Automaticly set to 19)."); 
	}
	else data[1] = Channel;
	
	data[2] = Time;
	// Measurements mode
	data[3] = Mode & (0x3 << 4);
	
	return 4;
}

// Message 03
unsigned int psMcuProtocol_MultiChannelRequest(unsigned char * data, unsigned char Channel)
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_MultiChannelRequest /-/: The data buffer is not indicated.");
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0x03;
	
	// Channels range  
	if (Channel > 7)
	{
		data[1] = 7;
		msAddMsg(msGMS(),"/-/ psMcuProtocol_MultiChannelRequest /-/: Channel > 7 (Automaticly set to 7).");  
	}
	else data[1] = Channel;
	
	return 2;
}

// Message 04
unsigned int psMcuProtocol_LoopBufferRequest(unsigned char * data, unsigned int Pointer)
{
	 if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_LoopBufferRequest /-/: The data buffer is not indicated.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0x04;
	
	if (Pointer > 4095)
	{
		data[1] = 0xFF;
		data[2] = 0xF;
		msAddMsg(msGMS(),"/-/ psMcuProtocol_LoopBufferRequest /-/: Pointer is out of range (should be not more than 0xFFFFFF) (Automaticly set to 0xFFFFFF)."); 
	}
	else
	{
		data[1] = 0xFF & Pointer;
		data[2] = 0xF & (Pointer >> 8);
	}
	
	return 3;
}


// Messages 80-87
unsigned int psMcuProtocol_WriteDAC(unsigned char * data, unsigned char channel, unsigned long code)
{
	if(!data)
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_WriteDAC /-/: The data buffer is not specified.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0x80 + channel;	
	
	data[4] = 0xFF & code;
	data[3] = 0xFF & (code >> 8);
	data[2] = 0xFF & (code >> 16);
	data[1] = 0xFF & (code >> 24);
	
	return 5;
}

// Messages 90-97
unsigned int psMcuProtocol_ReadDAC(unsigned char * data, unsigned char channel)
{
	if(!data)
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_ReadDAC /-/: The data buffer is not specified.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0x90 + channel;
	
	return 1;
}


// Message F8
unsigned int psMcuProtocol_RegistersRequest(unsigned char * data)
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_RegistersRequest /-/: The data buffer is not indicated.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0xF8;
	
	return 1;	
}

// Message F9
unsigned int psMcuProtocol_WriteToOutputRegister(unsigned char * data, unsigned char OutputRegister)
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_WriteToOutputRegister /-/: The data buffer is not indicated.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0xF9;
	data[1] = OutputRegister;
	
	return 2;	
}

// Message FE
unsigned int psMcuProtocol_StatusRequest(unsigned char * data)
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_StatusRequest /-/: The data buffer is not indicated.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0xFE;
	
	return 1;	
}

// Message FF
unsigned int psMcuProtocol_AttributesRequest(unsigned char * data)	  
{
	if(!data) 			// If we cannot write then send nothing. 
	{
		msAddMsg(msGMS(),"/-/ psMcuProtocol_AttributesRequest /-/: The data buffer is not indicated.");  
		return 0;		
	}
	
	memset(data,0,8);	// all bytes are set to zero
	
	// descriptor 
	data[0] = 0xFF;
	
	return 1;
}
