/* mbed Microcontroller Library
 * Copyright (c) 2016 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "mbed.h"
#include "math.h"
#include "sai_api.h"

using namespace utest::v1;

#define PI 3.14159265
#define SAMPLE_RATE (16000)
#define PITCH       (440)
#define AMP         (0.25)
#define COUNT (SAMPLE_RATE / PITCH)

static const sai_format_t k64f_pcm_mode = {
    .sample_rate = 16000,
    .bclk_polarity = true,
    .wclk_polarity = true,
    .ws_delay = false,
    .ws_length = 32,
    .frame_length = 1,
    .word_mask = 0,
    .word_length = 32,
    .data_length = 16,
    .lsb_first = false,
    .aligned_left = true,
    .bit_shift = 0
};


DigitalOut led1(LED2);
DigitalIn stop_button(SW3);
//DA7212     codec(PTC11, PTC10, 0x1A);

unsigned char divider;

uint32_t chan0[100];
uint32_t chan1[100];

void i2s_test(void)
{
    sai_init_t sai_init_data = {};
    sai_init_data.sync = sai_synchronicity_synced_to_tx;
    sai_init_data.mclk = SAI_MCLK;
    sai_init_data.mclk_internal_src = true;
    sai_init_data.mclk_freq = 120000000;

    sai_init_data.RX.enable = true;
    sai_init_data.RX.sd = SAI_RX_SD;
    sai_init_data.RX.bclk = SAI_RX_BCLK;
    sai_init_data.RX.wclk = SAI_RX_WCLK;
    sai_init_data.RX.format = k64f_pcm_mode;
    sai_init_data.RX.is_slave = true;

    sai_init_data.TX.enable = false;

    SAI sai(&sai_init_data);

    uint32_t i = 0;

    int16_t buffer[COUNT] = {0};
    printf("Count %u\r\n", COUNT);
    for (uint32_t i = 0; i < COUNT; i++) {
        double sample = sin((double)i * 2. * PI/(double)COUNT) * AMP;
        buffer[i] = sample * INT16_MAX;
        //printf("%2lu %6hd %9.6lf\r\n", i, buffer[i], sample);
    }

    printf("Starting loop...\r\n");




    int x = 0;

    while (true) {
        chan0[x] = sai.xfer(0, 0);
        chan1[x] = sai.xfer(1, 0);

        x++;
        if(x == 100) {
            x = 0;
            break;
        }

        if (stop_button) {
            break;
        }
    }

    for (int i=0; i<100; i++) {
        printf("chan1: %u \n", chan0[i]);
        printf("chan0: %u \n", chan1[i]);
    }
}


utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) {
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

Case cases[] = {
    Case("500us lp_ticker", i2s_test, greentea_failure_handler),

};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(20, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main() {
    Harness::run(specification);
}
