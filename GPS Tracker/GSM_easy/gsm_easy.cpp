/*
GSM_easy.cpp - Library for using the GSM-Modul Quectel M95 on the GSM_easy-Shield
Included Functions
Version:     4.0
Date:        24.04.2013
Company:     antrax Datentechnik GmbH
Uses with:   Arduino Duemilanove (ATmega328) and
             Arduino UNO (ATmega328)
             
General:
Please don't use Serial.println("...") to send commands to the mobile module! 
Use Serial.print( "... \r") instead. See "M95_AT_Commands_V1.0.pdf", page 9, Chapter 1.3             
             
WARNING: Incorrect or inappropriate programming of the mobile module can lead to increased fees!             
*/

#include "pins_arduino.h"
#include <GSM_easy.h>

GSM_easyClass GSM;

int state = 0;

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Setting of the used signals / Contacts between Arduino mainboard and the GSM-easy! - Shield
*/
void GSM_easyClass::begin()
{
  pinMode(PWRKEY, OUTPUT);                                                      // pin PWRKEY on the M95 (ATTENTION: signal inverted!)
  pinMode(EMERG, OUTPUT);                                                       // pin EMERG_OFF on the M95 (ATTENTION: signal inverted!)
  pinMode(GSM_ON, OUTPUT);                                                      // enable GSM-easy! - Shield (activ high)
  // pinMode(TESTPIN, OUTPUT);                                                  // only if you want to test something
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Initialisation of the GSM-easy! - Shield:
- Set data rate 
- Activate shield 
- Perform init-sequence of the M95 
- register the M95 in the GSM network

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::initialize(char simpin[4])
{
  int time;
  
  Serial.begin(9600);																			  // 9600 Baud data rate
  Serial.flush();

  digitalWrite(GSM_ON, HIGH);																	  // GSM_ON = HIGH --> switch on + Serial Line enable

  // Init sequence, see "M95_HardwareDesign_V1.2.pdf", page 30ff.
  // in any case: force a reset!
  digitalWrite(PWRKEY, LOW);																	  // PWRKEY = HIGH
  digitalWrite(EMERG, HIGH);																	  // EMERG_OFF = LOW
  delay(50);																						  // wait for 50ms
  
  digitalWrite(PWRKEY, LOW);																	  // PWRKEY = HIGH
  digitalWrite(EMERG, LOW);																	  // EMERG_OFF = HIGH
  delay(2100);																						  // wait for 2100ms
  
  digitalWrite(PWRKEY, HIGH);																	  // PWRKEY = LOW
  delay(1100);																						  // wait for 1100ms
  digitalWrite(PWRKEY, LOW);																	  // PWRKEY = HIGH
  
  // Start and Autobauding sequence ("M95_AT_Commands_V1.0.pdf", page 35)
  delay(3000);																						  // wait for 3000ms 

  state = 0;
  time = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT\r");                                                     // send the first "AT"      
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 1)
    {
		Serial.print("ATE0\r");                                                   // disable Echo   
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
    }

    if(state == 2)
    {																									  // after 0,5 - 10 sec., depends of the SIM card
      switch (WaitOfReaction(10000)) 														  // wait for initial URC presentation "+CPIN: SIM PIN" or similar
	   {                                                                         
	     case 2:  state += 2; break; 													     // get +CPIN: SIM PIN
	     case 3:  state += 3; break;												           // get +CPIN: READY
		  default: state += 1; break;
		}
    }

    if(state == 3)
    {
      switch (WaitOfReaction(10000)) 														  // new try: wait for initial URC presentation "+CPIN: SIM PIN" or similar
	   {                                                                         
	     case 2: state += 1; break; 														     // get +CPIN: SIM PIN
	     case 3: state += 2; break;												           // get +CPIN: READY
		  default: 
		  { 
			  Serial.print("Serious SIM-error: >");
           Serial.print(GSM.GSM_string);                                  		  //    here is the explanation
           Serial.println("<");
           while(1);				  																  // ATTENTION: check your SIM!!!! Don't restart the software!!!
	     }  
		}  
    }

    if(state == 4)
    {
		Serial.print("AT+CPIN=");                                                 // enter pin (SIM)     
      Serial.print(simpin);
      Serial.print("\r");
	   if(WaitOfReaction(1000) == 3) { state += 1; } else { state = 1000; } 
    }

    if(state == 5)
    {
      Serial.print("AT+IPR=9600\r");                                            // set Baudrate
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
    }

    if(state == 6)
    {
		Serial.print("AT+QIURC=0\r");                                             // disable initial URC presentation   
  		time = 0;  
		if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
    }

    if(state == 7)
    {
      delay(2000);                                                                                              
      Serial.print("AT+CREG?\r");                                               // Network Registration Report      
      if(WaitOfReaction(1000) == 4) 																		
	   { 
	     state += 1; 																			     // get: Registered in home network or roaming
	   } 
	   else 
	   { 
	     delay(2000);
		  if(time++ < 30)																		  	   
		  {
		    state = state;																	     // stay in this state until timeout
		  }
		  else
		  {
		    state = 1000;																		     // after 60 sek. (30 x 2000 ms) not registered	
		  } 
      } 
    }
      
    if(state == 8)
    {
      return 1;																					  // Registered successfully ... let's go ahead!
    }
  }
  while(state <= 999);
   
  return 0;																							  // ERROR ... no Registration in the network
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Determine current status of the mobile module e.g. of the GSM/GPRS-Networks
All current states are returned in the string "GSM_string"

