# STM32 Hardware Test Client

## 1. Overview

This project provides a Linux-based hardware testing application written in C, designed to communicate with an STM32F756ZG microcontroller using UDP.

The application allows users to execute hardware peripheral tests, receive test results, and store execution history in an SQLite3 database.

The system consists of two main components:

- **STM32 Firmware:** Executes hardware tests and sends results through UDP.
- **Linux Test Client:** Sends test requests, receives responses, displays results, and maintains a persistent test history.

## 2. Supported Hardware Tests

The following peripherals are supported:

| Peripheral | Test Description |
|---|---|
| TIMER | Verifies periodic timer interrupts |
| UART | Tests data transmission and DMA reception |
| SPI | Tests DMA-based data transmission and reception |
| I2C | Tests data communication using DMA and blocking transfers |
| ADC | Verifies analog-to-digital conversion results |
| ALL | Executes all supported hardware tests |

The application supports individual peripheral testing and combined testing.

## 3. System Requirements

### Hardware

- STM32F756ZG development board
- Required peripheral connections
- Ethernet connection between the STM32 and the Linux environment

### Software

- Linux operating system (Ubuntu)
- GCC compiler
- SQLite3 library and development headers
- STM32 firmware configured as a UDP server

On Ubuntu, the required development tools can be installed using:

```bash
sudo apt update
sudo apt install build-essential libsqlite3-dev sqlite3
```

## 4. Network Configuration

The Linux client communicates with the STM32 using UDP.

| Parameter | Value |
|---|---|
| STM32 IP Address | 10.0.0.200 |
| STM32 UDP Port | 5000 |
| Transport Protocol | UDP |

The Linux computer and STM32 must have compatible network configurations and be able to communicate over the network.

The STM32 firmware must be running before hardware tests are executed.

## 5. Compilation

Open a terminal in the directory containing `test_client.c`.

Compile the application using:

```bash
gcc -Wall -Wextra -std=c11 test_client.c -o test_client -lsqlite3
```

The command generates an executable file named `test_client`.

## 6. Execution

Run the application using:

```bash
./test_client
```

The following interactive menu is displayed:

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

After selecting a hardware test, the user enters the number of iterations.

For UART, SPI, and I2C tests, the user also specifies a data pattern.

Two pattern generation modes are available:

1. Manual hexadecimal byte entry.
2. Automatic sequential pattern generation.

## 7. Test Configuration

The application supports the following parameters:

| Parameter | Supported Range |
|---|---|
| Number of iterations | 1–255 |
| Pattern length | 1–255 bytes for UART, SPI, and I2C |
| Pattern byte value | 0x00–0xFF |

For TIMER-only and ADC-only tests, no data pattern is required.

## 8. UDP Communication Protocol

The Linux application sends test requests to the STM32 using a custom UDP packet format.

### Request Packet

| Field | Size | Description |
|---|---|---|
| Test ID | 4 bytes | Unique test identifier, big-endian |
| Peripheral Bitmask | 1 byte | Selected hardware peripherals |
| Iterations | 1 byte | Number of test repetitions |
| Pattern Length | 1 byte | Number of pattern bytes |
| Pattern Data | 0–255 bytes | Test data |

### Peripheral Bitmask Values

| Peripheral | Value |
|---|---|
| TIMER | 0x01 |
| UART | 0x02 |
| SPI | 0x04 |
| I2C | 0x08 |
| ADC | 0x10 |
| ALL | 0x1F |

Multiple peripherals can be selected by combining their bitmask values.

### Response Packet

| Field | Size | Description |
|---|---|---|
| Test ID | 4 bytes | Corresponding test identifier |
| Result | 1 byte | Test execution status |

### Response Status Values

- `0x01` — PASS
- `0xFF` — FAIL

The Linux client validates the response length, sender address, UDP port, and Test ID before accepting the result.

## 9. Test Data Verification

The STM32 firmware verifies the integrity of transferred data.

For patterns containing 1–100 bytes, the firmware uses direct byte comparison.

For patterns larger than 100 bytes, the firmware calculates and compares CRC-32 checksums.

The CRC-32 implementation uses:

- Reflected polynomial: `0xEDB88320`
- Initial value: `0xFFFFFFFF`
- Final XOR: `0xFFFFFFFF`

## 10. SQLite3 Database

The Linux client uses SQLite3 to store test results persistently.

The database file is named:

`tests.db`

### Automatic Database Initialization

The application automatically creates the SQLite3 database and the `test_results` table if they do not already exist.

Existing tables and stored test records are preserved.

No manual database initialization is required.

### Database Table Structure

The `test_results` table contains:

| Column | Data Type | Description |
|---|---|---|
| id | INTEGER PRIMARY KEY AUTOINCREMENT | Internal database record ID |
| test_id | INTEGER | Hardware test identifier |
| peripherals | INTEGER | Selected peripheral bitmask |
| result | TEXT | PASS, FAIL, TIMEOUT, ERROR, or INVALID_RESPONSE |
| tested_at | DATETIME | Request timestamp |
| duration_seconds | REAL | Test execution duration |

The application uses prepared SQL statements and parameter binding when inserting records.

### Test History

The application allows users to display the 10 most recent test records by selecting option 7 from the main menu.

The displayed information includes:

- Test ID
- Selected peripherals
- Test result
- Request timestamp
- Test duration

## 11. Timeout Handling

The application uses a UDP receive timeout to prevent indefinite waiting.

The default timeout is 10 seconds.

When the TIMER peripheral is selected, an additional 2 seconds are allocated for each iteration.

The timeout is calculated as:

```text
Timeout = 10 seconds

If TIMER is selected:
Timeout = 10 + (Iterations × 2) seconds
```

If no valid response is received before the timeout expires, the application records the result as `TIMEOUT`.

## 12. Error Handling

The application handles the following conditions:

- Invalid peripheral
