unsigned long currentTime, prevTime, totalSamples;

void setup() {
  Serial.begin(57600);
  pinMode(A0, INPUT);
  prevTime = millis();
}

void loop() {
  currentTime = millis();
  analogRead(A0); //Take sample from analog port
  totalSamples++; //Increment amount of samples
  if ((currentTime - prevTime) > 1000) { // 1 second has passed
    Serial.println(totalSamples); //print amount of samples
    totalSamples = 0;
    prevTime = millis();
  }
}
