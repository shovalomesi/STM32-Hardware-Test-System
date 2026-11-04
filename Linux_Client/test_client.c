
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <sqlite3.h>

#define STM32_IP "10.0.0.200"
#define STM32_PORT 5000
#define DB_FILE "tests.db"

#define TIMER 0x01
#define UART  0x02
#define SPI   0x04
#define I2C   0x08
#define ADC   0x10

#define MAX_PATTERN 255

/**
 * @brief Initializes the SQLite3 database.
 *
 * @details
 * Opens the database and creates the test_results table
 * if it does not already exist.
 *
 * Existing tables and stored test records are preserved.
 *
 * @return 0 on success, 1 on database initialization failure.
 */
static int init_database(void)
{
    sqlite3 *db = NULL;

    const char *sql =
        "CREATE TABLE IF NOT EXISTS test_results ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "test_id INTEGER, "
        "peripherals INTEGER, "
        "result TEXT, "
        "tested_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "duration_seconds REAL"
        ");";

    int rc = sqlite3_open(DB_FILE, &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Database error: %s\n",
                db ? sqlite3_errmsg(db) : "Unknown error");

        sqlite3_close(db);
        return 1;
    }

    char *error_message = NULL;

    rc = sqlite3_exec(db, sql, NULL, NULL, &error_message);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Database initialization error: %s\n",
                error_message ? error_message : "Unknown error");

        sqlite3_free(error_message);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}

/**
 * @brief Retrieves the next available test ID from SQLite3.
 *
 * @details
 * Opens the test results database and queries the highest
 * existing test ID.
 *
 * The next ID is calculated as MAX(test_id) + 1.
 * If no test records exist, the first ID is 1.
 *
 * Validates that the calculated ID is within the
 * supported uint32_t range.
 *
 * Closes the database connection before returning.
 *
 * @param next_id Pointer to the variable that receives
 *                the next available test ID.
 *
 * @return 0 on success, 1 on database or validation failure.
 */
int get_next_test_id(uint32_t *next_id)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT COALESCE(MAX(test_id), 0) + 1 "
        "FROM test_results;";

    int rc = sqlite3_open(DB_FILE, &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Database error: %s\n",
                db ? sqlite3_errmsg(db) : "Unknown error");
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n",
                sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "Cannot read Test ID\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_int64 value = sqlite3_column_int64(stmt, 0);

    if (value < 1 || value > UINT32_MAX)
    {
        fprintf(stderr, "Test ID out of range\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    *next_id = (uint32_t)value;

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

/**
 * @brief Saves a hardware test result to the SQLite3 database.
 *
 * @details
 * Opens the database and inserts a new record into
 * the test_results table.
 *
 * Each record contains the test ID, selected peripherals,
 * test result, send timestamp and test duration.
 *
 * Uses a prepared SQL statement with bound parameters.
 *
 * If the timestamp is NULL or the duration is negative,
 * the corresponding database field is stored as NULL.
 *
 * Finalizes the SQL statement and closes the database
 * connection before returning.
 *
 * @param test_id Unique identifier of the test.
 * @param peripherals Bitmask of the tested peripherals.
 * @param result String describing the test result.
 * @param sent_at Timestamp when the request was sent.
 * @param duration_seconds Test duration in seconds.
 *
 * @return 0 if the result was saved successfully,
 *         1 if a database operation failed.
 */
int save_result(uint32_t test_id, uint8_t peripherals,
                const char *result, const char *sent_at,
                double duration_seconds)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "INSERT INTO test_results "
        "(test_id, peripherals, result, tested_at, duration_seconds) "
        "VALUES (?, ?, ?, ?, ?);";

    int rc = sqlite3_open(DB_FILE, &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Database error: %s\n",
                db ? sqlite3_errmsg(db) : "Unknown error");
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n",
                sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)test_id);
    sqlite3_bind_int(stmt, 2, peripherals);
    sqlite3_bind_text(stmt, 3, result, -1, SQLITE_TRANSIENT);

    if (sent_at != NULL)
        sqlite3_bind_text(stmt, 4, sent_at, -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(stmt, 4);

    if (duration_seconds >= 0.0)
        sqlite3_bind_double(stmt, 5, duration_seconds);
    else
        sqlite3_bind_null(stmt, 5);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr, "Insert error: %s\n",
                sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return rc == SQLITE_DONE ? 0 : 1;
}

/**
 * @brief Calculates elapsed time using a monotonic clock.
 *
 * @details
 * Reads the current time using CLOCK_MONOTONIC
 * and calculates the difference from the supplied start time.
 *
 * Returns the elapsed duration in seconds,
 * including the fractional part.
 *
 * @param start Pointer to the starting timestamp.
 *
 * @return Elapsed time in seconds, or -1.0 if
 *         the system clock cannot be read.
 */
