# Multi-Modal Embedded Systems Project

This repository contains a multi-faceted embedded systems project that integrates real-time audio analysis, Bluetooth-controlled robotics, Spotify API interaction, and visual output via FPGA and VGA. The project showcases various technologies and protocols working together to create an interactive, music-driven robotics and visualization platform.

## Project Overview

The project consists of the following major components:

### 🎵 Python Spotify API Interface
- A Python script authenticates and communicates with the Spotify Web API.
- Retrieves current playback metadata, track audio features, and beat timing.
- Sends relevant track information (e.g., title, artist, length) to the embedded system via serial interface.

### 🤖 Bluetooth-Controlled Zumo Robots
- Embedded firmware on Zumo robots enables wireless control via Bluetooth.
- Robots respond to playback signals to perform synchronized, automated movements (e.g., dance routines).
- Supports control commands for starting, stopping, and changing behavior profiles.

### 🔊 Real-Time Audio Spectrum Analyzer
- Microcontroller captures audio input via ADC with DMA support.
- FFT is performed on the sampled audio using CMSIS-DSP libraries.
- A Hamming window is applied prior to FFT to improve frequency resolution.
- FFT output is processed into logarithmically scaled frequency bands.
- Extracted frequency band amplitudes are used for visualization.

### 🖥️ SPI Interface to FPGA for VGA Output
- Embedded system sends processed audio spectrum data over SPI to an external FPGA.
- FPGA interprets this data to drive a VGA output displaying a real-time frequency spectrum analyzer with live amplitude bars.

## Hardware Components
- Zumo robot with PWM motor driver and Bluetooth module
- STM32 microcontroller (STM32L4 series)
- FPGA (Altera DE2 Board) for VGA output
- Line-in audio capture circuit
- Supporting components for SPI, UART, ADC, and PWM

## Software Stack
- Embedded C for STM32 firmware
- CMSIS-DSP for FFT computation
- Python (requests, spotipy) for Spotify interaction
- Verilog or VHDL for FPGA VGA driver and SPI interface
