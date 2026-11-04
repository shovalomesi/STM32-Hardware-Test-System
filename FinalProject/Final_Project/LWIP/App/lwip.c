/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if (defined ( __CC_ARM ) || defined (__ARMCC_VERSION))  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include "ethernetif.h"

/* USER CODE BEGIN 0 */
#include "lwip/udp.h"
#include "lwip/pbuf.h"
/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
static void ethernet_link_status_updated(struct netif *netif);
static void Ethernet_Link_Periodic_Handle(struct netif *netif);
/* ETH Variables initialization ----------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN 1 */
#define UDP_SERVER_PORT 5000

static struct udp_pcb *udp_server_pcb = NULL;

static void UDP_Receive_Callback(void *arg,
                                 struct udp_pcb *pcb,
                                 struct pbuf *p,
                                 const ip_addr_t *addr,
                                 u16_t port);


#define UDP_REQUEST_HEADER_SIZE 7
#define UDP_RESPONSE_SIZE       5

#define TEST_TIMER 0x01
#define TEST_UART  0x02
#define TEST_SPI   0x04
#define TEST_I2C   0x08
#define TEST_ADC   0x10

#define TEST_PASS 0x01
#define TEST_FAIL 0xFF

/**
 * @brief Stores the parameters of a UDP hardware test request.
 *
 * Contains the test identifier, selected peripherals,
 * number of iterations, and the data pattern used
 * for hardware testing.
 *
 * The pattern buffer supports up to 255 bytes.
 */
typedef struct
{
    uint32_t test_id;
    uint8_t peripherals;
    uint8_t iterations;
    uint8_t pattern_length;
    uint8_t pattern[255];

} UDP_TestRequest;

static uint8_t UDP_ParseRequest(const struct pbuf *p,
                                UDP_TestRequest *request);

static UDP_TestRequest udp_pending_request;

static ip_addr_t udp_pending_addr;

static u16_t udp_pending_port = 0;

static uint8_t udp_request_pending = 0;

extern uint8_t Hardware_RunTests(uint8_t peripherals,
                                 const uint8_t *pattern,
                                 uint8_t length,
                                 uint8_t iterations);
/* USER CODE END 1 */
uint32_t EthernetLinkTimer;

/* Variables Initialization */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint8_t IP_ADDRESS[4];
uint8_t NETMASK_ADDRESS[4];
uint8_t GATEWAY_ADDRESS[4];

/* USER CODE BEGIN 2 */
/**
 * @brief Initializes the UDP server using the LwIP stack.
 *
 * Creates a new UDP protocol control block (PCB)
 * and binds it to the configured UDP server port
 * on all available local IP addresses.
 *
 * Registers UDP_Receive_Callback() to handle
 * incoming UDP test requests.
 *
 * If PCB allocation or port binding fails,
 * returns the corresponding error code and
 * releases allocated resources when necessary.
 *
 * @return ERR_OK if the UDP server is initialized successfully,
 *         ERR_MEM if PCB allocation fails,
 *         or the LwIP error code if binding fails.
 */
static err_t UDP_Server_Init(void)
{
    err_t err;

    udp_server_pcb = udp_new();

    if (udp_server_pcb == NULL)
    {
        return ERR_MEM;
    }

    err = udp_bind(udp_server_pcb, IP_ADDR_ANY, UDP_SERVER_PORT);

    if (err != ERR_OK)
    {
        udp_remove(udp_server_pcb);
        udp_server_pcb = NULL;
        return err;
    }
    udp_recv(udp_server_pcb, UDP_Receive_Callback, NULL);

    return ERR_OK;
}

/**
 * @brief Handles incoming UDP test requests.
 *
 * @details
 * Called by the LwIP stack whenever a UDP packet is received.
 *
 * Parses and validates the incoming request using
 * UDP_ParseRequest().
 *
 * If the request is valid and no other request is pending,
 * stores the test parameters and the client's IP address
 * and UDP port for later processing.
 *
 * Hardware tests are not executed inside this callback.
 * They are performed later by UDP_ProcessPendingTest().
 *
 * If the request is invalid or another request is already
 * pending, sends a failure response containing the original
 * 4-byte Test ID followed by TEST_FAIL, provided that
 * at least four bytes were received.
 *
 * The received packet buffer is released before returning.
 *
 * @param arg User-defined callback argument (unused).
 * @param pcb Pointer to the UDP protocol control block.
 * @param p Pointer to the received packet buffer.
 * @param addr Pointer to the sender's IP address.
 * @param port UDP source port of the sender.
 */
