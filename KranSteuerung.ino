//*******************************//
// Kran Steuerung
// Matthias Wagner
// https://debugginglab.wordpress.com
//*******************************//
#define VERSION "6.2"		//Version der Steuerung

#define sclk 	13  // Don't change
#define mosi 	11  // Don't change
#define cs_tft	3   // 9
#define dc   	4   // 8
#define rst  	10   // 7  // you can also connect this to the Arduino reset

#define cs_mcp	2		// CS for MCP3008

#define BTN1	8		//Button 1 Pin
#define BTN2	9		//Button 2 Pin
#define NOTAUS  5		//NOTAUS PIN

#define BAL_HEI		50		//Hoehe des Balken
#define BAL_TOP		46		//Anfangspunkt der Balken (obere Kante)
#define BAL_MID		BAL_TOP + BAL_HEI		//Mitte des Diagramms
#define BAL_BOT		BAL_MID + BAL_HEI		//Unteres ende des Diagramms

#define SERVO_MAX		2000
#define SERVO_MID		1500  //Servo mittelstellung
#define SERVO_MIN		1000

#define ANALOG_MAX	1024  //Analog Max Val
#define ANALOG_MID	512   //Analoge mittelstellung
#define ANALOG_MIN	0		

#define DEAD_SPOT		50		//Analog In delta 50 else 0

#define SPI_SPEED		SPI_CLOCK_DIV4

#define INTRO
//#define ANIMATION
//#define NUMBERS



#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_QDTech.h> // Hardware-specific library
#include <SPI.h>
//#include <Servo.h> 
#include <ServoShield.h>
#include <MCP3008.h>
#include "kran.h"
#include "warn.h"

Adafruit_QDTech tft = Adafruit_QDTech(cs_tft, dc, rst);  // Invoke custom library
//Adafruit_QDTech tft = Adafruit_QDTech(cs_tft, dc, mosi, sclk, rst );       // Invoke custom library

ServoShield servos;
MCP3008 mcp = MCP3008(cs_mcp);

int ain[] = {1020,1020,1020,1020,1020,1020};						//Array for analog Values
int pwrIn[] = {10,2,12,30,7,10};		//Array for Power Mes Values
char buffer[10][21];			//Text Buffer
char EmptyBuffer[21] = "                    ";
long int time = 0;			//Globel old time
int runs = 0;

int Mapping[6] = {4,0,2,1,5,3};
int Invert[6]  = {1,1,1,1,1,1};

// 1balken - 5motor - array1
// 2balken - 3motor - array3
// 3balken - 4motor - array2
// 4balken - 1motor - array5
// 5balken - 6motor - array0
// 6balken - 2motor - array4
//


//***************
//     Setup
//***************
void setup(void) {

	pinMode(NOTAUS, INPUT);


	pinMode(BTN1, INPUT_PULLUP);
	pinMode(BTN2, INPUT_PULLUP);
		
	

	
	uint16_t time = millis();
	
	
	//Init TFT Display
	tftINIT();
	
	
	
	#if defined(INTRO)
	delay(500);
	
	//Print Kran LOGO
	tft.drawBitmap(5,35,kran,118,125,QDTech_YELLOW);
	delay(1500);
	tft.setTextColor(QDTech_BLACK);
	tft.setTextSize(2);
	tft.setCursor(80,140);
	tft.print(VERSION);
	
	delay(1500);
	#endif
	
	
	
	// INIT MCP3008
	for (int i=0; i<6; i++)
		pwrIn[i] = mcp.readADC(i);
	
	//INIT Servos
	ServoInit();
	
	//Press 2 Buttons
	Warnung();
	
	tft.fillRect(0,35,128,125,QDTech_BLACK);
	tft.fillRect(0,35,128,125,QDTech_BLACK);
	
	tft.setTextColor(QDTech_WHITE);
	tft.setTextSize(1);
	time = millis();
		
}
// setup end



