# STM32 Hardware Testing System

**STM32F756ZG | Embedded C | DMA | UDP | Linux | SQLite3**

## 1. Project Overview

This project implements a hardware testing and verification system based on the STM32 NUCLEO-F756ZG development board.

A Linux command-line application communicates with the STM32 firmware over UDP, allowing users to execute peripheral tests, configure test parameters, and store results in an SQLite3 database.

The system consists of two main components:

- **STM32 Firmware:** Executes hardware tests, processes UDP requests, verifies data integrity, and sends test results.
- **Linux Test Client:** Provides an interactive interface, sends test requests, validates responses, and maintains persistent test history.

The firmware uses STM32 HAL drivers and the LwIP networking stack without an RTOS.

### System Architecture

```text
+--------------------------------+
| Linux Test Client              |
|                                |
| - Interactive CLI              |
| - Test Configuration           |
| - UDP Client                   |
| - SQLite3 Database             |
+---------------+----------------+
                |
                | UDP / Ethernet
                |
+---------------v----------------+
| STM32F756ZG                    |
|                                |
| - LwIP UDP Server              |
| - Hardware Test Dispatcher     |
| - TIMER / UART / SPI / I2C     |
| - ADC                          |
| - DMA and Interrupt Callbacks  |
| - Data Integrity Verification  |
+--------------------------------+
```

## 2. Supported Hardware Tests

| Test | Peripheral | Description |
|---|---|---|
| TIMER | TIM3 | Verifies periodic timer interrupts |
| UART | USART2 and USART6 | Bidirectional communication with DMA reception |
| SPI DMA | SPI1 and SPI4 | Master–slave communication using DMA |
| I2C DMA | I2C1 and I2C2 | Master–slave communication with DMA and blocking transfers |
| ADC | ADC1 | Analog input conversion and range verification |
| ALL | All peripherals | Executes the complete hardware test sequence |

The application supports iteration counts from 1 to 255 and data patterns containing 1 to 255 bytes for communication tests.

### Data Verification

The firmware verifies received data using:

- `memcmp()` for patterns of up to 100 bytes.
- Software CRC-32 comparison for patterns larger than 100 bytes.

CRC-32 configuration:

- Reflected polynomial: `0xEDB88320`
- Initial value: `0xFFFFFFFF`
- Final XOR: `0xFFFFFFFF`

## 3. Hardware Requirements

- STM32 NUCLEO-F756ZG development board
- USB cable for power, programming, and debugging
- Ethernet cable and network connection
- Jumper wires for peripheral communication
- Computer running STM32CubeIDE
- Linux computer or Linux virtual machine

## 4. Hardware Pin Configuration and Wiring

The following pin assignments were obtained from the STM32CubeMX configuration.

The communication tests use connections between peripherals on the same STM32 board.

**Important:** GPIO names identify the microcontroller signals. Except where explicitly documented, physical connector positions must be verified against the NUCLEO-F756ZG board pinout before connecting jumper wires.

### 4.1 UART — USART2 and USART6

USART2 and USART6 are configured for asynchronous serial communication.

| USART2 | USART6 | Signal Direction |
|---|---|---|
| PD5 — USART2_TX | PC7 — USART6_RX | USART2 → USART6 |
| PA3 — USART2_RX | PC6 — USART6_TX | USART6 → USART2 |

Required signal connections:

```text
PD5 (USART2_TX) ------ PC7 (USART6_RX)
PC6 (USART6_TX) ------ PA3 (USART2_RX)
```

The firmware performs bidirectional data verification using blocking transmission and DMA reception.

USART3 is also configured in STM32CubeMX but is not used by the UART hardware test.

### 4.2 SPI — SPI1 Master and SPI4 Slave

SPI1 operates as a full-duplex master, while SPI4 operates as a full-duplex slave.

| SPI1 / GPIO | SPI4 | Purpose |
|---|---|---|
| PA5 — SPI1_SCK | PE2 — SPI4_SCK | Clock |
| PB5 — SPI1_MOSI | PE6 — SPI4_MOSI | Master-to-slave data |
| PA6 — SPI1_MISO | PE5 — SPI4_MISO | Slave-to-master data |
| PB12 — GPIO CS | PE4 — SPI4_NSS | Slave selection |

Required signal connections:

```text
PA5  (SPI1_SCK)  ------ PE2 (SPI4_SCK)
PB5  (SPI1_MOSI) ------ PE6 (SPI4_MOSI)
PA6  (SPI1_MISO) ------ PE5 (SPI4_MISO)
PB12 (GPIO CS)   ------ PE4 (SPI4_NSS)
```

The firmware uses DMA for SPI data transfers and controls chip selection through GPIO PB12.