static void UDP_Receive_Callback(void *arg,
                                 struct udp_pcb *pcb,
                                 struct pbuf *p,
                                 const ip_addr_t *addr,
                                 u16_t port)
{
    LWIP_UNUSED_ARG(arg);

    if (p == NULL)
    {
        return;
    }

    UDP_TestRequest request;
    uint8_t result = UDP_ParseRequest(p, &request);

    if (result == TEST_PASS)
    {
        if (udp_request_pending == 0)
        {
            udp_pending_request = request;
            ip_addr_copy(udp_pending_addr, *addr);
            udp_pending_port = port;
            udp_request_pending = 1;
        }
        else
        {
            result = TEST_FAIL;
        }
    }

    uint8_t response[UDP_RESPONSE_SIZE];

    // Preserve Test-ID bytes exactly as received
    if (p->tot_len >= 4)
    {
        if (pbuf_copy_partial(p, response, 4, 0) != 4)
        {
            pbuf_free(p);
            return;
        }

        // Temporary result: request format validation only
        if (result == TEST_PASS)
        {
            // Valid request was queued.
            // Hardware result will be sent later.
            pbuf_free(p);
            return;
        }

        response[4] = TEST_FAIL;

        struct pbuf *tx = pbuf_alloc(PBUF_TRANSPORT,
                                    UDP_RESPONSE_SIZE,
                                    PBUF_RAM);

        if (tx != NULL)
        {
            if (pbuf_take(tx, response, UDP_RESPONSE_SIZE) == ERR_OK)
            {
                udp_sendto(pcb, tx, addr, port);
            }

            pbuf_free(tx);
        }
    }

    pbuf_free(p);
}


/**
 * @brief Parses and validates an incoming UDP test request.
 *
 * @details
 * Extracts the request header and test pattern from an
 * incoming LwIP pbuf.
 *
 * The request format is:
 * - Bytes 0-3: Test ID (32-bit, big-endian).
 * - Byte 4: Peripheral selection bitmask.
 * - Byte 5: Number of test iterations.
 * - Byte 6: Test pattern length.
 * - Remaining bytes: Test pattern (if present).
 *
 * Validates the packet length, peripheral selection,
 * iteration count, and pattern requirements.
 *
 * Supported peripherals are TIMER, UART, SPI, I2C and ADC.
 * UART, SPI and I2C require a non-empty test pattern.
 *
 * Returns failure immediately if any validation fails.
 *
 * @param p Pointer to the received UDP packet buffer.
 * @param request Pointer to the parsed test request structure.
 *
 * @return TEST_PASS if the request is valid,
 *         TEST_FAIL otherwise.
 */
static uint8_t UDP_ParseRequest(const struct pbuf *p,
                                UDP_TestRequest *request)
{
    uint8_t header[UDP_REQUEST_HEADER_SIZE];

    if (p == NULL || request == NULL)
    {
        return TEST_FAIL;
    }

    // Validate minimum request size
    if (p->tot_len < UDP_REQUEST_HEADER_SIZE)
    {
        return TEST_FAIL;
    }

    // Copy UDP request header
    if (pbuf_copy_partial(p, header,
                          UDP_REQUEST_HEADER_SIZE, 0)
        != UDP_REQUEST_HEADER_SIZE)
    {
        return TEST_FAIL;
    }

    // Extract Test ID (big-endian)
    request->test_id =
        ((uint32_t)header[0] << 24) |
        ((uint32_t)header[1] << 16) |
        ((uint32_t)header[2] << 8)  |
        ((uint32_t)header[3]);

    // Extract test parameters
    request->peripherals = header[4];
    request->iterations = header[5];
    request->pattern_length = header[6];

    // Validate exact UDP packet length
    if (p->tot_len !=
        UDP_REQUEST_HEADER_SIZE + request->pattern_length)
    {
        return TEST_FAIL;
    }

    // Validate peripheral selection and iterations
    if (request->peripherals == 0 ||
        (request->peripherals & 0xE0) != 0 ||
        request->iterations == 0)
    {
        return TEST_FAIL;
    }

    // UART, SPI and I2C require a non-empty pattern
    if ((request->peripherals & 0x0E) != 0 &&
        request->pattern_length == 0)
    {
        return TEST_FAIL;
    }

    // Copy test pattern if present
    if (request->pattern_length > 0)
    {
        if (pbuf_copy_partial(p,
                              request->pattern,
                              request->pattern_length,
                              UDP_REQUEST_HEADER_SIZE)
            != request->pattern_length)
        {
            return TEST_FAIL;
        }
    }

    return TEST_PASS;
}


/**
 * @brief Executes a pending UDP hardware test request.
 *
 * @details
 * Checks whether a valid UDP test request is waiting
 * to be processed.
 *
 * If a request is pending, executes the selected hardware
 * tests using Hardware_RunTests().
 *
 * Builds a 5-byte UDP response containing:
 * - Bytes 0-3: Test ID in big-endian format.
 * - Byte 4: Test result (0x01 for PASS, 0xFF for FAIL).
 *
 * Allocates an LwIP packet buffer and attempts to send
 * the response to the original client's IP address and port.
 *
 * Releases the allocated transmission buffer and clears
 * the pending request flag after processing.
 *
 * If packet allocation or transmission fails, the pending
 * request is still cleared.
 *
 * @note Returns immediately if no request is pending.
 * @note Hardware tests are executed outside the UDP
 *       receive callback.
 */