//***************
//     Loop
//***************
void loop() {
	static int fps = 0;
	static int fpsOLD = 0;
	static int bal[] = {10,2,12,30,7,10};
	static int balOLD[] = {0,0,0,0,0,0};
	static int balPWR[] = {0,0,0,0,0,0};
	static int balPWROLD[] = {0,0,0,0,0,0};
	static int ainADD[] = {20,30,14,-20,-30,17};
	static int pwrAll = 0;								//Gesamt Power
	static float pwrAllval = 0;
	static int pwrAllOLD = 0;
	static float pwrAllOLDval = 0;
	
	static int overMaxPWR[] = {0,0,0,0,0,0};		
	
	static int pause = 0;
	static int pauseOLD = 0;
	static int pauseCOUNT = 0;
	
	
	fps++;
	
	//********************************
	while(digitalRead(NOTAUS)){
		ServoInit();
		tftINIT();
		Warnung();
		tft.fillRect(0,35,128,125,QDTech_BLACK);
		tft.setTextColor(QDTech_WHITE);
		tft.setTextSize(1);
	}
	//********************************
	
	pwrAllOLD = pwrAll;
	pwrAllOLDval = pwrAllval;
	pwrAll = 0;
	pauseOLD = pause;
	pause = 0;
	
	
	//Analog einlesen und an die Servos senden
	for (int i=0; i < 6; i++)
	{
		balOLD[i] = bal[i];					//Save last Val
		balPWROLD[i] = balPWR[i];
		
		#if defined(ANIMATION)
		//TEST  TEST   TEST  TEST Animation
		ain[i] = ain[i] + ainADD[i];
		if (ain[i]>=SERVO_MAX || ain[i] <=SERVO_MIN){
			ainADD[i] = ainADD[i] * -1;
			ain[i] = ain[i] + ainADD[i];
		}
		
		bal[i] = map(ain[i], SERVO_MIN, SERVO_MAX, -BAL_HEI, BAL_HEI);
		
		//TEST  TEST   TEST  TEST Animation
		#else
		ain[i] = analogRead(Mapping[i]);
		
		if(Invert[i]==1)
			ain[i] = map(ain[i], ANALOG_MIN, ANALOG_MAX, ANALOG_MAX, ANALOG_MIN);

		//Noch kein Joystick
		/*
		if(!digitalRead(BTN2)){
			ain[i] = ANALOG_MID;
			if(i == 2)
				ain[2] = analogRead(0);
			if(i == 4)
				ain[4] = analogRead(1);
		}
		else{
			ain[2] = ANALOG_MID;
			ain[4] = ANALOG_MID;
		}
		*/
		
		//minimal move to effect Servos
		if ((ain[i] < (ANALOG_MID + DEAD_SPOT)) && (ain[i] > (ANALOG_MID - DEAD_SPOT))){
			ain[i] = ANALOG_MID;
			pause +=1;
		}
		
		//Analog IN auf Balken mappen
		bal[i] = map(ain[i], 0, ANALOG_MAX, -BAL_HEI, BAL_HEI);
		

		ain[i] = map(ain[i], ANALOG_MIN, ANALOG_MAX, SERVO_MAX, SERVO_MIN);
		
		
		#endif
		
		
		

		
		
		
		pwrIn[i] = mcp.readADC(i);													//Read Analog Values from Current Shunt
		
		//MAX Power control
		if( pwrIn[i] > 512 ){
			overMaxPWR[i] +=1;
			if (overMaxPWR[i] > 20){ 	//ca 1 Sekunden
				ain[i] = SERVO_MID;		//Mittelstellung
				overMaxPWR[i] = 0;
			}
		}
		
		servos.setposition(i,ain[i]);		// Servo ansteuern
		
		
		balPWR[i] = map(pwrIn[i],ANALOG_MIN,ANALOG_MAX,0,BAL_HEI);							//PwrIn auf Balkengröße umrechnen max 5A
		
		
		#if defined(ANIMATION)
		//TEST TEST TEST Animation
			//balPWR[i] = map(ain[5-i],SERVO_MIN,SERVO_MAX,0,BAL_HEI);
		//TEST TEST TEST
		#endif
		
		pwrAll += pwrIn[i];															//Balken für Gesamtstrom addieren
		balPWR[i] = bal[i] >= 0 ? balPWR[i] : -balPWR[i];					// Roter Balken nach oben oder unten zeigen lassen
		
		
		
		
		// Servos Pausieren (Interrupt Stoppen) 
		if(i == 5) //Letzter lauf im for loop
		{	if(pauseOLD == 6){
				if(pause != 6){
					servos.start();
					//TFT "PAUSE" löschen
					drawStandby(0);
				}
				else{
					pauseCOUNT += 1;
					// Zeit damit die Servos in Grundstellung fahren (min 5 Sek)
					if(pauseCOUNT == 100){
						// Stop Servo ones
						servos.stop();
					}
					//draw Standby Logo (so Refresh cant delete it) Jedes 20. mal (ca 150ms)
					if((pauseCOUNT >=100) && (pauseCOUNT % 10 == 0))
						drawStandby(1);
					
				}
			}
			else{
				if(pause == 6)
					pauseCOUNT = 0;
				}
		}
		
	}
	
	
	//Keine Ahnung was ich hier berechne...
	pwrAll *= 2;
	pwrAllval = (float)pwrAll/204.6;
	pwrAll = map(pwrAll,0,6*614,0,123);
	
	if(pwrAllOLD > pwrAll)
		tft.fillRect(2+pwrAll,BAL_TOP-5,pwrAllOLD-pwrAll,3,QDTech_BLACK);					//Black part of Gesamt bar
	else
		tft.fillRect(2+pwrAllOLD,BAL_TOP-5,pwrAll-pwrAllOLD,3,QDTech_RED);							//Gesamt Power bar
	
	if(pwrAllOLD != pwrAll){
		if((int)pwrAllOLDval > 9)
			tft.fillRect(95,BAL_TOP - 9,5,8,QDTech_BLACK);							//Delete old Number
		
		if((int)pwrAllval > 9)
			tft.setCursor(95,BAL_TOP - 9);
		else
			tft.setCursor(100,BAL_TOP - 9);
		
		tft.fillRect(100,BAL_TOP - 9,18,8,QDTech_BLACK);							//Delete old Number
		tft.print((int)pwrAllval); //24.6
		tft.print(".");
		tft.print( ((int)(pwrAllval*10)) %10  ); //2.46
		
		tft.setCursor(121,BAL_TOP - 9);
		tft.print("A");
		
	}
	
	
	
	//Obere und Untere Begrenzungslinien
	tft.drawFastHLine(2,BAL_TOP -1,123,QDTech_GREY);
	tft.drawFastHLine(2,BAL_BOT +1,123,QDTech_GREY);
	//Mittellinie
	tft.drawFastHLine(2,BAL_MID ,123,QDTech_GREY);
	
	
	for(int i=5;i<122;i=i+6){
		tft.drawFastHLine(i,BAL_MID - BAL_HEI/2,3,QDTech_GREY);						//gestrichelte Linie
		tft.drawFastHLine(i,BAL_MID + BAL_HEI/2,3,QDTech_GREY);						//gestrichelte Linie
	}
	
	
	
	//*******************
	// Diagramm Zeichnen
	//*******************
	for(int i=0; i<6; i++){
		//Balken zeichnen
		if(bal[i]>=0){
			if(balOLD[i] < 0){
				#if defined(NUMBERS)
				tft.fillRect(6+i*20,BAL_MID -10,15,10,QDTech_BLACK);						//Delete old negativ Number
				#endif
				tft.fillRect(6+i*20,BAL_MID,15,-balOLD[i],QDTech_BLACK);						//Black part of Blue bar				
				tft.fillRect(6+i*20,BAL_MID -bal[i],15,bal[i],QDTech_GREEN);							//after neg/pos change				
			}
			else if(balOLD[i] > bal[i])
				tft.fillRect(6+i*20,BAL_MID -balOLD[i],15,balOLD[i] -bal[i],QDTech_BLACK);				//Black part of Green bar
			else //balOLD[i] < bal[i]
				tft.fillRect(6+i*20,BAL_MID -bal[i],15,bal[i]-balOLD[i],QDTech_GREEN);					//Green bar
			
			
			if(balPWROLD[i] < 0){
				tft.fillRect(21+i*20,BAL_MID,2,BAL_HEI,QDTech_BLACK);											//Black part of negativ PWR bar
				tft.fillRect(21+i*20,BAL_MID -balPWR[i],2,balPWR[i],QDTech_RED);							//Pwr after neg/pos change
			}
			else if(balPWROLD[i] > balPWR[i])
				tft.fillRect(21+i*20,BAL_MID -balPWROLD[i],2,balPWROLD[i]- balPWR[i],QDTech_BLACK);	//Balck part of Pwr bar
			else //balPWROLD[i] < balPWR[i]
				tft.fillRect(21+i*20,BAL_MID -balPWR[i],2,balPWR[i]- balPWROLD[i],QDTech_RED);		//Pwr bar
			
			#if defined(NUMBERS)
			//Print Number
			//if(((balOLD[i]+2)<bal[i]) || ((balOLD[i]-2)>bal[i])){
			if(balOLD[i]!=bal[i]){
				tft.fillRect(6+i*20,BAL_MID +1,15,12,QDTech_BLACK);							//Delete old Number
				tft.setCursor(9+i*20,BAL_MID +3);
				tft.print(bal[i]);
			}
			#endif
		}
		else{
			if(balOLD[i] >= 0){
				#if defined(NUMBERS)
				tft.fillRect(6+i*20,BAL_MID +1,15,12,QDTech_BLACK);						//Delete old postiv Number
				#endif
				tft.fillRect(6+i*20,BAL_MID-balOLD[i],15,balOLD[i],QDTech_BLACK);						//Black part of Green bar				
				tft.fillRect(6+i*20,BAL_MID,15,-bal[i],QDTech_BLUE);									//after neg/pos change				
			}
			else if(balOLD[i] < bal[i])
				tft.fillRect(6+i*20,BAL_MID -bal[i],15,bal[i]-balOLD[i],QDTech_BLACK);	//Black part of Blue bar
			else //balOLD[i] > bal[i]
				tft.fillRect(6+i*20,BAL_MID-balOLD[i],15,balOLD[i]-bal[i],QDTech_BLUE);							//Blue bar
			
			
			if(balPWROLD[i] > 0){
				tft.fillRect(21+i*20,BAL_TOP,2,BAL_HEI,QDTech_BLACK);									//Balck part of postiv Pwr bar
				tft.fillRect(21+i*20,BAL_MID ,2,-balPWR[i],QDTech_RED);								//Pwr after neg/pos change
				//balPWROLD[i] = -1;
			}
			else if(balPWROLD[i] < balPWR[i])
				tft.fillRect(21+i*20,BAL_MID -balPWR[i],2,balPWR[i]-balPWROLD[i],QDTech_BLACK);							//Balck part of Pwr bar
			else
				tft.fillRect(21+i*20,BAL_MID -balPWROLD[i],2,balPWROLD[i]-balPWR[i],QDTech_RED);							//Pwr bar
				
			#if defined(NUMBERS)
			//Print Number
			//if(((balOLD[i]-2)>bal[i]) || ((balOLD[i]+2)<bal[i])){
			if(balOLD[i]!=bal[i]){
				tft.fillRect(6+i*20,BAL_MID -10,15,10,QDTech_BLACK);							//Delete old Number
				tft.setCursor(8+i*20,BAL_MID -9);
				tft.print(bal[i]*-1);				
			}
			#endif
		}
	}
	
	
	/*
	tft.fillRect(50,2,50,10,QDTech_BLACK);
	tft.setCursor(50,2);
	tft.print(bal[4]);
	tft.print(" ");
	tft.print(balOLD[4]);
	*/
	
	
	if (!digitalRead(BTN1)){
		tft.setCursor(75,151);
		tft.print("BTN1");
	}
	else{
		tft.setCursor(75,151);
		tft.setTextColor(QDTech_BLACK);
		tft.print("BTN1");
		tft.setTextColor(QDTech_WHITE);
	}		
	if (!digitalRead(BTN2)){
		tft.setCursor(100,151);
		tft.print("BTN2");
	}
	else{
		tft.setCursor(100,151);
		tft.setTextColor(QDTech_BLACK);
		tft.print("BTN2");
		tft.setTextColor(QDTech_WHITE);
	}
	
	
	
	//Frames per Second berechnen und am unteren Rand anzeigen
	if (millis()-time >= 1000 ){
		
		drawFPS(millis()-time, fps);
		time = millis();
		fps = 0;
		
		/*
		if(fpsOLD != fps){
			tft.fillRect(0,151,75,8,QDTech_BLACK);
			tft.setCursor(0,151);
			tft.print(fps);
			tft.print("fps  ");
			tft.print((millis()-time)/fps);
			tft.print("ms");
			time = millis();
			fpsOLD = fps;
			fps = 0;
		}
		*/
		
	/*
		//Refresh screen
		//20Pix Rects
		//Alle 20 Sekunden
		if (runs % 20 == 0)
			tft.fillRect(0,BAL_TOP-11 + (runs), 128, 20,QDTech_BLACK);
		runs = runs == 120 ? 0 : runs+1;
		*/
	}
	
	
}
//loop end