static double elapsed_seconds(const struct timespec *start)
{
    struct timespec end;

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0)
        return -1.0;

    return (double)(end.tv_sec - start->tv_sec) +
           (double)(end.tv_nsec - start->tv_nsec) / 1000000000.0;
}

/**
 * @brief Displays and stores a hardware test result.
 *
 * @details
 * Prints the test ID, selected peripherals,
 * iteration count and result to the terminal.
 *
 * Also displays the send timestamp and test duration
 * when valid values are available.
 *
 * Saves the test result to the SQLite3 database
 * using save_result().
 *
 * Reports an error if the database operation fails.
 *
 * @param test_id Unique identifier of the test.
 * @param peripherals Bitmask of the tested peripherals.
 * @param iterations Number of requested test repetitions.
 * @param result String describing the test result.
 * @param sent_at Timestamp when the request was sent,
 *                or NULL if unavailable.
 * @param duration_seconds Test duration in seconds,
 *                         or a negative value if unavailable.
 *
 * @return 0 if the result was saved successfully,
 *         1 otherwise.
 */
static int record_result(uint32_t test_id, uint8_t peripherals,
                         uint8_t iterations, const char *result,
                         const char *sent_at, double duration_seconds)
{
    printf("Test ID: %u\n", test_id);
    printf("Peripherals: 0x%02X\n", peripherals);
    printf("Iterations: %u\n", iterations);
    printf("Result: %s\n", result);

    if (sent_at != NULL)
        printf("Sent at: %s\n", sent_at);

    if (duration_seconds >= 0.0)
        printf("Duration: %.3f seconds\n", duration_seconds);

    if (save_result(test_id, peripherals, result,
                    sent_at, duration_seconds) != 0)
    {
        printf("Failed to save result to SQLite3\n");
        return 1;
    }

    printf("Saved to SQLite3 successfully!\n");
    return 0;
}

/**
 * @brief Sends a hardware test request to the STM32 via UDP.
 *
 * @details
 * Validates the test parameters and creates a UDP socket
 * for communication with the STM32 board.
 *
 * Builds a test request containing:
 * - Test ID (4 bytes, big-endian).
 * - Peripheral selection bitmask (1 byte).
 * - Number of iterations (1 byte).
 * - Pattern length (1 byte).
 * - Test pattern (0 to 255 bytes).
 *
 * Sends the request to the configured STM32 IP address
 * and UDP port, then waits for a response.
 *
 * The response timeout is 10 seconds, with an additional
 * 2 seconds per iteration when the TIMER test is selected.
 *
 * Measures the test duration using CLOCK_MONOTONIC
 * and records the request timestamp.
 *
 * Validates the received response by checking:
 * - Response length (5 bytes).
 * - Sender IP address and UDP port.
 * - Matching Test ID.
 * - Test result status (PASS or FAIL).
 *
 * Stores the test result, timestamp and duration
 * in the SQLite3 database using record_result().
 *
 * Handles communication errors, invalid responses
 * and timeouts.
 *
 * @param test_id Unique identifier of the test.
 * @param peripherals Bitmask of selected hardware peripherals.
 * @param iterations Number of test repetitions (1-255).
 * @param pattern Pointer to the test data buffer.
 *                May be NULL when no pattern is required.
 * @param length Number of pattern bytes (0-255).
 *
 * @return 0 if the test passes and the result is saved
 *         successfully, 1 otherwise.
 */
