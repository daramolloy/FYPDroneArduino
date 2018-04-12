// Drone Arduino Application
// Final Year Project
// Autonomously Landing a Drone on a Moving Target
// Dara Molloy - 4BP

// This code will take in and decode strings serially that contain 4 PPM variables
// Assign them variables to an array and, using code from 
// https://github.com/smilexth/Generate-PPM-Signal/blob/master/Generate-PPM-Signal.ino
// generate a PPM signal.
// It will also check the incoming signal for 'OFF' string and use it to control digital
// pin connected to a relay.


#define NumChannels 6  //Setting the number of channels
#define StartingPWMVal 1020  //Setting the default PPM value, defauly 1000
#define PPMFrameLength 20000  //Setting the PPM frame length in microseconds, default 22500.
#define PPMPulseLength 400  //Setting the pulse length, default 200
#define PPMPolarity 0  //Setting polarity of the pulses: 1 = positive, 0 = negative 
#define PPMOutputPin 10  //Setting PPM output digital pin of Arduino
#define RelayPin 11

const byte maxNumInputChar = 50;
char receivedChars[maxNumInputChar];   //Array to store received data
boolean NewDataAvailable = false;
String currString;
bool OFFStringFlag = true;

//Array to hold the wanted PWM values for each channel
int ppm[NumChannels];

void setup(){  
  //Initialise PPM signal
  for(int i=0; i<NumChannels; i++){
    ppm[i]= StartingPWMVal;
  }

  pinMode(RelayPin,OUTPUT);   //Setting up Relay digital pin
  digitalWrite(RelayPin,LOW); //Set Relay LOW by default
  pinMode(PPMOutputPin, OUTPUT);  //Setting up PPM digital pin
  digitalWrite(PPMOutputPin, !PPMPolarity);  //Setting PPM ditigal pin to default polarity

  //Setting up PPM signal interrupt and registers
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
  
  Serial.begin(9600); //Initialise comm link with HC-12 over hardware Serial port
}

void loop(){              //Loop continuously
  CheckOFFStringFlag();   //Check the OFF string flag 
  ReadLaptopStrings(); //Decode incoming string and assign PPM vars
}

void CheckOFFStringFlag(){
  if (OFFStringFlag == true){   //Checking if OFF string is found 
    digitalWrite(RelayPin,LOW);       //Deassert relay if OFF string found
                                //Giving control to users remote controller
  }
  else if (OFFStringFlag == false){
    digitalWrite(RelayPin,HIGH);    //Assert if flag set to false
  }  
}

void ReadLaptopStrings(){
while (Serial.available()) {        // If HC-12 has data

      static byte arrayPointer = 0;
      char delimiter = ',';
      char currChar;
    
      while (Serial.available() > 0 && NewDataAvailable == false) {
        currChar = Serial.read();     //Read the serial port
        if (currChar != delimiter) {  //If the current char isn't ,
            receivedChars[arrayPointer] = currChar; //Add it to the array
            arrayPointer++; //And increment the array pointer
        }
        else {    //If the current char is ,                       
            arrayPointer = 0; //Reset the pointer
            NewDataAvailable = true;  //Break from the loop because we've 
                                      //hit the end of the incoming string
        }
    }

    if (NewDataAvailable == true) { //Enter if theres a new string in the char arr
      currString = ""; //Reset current string
      for (int i =0; i<maxNumInputChar ;i++){ //Set current string equal to the curr array of chars
      currString = currString + (String)receivedChars[i];
      }
        if(currString.indexOf("T") > -1){   //If "T" is found in the string
          ppm[2] = currString.substring(1).toInt(); //Set the PPM[2] (Throttle) value to the value
                                                    //sent with T. Is (T1500) for example
          OFFStringFlag = false;  //Set the OffStringFlag to false because we wouldn't
                                  //of received a throttle value if the GCS switch was asserted
        }
        if(currString.indexOf("P") > -1){   //If is pitch string, set ppm[1] to new pitch value
          ppm[1] = currString.substring(1).toInt();
        }
        if(currString.indexOf("R") > -1){   //If is roll string, set ppm[0] to new roll value
          ppm[0] = currString.substring(1).toInt();
        }
        if(currString.indexOf("Y") > -1){   //If is yaw string, set ppm[3] to new yaw value
          ppm[3] = currString.substring(1).toInt();
        }
        if(currString.indexOf("X") > -1){ //If 'X' is the start of the string, this is the 'OFF'
           //String sent from the GCS Arduino if the switch is set to off so assert the OFFStringFlag
          OFFStringFlag = true;
        }
        NewDataAvailable = false; 
    }    
  }
}

//PPM interrupt code from
// https://github.com/smilexth/Generate-PPM-Signal/blob/master/Generate-PPM-Signal.ino

ISR(TIMER1_COMPA_vect){  
  static boolean state = true;
  
  TCNT1 = 0;
  
  if(state) {  //start pulse
    digitalWrite(PPMOutputPin, PPMPolarity);
    OCR1A = PPMPulseLength * 2;
    state = false;
  }
  else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(PPMOutputPin, !PPMPolarity);
    state = true;

    if(cur_chan_numb >= NumChannels){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPMPulseLength;// 
      OCR1A = (PPMFrameLength - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PPMPulseLength) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