void ServoInit(){
	// INIT Servos
	for (int servo = 0; servo < 6; servo++) //Initialize all 16 servos
	{
		servos.setbounds(servo, 500, 2000);   //Set the minimum and maximum pulse duration of the servo
		servos.setposition(servo, SERVO_MID);      //Set the initial position of the servo
	}
	servos.start();                         //Start the servo shield
}



void Warnung(){
	#if defined(INTRO)
	
	//Print Warning
	tft.fillRect(0,35,128,124,QDTech_BLACK);
	tft.drawBitmap(11,25,warn,105,105,QDTech_RED);
	
	
	//Delay to make sure Servos are in mid positon
	delay(3000);
	
	//Stop servos to avoid trembling
	servos.stop();
	

	boolean inv = true;
	while(digitalRead(NOTAUS)){
		inv = !inv;
		if(inv)
			tft.setTextColor(QDTech_YELLOW);
		else
			tft.setTextColor(QDTech_RED);
		tft.setTextSize(2);
		tft.setCursor(0,125);
		tft.println("  NOT AUS");
		tft.setCursor(4,141);
		tft.println("entriegeln");
		delay(600);
	}
	tft.invertDisplay(0);
	tft.fillRect(0,125,128,35,QDTech_BLACK);
	

	tft.setTextColor(QDTech_RED);
	tft.setCursor(0,125);
	tft.setTextSize(1);
	tft.println("     Bitte beide  ");
	tft.println("   Tasten druecken");
	
	delay(700);
	
	
	//Fenster für Ladebalcken
	tft.fillRect(12,143,104,17,QDTech_RED);
	tft.fillRect(14,145,100,13,QDTech_BLACK);
	
	
	int count = 0;
	while(count < 100){
		if(!digitalRead(BTN1) && !digitalRead(BTN2))
			count++;
		else if(count > 0){
			count = 0;
			tft.fillRect(14,145,100,13,QDTech_BLACK);
		}
		
		tft.fillRect(14,145,count,13,QDTech_YELLOW);
		delay(10);	//10ms
	}
	
	//Balken wird grün
	tft.fillRect(14,145,100,13,QDTech_GREEN);
	delay(1700);
	
	servos.start();


#endif
}

