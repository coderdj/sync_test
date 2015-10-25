#include <iostream>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include "CAENVMElib.h"
#include "CAENVMEtypes.h"

using namespace std;

#define NBoards 8


/*
 * This is a simple test function that repeatedly initializes,
 * reads and writes registers, reads and writes data, and closes
 * a single CAEN V1724. You should have the board hooked up to 
 * a PCI card with a single optical link going directly to the board.
 *
 * Requires CAENVMElib. The A2818 or A3818 driver must be installed.
*/

int WriteRegister(u_int32_t reg, u_int32_t val, int handle){
  int gBOARD_VME=0;
  int ret = CAENVME_WriteCycle(handle,gBOARD_VME+reg,
			       &val,cvA32_U_DATA,cvD32);    
  if(ret!=cvSuccess)
    cout<<"Failed to write register "<<hex<<reg<<" with value "<<val<<
      ", returned value: "<<dec<<ret<<endl;
  return ret;
}

int ReadRegister(u_int32_t reg, u_int32_t &val, int handle){
  int gBOARD_VME=0;
  int ret = CAENVME_ReadCycle(handle,gBOARD_VME+reg,
			      &val,cvA32_U_DATA,cvD32);
  if(ret!=cvSuccess)
    cout<<"Failed to read register "<<hex<<reg<<" with return value: "<<
      dec<<ret<<endl;
  return ret;
}

int initializeBoard(int link, int board){

  CVBoardTypes BType = cvV2718;
  int tempHandle = -1;
  int cerror;
  if((cerror=CAENVME_Init(BType,link,board,
			  &tempHandle))!=cvSuccess){
    stringstream therror;
    therror<<"DigiInterface::Initialize - Error in CAEN initialization link "
	   <<link<<" crate "<<board<<": "<<cerror;
    return -1;
  }
  
  if(tempHandle <0)
    return tempHandle;
  int handle=tempHandle;
  int r=0;

  r+= WriteRegister(0xEF24, 0x1, handle);
  r+= WriteRegister(0xEF1C, 0x1, handle);
  r+= WriteRegister(0x811C, 0x110, handle);
  r+= WriteRegister(0x81A0, 0x200, handle);
  r+= WriteRegister(0xEF00, 0x10, handle);
  r+= WriteRegister(0x8100, 0x5, handle);
  r+= WriteRegister(0x8120, 0xFF, handle);
  r+= WriteRegister(0x800C, 0xA, handle);
  r+= WriteRegister(0x8098, 0x1000, handle);
  r+= WriteRegister(0x8000, 0x310, handle);
  r+= WriteRegister(0x8080, 0x310000, handle);
  r+= WriteRegister(0x8034, 0x0, handle);
  r+= WriteRegister(0x8038, 0x64, handle);
  r+= WriteRegister(0x8020, 0x190, handle);
  r+= WriteRegister(0x8060, 0x3E8, handle);
  r+= WriteRegister(0x8078, 0x64, handle);
  

  return tempHandle;

}

int loopData(vector<int> handles)
{
  for(unsigned int ihandle=0;ihandle<handles.size();ihandle++){
    
    int handle = handles[ihandle];
    int gBOARD_VME=0;
    unsigned int blt_bytes=0, buff_size=10000, blt_size=524288;
    int nb=0,ret=-5;
    u_int32_t *buff = new u_int32_t[buff_size]; //too large is OK         
    do{
      ret = CAENVME_FIFOBLTReadCycle(handle,gBOARD_VME,
				     ((unsigned char*)buff)+blt_bytes,
				     blt_size,cvA32_U_BLT,cvD32,&nb);
      if((ret!=cvSuccess) && (ret!=cvBusError)){
	cout<<"Board read error: "<<ret<<" for board "<<handle<<endl;
	delete[] buff;
	return -1;
      }
      blt_bytes+=nb;
      if(blt_bytes>buff_size)   {
	cout<<"Buffer size too small for read!"<<endl;
	delete[] buff;
	return -1;
      }
    }while(ret!=cvBusError);
    
    int idx=0;
    while((u_int32_t)idx<(blt_bytes/sizeof(u_int32_t))&&  buff[idx]!=0xFFFFFFFF)   {
      if((buff[idx]>>20) == 0xA00) {
	cout<<"BUFFER FOR BOARD: "<<handle<<endl<<hex;
	cout<<buff[0]<<endl;
	cout<<buff[1]<<endl;
	cout<<buff[2]<<endl;
	cout<<buff[3]<<endl<<dec;

	u_int32_t mask = (buff[idx+1])&0xFF;
	int index = idx+4;
	for(int channel=0; channel<8;channel++){
	  if(!((mask>>channel)&1))      //Do we have this channel in the event?
	    continue;
	  u_int32_t channelSize = (buff[index]);
	  index++; //iterate past the size word             
	  u_int32_t channelTime = (buff[index])&0x7FFFFFFF;
	  index++;
	  index+=channelSize-2;
	  cout<<"Channel: "<<channel<<" size: "<<channelSize<<" time: "<<channelTime<<endl;
	}
	

	break;
      }
      idx++;
    }
    
    delete[] buff;

  
  }

  return 0;
}
  


int main(){
  
  cout<<"Starting program"<<endl;

  vector<int>links;
  vector<int>boards;
  for(unsigned int x=0;x<NBoards;x++){
    links.push_back(0);
    boards.push_back(x);
  }

  vector<int> handles;
  for(unsigned int x=0;x<links.size();x++){
    int bid = initializeBoard(links[x], boards[x]);
    if(bid<0){
      cout<<"Exiting!"<<endl;
      exit(1);
    }
    handles.push_back(bid);
  }

  while(1)
    loopData(handles);
      
  return 0;
}