This function can easily be extended to further queries, e.g. AT+GSN (= query IMEI), 
AT+QCCID (= query CCID), AT+CIMI (= query IMSI) etc.
 
ATTENTION: Please note length of "Status_string" - adjust if necessary

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::Status()
{
  char Status_string[100];
  
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+CREG?\r");                                               // Query register state of GSM network
      WaitOfReaction(1000);
		strcpy(Status_string, GSM_string);
		state += 1; 
	 }
    
    if(state == 1)
    {
      Serial.print("AT+CGREG?\r");                                              // Query register state of GPRS network
      WaitOfReaction(1000);
		strcat(Status_string, GSM_string);
		state += 1; 
	 }

    if(state == 2)
    {
      Serial.print("AT+CSQ\r");                                                 // Query the RF signal strength
      WaitOfReaction(1000);
		strcat(Status_string, GSM_string);
		state += 1; 
	 }
	 
    if(state == 3)
    {
      Serial.print("AT+COPS?\r");                                               // Query the current selected operator
      WaitOfReaction(1000);
		strcat(Status_string, GSM_string);
		state += 1; 
	 }

    if(state == 4)
    {	
		strcpy(GSM_string, Status_string); 
		return 1;
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while dialing
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Detect if there is an incoming call

Return value = 0 ---> there was no incoming call in the last 5 seconds 
Return value = 1 ---> there is a RING currently
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::RingStatus()
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      if(WaitOfReaction(5000) == 11)                                            // 11 = "RING" detected
		{ return 1; }                                                             // Congratulations ... ME rings
		else 
		{ return 0; }                                                             // "no one ever calls!" 
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
"Pick-up the phone" = Accept Voicecall 

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::pickUp()
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("ATA\r");                                                    // Answers an incoming call
      if(WaitOfReaction(1000) == 1) 
		{ return 1; }                                                             // Congratulations ... you are through
		else 
		{ return 0; }                                                             // ERROR
    
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
Send a SMS
Parameter:
char number[50] = Call number of the addressee (national or international format)
char text[180] = Text of the SMS (ASCII-Text)

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::sendSMS(char number[50], char text[180])
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+CREG?\r");                                               // Network Registration Report
      if(WaitOfReaction(1000) == 4) { state += 1; } else { state = 1000; } 
	 }
    
    if(state == 1)
    {	
      Serial.print("AT+CMGF=1\r");                                              // use text-format for SMS        
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
    }
    
    if(state == 2)
    {
      Serial.print("AT+CMGS=\"");                                               // send Message
      Serial.print(number);
		Serial.print("\"\r");
      if(WaitOfReaction(5000) == 5) { state += 1; } else { state = 1000; } 	  // get the prompt ">"
    }
      
    if(state == 3)
    {
      Serial.print(text);                                                       // Message-Text
      Serial.write(26);                                                         // CTRL-Z 
      if(WaitOfReaction(5000) == 1) 
		{ return 1; }                                                             // Congratulations ... the SMS is gone
		else 
		{ return 0; }                                                             // ERROR while sending a SMS 
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while sending a SMS
}
 
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Determine number of SMS stored on the SIM card

