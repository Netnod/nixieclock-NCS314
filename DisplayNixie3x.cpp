//============================================================================
// Name        : DisplayNixie.cpp
// Author      : GRA&AFCH
// Version     : v1.0
// Copyright   : Free
// Description : Display time on shields NCS314 v3.x
// Date		   : 27.04.2020
//============================================================================

// This version is modified by Patrik @ Netnod
// Copyright of changes Apache 2.0
// paf@netnod.se // https://www.netnod.se/

#define _VERSION "1.0 for NCS314 HW Version 3.x - paf mod"

#include <iostream>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <ctime>
#include <string.h>
#include <wiringPiI2C.h>
#include <softTone.h>
#include <softPwm.h>
#include <math.h>

#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 

#define MAXLINE 1024

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

using namespace std;
#define LEpin 3
#define SHDNpin 2
#define UP_BUTTON_PIN 1
#define DOWN_BUTTON_PIN 4
#define MODE_BUTTON_PIN 5
#define BUZZER_PIN 0
#define I2CAdress 0x68
#define I2CFlush 0

#define DEBOUNCE_DELAY 150
#define TOTAL_DELAY 17

#define SECOND_REGISTER 0x0
#define MINUTE_REGISTER 0x1
#define HOUR_REGISTER 0x2
#define WEEK_REGISTER 0x3
#define DAY_REGISTER 0x4
#define MONTH_REGISTER 0x5
#define YEAR_REGISTER 0x6

#define RED_LIGHT_PIN 28
#define GREEN_LIGHT_PIN 27
#define BLUE_LIGHT_PIN 29
#define MAX_POWER 100

#define UPPER_DOTS_MASK 0x80000000
#define LOWER_DOTS_MASK 0x40000000

#define LEFT_REPR_START 5
#define LEFT_BUFFER_START 0
#define RIGHT_REPR_START 2
#define RIGHT_BUFFER_START 4

