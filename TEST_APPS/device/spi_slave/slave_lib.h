#define SPI_SLAVE_MOSI      PTD3
#define SPI_SLAVE_MISO      PTD2
#define SPI_SLAVE_SS        PTD0
#define SPI_SLAVE_CLK       PTD1

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)
#define FREQ_MIN    (0)
#define FREQ_MAX    (0xFFFFFFFF)

#define TEST_SYM_CNT 5

#define DEBUG 0

#define CMDLINE_RETCODE_TEST_NOT_SUPPORTED      -100
#define CMDLINE_RETCODE_TEST_FAILED             -101

typedef enum
{
    FULL_DUPLEX, HALF_DUPLEX_MOSI, HALF_DUPLEX_MISO
} duplex_t;

/* SPI test configuration. */
typedef struct
{
    uint8_t symbol_size;
    spi_mode_t mode;
    spi_bit_ordering_t bit_ordering;
    uint32_t freq_hz;
    uint32_t master_tx_cnt;
    uint32_t master_rx_cnt;
    uint32_t slave_tx_cnt;
    uint32_t slave_rx_cnt;
    bool master_tx_defined;
    bool master_rx_defined;
    bool slave_tx_defined;
    bool slave_rx_defined;
    bool auto_ss;
    duplex_t duplex;
    bool sync;
} config_test_case_t;

/* Function inits specified buffer with given pattern. */
static void set_buffer(void * addr, uint32_t size, char val)
{
    if (addr == NULL) {
        return;
    }

    char *p_char = (char*) addr;

    for (uint32_t i = 0; i < size; i++) {
        p_char[i] = val;
    }
}

#ifdef DEVICE_SPI_ASYNCH
/* Callback function for SPI async transfers. */
static uint32_t context;
void spi_async_callback(spi_t *obj, void *ctx, spi_async_event_t *event) {
    *((uint32_t*)ctx) = event->transfered;
}
#endif

/* Function returns true if configuration is consistent with the capabilities of
 * the SPI peripheral, false otherwise. */
static int check_capabilities(spi_capabilities_t *p_cabs, uint32_t symbol_size, bool slave, bool half_duplex)
{
    if (!(p_cabs->word_length & (1 << (symbol_size - 1))) ||
        (slave && !p_cabs->support_slave_mode) ||
        (half_duplex && !p_cabs->half_duplex)) {
        return CMDLINE_RETCODE_TEST_NOT_SUPPORTED;
    }

    return CMDLINE_RETCODE_SUCCESS;
}

/* Function used to perform transmission using sync or async modes. */
static uint32_t sync_async_transfer(spi_t *obj, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len, const void *fill_symbol, bool sync_mode)
{
    uint32_t count = 0;

    if (sync_mode) {
        count = spi_transfer(obj, tx, tx_len, rx, rx_len, fill_symbol);
    }
#ifdef DEVICE_SPI_ASYNCH
    else {
        context = 0;
        bool ret = spi_transfer_async(obj, tx, tx_len, rx, rx_len, fill_symbol, spi_async_callback, &context, DMA_USAGE_NEVER);
        /* Wait here for the end of transmission. Callback will set context to the number of
         * transfered symbols. */
        while(!context);

        count = context;
    }
#endif

    return count;
}

