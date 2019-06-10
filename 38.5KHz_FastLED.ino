/*  38.5KHz_FastLED
 *  
 *  By : Kevin Alfsen
 * 
 *  Program for reading audio input with a sampling frequency of about 38.5KHz and processing 
 *  the input with FHT in order to display a frequency band representation
 *  
 *  The sampling frequency is achieved by configuring the ADMUX register 
 *  on the ATMEGA in free running mode with a prescaler value of 32
 *  
 *  Openmusiclabs' fht library is used to seperate the audio into frequency bands
 *  
 *  The result is visualized using the FastLED library to control a neopixel LED-ring  
 */
 

//defines needed to initate the FHT library
#define OCTAVE 1 //configures output method to be fht_oct_out(), further explained at http://wiki.openmusiclabs.com/wiki/ArduinoFHT
#define FHT_N 256 //Sets the amount of samples that will be used for the transformation

#define LOG_OUT 1

#include <FHT.h>
#include<FastLED.h>

//Defines needed to initate the FastLED library
#define LED_TYPE WS2812
#define LED_PIN 12
#define COLOR_ORDER GRB
#define N_LEDs 24
#define BRIGHTNESS 60
CRGB leds[N_LEDs]; //Holds the colors of the LED lights which will be passed to the LED-strip when FastLED.show() is called
CHSV colors[N_LEDs]; //Holds the colors in (Hue, Saturation, Value), which is a bit easier to work with in our case

#define DC_OFFSET 127
#define N_BANDS 4

#define noiseSquelch 4

#define BUTTON_PIN 6
#define CLIPPING_PIN 13

#define avgSensVal 4
#define delayAvgSensVal 10

byte sampleCount;
byte freqNoise[] = {85, 80, 50, 20};
byte maxFreqMag[] = {85, 25, 5, 5};
int newBands[4];
byte LEDsToShow[4];

bool debug_samples = false, debug_new_bands = true;
bool freqBinsRepresentation = true, avgRepresentation = false;
bool showSampleRate = true;
bool button_pressed = false;
long sampleSum, sampleStartTime, sampleEndTime;
byte n_sections = 4;

void setup() {
  if (debug_samples || debug_new_bands || showSampleRate) {
    Serial.begin(57600);
  }
  
  noInterrupts(); //Turn off interrupts while configuring LEDs and ADC-register

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, N_LEDs); //Create the FastLED object, used to pass colors to and turn on and off LEDs based on adress
  FastLED.setBrightness(BRIGHTNESS); //Sets the default brightness, takes a value between 0 and 255, we use half brightness to account for current limit
  FastLED.setMaxPowerInVoltsAndMilliamps(5,500); //Limits the brightness and amount of LEDs that can be turned on so that we never draw more than the specified current, this is to protect the arduino board 

  //Configuring the ADC to allow for 38.5kHz sampling, this is explained more thoroughly int the report 
  ADCSRA = 0; //Setting both ADC registers to 0 to disable them
  ADCSRB = 0; //We will not be using ADCSRB so this register is left at 0 (disabled)

  //ADMUX is an 8-bit register containing settings for reference voltage source (REFS0, REFS1), analog port to be used (MUX3...MUX0) and if the result should be left or right justified (ADLAR)
  ADMUX |= (1 << REFS0); //All bits default to 0, so here we are setting the two reference voltage bits to 01, which means using the internal voltage of the arduino = 5v
  ADMUX |= (1 << ADLAR); //Setting ADLAR to 1 means using left-justified result
  //analog port to be used defaults to 0

  //ADCSRA is an 8-bit register containing settings for enabling ADC, start ADC convertion, prescaler selection and interrupt control
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //ADPS2...ADPS0 controls prescaler value, her it is set to 101, which means 32 prescaler value

  ADCSRA |= (1 << ADATE); //ADATE enables autotrigger wich means the register will continuously convert instead of waiting for a start signal after each convertion
  ADCSRA |= (1 << ADEN); //ADEN enables the ADCSRA register, setting this to 0 (disabling the register), means no conversions will happen
  ADCSRA |= (1 << ADIE); //ADIE enables interrupts, so that an interrupt routine sensitive to ADC_vect will trigger once the ADC is done converting
  ADCSRA |= (1 << ADSC); //ADSC start the conversion, meaning the ADC is now accepting and converting analog signals

  LedColors(); //This sets the color hues of the LEDs to a preset value

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CLIPPING_PIN, OUTPUT);

  interrupts(); //Reenable interrupts to start sampling
}

