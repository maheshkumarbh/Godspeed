/*
GSM_easy.h - Libary for using the GSM-Modul on the GSM_easy-Shield
Version:     4.0
Date:        24.04.2013
Company:     antrax Datentechnik GmbH
*/

#ifndef GSM_easy_h
#define GSM_easy_h
  
#include <Arduino.h>

//-------------------------------
// PIN definition
#define RXD       		0
#define TXD       		1
#define PWRKEY    		4
#define EMERG     		5
#define GSM_ON     		6
#define TESTPIN    		7

#define BUFFER_SIZE		250

//------------------------------------------------------------------------------

class GSM_easyClass
{     
    public:
      // Variables
      char GSM_string[250];
      
      // Functions
      void begin();
		int  initialize(char simpin[4]);
		int  Status();
		int  RingStatus();
		int  pickUp();
      
		int  numberofSMS();
		int  readSMS(int index);
		int  deleteSMS(int index);
		int  sendSMS(char number[50], char text[180]);
      
      int  dialCall(char number[50]);
      int  sendDTMF(char dtmf);
      int  exitCall();
      
      int  EMAILconfigureSMTP(char SMTP[50], int PORT, char USER[30], char PWD[30]);
      int  EMAILconfigureSender(char SENDERNAME[30], char SENDEREMAIL[30]);
      int  EMAILrecipients(int TYPE, char RECIPIENT[30]);
      int  EMAILbody(char TITLE[30], char BODY[200]);
      int  EMAILsend();
      
      int  connectGPRS(char APN[50], char USER[30], char PWD[50]);
      int  sendHTTPGET(char server[50], char parameter_string[200]);
      int  FTPopen(char HOST[50], int PORT, char USER[30], char PASS[30]);
      int  FTPdownload(char PATH[50], char FILENAME[50]);
      int  FTPclose();
      int  sendPING(char server[50], int timeout);
      void disconnectGPRS();
    
    private:
      int  WaitOfReaction(int timeout);
      int  WaitOfDownload(int timeout);
};

//------------------------------------------------------------------------------
extern GSM_easyClass GSM;

#endif