### 4.3 I2C — I2C1 Master and I2C2 Slave

I2C1 operates as the master and I2C2 as the slave.

| I2C1 | I2C2 | Purpose |
|---|---|---|
| PB8 — I2C1_SCL | PF1 — I2C2_SCL | Clock |
| PB9 — I2C1_SDA | PF0 — I2C2_SDA | Data |

Required signal connections:

```text
PB8 (I2C1_SCL) ------ PF1 (I2C2_SCL)
PB9 (I2C1_SDA) ------ PF0 (I2C2_SDA)
```

The I2C test uses DMA and blocking transfers for bidirectional communication.

The I2C lines require suitable pull-up resistors. The STM32CubeMX configuration enables internal GPIO pull-ups, but external pull-ups may be required for reliable operation depending on the hardware setup.

### 4.4 ADC — ADC1 Channel IN0

The ADC test uses a physical jumper connection between the analog input and the board's 3.3V supply.

| Board Connector | Connector Pin | Signal |
|---|---|---|
| CN10 | 29 | PA0 / ADC1_IN0 |
| CN8 | 7 | 3V3 |

Physical connection:

```text
CN8 Pin 7 (3V3)
       |
       | Jumper wire
       |
CN10 Pin 29 (PA0 / ADC1_IN0)
```

This connection supplies approximately 3.3V to the ADC input.

The firmware performs five ADC conversions, calculates the median reading, and verifies that the result is within the expected range of 3800–4095 for the configured 12-bit conversion.

### 4.5 TIMER — TIM3

TIM3 generates periodic interrupts.

The firmware verifies the timer by counting approximately 1,000 interrupts during a one-second measurement interval.

The accepted range is 990–1010 ticks.

No external jumper connection is required.

### 4.6 Ethernet and USB

**Ethernet**

Connect the board's RJ45 Ethernet interface to the network accessible by the Linux client.

The firmware uses LwIP for UDP communication.

**USB**

Connect the STM32 development board to the computer using USB for power, programming, and debugging through ST-LINK.

## 5. Software Requirements

### STM32 Firmware

- STM32CubeIDE
- STM32 HAL drivers
- LwIP networking stack
- ARM GCC toolchain

### Linux Client

- Ubuntu Linux or compatible distribution
- GCC compiler
- SQLite3 library and development headers
- SQLite3 command-line utility (optional)

Install the required packages:

```bash
sudo apt update
sudo apt install build-essential libsqlite3-dev sqlite3
```

## 6. Project Structure

```text
STM32-Hardware-Test-System/
|
|-- FinalProject/
|   |-- Core/
|   |-- Drivers/
|   |-- LWIP/
|   |-- Middlewares/
|   |-- Final_Project.ioc
|   |-- .project
|   |-- .cproject
|   `-- STM32F756ZGTX_FLASH.ld
|
|-- Linux_Client/
|   |-- test_client.c
|   |-- README.md
|   `-- tests.db
|
|-- .gitignore
`-- README.md
```

## 7. Installation and Setup

### 7.1 STM32 Firmware

1. Install STM32CubeIDE.
2. Import the `FinalProject` directory as an existing STM32CubeIDE project.
3. Open `Final_Project.ioc` to inspect the peripheral configuration.
4. Connect the required jumper wires according to the hardware wiring section.
5. Connect the board using USB and Ethernet.
6. Build the firmware.
7. Flash the firmware to the STM32.
8. Start or resume firmware execution.

### 7.2 Linux Client

Navigate to the client directory:

```bash
cd Linux_Client
```

Compile:

```bash
gcc -Wall -Wextra -std=c11 test_client.c -o test_client -lsqlite3
```

Run:

```bash
./test_client
```

The client automatically initializes the SQLite3 database and creates the required table if it does not exist.

### 7.3 Network Configuration

| Parameter | Value |
|---|---|
| STM32 IP | `10.0.0.200` |
| UDP Port | `5000` |
| Protocol | UDP |

The Linux computer must have network connectivity to the STM32.

When using VirtualBox, configure the virtual machine network so it can reach the board.

## 8. Usage and Examples

### Main Menu

```text
=== STM32 Hardware Test Client ===

Choose test:
1. TIMER
2. UART
3. SPI DMA
4. I2C DMA
5. ALL (Timer + UART + SPI + I2C + ADC)
6. ADC
7. View Test History
0. Exit
```

### Example 1 — UART Test

Select UART and configure one iteration with a one-byte test pattern.

Example successful output:

```text
Sending test 1 to STM32...
Waiting up to 10 seconds for response...

