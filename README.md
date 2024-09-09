# Wireless P2P Messaging System

## Overview

This project leverages STM32 Cortex-M4 boards with integrated LoRa (SX126X) to enable wireless messaging between two client computers. The implementation is written in multi-threaded C and utilizes a custom peer-to-peer (P2P) messaging protocol. The Wio-E5 mini development board is used for this project. This project has only been tested on macos.

## Features

- Wireless communication between client computers using LoRa technology
- Multi-threaded C implementation
- Custom P2P messaging protocol
- Only requires one external library (libsodium)

## Hardware

- **Development Board:** Wio-E5 mini (STM32 Cortex-M4 with LoRa SX126X)

## Getting Started

### Prerequisites

- Make (for building the project)

## Building the Project

### 1. Clone the Repository

   ```
   git clone git@github.com:maalwan/serlora.git
   cd git@github.com:maalwan/serlora.git
   ```

### 2. Building the Project

Use the included `Makefile` to build the project:

   ```
   make
   ```

This will compile the source code and generate the necessary binaries.

## Usage (for macos)

### Set Up the Hardware
Connect two Wio-E5 mini boards and ensure they are powered and within range of each other.

### Get Dev Path Info
Run ```ls /dev/cu.*``` after plugging in the device to find the device paths. It should look something like ```/dev/cu.usbserial-12130```. You will use the part after the period (i.e. ```usbserial-12130```) as the "truncated_dev_path".

### Run the Application
After connecting the boards and locating their device paths, run
   ```
   ./wio truncated_dev_path passkey
   ```
to start the messaging client. Ensure the same passkey is used for both devices (otherwise, you won't be able to decrypt the recieved messages). Also ensure that your messages are not over ~200 characters in length (LoRa packets only support 255 byte packets maximum). Your client should look something like this (sample):
   ```
   $ ./wio truncated_dev_path passkey
   ~$ hi
   Recieved: hi, this is from the other dev board
   ~$ this is from my dev board
   Recieved: bye
   ~$ bye
   ```
Press delete to exit. You can use arrows like in the terminal to recall previous messages.
## Directory Structure

- `src/` - Contains source code
- `include/` - Contains header files
- `Makefile` - Makefile for building the project
- `README.md` - This file

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgments

- Wio-E5 mini documentation for hardware references