template<typename T>
int slave_transfer(spi_t *obj, config_test_case_t *config)
{
    int status = CMDLINE_RETCODE_SUCCESS;
    uint32_t count;
    T tx_buff[TEST_SYM_CNT];
    T rx_buff[TEST_SYM_CNT];
    T fill_symbol;
    uint32_t clocked_symbols_1 = TEST_SYM_CNT;
    uint32_t clocked_symbols_2 = TEST_SYM_CNT;

    if (config->duplex != FULL_DUPLEX) {
        clocked_symbols_1 = (config->slave_tx_cnt + config->slave_rx_cnt);
        clocked_symbols_2 = (TEST_SYM_CNT + TEST_SYM_CNT);
    }

    set_buffer(&tx_buff[0], sizeof(T), 0x11);
    set_buffer(&tx_buff[1], sizeof(T), 0x22);
    set_buffer(&tx_buff[2], sizeof(T), 0x33);
    set_buffer(&tx_buff[3], sizeof(T), 0x44);
    set_buffer(&tx_buff[4], sizeof(T), 0x55);

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    set_buffer(&fill_symbol, sizeof(fill_symbol), 0xFF);

    void *p_tx_buff = tx_buff;
    void *p_rx_buff = rx_buff;

    if (!config->slave_tx_defined) {
        p_tx_buff = NULL;
    }

    if (!config->slave_rx_defined) {
        p_rx_buff = NULL;
    }

    count = sync_async_transfer(obj, p_tx_buff, config->slave_tx_cnt, p_rx_buff, config->slave_rx_cnt, &fill_symbol, config->sync);

    if (clocked_symbols_1 != count) {
        status = CMDLINE_RETCODE_TEST_FAILED;
    }

    /* Send data received from master in the previous transmission if possible. */
    if (p_tx_buff && p_rx_buff) {
        memcpy(p_tx_buff, p_rx_buff, sizeof(tx_buff));
    }

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    count = sync_async_transfer(obj, p_tx_buff, TEST_SYM_CNT, p_rx_buff, TEST_SYM_CNT, &fill_symbol, config->sync);

    if (clocked_symbols_2 != count) {
        status = CMDLINE_RETCODE_TEST_FAILED;
    }

    return status;
}

/* Debug function to print received configuration details. */
void dump_config(config_test_case_t *config)
{
#if DEBUG
    Timer tim;
    tim.reset();
    tim.start();
    printf("TEST CASE CONFIGURATION\r\n");
    printf("symbol_size: %lu\r\n", (uint32_t) config->symbol_size);
    printf("spi_mode: %lu\r\n", (uint32_t) config->mode);
    printf("bit_ordering: %lu\r\n", (uint32_t) config->bit_ordering);
    printf("freq: %lu\r\n", (uint32_t) config->freq_hz);
    printf("master tx cnt: %lu\r\n", (uint32_t) config->master_tx_cnt);
    printf("master rx cnt: %lu\r\n", (uint32_t) config->master_rx_cnt);
    printf("slave tx cnt: %lu\r\n", (uint32_t) config->slave_tx_cnt);
    printf("slave rx cnt: %lu\r\n", (uint32_t) config->slave_rx_cnt);
    printf("master tx defined: %lu\r\n", (uint32_t) config->master_tx_defined);
    printf("master rx defined: %lu\r\n", (uint32_t) config->master_rx_defined);
    printf("slave tx defined: %lu\r\n", (uint32_t) config->slave_tx_defined);
    printf("slave rx defined: %lu\r\n", (uint32_t) config->slave_rx_defined);
    printf("auto ss: %lu\r\n", (uint32_t) config->slave_rx_defined);
    printf("full duplex: %lu\r\n", (uint32_t) config->duplex);
    printf("log time: %lu [us]\r\n", (uint32_t) tim.read_us());
    printf("---\r\n");
    tim.stop();
#endif
}

int test_transfer_slave(config_test_case_t *config)
{
    int status = CMDLINE_RETCODE_SUCCESS;
    spi_t spi_slave = { 0 };
    uint8_t fill_symbol = 0xFF;
    spi_capabilities_t capabilities = { 0 };

    spi_get_capabilities(spi_get_module(SPI_SLAVE_MOSI,
                                        SPI_SLAVE_MISO,
                                        SPI_SLAVE_CLK),
                         SPI_SLAVE_SS,
                         &capabilities);

    PinName miso = SPI_SLAVE_MISO;
    PinName mosi = SPI_SLAVE_MOSI;

    /* Adapt Full duplex/Half duplex settings. */
    switch (config->duplex)
    {
        case HALF_DUPLEX_MOSI:
            miso = NC;
            break;

        case HALF_DUPLEX_MISO:
            mosi = NC;
            break;

        default:

            break;
    }

    spi_init(&spi_slave, true, mosi, miso, SPI_SLAVE_CLK, SPI_SLAVE_SS);

    spi_format(&spi_slave, config->symbol_size, config->mode, config->bit_ordering);

    if (config->symbol_size <= 8) {
        status = slave_transfer<uint8_t>(&spi_slave, config);
    } else if (config->symbol_size <= 16) {
        status = slave_transfer<uint16_t>(&spi_slave, config);
    } else {
        status = slave_transfer<uint16_t>(&spi_slave, config);
    }

    spi_free(&spi_slave);

    return status;
}