Test ID: 1
Peripherals: 0x02
Iterations: 1
Result: PASS
Duration: 0.012 seconds
Saved to SQLite3 successfully!
```

### Example 2 — Combined Test

Select option `5` to execute all supported peripherals.

Example configuration:

```text
Test: ALL
Iterations: 5
Pattern length: 255
Pattern mode: Automatic
```

The firmware runs the selected peripheral tests and returns an overall PASS or FAIL result.

### Example 3 — Test History

Select option `7` to display the 10 most recent test results stored in SQLite3.

## 9. UDP Communication Protocol

### Request Format

| Field | Size |
|---|---|
| Test ID | 4 bytes, big-endian |
| Peripheral bitmask | 1 byte |
| Iterations | 1 byte |
| Pattern length | 1 byte |
| Pattern data | 0–255 bytes |

### Peripheral Bitmask

| Peripheral | Value |
|---|---|
| TIMER | `0x01` |
| UART | `0x02` |
| SPI | `0x04` |
| I2C | `0x08` |
| ADC | `0x10` |
| ALL | `0x1F` |

### Response Format

| Field | Size |
|---|---|
| Test ID | 4 bytes, big-endian |
| Result | 1 byte |

Result codes:

- `0x01` — PASS
- `0xFF` — FAIL

## 10. SQLite3 Database

Test results are stored in `tests.db`.

The application automatically creates the database and the `test_results` table when needed.

The database stores:

- Test ID
- Selected peripherals
- Test result
- Request timestamp
- Test duration

The client can display the 10 most recent test results.

To inspect the database manually:

```bash
sqlite3 tests.db
```

Example SQL query:

```sql
SELECT * FROM test_results;
```

Exit using:

```text
.quit
```

## 11. Timeout and Error Handling

The client validates test parameters and handles communication failures.

For tests without TIMER, the response timeout is 10 seconds.

For tests that include TIMER, the timeout is calculated as:

```text
Timeout = 10 + (2 × iterations) seconds
```

The application handles:

- Invalid user input
- UDP socket errors
- Response timeouts
- Invalid response packets
- Unexpected Test IDs
- SQLite3 errors

## 12. Common Issues and Troubleshooting

| Issue | Possible Cause | Solution |
|---|---|---|
| UDP timeout | Firmware not running | Start or resume STM32 execution |
| UDP timeout | Incorrect network settings | Verify IP configuration and Ethernet connection |
| UART failure | Incorrect jumper connections | Check USART2 and USART6 wiring |
| SPI failure | Incorrect SPI or CS wiring | Verify SPI1, SPI4, and NSS connections |
| I2C failure | Incorrect wiring or pull-ups | Verify SCL, SDA, and pull-up configuration |
| ADC failure | Missing analog input connection | Verify CN10 pin 29 to CN8 pin 7 |
| Compilation error | Missing SQLite3 development headers | Install `libsqlite3-dev` |
| Database error | Incorrect file permissions | Verify write permissions |

## 13. Uninstallation and Cleanup

### Linux Client

Remove the compiled executable:

```bash
rm -f test_client
```

To remove the SQLite3 database:

```bash
rm -f tests.db
```

**Warning:** Deleting `tests.db` permanently removes its stored test history.

The Linux client does not require system-wide installation.

### STM32 Firmware

The STM32CubeIDE project can be removed from the IDE workspace.

Deleting the project directory removes the local source files but does not erase firmware already programmed into the STM32.

To replace the programmed firmware, flash another application or erase the device using an appropriate STM32 programming tool.

## 14. Validation and Test Results

The system was tested using several scenarios:

| Scenario | Result |
|---|---|
| Combined test, 255-byte pattern, 5 iterations | PASS |
| UART test, 1-byte pattern, 1 iteration | PASS |
| UART test, 1-byte pattern, 255 iterations | PASS |
| Invalid iteration count (0) | Rejected |
| UDP request while STM32 execution was suspended | TIMEOUT |
| SQLite3 automatic initialization | PASS |
| SQLite3 result storage | PASS |
| Linux compilation with GCC warning flags | PASS |

## 15. Code Documentation and Design

The project uses Doxygen-style comments for documenting firmware and Linux functions.

The STM32 firmware separates UDP request reception from hardware test execution.

Hardware tests run outside the UDP receive callback, while peripheral completion events are handled using interrupts and DMA callbacks.

The design avoids lengthy hardware test execution inside interrupt handlers.

## 16. Additional Documentation

See `Linux_Client/README.md` for Linux-client-specific documentation.

The `FinalProject` directory contains the STM32CubeIDE firmware, peripheral configuration, and hardware test implementation.

---

**Technologies:** STM32F756ZG, Embedded C, STM32 HAL, LwIP, UDP, DMA, GCC, Linux, SQLite3