void drawStandby(bool d){
	//TFT Anzeige
	tft.setCursor(24,BAL_MID -18);
	tft.setTextSize(2);
	if(d == true)
		tft.setTextColor(QDTech_GREEN);
	else
		tft.setTextColor(QDTech_BLACK);
	tft.print("STANDBY");
	tft.setTextSize(1);
	tft.setTextColor(QDTech_WHITE);
}

void drawFPS(long int dt, int f){
	static int fOLD = 0;
	static int dtfOLD = 0;
	
	int dtf = dt/f >999 ? 999 : dt/f;
	
	
	if(fOLD != f){
		//Delete OLD fps
		tft.setCursor(0,151);
		tft.setTextColor(QDTech_BLACK);
		tft.print(fOLD);
		
		//Print NEW fps
		tft.setTextColor(QDTech_WHITE);
		if(f <10)
			tft.setCursor(6,151);
		else
			tft.setCursor(0,151);
		tft.print(f);
		
		tft.print("fps");	
	}
	
	if(dtfOLD != dtf){
		
		//Delete OLD frame Time
		tft.setCursor(48,151);
		tft.setTextColor(QDTech_BLACK);
		if(dtfOLD <10)
			tft.setCursor(54,151);
		else if (dtfOLD >= 100)
			tft.setCursor(42,151);
		else
			tft.setCursor(48,151);
		tft.print(dtfOLD);
		
		//Print NEW frame Time
		tft.setTextColor(QDTech_WHITE);
		if(dtf <10)
			tft.setCursor(54,151);
		else if (dtf >= 100)
			tft.setCursor(42,151);
		else
			tft.setCursor(48,151);
		tft.print(dtf);
		
		tft.print("ms");
	}
	
	fOLD = f;
	dtfOLD = dtf;
}

void tftINIT(){
	tft.init();
	SPI.setClockDivider(SPI_SPEED);  //16 wählen // geht nicht so super
	
	
	tft.setRotation(0);	// 0 - Portrait, 1 - Lanscape
	tft.fillScreen(QDTech_BLACK);
	tft.setTextWrap(true);
	
	//Print Kran Steuerung
	tft.setTextSize(2);
	tft.setCursor(1,1);
	tft.setTextColor(QDTech_GREY);
	tft.println("Kran");
	tft.println("Steuerung");
	tft.setCursor(0,0);
	tft.setTextColor(QDTech_YELLOW);
	tft.println("Kran");
	tft.println("Steuerung");
	
	tft.setTextColor(QDTech_YELLOW);
	tft.setCursor(90,1); //103
	tft.setTextSize(1);
	tft.print("v");
	tft.print(VERSION);
}