uint16_t SymbolArray[10]={1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

int fileDesc;
int redLight = 100;
int greenLight = 0;
int blueLight = 0;
int lightCycle = 0;
bool dotState = 0;
int rotator = 0;

// paf mod
// Set initial antiPoinoning mode true=on; false=off
bool doAntiPoisoning = false;
// Set initial display IP address mode to false (off)
bool displayIPAddress = false;
// Initially, have the display on
bool displayOn = true;
// Keep track of when to turn the clock on and off
int turn_on[7];
int turn_off[7];
// Name of the interface to check IP address of
char interface[80];
// Set default clock mode
// NOTE:  true means rely on system to keep time (e.g. NTP assisted for accuracy).
bool useSystemRTC = true;
// Port number to listen from commands
int port = 5000;
// Set the hour mode
// Set use12hour = true for hours 0-12 and 1-11 (e.g. a.m./p.m. implied)
// Set use12hour = false for hours 0-23
bool use12hour = false;


int bcdToDec(int val) {
	return ((val / 16  * 10) + (val % 16));
}

int decToBcd(int val) {
	return ((val / 10  * 16) + (val % 10));
}

tm addHourToDate(tm date) {
	date.tm_hour += 1;
	mktime(&date);
	return date;
}

tm addMinuteToDate(tm date) {
	date.tm_min += 1;
	mktime(&date);
	return date;
}

tm getRTCDate() {
	wiringPiI2CWrite(fileDesc, I2CFlush);
	tm date;
	date.tm_sec =  bcdToDec(wiringPiI2CReadReg8(fileDesc,SECOND_REGISTER));
	date.tm_min =  bcdToDec(wiringPiI2CReadReg8(fileDesc,MINUTE_REGISTER));
	date.tm_hour = bcdToDec(wiringPiI2CReadReg8(fileDesc,HOUR_REGISTER));
	date.tm_wday = bcdToDec(wiringPiI2CReadReg8(fileDesc,WEEK_REGISTER));
	date.tm_mday = bcdToDec(wiringPiI2CReadReg8(fileDesc,DAY_REGISTER));
	date.tm_mon =  bcdToDec(wiringPiI2CReadReg8(fileDesc,MONTH_REGISTER));
	date.tm_year = bcdToDec(wiringPiI2CReadReg8(fileDesc,YEAR_REGISTER));
	date.tm_isdst = 0;
	return date;
}

void updateRTCHour(tm date) {
	wiringPiI2CWrite(fileDesc, I2CFlush);
	wiringPiI2CWriteReg8(fileDesc,HOUR_REGISTER,decToBcd(date.tm_hour));
	wiringPiI2CWrite(fileDesc, I2CFlush);
}

void updateRTCMinute(tm date) {
	wiringPiI2CWrite(fileDesc, I2CFlush);
	wiringPiI2CWriteReg8(fileDesc,MINUTE_REGISTER,decToBcd(date.tm_min));
	wiringPiI2CWriteReg8(fileDesc,HOUR_REGISTER,decToBcd(date.tm_hour));
	wiringPiI2CWrite(fileDesc, I2CFlush);
}
void resetRTCSecond() {
	wiringPiI2CWrite(fileDesc, I2CFlush);
	wiringPiI2CWriteReg8(fileDesc,SECOND_REGISTER, 0);
	wiringPiI2CWrite(fileDesc, I2CFlush);
}

void writeRTCDate(tm date) {
	wiringPiI2CWrite(fileDesc, I2CFlush);
	wiringPiI2CWriteReg8(fileDesc,SECOND_REGISTER,decToBcd(date.tm_sec));
	wiringPiI2CWriteReg8(fileDesc,MINUTE_REGISTER,decToBcd(date.tm_min));
	wiringPiI2CWriteReg8(fileDesc,HOUR_REGISTER,decToBcd(date.tm_hour));
	wiringPiI2CWriteReg8(fileDesc,WEEK_REGISTER,decToBcd(date.tm_wday));
	wiringPiI2CWriteReg8(fileDesc,DAY_REGISTER,decToBcd(date.tm_mday));
	wiringPiI2CWriteReg8(fileDesc,MONTH_REGISTER,decToBcd(date.tm_mon));
	wiringPiI2CWriteReg8(fileDesc,YEAR_REGISTER,decToBcd(date.tm_year));
	wiringPiI2CWrite(fileDesc, I2CFlush);
}

void initPin(int pin) {
	pinMode(pin, INPUT);
	pullUpDnControl(pin, PUD_UP);

}

void funcMode(void) {
	static unsigned long modeTime = 0;
	if ((millis() - modeTime) > DEBOUNCE_DELAY) {
		puts("MODE button was pressed.");
// paf mod
// Mode Switch turns on IP address display
                displayIPAddress = true;
                displayOn = !displayOn;
		dotState = displayOn;
//		resetRTCSecond();
		modeTime = millis();
	}
}

void dotBlink()
{
  // paf mod
  // Just change state, do not try to guess when blinking next time

        // static unsigned int lastTimeBlink=millis();

	// if ((millis() - lastTimeBlink) > 1000)
	// {
	// 	lastTimeBlink=millis();
  if(displayOn) {
    dotState = !dotState;
  } else {
    dotState = false;
  }
	// }
}

/*void rotateFireWorks() {
	int fireworks[] = {0,0,1,
					  -1,0,0,
			           0,1,0,
					   0,0,-1,
					   1,0,0,
					   0,-1,0
	};
	redLight += fireworks[rotator * 3];
	greenLight += fireworks[rotator * 3 + 1];
	blueLight += fireworks[rotator * 3 + 2];
	softPwmWrite(RED_LIGHT_PIN, redLight);
	softPwmWrite(GREEN_LIGHT_PIN, greenLight);
	softPwmWrite(BLUE_LIGHT_PIN, blueLight);
	lightCycle = lightCycle + 1;
	if (lightCycle == MAX_POWER) {
		rotator = rotator + 1;
		lightCycle  = 0;
	}
	if (rotator > 5)
		rotator = 0;
}*/

uint32_t get32Rep(char * _stringToDisplay, int start) {
	uint32_t var32 = 0;

	var32= (SymbolArray[_stringToDisplay[start]-0x30])<<20;
	var32|=(SymbolArray[_stringToDisplay[start - 1]-0x30])<<10;
	var32|=(SymbolArray[_stringToDisplay[start - 2]-0x30]);
	return var32;
}

void fillBuffer(uint32_t var32, unsigned char * buffer, int start) {

	buffer[start] = var32>>24;
	buffer[start + 1] = var32>>16;
	buffer[start + 2] = var32>>8;
	buffer[start + 3] = var32;
}


uint32_t addBlinkTo32Rep(uint32_t var) {
	if (dotState)
	{
		var &=~LOWER_DOTS_MASK;
		var &=~UPPER_DOTS_MASK;
	}
	else
	{
		var |=LOWER_DOTS_MASK;
		var |=UPPER_DOTS_MASK;
	}
	return var;
}

// paf mod
// Check what IP address we have on wlan0 interface
char *parse_ioctl(const char *ifname)
{
  int sock;
  struct ifreq ifr;
  struct sockaddr_in *ipaddr;
  char address[INET_ADDRSTRLEN];
  size_t ifnamelen;
  char *s = NULL;

  /* copy ifname to ifr object */

  ifnamelen = strlen(ifname);
  if (ifnamelen >= sizeof(ifr.ifr_name)) {
    return(NULL);
  }
  memcpy(ifr.ifr_name, ifname, ifnamelen);
  ifr.ifr_name[ifnamelen] = '\0';

  /* open socket */
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    return(NULL);
  }

  /* die if cannot get address */
  if (ioctl(sock, SIOCGIFADDR, &ifr) == -1) {
    close(sock);
    return(NULL);
  }

  /* process ip */
  ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
  if (inet_ntop(AF_INET, &ipaddr->sin_addr, address, sizeof(address)) != NULL) {
    s = strdup(address);
    /*    printf("Ip address: %s\n", address); */
  }

  close(sock);
  return(s);
}

