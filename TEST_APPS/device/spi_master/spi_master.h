#include <Thread.h>

#define SPI_MASTER_MOSI      PTD13
#define SPI_MASTER_MISO      PTB23
#define SPI_MASTER_SS        PTE25
#define SPI_MASTER_CLK       PTD12

#define FREQ_200KHZ (200000)
#define FREQ_1MHZ   (1000000)
#define FREQ_2MHZ   (2000000)
#define FREQ_MIN    (0)
#define FREQ_MAX    (0xFFFFFFFF)

#define TEST_SYM_CNT 5

#define TRANSMISSION_DELAY_MS 1000
#define TRANSMISSION_BUTTON SW3

#define CMDLINE_RETCODE_TEST_NOT_SUPPORTED      -100
#define CMDLINE_RETCODE_TEST_FAILED             -101

#define DEBUG 1

unsigned int counter = 0;

DigitalOut led(LED1);

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

template<typename T>
static void dump_buffers(T *tx_pattern, T *rx1_pattern, T *rx2_pattern, T *tx_buff, T *rx_buff)
{
#if DEBUG
    printf("Dump buffers: \n");
    printf("tx_pattern : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", tx_pattern[0], tx_pattern[1], tx_pattern[2], tx_pattern[3], tx_pattern[4]);
    printf("rx1_pattern: 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", rx1_pattern[0], rx1_pattern[1], rx1_pattern[2], rx1_pattern[3], rx1_pattern[4]);
    printf("rx2_pattern: 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", rx2_pattern[0], rx2_pattern[1], rx2_pattern[2], rx2_pattern[3], rx2_pattern[4]);
    printf("tx_buff    : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", tx_buff[0], tx_buff[1], tx_buff[2], tx_buff[3], tx_buff[4]);
    printf("rx_buff    : 0x%X 0x%X 0x%X 0x%X 0x%X \r\n", rx_buff[0], rx_buff[1], rx_buff[2], rx_buff[3], rx_buff[4]);
#endif
}

/* Debug function to print received configuration details. */
void dump_config(config_test_case_t *config)
{
#if 0
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
    printf("auto ss: %lu\r\n", (uint32_t) config->auto_ss);
    printf("full duplex: %lu\r\n", (uint32_t) config->duplex);
    printf("---\r\n");
#endif
}

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
        //TEST_ASSERT_EQUAL(true, ret);
        /* Wait here for the end of transmission. Callback will set context to the number of
         * transfered symbols. */
        while(!context);

        count = context;
    }
#endif

    return count;
}

/* Function waits before the transmission is triggered on the master side. */
static void wait_before_transmission()
{
    if (TRANSMISSION_DELAY_MS) {
        wait_ms(TRANSMISSION_DELAY_MS);
    } else {
        DigitalIn button(TRANSMISSION_BUTTON);

        while (button.read() == 1);
        while (button.read() == 0);
    }
}

/* Function compares given buffers and returns true when equal, false otherwise.
 * In case when buffer is undefined (NULL) function returns true. */
static bool check_buffers(void *p_pattern, void *p_buffer, uint32_t size)
{
    const char * p_byte_pattern = (const char*) p_pattern;
    const char * p_byte_buffer = (const char*) p_buffer;

    if (p_buffer == NULL || p_pattern == NULL) {
        return true;
    }

    while (size) {
        if (*p_byte_pattern != *p_byte_buffer) {
            return false;
        }
        p_byte_pattern++;
        p_byte_buffer++;
        size--;
    }

    return true;
}