void UDP_ProcessPendingTest(void)
{
    if (udp_request_pending == 0)
    {
        return;
    }

    // Run the requested hardware tests
    uint8_t result = Hardware_RunTests(
        udp_pending_request.peripherals,
        udp_pending_request.pattern,
        udp_pending_request.pattern_length,
        udp_pending_request.iterations
    );

    // Prepare response: 4-byte Test-ID + 1-byte result
    uint8_t response[UDP_RESPONSE_SIZE];

    response[0] = (uint8_t)(udp_pending_request.test_id >> 24);
    response[1] = (uint8_t)(udp_pending_request.test_id >> 16);
    response[2] = (uint8_t)(udp_pending_request.test_id >> 8);
    response[3] = (uint8_t)(udp_pending_request.test_id);
    response[4] = result;

    struct pbuf *tx = pbuf_alloc(PBUF_TRANSPORT,
                                UDP_RESPONSE_SIZE,
                                PBUF_RAM);

    if (tx != NULL)
    {
        if (pbuf_take(tx, response, UDP_RESPONSE_SIZE) == ERR_OK)
        {
            udp_sendto(udp_server_pcb,
                       tx,
                       &udp_pending_addr,
                       udp_pending_port);
        }

        pbuf_free(tx);
    }

    udp_request_pending = 0;
}


/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
/**
 * @brief Initializes the LwIP network stack and UDP server.
 *
 * Configures the static IPv4 address, subnet mask,
 * and default gateway for the Ethernet interface.
 *
 * Initializes the LwIP stack without an RTOS,
 * registers and activates the network interface,
 * and configures the Ethernet link status callback.
 *
 * Initializes the UDP server using UDP_Server_Init().
 * Calls Error_Handler() if UDP server initialization fails.
 *
 * @note This function is generated by STM32CubeMX.
 */
void MX_LWIP_Init(void)
{
  /* IP addresses initialization */
  IP_ADDRESS[0] = 10;
  IP_ADDRESS[1] = 0;
  IP_ADDRESS[2] = 0;
  IP_ADDRESS[3] = 200;
  NETMASK_ADDRESS[0] = 255;
  NETMASK_ADDRESS[1] = 255;
  NETMASK_ADDRESS[2] = 255;
  NETMASK_ADDRESS[3] = 0;
  GATEWAY_ADDRESS[0] = 10;
  GATEWAY_ADDRESS[1] = 0;
  GATEWAY_ADDRESS[2] = 0;
  GATEWAY_ADDRESS[3] = 138;

/* USER CODE BEGIN IP_ADDRESSES */
/* USER CODE END IP_ADDRESSES */

  /* Initialize the LwIP stack without RTOS */
  lwip_init();

  /* IP addresses initialization without DHCP (IPv4) */
  IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
  IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1] , NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
  IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);

  /* add the network interface (IPv4/IPv6) without RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  /* We must always bring the network interface up connection or not... */
  netif_set_up(&gnetif);

  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);

/* USER CODE BEGIN 3 */

  if (UDP_Server_Init() != ERR_OK)
  {
      Error_Handler();
  }

/* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

/**
  * @brief  Ethernet Link periodic check
  * @param  netif
  * @retval None
  */
static void Ethernet_Link_Periodic_Handle(struct netif *netif)
{
/* USER CODE BEGIN 4_4_1 */
/* USER CODE END 4_4_1 */

  /* Ethernet Link every 100ms */
  if (HAL_GetTick() - EthernetLinkTimer >= 100)
  {
    EthernetLinkTimer = HAL_GetTick();
    ethernet_link_check_state(netif);
  }
/* USER CODE BEGIN 4_4 */
/* USER CODE END 4_4 */
}

/**
 * ----------------------------------------------------------------------
 * Function given to help user to continue LwIP Initialization
 * Up to user to complete or change this function ...
 * Up to user to call this function in main.c in while (1) of main(void)
 *-----------------------------------------------------------------------
 * Read a received packet from the Ethernet buffers
 * Send it to the lwIP stack for handling
 * Handle timeouts if LWIP_TIMERS is set and without RTOS
 * Handle the llink status if LWIP_NETIF_LINK_CALLBACK is set and without RTOS
 */
void MX_LWIP_Process(void)
{
/* USER CODE BEGIN 4_1 */
/* USER CODE END 4_1 */
  ethernetif_input(&gnetif);

/* USER CODE BEGIN 4_2 */
/* USER CODE END 4_2 */
  /* Handle timeouts */
  sys_check_timeouts();

  Ethernet_Link_Periodic_Handle(&gnetif);

/* USER CODE BEGIN 4_3 */
/* USER CODE END 4_3 */
}

/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
static void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
/* USER CODE BEGIN 5 */
/* USER CODE END 5 */
  }
  else /* netif is down */
  {
/* USER CODE BEGIN 6 */
/* USER CODE END 6 */
  }
}

#if (defined ( __CC_ARM ) || defined (__ARMCC_VERSION))  /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

/* USER CODE BEGIN 7 */
  sd = 0; // dummy code
/* USER CODE END 7 */

  return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
/* USER CODE BEGIN 8 */
/* USER CODE END 8 */
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 9 */
  recved_bytes = 0; // dummy code
/* USER CODE END 9 */
  return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 10 */
  recved_bytes = 0; // dummy code
/* USER CODE END 10 */
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