int run_test(uint32_t test_id, uint8_t peripherals,
             uint8_t iterations,
             const uint8_t *pattern, uint8_t length)
{
    if (peripherals == 0 ||
        (peripherals & 0xE0) != 0 ||
        iterations == 0 ||
        (length > 0 && pattern == NULL) ||
        ((peripherals & (UART | SPI | I2C)) && length == 0))
    {
        printf("Invalid test parameters\n");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("Socket creation failed");
        record_result(test_id, peripherals, iterations,
                      "ERROR", NULL, -1.0);
        return 1;
    }

    long timeout_seconds = 10;

    if (peripherals & TIMER)
        timeout_seconds += (long)iterations * 2;

    struct timeval timeout = {timeout_seconds, 0};

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0)
    {
        perror("Socket timeout setup failed");
        close(sock);
        record_result(test_id, peripherals, iterations,
                      "ERROR", NULL, -1.0);
        return 1;
    }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(STM32_PORT);

    if (inet_pton(AF_INET, STM32_IP, &server.sin_addr) != 1)
    {
        printf("Invalid STM32 IP address");
        close(sock);
        record_result(test_id, peripherals, iterations,
                      "ERROR", NULL, -1.0);
        return 1;
    }

    uint8_t request[7 + MAX_PATTERN] = {0};

    request[0] = (uint8_t)(test_id >> 24);
    request[1] = (uint8_t)(test_id >> 16);
    request[2] = (uint8_t)(test_id >> 8);
    request[3] = (uint8_t)test_id;
    request[4] = peripherals;
    request[5] = iterations;
    request[6] = length;

    if (length > 0)
        memcpy(request + 7, pattern, length);

    printf("\nSending test %u to STM32...\n", test_id);
    printf("Waiting up to %ld seconds for response...\n",
           timeout_seconds);

    struct timespec start;
    time_t wall_time = time(NULL);
    struct tm local_time;
    char sent_at[32];

    if (wall_time == (time_t)-1 ||
        localtime_r(&wall_time, &local_time) == NULL ||
        strftime(sent_at, sizeof(sent_at),
                 "%Y-%m-%d %H:%M:%S",
                 &local_time) == 0)
    {
        printf("Cannot read send timestamp\n");
        close(sock);
        record_result(test_id, peripherals, iterations,
                      "ERROR", NULL, -1.0);
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
    {
        perror("Clock error");
        close(sock);
        record_result(test_id, peripherals, iterations,
                      "ERROR", NULL, -1.0);
        return 1;
    }

    ssize_t sent = sendto(
        sock, request, 7 + length, 0,
        (struct sockaddr *)&server, sizeof(server));

    if (sent != (ssize_t)(7 + length))
    {
        if (sent < 0)
            perror("UDP send failed");
        else
            printf("Incomplete UDP send\n");

        double duration = elapsed_seconds(&start);

        close(sock);

        record_result(test_id, peripherals, iterations,
                      "ERROR", sent_at, duration);
        return 1;
    }

    uint8_t response[6] = {0};
    struct sockaddr_in sender = {0};
    socklen_t sender_len = sizeof(sender);

    ssize_t received = recvfrom(
        sock, response, sizeof(response), 0,
        (struct sockaddr *)&sender, &sender_len);

    double duration = elapsed_seconds(&start);

    close(sock);

    if (received < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("STM32 response timeout\n");

            record_result(test_id, peripherals, iterations,
                          "TIMEOUT", sent_at, duration);
        }
        else
        {
            perror("UDP receive error");

            record_result(test_id, peripherals, iterations,
                          "ERROR", sent_at, duration);
        }

        return 1;
    }

    if (received != 5 ||
        sender.sin_addr.s_addr != server.sin_addr.s_addr ||
        sender.sin_port != server.sin_port ||
        memcmp(response, request, 4) != 0)
    {
        printf("Invalid UDP response\n");

        record_result(test_id, peripherals, iterations,
                      "INVALID_RESPONSE", sent_at, duration);
        return 1;
    }

    const char *result;

    if (response[4] == 0x01)
        result = "PASS";
    else if (response[4] == 0xFF)
        result = "FAIL";
    else
        result = "INVALID_RESPONSE";

    int saved = record_result(test_id, peripherals,
                              iterations, result,
                              sent_at, duration);

    if (saved != 0)
        return 1;

    return strcmp(result, "PASS") == 0 ? 0 : 1;
}


/**
 * @brief Displays the 10 most recent hardware test results.
 *
 * @details
 * Opens the SQLite3 database and retrieves the 10 most
 * recent records from the test_results table.
 *
 * Records are ordered by their database ID in
 * descending order.
 *
 * Displays the following information:
 * - Test ID.
 * - Selected peripherals bitmask.
 * - Test result.
 * - Request timestamp.
 * - Test duration in seconds.
 *
 * Handles NULL database values and reports
 * database errors when necessary.
 *
 * Finalizes the SQL statement and closes the
 * database connection before returning.
 *
 * @return None.
 */
void show_test_history(void)
{
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT test_id, peripherals, result, tested_at, "
        "ROUND(duration_seconds, 3) "
        "FROM test_results "
        "ORDER BY id DESC LIMIT 10;";

    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK)
    {
        fprintf(stderr, "Database error: %s\n",
                db ? sqlite3_errmsg(db) : "Unknown error");
        sqlite3_close(db);
        return;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    printf("\n=== Last 10 Test Results ===\n");
    printf("%-8s %-12s %-18s %-20s %s\n",
           "Test ID", "Peripherals", "Result",
           "Sent At", "Duration (s)");

    int rc;
    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        uint32_t test_id =
            (uint32_t)sqlite3_column_int64(stmt, 0);

        int peripherals = sqlite3_column_int(stmt, 1);

        const unsigned char *result =
            sqlite3_column_text(stmt, 2);

        const unsigned char *sent_at =
            sqlite3_column_text(stmt, 3);

        printf("%-8u 0x%02X         %-18s %-20s ",
               test_id,
               peripherals,
               result ? (const char *)result : "N/A",
               sent_at ? (const char *)sent_at : "N/A");

        if (sqlite3_column_type(stmt, 4) == SQLITE_NULL)
            printf("N/A\n");
        else
            printf("%.3f\n", sqlite3_column_double(stmt, 4));

        count++;
    }

    if (rc != SQLITE_DONE)
        fprintf(stderr, "History read error: %s\n",
                sqlite3_errmsg(db));

    if (count == 0)
        printf("No test records found.\n");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

