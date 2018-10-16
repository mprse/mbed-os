/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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
#include "drivers/SPI.h"
#include "platform/mbed_critical.h"

#if 0
#include "platform/mbed_power_mgmt.h"
#endif

#if DEVICE_SPI

namespace mbed {

#if 0 && TRANSACTION_QUEUE_SIZE_SPI
CircularBuffer<Transaction<SPI>, TRANSACTION_QUEUE_SIZE_SPI> SPI::_transaction_buffer;
#endif
SPI::spi_peripheral_s SPI::_peripherals[SPI_COUNT];

SPI::SPI(PinName mosi, PinName miso, PinName sclk, PinName ssel) :
    _self(),
#if 0
    _usage(DMA_USAGE_NEVER),
    _deep_sleep_locked(false),
#endif
    _bits(8),
    _mode(SPI_MODE_IDLE_LOW_SAMPLE_FIRST_EDGE),
    _msb_first(true),
    _hz(1000000),
    _write_fill(0xFF)
{
    // No lock needed in the constructor
    // get_module

    SPIName name = spi_get_module(mosi, miso, sclk);

    // this is a lazy allocation for the mutex.
    // unless not assigned to a peripheral, this mutex will never be deleted.
    PlatformMutex *mutex = new PlatformMutex();

    core_util_critical_section_enter();
    // lookup in a critical section if we already have it else initialize it
    _self = SPI::lookup(name, true);
    if (_self->mutex == NULL) {
        _self->mutex = mutex;
    }
    if (_self->name == 0) {
        _self->name = name;
        spi_init(&_self->spi, false, mosi, miso, sclk, NC);
    }
    core_util_critical_section_exit();

    if (_self->mutex != mutex) {
        delete mutex;
    }

    // we don't need to _acquire at this stage.
    // this will be done anyway before any operation.
}

struct SPI::spi_peripheral_s *SPI::lookup(SPIName name, bool or_last) {
    struct SPI::spi_peripheral_s *result = NULL;
    core_util_critical_section_enter();
    for (uint32_t idx = 0; idx < SPI_COUNT; idx++) {
        printf("SPI::lookup(%08x) found at %lu", name, idx);
        if ((_peripherals[idx].name == name) ||
            (_peripherals[idx].name == 0)) {
            printf("SPI::lookup(%08x) found at %lu", name, idx);
            result = &_peripherals[idx];
            break;
        }
        // XXX: we may want to ensure that it was previously initialized with the same mosi/miso/sclk/ss pins
    }
    if (!or_last && (result != NULL) && (result->name == 0)) {
        result = NULL;
    }

