/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Wire.h"
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

static const uint I2C_SLAVE_ADDRESS = 0x17;
static const uint I2C_BAUDRATE = 100000; // 100 kHz

// For this example, we run both the master and slave from the same board.
// You'll need to wire pin GP4 to GP6 (SDA), and pin GP5 to GP7 (SCL).
static const uint I2C_SLAVE_SDA_PIN = PICO_DEFAULT_I2C_SDA_PIN; // 4
static const uint I2C_SLAVE_SCL_PIN = PICO_DEFAULT_I2C_SCL_PIN; // 5
static const uint I2C_MASTER_SDA_PIN = 6;
static const uint I2C_MASTER_SCL_PIN = 7;

// The slave implements a 256 byte memory. To write a series of bytes, the master first
// writes the memory address, followed by the data. The address is automatically incremented
// for each byte transferred, looping back to 0 upon reaching the end. Reading is done
// sequentially from the current memory address.
static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
} context;

static void slave_on_receive(int count) {
    // writes always start with the memory address
    hard_assert(Wire.available());
    context.mem_address = (uint8_t)Wire.read();

    while (Wire.available()) {
        // save into memory
        context.mem[context.mem_address++] = (uint8_t)Wire.read();
    }
}

static void slave_on_request() {
    // load from memory
    uint8_t value = context.mem[context.mem_address++];
    Wire.write(value);
}

static void setup_slave() {
    gpio_init(I2C_SLAVE_SDA_PIN);
    gpio_set_function(I2C_SLAVE_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SDA_PIN);

    gpio_init(I2C_SLAVE_SCL_PIN);
    gpio_set_function(I2C_SLAVE_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SLAVE_SCL_PIN);

    i2c_init(i2c0, I2C_BAUDRATE);

    // setup Wire for slave mode
    Wire.onReceive(slave_on_receive);
    Wire.onRequest(slave_on_request);
    // in this implementation, the user is responsible for initializing the I2C instance and GPIO
    // pins before calling Wire::begin()
    Wire.begin(I2C_SLAVE_ADDRESS);
}

static void run_master() {
    gpio_init(I2C_MASTER_SDA_PIN);
    gpio_set_function(I2C_MASTER_SDA_PIN, GPIO_FUNC_I2C);
    // pull-ups are already active on slave side, this is just a fail-safe in case the wiring is faulty
    gpio_pull_up(I2C_MASTER_SDA_PIN);

    gpio_init(I2C_MASTER_SCL_PIN);
    gpio_set_function(I2C_MASTER_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_MASTER_SCL_PIN);

    i2c_init(i2c1, I2C_BAUDRATE);

    // setup Wire for master mode on i2c1
    Wire1.begin();

    for (uint8_t mem_address = 0;; mem_address = (mem_address + 32) % 256) {
        char msg[32];
        snprintf(msg, sizeof(msg), "Hello, I2C slave! - 0x%02X", mem_address);
        uint8_t msg_len = strlen(msg);

        // write message at mem_address
        printf("Write at 0x%02X: '%s'\n", mem_address, msg);
        Wire1.beginTransmission(I2C_SLAVE_ADDRESS);
        Wire1.write(mem_address);
        Wire1.write((const uint8_t *)msg, msg_len);
        uint8_t result = Wire1.endTransmission(true /* sendStop */);
        if (result != 0) {
            puts("Couldn't write to slave, please check your wiring!");
            return;
        }

        // seek to mem_address
        Wire1.beginTransmission(I2C_SLAVE_ADDRESS);
        Wire1.write(mem_address);
        result = Wire1.endTransmission(false /* sendStop */);
        hard_assert(result == 0);
        uint8_t buf[32];
        // partial read
        uint8_t split = 5;
        uint8_t count = Wire1.requestFrom(I2C_SLAVE_ADDRESS, split, false /* sendStop */);
        hard_assert(count == split);
        for (size_t i = 0; i < count; i++) {
            buf[i] = Wire1.read();
        }
        buf[count] = '\0';
        printf("Read  at 0x%02X: '%s'\n", mem_address, buf);
        hard_assert(memcmp(buf, msg, split) == 0);
        // read the remaining bytes, continuing from last address
        count = Wire1.requestFrom(I2C_SLAVE_ADDRESS, msg_len - split, true /* sendStop */);
        hard_assert(count == (size_t)(msg_len - split));
        for (size_t i = 0; i < count; i++) {
            buf[i] = Wire1.read();
        }
        buf[count] = '\0';
        printf("Read  at 0x%02X: '%s'\n", mem_address + split, buf);
        hard_assert(memcmp(buf, msg + split, msg_len - split) == 0);

        puts("");
        sleep_ms(2000);
    }
}

int main() {
    stdio_init_all();
    puts("\nI2C slave example with Wire API");

    setup_slave();
    run_master();
}