/**
 * @brief Main entry point of the STM32 hardware test client.
 *
 * @details
 * Initializes the next test ID using the SQLite3 database
 * and displays an interactive terminal menu.
 *
 * Allows the user to:
 * - Run TIMER tests.
 * - Run UART tests.
 * - Run SPI DMA tests.
 * - Run I2C DMA tests.
 * - Run ADC tests.
 * - Run all hardware tests together.
 * - View the 10 most recent test results.
 *
 * For each selected test, the user specifies the
 * number of iterations.
 *
 * For UART, SPI and I2C tests, the user also selects
 * a data pattern length and chooses between:
 * - Manual hexadecimal byte input.
 * - Automatic sequential pattern generation.
 *
 * Executes the selected test using run_test()
 * and increments the test ID after each attempt.
 *
 * The program continues displaying the menu
 * until the user exits or an input error occurs.
 *
 * @return 0 on normal program termination,
 *         1 if initialization or certain input operations fail.
 */
int main(void)
{
    int choice;
    unsigned int iterations;
    unsigned int pattern_length;

    uint32_t test_id;
    uint8_t pattern[MAX_PATTERN];
    
    if (init_database() != 0)
    {
        printf("Cannot initialize SQLite3 database\n");
        return 1;
    }
    if (get_next_test_id(&test_id) != 0)
    {
        printf("Cannot initialize Test ID\n");
        return 1;
    }

    printf("=== STM32 Hardware Test Client ===\n");
    printf("Next Test ID: %u\n", test_id);

    while (1)
    {
        printf("\nChoose test:\n");
        printf("1. TIMER\n");
        printf("2. UART\n");
        printf("3. SPI DMA\n");
        printf("4. I2C DMA\n");
        printf("5. ALL (Timer + UART + SPI + I2C + ADC)\n");
        printf("6. ADC\n");
        printf("7. View Test History\n");
        printf("0. Exit\n");
        printf("Selection: ");
        fflush(stdout);

        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input\n");
            break;
        }

        if (choice == 0)
        {
            printf("Goodbye!\n");
            break;
        }
        
        if (choice == 7)
        {
            show_test_history();
            continue;
        }
        uint8_t peripherals;

        switch (choice)
        {
            case 1: peripherals = TIMER; break;
            case 2: peripherals = UART; break;
            case 3: peripherals = SPI; break;
            case 4: peripherals = I2C; break;
            case 5:
                peripherals = TIMER | UART | SPI | I2C | ADC;
                break;
            case 6: peripherals = ADC; break;
            default:
                printf("Invalid selection\n");
                continue;
        }

        printf("Number of iterations (1-255): ");

        if (scanf("%u", &iterations) != 1 ||
            iterations < 1 || iterations > 255)
        {
            printf("Invalid iterations\n");
            break;
        }

        uint8_t length = 0;

        if (peripherals & (UART | SPI | I2C))
        {
            printf("Pattern length (1-%d): ", MAX_PATTERN);

            if (scanf("%u", &pattern_length) != 1 ||
                pattern_length < 1 ||
                pattern_length > MAX_PATTERN)
            {
                printf("Invalid pattern length\n");
                break;
            }

            length = (uint8_t)pattern_length;

            
            unsigned int pattern_mode;

            printf("Choose pattern mode:\n");
            printf("1. Enter HEX bytes manually\n");
            printf("2. Generate pattern automatically\n");
            printf("Selection: ");

            if (scanf("%u", &pattern_mode) != 1 ||
                (pattern_mode != 1 && pattern_mode != 2))
            {
                printf("Invalid pattern mode\n");
                return 1;
            }

            if (pattern_mode == 1)
            {
                printf("Enter %u HEX bytes separated by spaces:\n",
                       pattern_length);

                for (unsigned int i = 0; i < pattern_length; i++)
                {
                    unsigned int value;

                    if (scanf("%x", &value) != 1 || value > 0xFF)
                    {
                        printf("Invalid HEX byte\n");
                        return 1;
                    }

                    pattern[i] = (uint8_t)value;
                }
            }
            else
            {
                for (unsigned int i = 0; i < pattern_length; i++)
                {
                    pattern[i] = (uint8_t)i;
                }

                printf("Generated %u pattern bytes automatically.\n",
                       pattern_length);
            }

        }

        run_test(test_id, peripherals,
                 (uint8_t)iterations,
                 length > 0 ? pattern : NULL,
                 length);

        if (test_id == UINT32_MAX)
        {
            printf("Test ID limit reached\n");
            break;
        }

        test_id++;
    }

    return 0;
}

