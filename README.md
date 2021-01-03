# Arduino Music Visualizer

## Introduction

This was a school project early in my studies. The assignment was to create a lightshow using what we had learned in digital electronics. Our group chose to work with the arduino and a programmable LED ring from Adafruit.

## Contents

This repository contains 3 arduino skteches.
analogread sample is a program that attemts to measure the sample rate of the AnalogRead() function in the arduino library.
sample rate is a a program that attempts to measure the sample rate of the IRS(), interrupt routine, using the described settings
FHT is the complete program for the assignment. It reads an audio input from the described circuit, analyses it using the FHT library and visualizes the results through the FastLED libray.

Flowcharts describing the flow of the main program

Circuit diagram of the circuit needed to amplify and transform the signal from an audio jack

Technical report with a complete description of the solution
