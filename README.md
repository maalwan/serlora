# Wireless P2P Messaging System

## Overview

This project leverages STM32 Cortex-M4 boards with integrated LoRa (SX126X) to enable wireless messaging between two client computers. The implementation is written in multi-threaded C and utilizes a custom peer-to-peer (P2P) messaging protocol. The Wio-E5 mini development board is used for this project.

## Features

- Wireless communication between client computers using LoRa technology
- Multi-threaded C implementation
- Custom P2P messaging protocol
- No external libraries required

## Hardware

- **Development Board:** Wio-E5 mini (STM32 Cortex-M4 with LoRa SX126X)

## Getting Started

### Prerequisites

- Make (for building the project)

## Building the Project

### 1. Clone the Repository

   ```
   git clone <repository-url>
   cd <repository-directory>
   ```

### 2. Building the Project

Use the included `Makefile` to build the project:

   ```
   make
   ```

This will compile the source code and generate the necessary binaries.

## Flashing the Firmware
Refer to the documentation of your STM32 toolchain for instructions on how to flash the firmware onto the Wio-E5 mini development board.

## Usage

### Set Up the Hardware
Connect two Wio-E5 mini boards and ensure they are powered and within range of each other.

### Run the Application
After flashing the firmware, power on both boards. They will automatically begin the wireless messaging process using the custom P2P protocol.

## Directory Structure

- `src/` - Contains source code
- `include/` - Contains header files
- `Makefile` - Makefile for building the project
- `README.md` - This file

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgments

- Wio-E5 mini documentation for hardware references