/* Function initialises RX, TX buffers before transmission. */
template<typename T>
static void init_transmission_buffers(config_test_case_t *config, T *p_tx_pattern, T *p_rx1_pattern, T *p_rx2_pattern, T *p_tx_buff, T *p_rx_buff, T *p_fill_symbol)
{
    /* Default patterns for TX/RX buffers. */
    set_buffer(&p_tx_pattern[0], sizeof(T), 0xAA);
    set_buffer(&p_tx_pattern[1], sizeof(T), 0xBB);
    set_buffer(&p_tx_pattern[2], sizeof(T), 0xCC);
    set_buffer(&p_tx_pattern[3], sizeof(T), 0xDD);
    set_buffer(&p_tx_pattern[4], sizeof(T), 0xEE);

    set_buffer(&p_rx1_pattern[0], sizeof(T), 0x11);
    set_buffer(&p_rx1_pattern[1], sizeof(T), 0x22);
    set_buffer(&p_rx1_pattern[2], sizeof(T), 0x33);
    set_buffer(&p_rx1_pattern[3], sizeof(T), 0x44);
    set_buffer(&p_rx1_pattern[4], sizeof(T), 0x55);

    set_buffer(&p_rx2_pattern[0], sizeof(T), 0xAA);
    set_buffer(&p_rx2_pattern[1], sizeof(T), 0xBB);
    set_buffer(&p_rx2_pattern[2], sizeof(T), 0xCC);
    set_buffer(&p_rx2_pattern[3], sizeof(T), 0xDD);
    set_buffer(&p_rx2_pattern[4], sizeof(T), 0xEE);

    set_buffer(p_fill_symbol, sizeof(T), 0xAB);

    /* Exception: master TX > master RX . */
    if (config->master_tx_cnt > config->master_rx_cnt) {
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x00);
    }
    /* Exception: master TX < master RX . */
    if (config->master_tx_cnt < config->master_rx_cnt) {
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: slave TX > slave RX . */
    if (config->slave_tx_cnt > config->slave_rx_cnt) {
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x00);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x00);
    }
    /* Exception: slave TX < slave RX . */
    if (config->slave_tx_cnt < config->slave_rx_cnt) {
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: master TX buffer undefined . */
    if (!config->master_tx_defined) {
        set_buffer(&p_rx2_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: master RX buffer undefined . */
    if (!config->master_rx_defined) {
        set_buffer(&p_rx1_pattern[0], sizeof(T), 0xAA);
        set_buffer(&p_rx1_pattern[1], sizeof(T), 0xBB);
        set_buffer(&p_rx1_pattern[2], sizeof(T), 0xCC);
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xDD);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xEE);
    }

    /* Exception: slave TX buffer undefined . */
    if (!config->slave_tx_defined) {
        set_buffer(&p_rx1_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx1_pattern[4], sizeof(T), 0xFF);

        set_buffer(&p_rx2_pattern[0], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0xFF);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0xFF);
    }

    /* Exception: slave RX buffer undefined . */
    if (!config->slave_rx_defined) {
        set_buffer(&p_rx2_pattern[0], sizeof(T), 0x11);
        set_buffer(&p_rx2_pattern[1], sizeof(T), 0x22);
        set_buffer(&p_rx2_pattern[2], sizeof(T), 0x33);
        set_buffer(&p_rx2_pattern[3], sizeof(T), 0x44);
        set_buffer(&p_rx2_pattern[4], sizeof(T), 0x55);
    }

    /* Handle symbol size. */
    T sym_mask = ((1 << config->symbol_size) - 1);

    for (uint32_t i = 0; i < TEST_SYM_CNT; i++) {
        p_tx_pattern[i] = (p_tx_pattern[i] & sym_mask);
        p_rx1_pattern[i] = (p_rx1_pattern[i] & sym_mask);
        p_rx2_pattern[i] = (p_rx2_pattern[i] & sym_mask);
    }

    memcpy(p_tx_buff, p_tx_pattern, sizeof(T) * TEST_SYM_CNT);
    set_buffer(p_rx_buff, sizeof(T) * TEST_SYM_CNT, 0x00);
}

/* Function handles <ss> line if <ss> is specified.
 * When manual <ss> handling is selected <ss> is defined.
 */
static void handle_ss(DigitalOut * ss, bool select)
{
    if (ss) {
        if (select) {
            *ss = 0;
        } else {
            *ss = 1;
        }
    }
}

