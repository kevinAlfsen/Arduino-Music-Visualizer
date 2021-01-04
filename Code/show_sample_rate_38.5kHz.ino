unsigned long prevTime, currentTime, totalSamples;

void setup() {
  noInterrupts(); //disable interrupts while configuring ADC
  Serial.begin(57600); //Start serial communication
  
  //Configure ADC for 38.5kHz 8 bit sampling
  ADCSRA = 0; //Clear ADCSRA and ADCSRB registers
  ADCSRB = 0;

  ADMUX |= (1 << REFS0); //Set reference voltage to internal 5V
  ADMUX |= (1 << ADLAR); // Leftshift ADC register so we can reset by only reading 8 bits

  //Set prescaler to 32 -> 16MHz/32 =~ 500kHz
  //Reading ADCH requires 13 operations -> 500kHz/13 =~ 38.5kHz
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
  
  ADCSRA |= (1 << ADATE); //Enable auto trigger
  ADCSRA |= (1 << ADEN); //Enable ADCSRA register
  ADCSRA |= (1 << ADIE); //Enable interrupts
  ADCSRA |= (1 << ADSC); //Start conversion

  prevTime = millis(); //set prevTime as startup time
  
  interrupts(); //re-enable interrupts
}

void loop() {
  currentTime = millis(); //update currentTime every loop
  if ((currentTime - prevTime) > 1000) { //If 1 second has passed
    Serial.println(totalSamples); //print total amount of samples per second
    totalSamples = 0;
    prevTime = millis();
  }

}

ISR(ADC_vect) {
  byte sample = ADCH;
  totalSamples++;
}