ATTENTION: The return value contains the "total number" of stored SMS! The index of the most recent SMS 
			  is possibly higher, if SMS in the "lower" part from the list have already been deleted!

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module			
*/
int GSM_easyClass::numberofSMS()
{
  int numberofSMS;
  
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+CPMS?\r");                                               // Preferred SMS message storage
      if(WaitOfReaction(1000) == 13) { state += 1; } else { state = 1000; } 
	 }
    
    if(state == 1)
    {	
      // string to analyse e.g.: +CPMS: "SM",3,20,"SM",3,20,"SM",3,20
      // look for the parameter between the first two ","
      // (how "strtok" works --> http://www.cplusplus.com/reference/cstring/strtok/)
      char* s = strtok(GSM_string, ",");
      s = strtok(NULL, ",");
		numberofSMS = atoi(s);

		return numberofSMS;
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while sending a SMS
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Read SMS stored on the SIM card
Parameter "index" = 0       --->  write *all* SMS in succession in the string "GSM_string" 
Parameter "index" = 1 .. n  --->  write SMS with the indicated index in the string "GSM_string" 

ATTENTION: Please note length of "GSM_string" - adjust if necessary

Return value = 0 ---> Error occured  
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::readSMS(int index)
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+CREG?\r");                                               // Network Registration Report
      if(WaitOfReaction(1000) == 4) { state += 1; } else { state = 1000; } 
	 }
    
    if(state == 1)
    {	
      Serial.print("AT+CMGF=1\r");                                              // use text-format for SMS        
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
    }
 
    if(state == 2)
    {	
		if (index == 0)
		{
		  Serial.print("AT+CMGL=\"ALL\"\r");                                      // read *all* SMS messages      
		}
		else
		{
        Serial.print("AT+CMGR=");                                               // read SMS message (page 88)      
        Serial.print(index);														    		  // index range = 1 ... x
        Serial.print("\r");
		}  
      if(WaitOfReaction(1000) == 1) 
      // the SMS message is in "GSM_string" now
		{ return 1; }                                                             // Congratulations ... the SMS with the given index is read
		else 
		{ return 0; }                                                             // ERROR while reading a SMS 
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while reading a SMS
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Delete SMS stored on the SIM card
Parameter "index" = 0       --->  delete *all* SMS from the SIM card
Parameter "index" = 1 .. n  --->  delete SMS with the indicated index 

ATTENTION: Potential risk!
           If single SMS are deleted, the highest index (= index of the latest SMS) possibly does not correspond anymore 
	        with the number of the in total stored SMS
			
Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::deleteSMS(int index)
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+CREG?\r");                                               // Network Registration Report
      if(WaitOfReaction(1000) == 4) { state += 1; } else { state = 1000; } 
	 }
    
    if(state == 1)
    {	
      Serial.print("AT+CMGF=1\r");                                              // use text-format for SMS        
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
    }
 
    if(state == 2)
    {	
		if (index == 0)
		{
		  Serial.print("AT+CMGD=1,4\r");                                          // Ignore the value of index and delete *all* SMS messages      
		}
		else
		{
        Serial.print("AT+CMGD=");                                               // delete the SMS with the given index (page 84)      
        Serial.print(index);														    		  // index range = 1 ... x
        Serial.print("\r");
		}  
      if(WaitOfReaction(1000) == 1) 
		{ return 1; }                                                             // Congratulations ... the SMS is/are deleted
		else 
		{ return 0; }                                                             // ERROR while deleting a SMS 
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while deleting a SMS
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
Start Voicecall 
Parameter:
char number[50] = Call number of the recipient (national or international format)

ATTENTION: The SIM card must be suited or enabled for "Voice". Not all SIM cards 
		     (for example M2M "machine-to-machine" SIM cards) are automatically enabled!!!        

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::dialCall(char number[50])
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+CREG?\r");                                               // Network Registration Report
      if(WaitOfReaction(1000) == 4) { state += 1; } else { state = 1000; } 
	 }
    
    if(state == 1)
    {
      Serial.print("AT+COLP=1\r");                                              // Connected line identification presentation
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; } 
	 }
	 
    if(state == 2)
    {	
      Serial.print("ATD ");                                                     // dial number
      Serial.print(number);
      Serial.print(";\r");
      WaitOfReaction(30000);
		state += 1; 
    } 

    if(state == 3)
    {	
      Serial.print("AT+CLCC\r");                                                // List current calls of ME
      if(WaitOfReaction(1000) == 1)       
		{ return 1; }                                                             // Congratulations ... 
		else 
		{ return 0; }                                                             // ERROR
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while dialing
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Send a DTMF-tone
Parameter:
char dtmf = ASCII characters 0-9, #, *, A-D

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::sendDTMF(char dtmf)										              // See "M95_AT_Commands_V1.0.pdf", page 71 ff., Chapter 3.2.39
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("AT+VTS=");                                                  // send a DTMF tone/string
      Serial.print(dtmf);
      Serial.print("\r");
      if(WaitOfReaction(2000) == 1) 
		{ return 1; }                                                             // OK
		else 
		{ return 0; }                                                             // ERROR while sending a DTMF tone
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while sending a DTMF tone
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
End Voicecall 

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::exitCall()
{
  state = 0;
  do    
  {            
    if(state == 0)
    {
      Serial.print("ATH\r");                                               	  // Hang up!
      if(WaitOfReaction(2000) == 1) 
		{ return 1; }                                                             // OK
		else 
		{ return 0; }                                                             // ERROR while sending a ATH 
    } 
  }  
  while(state <= 999);
  
  return 0;                                                                     // ERROR while sending a ATH
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//-- G P R S + T C P / I P ---------------------------------------------------------------------------------------------------------------------------
//-- see Quectel application note "GSM_TCPIP_AN_V1.1.pdf" --------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Initialise GPRS connection (previously the module needs to be logged into the GSM network already)
With the successful set-up of the GPRS connection the base for processing internet (TCP(IP, UDP etc.), e-mail (SMTP) 
and PING commands is given.

Parameter:
char APN[50] = APN of the SIM card provider
char USER[30] = Username for this
char PWD[50] = Password for this

ATTENTION: This SIM card data is provider-dependent and can be obtained from them. 
           For example "internet.t-mobile.de","t-mobile","whatever" for T-Mobile, Germany
           and "gprs.vodafone.de","whatever","whatever" for Vodafone, Germany
ATTENTION: The SIM card must be suitable or enabled for GPRS data transmission. Not all SIM cards
			  (as for example very inexpensive SIM cards) are automatically enabled!!!        

Return value  = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::connectGPRS(char APN[50], char USER[30], char PWD[50])
{
  int time = 0;
  
  state = 0;
  do
  {
    if(state == 0)
	 {
      Serial.print("AT+CREG?\r");                                               // Network Registration Report
      if(WaitOfReaction(1000) == 4) { state += 1; } else { state = 1000; }      // need 0,1 or 0,5
	 }

    if(state == 1)                                                              // Judge network?
    {
      Serial.print("AT+CGATT?\r");                                              // attach to GPRS service?      
      if(WaitOfReaction(1000) == 7) 														  // need +CGATT: 1			
	   { 
	     state += 1; 																			     // get: attach
	   } 
	   else 
	   { 
	     delay(2000);
		  if(time++ < 30)																		  	   
		  {
		    state = state;																	     // stay in this state until timeout
		  }
		  else
		  {
		    state = 1000;																		     // after 60 sek. (30 x 2000 ms) not attach	
		  } 
      }
	 } 

    if(state == 2)
    {
      Serial.print("AT+QISTAT\r");                                              // Query current connection status
      if(WaitOfReaction(1000) == 8) { state += 1; } else { state = 1000; }      // need STATE: IP INITIAL 
    }
    
    if(state == 3)
    {
      Serial.print("AT+QICSGP=1,\"");				    								     // Select GPRS as the bearer
      Serial.print(APN);
		Serial.print("\",\"");
		Serial.print(USER);
		Serial.print("\",\"");
		Serial.print(PWD);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 4)
    {
      Serial.print("AT+QIDNSIP=1\r");                                           // Connect via domain name (not via IP address!)
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 5)
    {
      Serial.print("AT+QISTAT\r");                                              // Query current connection status
      if(WaitOfReaction(1000) == 8) { state += 1; } else { state = 1000; }      // need STATE: IP INITIAL, IP STATUS or IP CLOSE
    }

    if(state == 6)
    {
      return 1;																					  // GPRS connect successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... no GPRS connect
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Send HTTP GET 
This corresponds to the "call" of a URL (such as the with the internet browser) with appended parameters

Parameter:
char server[50] = Address des server (= URL)
char parameter_string[200] = parameters to be appended

ATTENTION: Before using this function a GPRS connection must be set up. 

You may use the "antrax Test Loop Server" for testing. All HTTP GETs to the server are logged in a list,
that can be viewed on the internet (http://www.antrax.de/WebServices/responderlist.html)

Example for a correct URL for the transmission of the information "HelloWorld":

   http://www.antrax.de/WebServices/responder.php?HelloWorld
      whereby 
	   - "www.antrax.de" is the server name 
	   - "GET /WebServices/responder.php?HelloWorld HTTP/1.1" the parameter
	
On the server the URL of the PHP script "responder.php" is accepted and analysed in the subdirectory "WebServices". 
The part after the "?" corresponds to the transmitted parameters. ATTENTION: The parameters must not 
contain spaces. The source code of the PHP script "responder.php" is located in the documentation. 

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::sendHTTPGET(char server[50], char parameter_string[200])
{
  int time = 0;
  
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QIOPEN=\"TCP\",\"");		    								     // Start up TCP connection
      Serial.print(server);
		Serial.print("\",80\r");
      if(WaitOfReaction(2000) == 1) { state += 1; } else { state = 1000; }      // need OK
    }
	 
    if(state == 1)
    {
      if(WaitOfReaction(20000) == 9) { state += 1; } else { state = 1000; }     // need CONNECT OK or ALREADY CONNECT
    }

    if(state == 2)
    {
      Serial.print("AT+QISEND\r");                                              // Send data to the remote server
      if(WaitOfReaction(5000) == 5) { state += 1; } else { state = 1000; } 	  // get the prompt ">"
    }
      
    if(state == 3)
    {
      // for HTTP GET must include: "GET /subdirectory/name.php?test=parameter_to_transmit HTTP/1.1"
      // for example to use with "www.antrax.de/WebServices/responderlist.html":
      // "GET /WebServices/responder.php?test=HelloWorld HTTP/1.1"
		Serial.print(parameter_string);                                           // Message-Text
      
      // Header Field Definitions in http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
      Serial.print("\r\nHost: ");                                               // Header Field "Host"
      Serial.print(server);                                                     
      Serial.print("\r\nUser-Agent: antrax");                                   // Header Field "User-Agent" (MUST be "antrax" when use with portal "WebServices")
      Serial.print("\r\nConnection: close\r\n\r\n");                            // Header Field "Connection"
      Serial.write(26);                                                         // CTRL-Z 
      if(WaitOfReaction(20000) == 10) { state += 1; } else { state = 1000; }    // Congratulations ... the parameter_string was send to the server
    } 

    if(state == 4)
    {
      WaitOfReaction(5000);                                                     // wait of ack from remote server
		state += 1;
    }
	 
    if(state == 5)
    {
      return 1;																					  // GPRS connect successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... no GPRS connect
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Send PING
Ping is a diagnostic funtion, which is used to check whether a given host can be reached via GPRS.
The complete description of the PING command (AT+QPING) can be viewed here: "M95_AT_Commands_V1.0.pdf", page 167, Chapter 7.2.31

Parameter:
char server[50] = Address of the server (= URL)
char timeout = Timeout for reaction, 1...255 seconds

ATTENTION: Before using this function a GPRS connection must be set up. 

For testing, a well-to-reach server can be specified, e.g. "www.google.com"

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::sendPING(char server[50], int timeout)
{
  int time = 0;
  
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QPING=\"");		    			          					     // send a Ping
      Serial.print(server);
		Serial.print("\",");
		Serial.print(timeout);
		Serial.print(",1\r");
      if(WaitOfReaction(2000) == 1) { state += 1; } else { state = 1000; }      // need OK
	 }                                                                            

    if(state == 1)
    {
      if(WaitOfReaction(20000) == 12) 														  // wait of "+QPING:"
		{ return 1; }                                                             // OK
		else 
		{ return 0; }
	 }                                                                            
  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... 
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Disconnect GPRS connection

ATTENTION: The frequent disconnection and rebuilding of GPRS connections can lead to unnecessarily high charges 
		 	  (e.g. due to "rounding up costs"). It is necessary to consider whether a GPRS connection for a longer time period 
			  (without data transmission) shall remain active! 

No return value 
The public variable "GSM_string" contains the last response from the mobile module
*/
void GSM_easyClass::disconnectGPRS()
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QIDEACT\r");                                             // Deactivate GPRS context
      if(WaitOfReaction(10000) == 1) { state += 1; } else { state = 1000; }     // ein OK wäre schön ...
    }
  }
  while(state <= 999);  
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//-- E -M A I L ---------------------------------------------------------------------------------------------------------------------------------------
//-- see Quectel application note "GSM_SMTP_ATC_V1.1.pdf" --------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Settting up the SMTP-Server (outgoing mail server) to be used

Parameter:
char SMTP[50] = Address of the SMTP server
char PORT = port to be used (mostly 25)
char USER[30] = Username for identification with the SMTP server
char PWD[30] = Password for identification with the SMTP server

All figures above are identic to the figures used for installation of an e-mail program on a PC 
(for example Outlook, Thunderbird etc.).

With this function the listed parameters are transferred to the mobile module, but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::EMAILconfigureSMTP(char SMTP[50], int PORT, char USER[30], char PWD[30])
{
  state = 0;
  do
  {
    if(state == 0)
	 {
      Serial.print("AT+QSMTPCLR\r");                                            // Clear all configurations and contents of the email
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK
	 }

    if(state == 1)
    {
      Serial.print("AT+QSMTPSRV=\"");			    	    								  // Configure SMTP server
      Serial.print(SMTP);
		Serial.print("\",");
		Serial.print(PORT);
		Serial.print("\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 2)
    {
      Serial.print("AT+QSMTPUSER=\"");			    	    								  // Configure USER NAME
      Serial.print(USER);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 3)
    {
      Serial.print("AT+QSMTPPWD=\"");			    	    								  // Configure PASSWORD
      Serial.print(PWD);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 4)
    {
      return 1;																					  // Configure SMTP server successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during configure SMTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Configure and prepare an outgoign e-mail

Parameter:
char SENDERNAME[30] = Name of the sender
char SENDEREMAIL[30] = E-mail address of the sender

With this function the listed parameters are transferred to the mobile module, but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response of the mobile module
*/
int GSM_easyClass::EMAILconfigureSender(char SENDERNAME[30], char SENDEREMAIL[30])
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QSMTPNAME=\"");			    	    								  // Configure senders NAME
      Serial.print(SENDERNAME);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 1)
    {
      Serial.print("AT+QSMTPADDR=\"");			    	    								  // Configure senders EMAIL ADDRESS
      Serial.print(SENDEREMAIL);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 2)
    {
      return 1;																					  // Configure sender information successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during configure sender information
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Configure or prepare to send an e-mail

Parameter:
char TYPE = Type of recipient, 1 = Recipient, 2 = Copy (CC), 3 = Blind copy (BCC)
char RECIPIENT[30] = E-mail address of the recipient

With this function the listed parameters are transferred to the mobile module, but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::EMAILrecipients(int TYPE, char RECIPIENT[30])
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QSMTPDST=1,");			    	       						     // Add a recipient
		Serial.print(TYPE);
		Serial.print(",\"");
      Serial.print(RECIPIENT);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 1)
    {
      return 1;																					  // Configure sender information successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during configure sender information
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Configure or prepare to send an e-mail

Parameter:
char TITLE[30] = Subject of the e-mail
char BODY[200] = Text of the e-mail

With this function the listed parameters are transferred to the mobile module, but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::EMAILbody(char TITLE[30], char BODY[200])
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QSMTPSUB=0,\"");			    	    							  // Configure the title of the email
      Serial.print(TITLE);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 1)
    {
      Serial.print("AT+QSMTPBODY=1,10\r");		 	    	    						  // Configure the text of the email
		if(WaitOfReaction(2000) == 14) { state += 1; } else { state = 1000; }     // wait of OK CONNECT 
    }

    if(state == 2)
    {
      Serial.print(BODY);			    	    							                 // send the text of the email to the module memory
      delay(1000);                                                              // 1. part of the escape sequence
		Serial.print("+++");																		  // 2. part of the escape sequence	
      if(WaitOfReaction(2000) == 15) { state += 1; } else { state = 1000; }     // need "+QSMTPBODY:" as reaction
      delay(1000);                                                              // 3. part of the escape sequence
    }

    if(state == 3)
    {
      return 1;																					  // Setting of Titel/Text successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during configure Titel/Text
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Actually send the e-mail with the previously set parameters
Timeout for sending of the e-mail: 120 seconds

Set up all settings and parameter for the SMTP server e.g. the E-mail before with the functions:
 
  - EMAILconfigureSMTP(char SMTP[50], int PORT, char USER[30], char PWD[30])
  - EMAILconfigureSender(char SENDERNAME[30], char SENDEREMAIL[30])
  - EMAILrecipients(int TYPE, char RECIPIENT[30])
  - EMAILbody(char TITLE[30], char BODY[200])

BEFORE sending!

ATTENTION: Before using these functions set up a GPRS connection. 

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::EMAILsend()
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QSMTPPUT\r");			    	       						     // Send Email!
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 1)
    {
      if(WaitOfReaction(120000) == 16) { state += 1; } else { state = 1000; }   // wait 120 seconds of "+QSMTPPUT: 0" as reaction
    }

    if(state == 2)
    {
      return 1;																					  // Sending process successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during sending process
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//-- F T P -------------------------------------------------------------------------------------------------------------------------------------------
//-- see Quectel application note "GSM_FTP_ATC_V1.1.pdf" ---------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Set up to use and process/open a connection to the FTP server

Parameter:
char HOST[50] = Address of the FTP server
char PORT = Port to be used (mostly 21)
char USER[30] = Username for the identification with the FTP server
char PASS[30] = Password for the identification with the FTP server

All figures above are identic to the figures used for installation of a FTP program on a PC 
(for example FileZilla, FTP Voyager etc.).

With this function the listed parameters are transferred to the mobile module, but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::FTPopen(char HOST[50], int PORT, char USER[30], char PASS[30])
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QFTPUSER=\"");			    	    								  // Configure USER NAME
      Serial.print(USER);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 1)
    {
      Serial.print("AT+QFTPPASS=\"");			    	    								  // Configure PASSWORD
      Serial.print(PASS);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 2)
    {
      Serial.print("AT+QFTPOPEN=\"");			    	    								  // Configure FTP server
      Serial.print(HOST);
		Serial.print("\",");
		Serial.print(PORT);
		Serial.print("\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 3)
    {
      if(WaitOfReaction(120000) == 17) { state += 1; } else { state = 1000; }   // wait 120 seconds of "+QFTPOPEN: 0" as reaction
    }

    if(state == 4)
    {
      return 1;																					  // open FTP server successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during opening FTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Setting the path and name of the file to be loaded from the FTP server, 
then start the download

Parameter:
char PATH[50] = Path of the file
char FILENAME[50] = Name of the file

The contents of the file to be downloaded via FTP is written into the variable "GSM_string".

ATTENTION: Please note length "GSM_string" - adjust if necessary

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::FTPdownload(char PATH[50], char FILENAME[50])
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QFTPPATH=\"");			    	    								  // Configure PATH
      Serial.print(PATH);
		Serial.print("\"\r");
      switch (WaitOfReaction(1000))
		{
         case 1:  state = state + 1;														  // get only "OK", need also "+QFTPPATH:0"
         	  		break;
         case 17: state = state + 2;														  // get "+QFTPPATH:0" ---> that's OK!
				  		break;	  		
		   default: state = 1000;
		   			break;
		}   			
    }
	 
    if(state == 1)
    {
      if(WaitOfReaction(120000) == 17) { state += 1; } else { state = 1000; }   // wait 120 seconds of "+QFTPPATH:0" as reaction
    }
	 
    if(state == 2)
    {
 		Serial.print("AT+QFTPGET=\"");			    	    								  // Configure FILENAME
      Serial.print(FILENAME);
		Serial.print("\"\r");
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 3)
    {
      WaitOfDownload(10000);                                                    // wait 10 seconds of the content of the file
		state += 1;                                                               // and of "+QFTPGET:" as reaction
    }                  

    if(state == 4)
    {
      return 1;																					  // downloading file from the server successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during opening FTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Disconnecto from the FTP server and GPRS connection

Parameter:
none

ATTENTION: If further operations via GPRS need to be processed, don't disconnect the GPRS connection at this point ("AT+QIDEACT"), 
			  in order to avoid unnecessary costs!

Return value = 0 ---> Error occured 
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::FTPclose()
{
  state = 0;
  do
  {
    if(state == 0)
    {
      Serial.print("AT+QFTPCLOSE\r");			    	       						     // close the FTP service
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }
	 
    if(state == 1)
    {
      if(WaitOfReaction(10000) == 17) { state += 1; } else { state = 1000; }     // wait 10 seconds of "+QFTPCLOSE:0" as reaction
    }

    if(state == 2)
    {
      Serial.print("AT+QIDEACT\r");			    	       						        // deactivate GPRS/CSD context
      if(WaitOfReaction(1000) == 1) { state += 1; } else { state = 1000; }      // need OK 
    }

    if(state == 3)
    {
      return 1;																					  // close FTP server successfully ... let's go ahead!
    }

  } 
  while(state <= 999);
  
  return 0;																							  // ERROR ... during closing FTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Special receive routine of the serial interface for downloads
Used for receiving files from FTP

similar to "int GSM_easyClass::WaitOfReaction(int timeout)"
*/
int GSM_easyClass::WaitOfDownload(int timeout)
{
  int  index = 0;
  int  inByte = 0;
  char WS[3];

  //----- erase GSM_string
  memset(GSM_string, 0, BUFFER_SIZE);

  //----- clear Serial Line Buffer
  Serial.flush();
  while(Serial.available()) { Serial.read(); }
  
  //----- wait of the first character for "timeout" ms
  Serial.setTimeout(timeout);
  inByte = Serial.readBytes(WS, 1);
  GSM_string[index++] = WS[0];
  
  //----- wait of further characters until a pause of "timeout" ms occures
  while(inByte > 0)
  {
    inByte = Serial.readBytes(WS, 1);
    GSM_string[index++] = WS[0];
    if((strstr(GSM_string, "+QFTPGET:")) && (WS[0] == '\r'))  { break; }
  }

  delay(100);
  return 0;
}        
 
//----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Central receive routine of the serial interface

ATTENTION: This function is used by all other functions (see above) and must not be deleted

Parameter:
int TIMEOUT = Waiting time to receive the first character (in milliseconds)


Then contents of the received characters is written to the variable "GSM_string". If no characters is received 
for 30 milliseconds, the reception is completed and the contents of "GSM_string" is being analysed.

ATTENTION: The length of the reaction times of the mobile module depend on the condition of the mobile module, for example  
			  quality of wireless connection, provider, etc. and thus can vary. Please keep this in mind in case this routine is 
			  is changed.

Return value = 0      ---> No known response of the mobile module detected 
Rückgabewert = 1 - 18 ---> Response detected (see below)
The public variable "GSM_string" contains the last response from the mobile module
*/
int GSM_easyClass::WaitOfReaction(int timeout)
{
  int  index = 0;
  int  inByte = 0;
  char WS[3];

  //----- erase GSM_string
  memset(GSM_string, 0, BUFFER_SIZE);
  memset(WS, 0, 3);

  //----- clear Serial Line Buffer
  Serial.flush();
  while(Serial.available()) { Serial.read(); }
  
  //----- wait of the first character for "timeout" ms
  Serial.setTimeout(timeout);
  inByte = Serial.readBytes(WS, 1);
  GSM_string[index++] = WS[0];
  
  //----- wait of further characters until a pause of 30 ms occures
  while(inByte > 0)
  {
    Serial.setTimeout(30);
    inByte = Serial.readBytes(WS, 1);
    GSM_string[index++] = WS[0];
  }

  //----- analyse the reaction of the mobile module
  if(strstr(GSM_string, "SIM PIN\r\n"))	      	{ return 2; }
  if(strstr(GSM_string, "READY\r\n"))		      	{ return 3; }
  if(strstr(GSM_string, "0,1\r\n"))						{ return 4; }
  if(strstr(GSM_string, "0,5\r\n"))						{ return 4; }
  if(strstr(GSM_string, "\n>"))							{ return 5; }					  // prompt for SMS text
  if(strstr(GSM_string, "NO CARRIER\r\n"))			{ return 6; }
  if(strstr(GSM_string, "+CGATT: 1\r\n"))				{ return 7; }
  if(strstr(GSM_string, "IP INITIAL\r\n"))			{ return 8; }
  if(strstr(GSM_string, "IP STATUS\r\n"))				{ return 8; }
  if(strstr(GSM_string, "IP CLOSE\r\n"))				{ return 8; }
  if(strstr(GSM_string, "CONNECT OK\r\n"))			{ return 9; }
  if(strstr(GSM_string, "ALREADY CONNECT\r\n"))		{ return 9; }
  if(strstr(GSM_string, "SEND OK\r\n"))				{ return 10; }
  if(strstr(GSM_string, "RING\r\n"))					{ return 11; }
  if(strstr(GSM_string, "+QPING:"))				   	{ return 12; }
  if(strstr(GSM_string, "+CPMS:"))				   	{ return 13; }
  if(strstr(GSM_string, "OK\r\n\r\nCONNECT\r\n"))	{ return 14; }
  if(strstr(GSM_string, "+QSMTPBODY:"))			  	{ return 15; }
  if(strstr(GSM_string, "+QSMTPPUT: 0"))		    	{ return 16; }
  if(strstr(GSM_string, ":0\r\n"))		    	      { return 17; }
  if(strstr(GSM_string, "+QFTPGET:"))		  	      { return 18; }

  if(strstr(GSM_string, "OK\r\n"))						{ return 1; }
  
  return 0;
}        
      
//----------------------------------------------------------------------------------------------------------------------------------------------------