/* Function which perform transfer using specified config on the master side. */
template<typename T>
int transfer_master(spi_t *obj, config_test_case_t *config, DigitalOut *ss)
{
    uint32_t count;
    int status = CMDLINE_RETCODE_SUCCESS;
    uint32_t clocked_symbols_1 = TEST_SYM_CNT;
    uint32_t clocked_symbols_2 = TEST_SYM_CNT;

    if (config->duplex != FULL_DUPLEX) {
        clocked_symbols_1 = (config->master_tx_cnt + config->master_rx_cnt);
        clocked_symbols_2 = (TEST_SYM_CNT + TEST_SYM_CNT);
    }

    T tx_pattern[TEST_SYM_CNT];
    T rx1_pattern[TEST_SYM_CNT];
    T rx2_pattern[TEST_SYM_CNT];
    T tx_buff[TEST_SYM_CNT];
    T rx_buff[TEST_SYM_CNT];
    T fill_symbol;

    void *p_tx_buff = tx_buff;
    void *p_rx_buff = rx_buff;

    if (!config->master_tx_defined) {
        p_tx_buff = NULL;
    }

    if (!config->master_rx_defined) {
        p_rx_buff = NULL;
    }

    init_transmission_buffers<T>(config, &tx_pattern[0], &rx1_pattern[0], &rx2_pattern[0], &tx_buff[0], &rx_buff[0], &fill_symbol);

    //dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);

    wait_before_transmission();
    led = 1;
    handle_ss(ss, true);
    count = sync_async_transfer(obj, p_tx_buff, config->master_tx_cnt, p_rx_buff, config->master_rx_cnt, (void*) &fill_symbol, config->sync);
    handle_ss(ss, false);

    if (!check_buffers(rx1_pattern, p_rx_buff, sizeof(T) * TEST_SYM_CNT) ||
        !check_buffers(tx_pattern, p_tx_buff, sizeof(T) * TEST_SYM_CNT) ||
        clocked_symbols_1 != count) {
        status = CMDLINE_RETCODE_TEST_FAILED;
        dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);
    }

    /* Init TX buffer with data received from slave if possible. */

    if (p_tx_buff && p_rx_buff) {
        memcpy(p_tx_buff, p_rx_buff, sizeof(tx_buff));
        memcpy(tx_pattern, p_rx_buff, sizeof(tx_buff));
    }

    set_buffer(rx_buff, sizeof(rx_buff), 0x00);

    wait_before_transmission();
    handle_ss(ss, true);
    count = sync_async_transfer(obj, tx_buff, TEST_SYM_CNT, rx_buff, TEST_SYM_CNT, (void*) &fill_symbol, config->sync);
    handle_ss(ss, false);

    if (!check_buffers(rx2_pattern, p_rx_buff, sizeof(T) * TEST_SYM_CNT) ||
        !check_buffers(tx_pattern, p_tx_buff, sizeof(T) * TEST_SYM_CNT) ||
        clocked_symbols_2 != count) {
        status = CMDLINE_RETCODE_TEST_FAILED;
        dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);
    }

    //dump_buffers<T>(tx_pattern, rx1_pattern, rx2_pattern, tx_buff, rx_buff);

    return status;
}

int test_init_master(spi_t * obj, config_test_case_t *config, DigitalOut ** ss)
{
    spi_capabilities_t capabilities = { 0 };
    PinName ss_pin = SPI_MASTER_SS;
    PinName miso = SPI_MASTER_MISO;
    PinName mosi = SPI_MASTER_MOSI;



    spi_get_capabilities(spi_get_module(SPI_MASTER_MOSI, SPI_MASTER_MISO, SPI_MASTER_CLK), NC, &capabilities);

    /* Adapt Full Duplex/Half Duplex settings. */
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

    /* Adapt min/max frequency for testing based of capabilities. */
    switch (config->freq_hz)
    {
        case FREQ_MIN:
            config->freq_hz = capabilities.minimum_frequency;
            break;

        case FREQ_MAX:
            config->freq_hz = capabilities.maximum_frequency;
            break;

        default:
            break;
    }

    /* Adapt manual/auto SS handling by master. */
    if (!config->auto_ss) {
        ss_pin = NC;
        *ss = new DigitalOut(SPI_MASTER_SS);
        **ss = 1;
    }

    led = 0;

    spi_init(obj, false, mosi, miso, SPI_MASTER_CLK, ss_pin);

    spi_format(obj, config->symbol_size, config->mode, config->bit_ordering);

    spi_frequency(obj, config->freq_hz);

    return CMDLINE_RETCODE_SUCCESS;
}

int test_transfer_master(spi_t * obj, config_test_case_t *config, DigitalOut * ss)
{
    int status = CMDLINE_RETCODE_SUCCESS;

    if (config->symbol_size <= 8) {
        status = transfer_master<uint8_t>(obj, config, ss);
    } else if (config->symbol_size <= 16) {
        status = transfer_master<uint16_t>(obj,config, ss);
    } else {
        status = transfer_master<uint32_t>(obj,config, ss);
    }

    return status;
}

int test_finish_master(spi_t * obj, config_test_case_t *config)
{
    spi_free(obj);

    return CMDLINE_RETCODE_SUCCESS;
}

