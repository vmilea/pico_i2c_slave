/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _WIRE_H_
#define _WIRE_H_

#include <i2c_slave.h>

/** \file Wire.h
 *
 * \brief Wire API wrapper for I2C.
 */

#ifndef WIRE_BUFFER_LENGTH
#define WIRE_BUFFER_LENGTH 32
#endif

/**
 * \brief Called in slave mode after receiving data from master.
 * 
 * The received data is buffered internally, and the handler is called once the transfer has
 * completed (after the master sends a Stop or Start signal).
 * 
 * The maximum transfer size is determined by WIRE_BUFFER_LENGTH. Because of how the I2C
 * hardware operates on RP2040, there is no way to NACK once the buffer is full, so excess
 * data is simply discarded.
 * 
 * \param count The number of bytes available for reading.
 */
using WireReceiveHandler = void (*)(int count);

/**
 * \brief Called in slave mode when the master is requesting data.
 */
using WireRequestHandler = void (*)();

/**
 * \brief Wire API wrapper.
 */
class TwoWire final
{
public:
    /**
     * \brief Don't call, use the global Wire and Wire1 instances instead.
     */
    TwoWire();

    /**
     * \brief Get the associated I2C instance (i2c0 or i2c1).
     */
    i2c_inst_t *i2c() const;

    /**
     * \brief Initialize in master mode.
     * 
     * Note: The Wire library typically initializes predefined I2C pins and enables internal
     *       pull-up resistors here. In this implementation, the user is responsible for setting
     *       up the I2C instance and GPIO pins in advance.
     */
    void begin();

    /**
     * \brief Initialize in slave mode.
     * 
     * Note: The Wire library typically initializes predefined I2C pins and enables internal
     *       pull-up resistors here. In this implementation, the user is responsible for setting
     *       up the I2C instance and GPIO pins in advance.
     *
     * \param selfAddress Slave address.
     */
    void begin(uint8_t selfAddress);

    /**
     * \brief Begin writing to a slave.
     * 
     * Available in master mode.
     * 
     * The maximum transfer size is determined by WIRE_BUFFER_LENGTH. Excess data will be ignored.
     * 
     * \param address Slave address.
     */
    void beginTransmission(uint8_t address);

    /**
     * \brief Finish writing to the slave.
     * 
     * Available in master mode.
     * 
     * Flushes the write buffer and blocks until completion.
     * 
     * \param sendStop Whether to send Stop signal at the end.
     * \return 0 on success. 
     *         3 if transfer was interrupted by the slave via NACK.
     *         4 on other error.
     */
    uint8_t endTransmission(bool sendStop = true);

    /**
     * \brief Read data from a slave.
     * 
     * Available in master mode.
     * 
     * \param address Slave address.
     * \param count Amount of data to read.
     * \param sendStop Whether to send Stop signal at the end.
     * \return 0 on error.
     *         `count` on success.
     */
    uint8_t requestFrom(uint8_t address, size_t count, bool sendStop);

    /**
     * \brief Get the amount of data in the read buffer.
     */
    size_t available() const;

    /**
     * \brief Get the next byte from the read buffer without removing it.
     * 
     * \return the buffer value, or -1 if the read buffer is empty.
     */
    int peek() const;

    /**
     * \brief Get the next byte from the read buffer.
     * 
     * \return the buffer value, or -1 if the read buffer is empty.
     */
    int read();

    /**
     * \brief Write a single byte.
     * 
     * \param value Byte value.
     * \return 1 on success.
     *         0 if the buffer is full (when transmitting in master mode).
     */
    size_t write(uint8_t value);

    /**
     * \brief Write data.
     * 
     * \param data Data pointer.
     * \param size Data size.
     * \return The amount of data written. May be less than len if the buffer fills up (when
     *         transmitting in master mode).
     */
    size_t write(const uint8_t *data, size_t size);

    /**
     * \brief Set the receive handler for slave mode.
     * 
     * \param handler Receive handler.
     */
    void onReceive(WireReceiveHandler handler);

    /**
     * \brief Set the request handler for slave mode.
     * 
     * \param handler Request handler.
     */
    void onRequest(WireRequestHandler handler);

private:
    static constexpr uint8_t NO_ADDRESS = 255;

    enum Mode : uint8_t
    {
        Unassigned,
        Master,
        Slave,
    };

    TwoWire(const TwoWire &other) = delete; // not copyable

    TwoWire &operator=(const TwoWire &other) = delete; // not copyable

    static void handleEvent(i2c_inst_t *i2c, i2c_slave_event_t event);

    WireReceiveHandler receiveHandler_ = nullptr;
    WireRequestHandler requestHandler_ = nullptr;
    Mode mode_ = Unassigned;
    uint8_t txAddress_ = 255;
    uint8_t buf_[WIRE_BUFFER_LENGTH];
    uint8_t bufLen_ = 0;
    uint8_t bufPos_ = 0;
};

/**
 * \brief Wire instance for i2c0
 */
extern TwoWire Wire;

/**
 * \brief Wire instance for i2c1
 */
extern TwoWire Wire1;

//
// inline members
//

inline size_t TwoWire::available() const {
    assert(mode_ != Unassigned); // missing begin
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission

    return bufLen_ - bufPos_;
}

inline int TwoWire::peek() const {
    assert(mode_ != Unassigned); // missing begin
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission

    return bufPos_ < bufLen_ ? (int)buf_[bufPos_] : -1;
}

inline int TwoWire::read() {
    assert(mode_ != Unassigned); // missing begin
    assert(txAddress_ == NO_ADDRESS); // not allowed during transmission

    return bufPos_ < bufLen_ ? (int)buf_[bufPos_++] : -1;
}

#endif