void loop() { 
  if (sampleCount >= FHT_N - 1) { //Wait for 256 samples to be collected before starting the processing of the samples

    if (freqBinsRepresentation) {
      //At this point the ADC register has been disabled and we can process the collected samples without having the interrupt routine changing the values in our array of samples
      fht_window();
      fht_reorder();
      fht_run();
      fht_mag_octave();
    
      //once the samples have been processed, the array of samples can once again be updated, so we turn the ADC back on whith the same settings as before
      sampleCount = 0; //sampleCount is both the amount of samples collected and the index for the samples in the array, so this needs to be set to zero to overwrite the array of samples
      
      ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
    
      ADCSRA |= (1 << ADATE);
      ADCSRA |= (1 << ADEN);
      ADCSRA |= (1 << ADIE);
      ADCSRA |= (1 << ADSC);
    
      ProcessFHTResults();
      ShowResultsLED();
    } else if (avgRepresentation) {
      byte sampleAverage;
      sampleAverage = sampleSum/sampleCount;

      sampleSum = 0;
      sampleCount = 0;

      ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
    
      ADCSRA |= (1 << ADATE);
      ADCSRA |= (1 << ADEN);
      ADCSRA |= (1 << ADIE);
      ADCSRA |= (1 << ADSC);

      byte LEDsToShowAvg = sampleAverage*(N_LEDs/2)/(255/avgSensVal);
      showAvgLED(LEDsToShowAvg);
      //delay(delayAvgSensVal);
    }

    //Button code.
    if (digitalRead(BUTTON_PIN) == LOW && !button_pressed) {
      button_pressed = true;
      ADCSRA = 0;
      if (avgRepresentation) {
        avgRepresentation = false;
        freqBinsRepresentation = true;
        n_sections = 4;
      } else {
        avgRepresentation = true;
        freqBinsRepresentation = false;
        n_sections = 2;
      }
      LedColors();
      sampleSum = 0;
      sampleCount = 0;

      ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
    
      ADCSRA |= (1 << ADATE);
      ADCSRA |= (1 << ADEN);
      ADCSRA |= (1 << ADIE);
      ADCSRA |= (1 << ADSC);
    } else if (digitalRead(BUTTON_PIN) == HIGH) {
      button_pressed = false;
    }
  }
}

void LedColors() { 
  //Method for initating LED colors and saturation, index pattern explained further in report
  if (freqBinsRepresentation) {
    byte LEDsInSection = N_LEDs/n_sections; //The strip is divided into n_sections sections, each containing LEDsInSection LEDs in each section
    byte HuesInSection = LEDsInSection/3; //Each section will have 3 different hue values
    for(int i = 0; i < LEDsInSection; i++) {
      //Setting each of the colors' saturation to maximum value, this will not be changed later so the colors will alway have the same saturation 
      colors[i].saturation = 255;
      colors[11-i].saturation = 255;
      colors[i+12].saturation = 255;
      colors[23-i].saturation = 255;
  
      //Setting each of the colors' hues to different ranges so that different frequency ranges are represented
      if (i < HuesInSection) {
        colors[i].hue = 0;
        colors[11-i].hue = 180;
        colors[i+12].hue = 180;
        colors[23-i].hue = 0;
      }
      else if (i < HuesInSection*2) {
        colors[i].hue = 45;
        colors[11-i].hue = 135;
        colors[i+12].hue = 225;
        colors[23-i].hue = 315;
      }
      else if (i < HuesInSection*3){
        colors[i].hue = 90;
        colors[11-i].hue = 90;
        colors[i+12].hue = 270;
        colors[23-i].hue = 270;
      }
    }

  } else if (avgRepresentation) {
    for (int i = 0; i < N_LEDs/n_sections; i++) {
      if (i < 6) {
        colors[i] = CHSV(85, 255, 0);
        colors[23-i] = CHSV(85, 255, 0);
      } else if (i < 10){
        colors[i] = CHSV(42, 255, 0);
        colors[23-i] = CHSV(42, 255, 0);
      } else {
        colors[i] = CHSV(0, 255, 0);
        colors[23-i] = CHSV(0, 255, 0);
      }
    }
  }
}