    core_util_critical_section_exit();
    return result;
}

void SPI::format(int bits, int mode)
{
    format(bits, (spi_mode_t)mode, true);
}

void SPI::format(uint8_t bits, spi_mode_t mode, bool msb_first)
{
    lock();
    _bits = bits;
    _mode = mode;
    _msb_first = msb_first;
    // If changing format while you are the owner then just
    // update format, but if owner is changed then even frequency should be
    // updated which is done by acquire.
    if (_self->owner == this) {
        spi_format(&_self->spi, _bits, _mode, _msb_first?SPI_BIT_ORDERING_MSB_FIRST:SPI_BIT_ORDERING_LSB_FIRST);
    } else {
        _acquire();
    }
    unlock();
}

uint32_t SPI::frequency(uint32_t hz)
{
    uint32_t actual_hz;
    lock();
    _hz = hz;
    // If changing format while you are the owner then just
    // update frequency, but if owner is changed then even frequency should be
    // updated which is done by acquire.
    if (_self->owner == this) {
        actual_hz = spi_frequency(&_self->spi, _hz);
    } else {
        actual_hz = _acquire();
    }
    unlock();
    return actual_hz;
}


void SPI::acquire()
{
    lock();
    _acquire();
    unlock();
}

// Note: Private function with no locking
uint32_t SPI::_acquire()
{
    uint32_t actual_hz = 0;
    if (_self->owner != this) {
        spi_format(&_self->spi, _bits, _mode, _msb_first?SPI_BIT_ORDERING_MSB_FIRST:SPI_BIT_ORDERING_LSB_FIRST);
        actual_hz = spi_frequency(&_self->spi, _hz);
        _self->owner = this;
    }
    return actual_hz;
}

int SPI::write(int value)
{
    lock();
    _acquire();
    uint32_t ret = 0;
    spi_transfer(&_self->spi, &value, _bits/8, &ret, _bits/8, NULL);
    unlock();
    return ret;
}

int SPI::write(const char *tx_buffer, int tx_length, char *rx_buffer, int rx_length)
{
    lock();
    _acquire();
    int ret = spi_transfer(&_self->spi, tx_buffer, tx_length, rx_buffer, rx_length, &_write_fill);
    unlock();
    return ret;
}

void SPI::lock()
{
    _self->mutex->lock();
}

void SPI::unlock()
{
    _self->mutex->unlock();
}

void SPI::set_default_write_value(char data)
{
    lock();
    _write_fill = (uint32_t)data;
    unlock();
}

#if 0

int SPI::transfer(const void *tx_buffer, int tx_length, void *rx_buffer, int rx_length, unsigned char bit_width, const event_callback_t &callback, int event)
{
    if (spi_active(&_spi)) {
        return queue_transfer(tx_buffer, tx_length, rx_buffer, rx_length, bit_width, callback, event);
    }
    start_transfer(tx_buffer, tx_length, rx_buffer, rx_length, bit_width, callback, event);
    return 0;
}

void SPI::abort_transfer()
{
    spi_abort_asynch(&_spi);
    unlock_deep_sleep();
#if TRANSACTION_QUEUE_SIZE_SPI
    dequeue_transaction();
#endif
}


void SPI::clear_transfer_buffer()
{
#if TRANSACTION_QUEUE_SIZE_SPI
    _transaction_buffer.reset();
#endif
}

void SPI::abort_all_transfers()
{
    clear_transfer_buffer();
    abort_transfer();
}

int SPI::set_dma_usage(DMAUsage usage)
{
    if (spi_active(&_spi)) {
        return -1;
    }
    _usage = usage;
    return  0;
}

int SPI::queue_transfer(const void *tx_buffer, int tx_length, void *rx_buffer, int rx_length, unsigned char bit_width, const event_callback_t &callback, int event)
{
#if TRANSACTION_QUEUE_SIZE_SPI
    transaction_t t;

    t.tx_buffer = const_cast<void *>(tx_buffer);
    t.tx_length = tx_length;
    t.rx_buffer = rx_buffer;
    t.rx_length = rx_length;
    t.event = event;
    t.callback = callback;
    t.width = bit_width;
    Transaction<SPI> transaction(this, t);
    if (_transaction_buffer.full()) {
        return -1; // the buffer is full
    } else {
        core_util_critical_section_enter();
        _transaction_buffer.push(transaction);
        if (!spi_active(&_spi)) {
            dequeue_transaction();
        }
        core_util_critical_section_exit();
        return 0;
    }
#else
    return -1;
#endif
}

void SPI::start_transfer(const void *tx_buffer, int tx_length, void *rx_buffer, int rx_length, unsigned char bit_width, const event_callback_t &callback, int event)
{
    lock_deep_sleep();
    _acquire();
    _callback = callback;
    _irq.callback(&SPI::irq_handler_asynch);
    spi_master_transfer(&_spi, tx_buffer, tx_length, rx_buffer, rx_length, bit_width, _irq.entry(), event, _usage);
}

void SPI::lock_deep_sleep()
{
    if (_deep_sleep_locked == false) {
        sleep_manager_lock_deep_sleep();
        _deep_sleep_locked = true;
    }
}

void SPI::unlock_deep_sleep()
{
    if (_deep_sleep_locked == true) {
        sleep_manager_unlock_deep_sleep();
        _deep_sleep_locked = false;
    }
}

#if TRANSACTION_QUEUE_SIZE_SPI

void SPI::start_transaction(transaction_t *data)
{
    start_transfer(data->tx_buffer, data->tx_length, data->rx_buffer, data->rx_length, data->width, data->callback, data->event);
}

void SPI::dequeue_transaction()
{
    Transaction<SPI> t;
    if (_transaction_buffer.pop(t)) {
        SPI *obj = t.get_object();
        transaction_t *data = t.get_transaction();
        obj->start_transaction(data);
    }
}

#endif

void SPI::irq_handler_asynch(void)
{
    int event = spi_irq_handler_asynch(&_spi);
    if (_callback && (event & SPI_EVENT_ALL)) {
        unlock_deep_sleep();
        _callback.call(event & SPI_EVENT_ALL);
    }
#if TRANSACTION_QUEUE_SIZE_SPI
    if (event & (SPI_EVENT_ALL | SPI_EVENT_INTERNAL_TRANSFER_COMPLETE)) {
        // SPI peripheral is free (event happened), dequeue transaction
        dequeue_transaction();
    }
#endif
}

#endif

} // namespace mbed

#endif
