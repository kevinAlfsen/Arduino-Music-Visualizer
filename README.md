# Arduino Music Visualizer

## Introduction

This was a school project early in my studies. The assignment was to create a lightshow using what we had learned in digital electronics. Our group chose to work with the arduino and a programmable LED ring from Adafruit.

The goal was to make the LED ring light up in harmony with the music in some way or another. The chosen solution has two modes, one showing the average power of the signal and one showing the abundance of different frequency bands in the signal. 

## Contents

### Code

#### 388.5kHz_FHT_FastLED_Final

Complete arduino sketch described in the report.

#### show_sample_rate_38.5kHz

Test sketch to messure the sample rate of the interrupt routine with the described configuration.

#### show_sample_rate_analogRead

Test sketch to test the sample rate of the analogRead function found in the arduino library.

### Documents

#### Circuit Diagram

Technical drawing of the circuit containing an amplifier and a voltage divider.

#### Flow Charts

Flow charts describing the different parts and methods in the program. 

#### Report

Full technical report describing the theoruy, methods and materials used in this project.

## Description

The first step was to design and build a circuit that amplifies and converts the signal into a form readable by the arduino.

Once the signal from an audio jack could be sent to and read by the arduino, the next step was to chose a sample rate. The analogRead function in the arduino library uses a lot of operations and so it didnt´t provide a very high sample rate. Instead we chose to read the register on the ADC directly, achieving a sample rate of about 38.5kHz, which was the closest configuration to the standard sample rate of music. 

The reason for the sample rate demand, was to enable us to use harley transform to excract the prominence of different frequencies in the music signal. This is done through a library called FHT.

We also added a mode where the average magnitude of the signal is calculated from a set number of samples, resulting in a visualization that can represent the volume of the music at a given point in time. 

Once the signal is read and processed it needed to be visualised. Controlling the LEDs is done through a library called FastLED. 

A way more detailed description of this project is provided in the technical report.

The main challenge and point of interest in this project was how to achieve the high sample rate. the ADC is configured and read directly by accessing the byte values of different relevant registers. Although the result was very satisfying and definetly within the goals of the project, there are plenty of improvememts to  be made. The signal is read via an audio jack and the only way to listen to the music while analysing it, is to connect a headst or speakers in paralell with the input of the circuit. Ideally the signal could be sent wirelessly to the arduino while simultaneuosly playing the music. The frequency representation. though interesting theoretically, was not very visually appealling. The frequency bands where chosen quite arbitrarily and the led ring had few leds, resulting in the lighting of the leds seeming quite random. The result would probably be more visually interesting had the frequency bands been extrcacted through hardware and had the led ring been bigger. 