void ProcessFHTResults() {
  //Method for converting the raw result data from the fht into values that represent how many LEDs to show 
  for (int i = 0; i < N_BANDS; i++) {
    //combing two and two frequency bands in order to be left with 4 bands to show instead of 8
    int newBandValue = (fht_oct_out[2*i]+fht_oct_out[2*i + 1]);
    int noise = freqNoise[i]; //setting a threshold to remove noise, some bands will never have values higher than the noise in other bands, so they need different thresholds which are held in an array
    newBands[i] = (newBandValue > noise) ? newBandValue - noise : 0; //removing the noise threshold before adding the value to the array, this array is added mostly for readability and for ease of debugging
    
    if (debug_new_bands) {
      Serial.print("  ");
      Serial.print(newBands[i]);
    }
    byte LEDsToShowTemp = newBands[i]*(N_LEDs/n_sections)/maxFreqMag[i]; //Maps the values of each frequency band (0, maxFreqMag[i]) to the amount of LEDs in each section (0, N_LEDs/n_sections)
    LEDsToShow[i] = (LEDsToShowTemp < N_LEDs/n_sections) ? LEDsToShowTemp : N_LEDs/n_sections; //Makes sure that the amount of LEDs to be turned on will never exceed the number of LEDs in each section
  }

  if (debug_new_bands) {
    Serial.print("  ");
    Serial.print(255);
    Serial.print("  ");
    Serial.println(0);
  }
}

void ShowResultsLED() {
  //This method decides which LEDs in each section should be turned on or off, based on the values calculated in ProcessFHTResults()
  //Here the values of each color is used to determine wether it is on or off
  
  //Do this for every LED in each section, and do one check for every section, resulting in one check for each individual LED
  //This could have been done in two for loops, but this method makes the index pattern easier to implement and more readable
  for (int i = 0; i < N_LEDs/n_sections; i++) { 
    if (i < LEDsToShow[0]) {
      colors[i].value = 100; //LED on
      leds[i] = colors[i];
    } else {
      colors[i].value = colors[i].value/2; //LED off, instead of setting value directly to 0, it is halved, making a fading pattern
      leds[i] = colors[i]; //This converts the color to RGB which is what the FastLED library is using and puts it in the array that has been passed to the FastLED object
    }
    if (i < LEDsToShow[1]) {
      colors[11-i].value = 100;
      leds[11-i] = colors[11-i];
    } else {
      colors[11-i].value = colors[11-i].value/2;
      leds[11-i] = colors[11-i];
    }
    if (i < LEDsToShow[2]) {
      colors[i+12].value = 100;
      leds[i+12] = colors[i+12];
    } else {
      colors[i+12].value = colors[i+12].value/2;
      leds[i+12] = colors[i+12];
    }
    if (i < LEDsToShow[3]) {
      colors[23-i].value = 100;
      leds[23-i] = colors[23-i];
    } else {
      colors[23-i].value = colors[23-i].value/2;
      leds[23-i] = colors[23-i];
    }
  }
  
  FastLED.show(); //After updating all the color value, this has to be called to push the updates to the actual LED-ring
}

void showAvgLED(byte n) {
  for (int i = 0; i < 12; i++) {
    if (i < n) {
      colors[i].value = 100;
      colors[23-i].value = 100;
      leds[i] = colors[i];
      leds[23-i] = colors[23-i];
    } else {
      colors[i].value = colors[i].value/2;
      colors[23-i].value = colors[23-i].value/2;
      leds[i] = colors[i];
      leds[23-i] = colors[23-i];
    }  
  }

  FastLED.show();
}

ISR(ADC_vect) {
  //This is the interrupt routine that collects the samples from the ADC and puts them in the sample array
  //How this works is explained further in the report
  int sample = ADCH; //Gets the value from the ADC and resets the register so that a new conversion can begin

  if (debug_samples) {
    Serial.print("  ");
    Serial.print(sample);
    Serial.print("  ");
    Serial.print(255);
    Serial.print("  ");
    Serial.println(0);
  }

  if (sample >= 255 || sample <= 0) {
    PORTB |= B00100000;
  } else {
    PORTB &= ~B00100000;
  }

  if (showSampleRate && sampleCount < 1) {
    sampleStartTime = millis();
  }
  
  sample = sample - DC_OFFSET; //Subtracts the DC_OFFSET value in order to center the signal around 0 as the signal was initally sent from the audio jack
  
  if (freqBinsRepresentation) {
    fht_input[sampleCount++] = sample; //Puts the sample into the array of samples and increments the amount of samples
  } else if (avgRepresentation) {
    sample = abs(sample);
    sample = (sample <= noiseSquelch) ? 0 : sample - noiseSquelch;
    sampleSum += sample;
    sampleCount++;
  }
  
  if (sampleCount >= FHT_N - 1) { //Turn off ADC when enough samples have been collected
    ADCSRA = 0; //This disables the ADC so that the array of samples can be processed with fht without the interrupt routine changing the values in the array as the processing is happening
    if (showSampleRate) {
      sampleEndTime = millis();
    }
  }
}