void
read_config(char *filename) {
  FILE *fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int theday, maybeday, maybestart, maybeend;

  printf("Reading config file %s\n", filename);

  fp = fopen(filename, "r");
  if (fp == NULL) {
    perror(filename);
    exit(1);
  }
  strcpy(interface, "wlan0");
  while ((read = getline(&line, &len, fp)) != -1) {
    if(sscanf(line, "if: %s", interface)) {
      printf("Use IP address of interface %s\n", interface);
    } else if(sscanf(line, "port: %d", &port)) {
      printf("Use port %d\n", port);
    } else if(sscanf(line, "%d:%d-%d", &maybeday, &maybestart, &maybeend) == 3) {
      if(maybeday >= 0 && maybeday < 7 && maybestart >=0 && maybestart <= 24 && maybeend >= 0 && maybeend <= 24) {
	turn_on[maybeday] = maybestart;
	turn_off[maybeday] = maybeend;
      } else {
	printf("Error in config file at %s", line);
      }
    }
  }
  for(theday = 0; theday < 7; theday++) {
    if(theday == 0) printf("Sunday   : ");
    if(theday == 1) printf("Monday   : ");
    if(theday == 2) printf("Tuesday  : ");
    if(theday == 3) printf("Wednesday: ");
    if(theday == 4) printf("Thursday : ");
    if(theday == 5) printf("Friday   : ");
    if(theday == 6) printf("Saturday : ");
    printf("%02d:00 - %02d:59\n", turn_on[theday], turn_off[theday]);
  }
  fclose(fp);
  if (line) {
    free(line);
  }
  printf("End of config file.\n");
  return;
}

char _stringToDisplay[8];
void doIndication();

