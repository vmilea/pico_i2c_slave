/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Wire.h"
#include <i2c_fifo.h>

TwoWire Wire;
TwoWire Wire1;

//
// TwoWire
//

TwoWire::TwoWire() = default;

i2c_inst_t *TwoWire::i2c() const {
    assert(this == &Wire || this == &Wire1);

    return this == &Wire ? i2c0 : i2c1;
}

void TwoWire::begin() {
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission

    if (mode_ != Unassigned) {
        i2c_slave_deinit(i2c());
    }
    mode_ = Master;
    bufLen_ = 0;
    bufPos_ = 0;
}

void TwoWire::begin(uint8_t selfAddress) {
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission

    if (mode_ != Unassigned) {
        i2c_slave_deinit(i2c());
    }
    mode_ = Slave;
    bufLen_ = 0;
    bufPos_ = 0;
    i2c_slave_init(i2c(), selfAddress, &handleEvent);
}

void TwoWire::beginTransmission(uint8_t address) {
    assert(mode_ == Master); // not allowed for slave
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission
    assert(address != NO_ADDRESS);

    txAddress_ = address;
    bufLen_ = 0;
    bufPos_ = 0;
}

uint8_t TwoWire::endTransmission(bool sendStop) {
    assert(mode_ == Master); // not allowed for slave
    assert(txAddress_ != NO_ADDRESS); // must follow beginTransmission()
    assert(bufPos_ == 0);

    int result = i2c_write_blocking(i2c(), txAddress_, buf_, bufLen_, !sendStop);
    txAddress_ = NO_ADDRESS;
    bufLen_ = 0;
    if (result < 0) {
        return 4; // other error
    } else if (result < bufLen_) {
        return 3; // data transfer interrupted after NACK
    } else {
        return 0; // success;
    }
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t count, bool sendStop) {
    assert(mode_ == Master); // not allowed for slave
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission

    count = MIN(count, WIRE_BUFFER_LENGTH);
    int result = i2c_read_blocking(i2c(), address, buf_, count, !sendStop);
    if (result < 0) {
        result = 0;
    }
    bufLen_ = (uint8_t)result;
    bufPos_ = 0;
    return (uint8_t)result;
}

size_t TwoWire::write(uint8_t value) {
    assert(mode_ != Unassigned); // begin not called

    if (mode_ == Master) {
        assert(txAddress_ != NO_ADDRESS); // allowed between begin...end transmission
        if (bufLen_ == WIRE_BUFFER_LENGTH) {
            return 0; // buffer is full
        }
        buf_[bufLen_++] = value;
    } else {
        auto i2c = this->i2c();
        auto hw = i2c_get_hw(i2c);
        while ((hw->status & I2C_IC_STATUS_TFNF_BITS) == 0) {
            tight_loop_contents(); // wait until Tx FIFO is no longer full
        }
        i2c_write_byte(i2c, value);
    }
    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t size) {
    assert(mode_ != Unassigned); // missing begin

    if (mode_ == Master) {
        assert(txAddress_ != NO_ADDRESS); // allowed between begin...end transmission
        size = MIN(size, WIRE_BUFFER_LENGTH - (size_t)bufLen_);
        for (size_t i = 0; i < size; i++) {
            buf_[bufLen_++] = data[i];
        }
    } else {
        auto i2c = this->i2c();
        auto hw = i2c_get_hw(i2c);
        for (size_t i = 0; i < size; i++) {
            while ((hw->status & I2C_IC_STATUS_TFNF_BITS) == 0) {
                tight_loop_contents(); // wait until Tx FIFO is no longer full
            }
            i2c_write_byte(i2c, data[i]);
        }
    }
    return size;
}

void TwoWire::onReceive(WireReceiveHandler handler) {
    receiveHandler_ = handler;
}

void TwoWire::onRequest(WireRequestHandler handler) {
    requestHandler_ = handler;
}

//
// private members
//

void __not_in_flash_func(TwoWire::handleEvent)(i2c_inst_t *i2c, i2c_slave_event_t event) {
    auto &wire = (i2c_hw_index(i2c) == 0 ? Wire : Wire1);
    assert(wire.mode_ == TwoWire::Slave);
    assert(wire.bufPos_ == 0);

    switch (event) {
    case I2C_SLAVE_RECEIVE:
        for (size_t n = i2c_get_read_available(i2c); n > 0; n--) {
            uint8_t value = i2c_read_byte(i2c);
            if (wire.bufLen_ < WIRE_BUFFER_LENGTH) {
                wire.buf_[wire.bufLen_++] = value;
            } else {
                // we can't respond with NACK when the buffer is full on DW_apb_i2c,
                // so the excess data is simply discarded
            }
        }
        break;
    case I2C_SLAVE_REQUEST:
        assert(wire.bufLen_ == 0);
        assert(wire.bufPos_ == 0);
        if (wire.requestHandler_ != nullptr) {
            wire.requestHandler_();
        }
        break;
    case I2C_SLAVE_FINISH:
        if (0 < wire.bufLen_) {
            if (wire.receiveHandler_ != nullptr) {
                wire.receiveHandler_(wire.bufLen_);
            }
            wire.bufLen_ = 0;
            wire.bufPos_ = 0;
        }
        assert(wire.bufPos_ == 0);
        break;
    default:
        break;
    }
}