int main(int argc, char* argv[]) {
  int theday;

  int listenfd, connfd = -1, nready, maxfdp1;
  char buffer[MAXLINE]; 
  char message[MAXLINE];
  fd_set rset; 
  socklen_t len; 
  struct sockaddr_in cliaddr, servaddr;
  void sig_chld(int); 
  struct timeval wait;
  
  printf("Nixie Clock v%s \n\r", _VERSION);
  wiringPiSetup();
  //softToneCreate (BUZZER_PIN);
  //softToneWrite(BUZZER_PIN, 1000);
  softPwmCreate(RED_LIGHT_PIN, 100, MAX_POWER);
  softPwmCreate(GREEN_LIGHT_PIN, 0, MAX_POWER);
  softPwmCreate(BLUE_LIGHT_PIN, 0, MAX_POWER);
  initPin(UP_BUTTON_PIN);
  initPin(DOWN_BUTTON_PIN);
  initPin(MODE_BUTTON_PIN);
  wiringPiISR(MODE_BUTTON_PIN,INT_EDGE_RISING,&funcMode);
  fileDesc = wiringPiI2CSetup(I2CAdress);
  
  tm date = getRTCDate();
  time_t seconds = time(NULL);
  tm* timeinfo = localtime (&seconds);
  date.tm_mday = timeinfo->tm_mday;
  date.tm_wday = timeinfo->tm_wday;
  date.tm_mon =  timeinfo->tm_mon + 1;
  date.tm_year = timeinfo->tm_year - 100;
  writeRTCDate(date);
  
  if (wiringPiSPISetupMode (0, 2000000, 3)) {
    puts("SPI ok");
  }
  else {
    puts("SPI NOT ok");
    return 0;
  }
  
  pinMode(SHDNpin, OUTPUT);
  digitalWrite(SHDNpin, HIGH); //HIGH = ON
  
  long hourDelay = millis();
  long minuteDelay = hourDelay;
  
  // paf mod
  
  // Init time when clock is turned on
  for(theday = 0; theday < 7; theday++) {
    turn_on[theday] = 0;
    turn_off[theday] = 23;
  }
  
  // Save last string displayed -- blink with dots if display changed (i.e. second changed)
  char _stringDisplayed[8];
  _stringDisplayed[0] = (char)NULL;
  // What time is it?
  char _stringNow[8];
  _stringNow[0] = (char)NULL;
  // Save my IP address
  char *myIPAddress = (char *)NULL;
  // Octet of IP address to print 0 <= 3, if displayIPAddress is true
  int theOctet = 0;
  // Number to display for anti-poisoning
  int thePoison = 0;
  // end of paf mod
  
  // Crude command-line argument handling
  if (argc >= 2) {
    int curr_arg = 1;
    while (curr_arg < argc) {
      if (!strcmp(argv[curr_arg],"-c")) {
	curr_arg++;
	read_config(argv[curr_arg]);
      }
      curr_arg += 1;
    }
  }
  
  // Check whether clock should be turned on or not
  if(date.tm_hour >= turn_on[date.tm_wday] && date.tm_hour <= turn_off[date.tm_wday]) {
    printf("Start with clock turned on, toggle on Mode button\n");
    displayOn = true;
  } else {
    printf("Start with clock turned off, toggle on Mode button\n");
    displayOn = false;
  }
  
  /* create listening TCP socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0); 
  bzero(&servaddr, sizeof(servaddr)); 
  servaddr.sin_family = AF_INET; 
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
  servaddr.sin_port = htons(port); 
  printf("Binding to port %d\n", port);
  // binding server addr structure to listenfd 
  bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
  listen(listenfd, 10); 
  // clear the descriptor set 
  FD_ZERO(&rset); 
  // get maxfd 
  maxfdp1 = listenfd + 1; 
  
  do {
    //char _stringToDisplay[8];
    
    seconds = time(NULL);
    timeinfo = localtime (&seconds);
    date.tm_mday = timeinfo->tm_mday;
    date.tm_wday = timeinfo->tm_wday;
    date.tm_mon =  timeinfo->tm_mon + 1;
    date.tm_year = timeinfo->tm_year - 100;
    writeRTCDate(*timeinfo);
    
    date = getRTCDate();
    const char* format ="%H%M%S";
    strftime(_stringNow, 8, format, &date);
    //strftime(_stringToDisplay, 8, format, &date);
    
    pinMode(LEpin, OUTPUT);
    
    // paf mod
    if(doAntiPoisoning) {
      sprintf(_stringToDisplay, "%d%d%d%d%d%d", thePoison, thePoison, thePoison, thePoison, thePoison, thePoison);
      thePoison++;
      if(thePoison > 9) {
	thePoison = 0;
      }
      doIndication();
    }
    // If we are in a new second the "_stringNow" with current time has changed                        
    if (strcmp(_stringNow, _stringDisplayed) != 0) {
      strcpy(_stringToDisplay, _stringNow);
      strcpy(_stringDisplayed, _stringNow);
      if(doAntiPoisoning) {
	// Only do AntiPoisoning for one second, so turn off if it was turned on                       
	doAntiPoisoning = false;
      }
      dotBlink();
      
      if(displayIPAddress) {
	int i[4];
	free(myIPAddress);
	myIPAddress = parse_ioctl(interface);
	if (4 == sscanf(myIPAddress, "%d.%d.%d.%d", &(i[0]), &(i[1]), &(i[2]), &(i[3]))) {
	  sprintf(_stringToDisplay, "000%03d", i[theOctet]);
	  if(theOctet == 0) {
	    printf("Display IP address: %s (%s)\n", myIPAddress, _stringDisplayed);
	  };
	};
	if(theOctet > 3) {
	  displayIPAddress = false;
	  theOctet = 0;
	  doAntiPoisoning = true;
	  thePoison = 0;
	} else {
	  theOctet++;
	}
      }
      if(strcmp(&(_stringNow[2]), "5959") == 0) {
	doAntiPoisoning = true;
	// date.tm_wday is 0-6 which indicates "days since Sunday", i.e. Sunday is 0, Saturday is 6
	if(((date.tm_hour + 1) % 24) == turn_on[date.tm_wday]) {
	  displayOn = true;
	}
	if(date.tm_hour == turn_off[date.tm_wday]) {
	  displayOn = false;
	}
      }
      if(!doAntiPoisoning && !displayIPAddress && !displayOn) {
	strcpy(_stringToDisplay, "      ");
      }
      doIndication();
    }
    // end of paf mod                                                                                                  
    
    if (digitalRead(UP_BUTTON_PIN) == 0 && (millis() - hourDelay) > DEBOUNCE_DELAY) {
      updateRTCHour(addHourToDate(date));
      hourDelay = millis();
    }
    if (digitalRead(DOWN_BUTTON_PIN) == 0 && (millis() - minuteDelay) > DEBOUNCE_DELAY) {
      updateRTCMinute(addMinuteToDate(date));
      minuteDelay = millis();
    }
    
    // rotateFireWorks();
    if(doAntiPoisoning) {
      wait.tv_sec = 0;
      wait.tv_usec = 0; // Do not wait at all
    } else {
      wait.tv_sec = 0;
      wait.tv_usec = 100000; // One 10th of a second
    }

    if(connfd < 0) {
      // No ongoing connection
      // set listenfd in readset 
      FD_SET(listenfd, &rset); 
      // select the ready descriptor 
      nready = select(maxfdp1, &rset, NULL, NULL, &wait); 
      if(nready == 0) {
	// printf("Timeout\n");
      }
      // if tcp socket is readable then handle 
      // it by accepting the connection 
      if (FD_ISSET(listenfd, &rset)) { 
	int flags;
	len = sizeof(cliaddr); 
	connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len); 
	flags = fcntl(connfd, F_GETFL, 0);
	fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
	// printf("Got a connection\n");
      }
    } else {
      // We have a connection using connfd
      bzero(buffer, sizeof(buffer)); 
      // printf("Checking connfd\n");
      if(read(connfd, buffer, sizeof(buffer)) > 0) {
	// printf("Message From TCP client: \n"); 
	// puts(buffer);
	strcpy(message,"SYNTAX ERROR\n");
	if(strncmp(buffer,"GET /ON HTTP/", strlen("GET /ON HTTP/")) == 0) {
	  displayOn = true;
	  strcpy(message,"ON\n");
	}
	if(strncmp(buffer,"GET /OFF HTTP/", strlen("GET /OFF HTTP/")) == 0) {
	  displayOn = false;
	  strcpy(message,"OFF\n");
	}
	write(connfd, (const char*)message, strlen(message)); 
	close(connfd); 
	connfd = -1;
      }
    }
  }
  while (true);
  return 0;
}


#define UpperDotsMask 0x80000000
#define LowerDotsMask 0x40000000

void doIndication()
{

  unsigned long Var32=0;
  unsigned long New32_L=0;
  unsigned long New32_H=0;
  unsigned char buff[8];

  long digits=atoi(_stringToDisplay);

  /**********************************************************
   * Data format incomin [H1][H2][M1][M2][S1][S2]
   *********************************************************/

 //-------- REG 1 -----------------------------------------------
  Var32=0;

  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10])<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]); //m2
  digits=digits/10;

  // If the display is to be dark, turn off tubes
  if(!doAntiPoisoning && !displayIPAddress && !displayOn) {
    Var32 = 0;
  }

  if (dotState) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;

  if (dotState) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;

  for (int i=1; i<=32; i++)
  {
   i=i+32;
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4);
   i=i-32;
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }
 //-------------------------------------------------------------------------

 //-------- REG 0 -----------------------------------------------
  Var32=0;

  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10])<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]; //h1
  digits=digits/10;

  // If the display is to be dark, turn off tubes
  if(!doAntiPoisoning && !displayIPAddress && !displayOn) {
    Var32 = 0;
  }

  if (dotState) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;

  if (dotState) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;

  for (int i=1; i<=32; i++)
  {
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4);
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }

  buff[0] = New32_H>>24;
  buff[1] = New32_H>>16;
  buff[2] = New32_H>>8;
  buff[3] = New32_H;

  buff[4] = New32_L>>24;
  buff[5] = New32_L>>16;
  buff[6] = New32_L>>8;
  buff[7] = New32_L;

  wiringPiSPIDataRW(0, buff, 8);

  digitalWrite(LEpin, HIGH); // <<-- H -> L

  digitalWrite(LEpin, LOW); //<<--  H -> L

//-------------------------------------------------------------------------
}

