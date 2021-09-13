/*
 * Copyright (c) 2021 Valentin Milea <valentin.milea@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <i2c_slave.h>
#include <hardware/irq.h>
#include <hardware/sync.h>

typedef struct i2c_slave_t
{
    i2c_inst_t *i2c;
    i2c_slave_handler_t handler;
    bool transfer_in_progress;
} i2c_slave_t;

static i2c_slave_t i2c_core0_slave;
static i2c_slave_t i2c_core1_slave;

static inline void finish_transfer(i2c_slave_t *slave) {
    if (slave->transfer_in_progress) {
        slave->handler(slave->i2c, I2C_SLAVE_FINISH);
        slave->transfer_in_progress = false;
    }
}

static void __not_in_flash_func(i2c_slave_irq_handler)() {
    i2c_slave_t *slave = (get_core_num() ? &i2c_core1_slave : &i2c_core0_slave);
    i2c_inst_t *i2c = slave->i2c;
    i2c_hw_t *hw = i2c_get_hw(i2c);

    uint32_t intr_stat = hw->intr_stat;
    if (intr_stat == 0) {
        return;
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
        finish_transfer(slave);
        hw->clr_tx_abrt;
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_START_DET_BITS) {
        finish_transfer(slave);
        hw->clr_start_det;
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
        finish_transfer(slave);
        hw->clr_stop_det;
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        slave->transfer_in_progress = true;
        slave->handler(i2c, I2C_SLAVE_RECEIVE);
    }
    if (intr_stat & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
        slave->transfer_in_progress = true;
        slave->handler(i2c, I2C_SLAVE_REQUEST);
        hw->clr_rd_req;
    }
}

void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler) {
    invalid_params_if(I2C, i2c != i2c0 && i2c != i2c1);
    invalid_params_if(I2C, handler == NULL);

    i2c_slave_t *slave = (get_core_num() ? &i2c_core1_slave : &i2c_core0_slave);
    slave->i2c = i2c;
    slave->handler = handler;

    // Note: The I2C slave does clock stretching implicitly after a RD_REQ, while the Tx FIFO is empty.
    // There is also an option to enable clock stretching while the Rx FIFO is full, but we leave it
    // disabled since the Rx FIFO should never fill up (unless slave->handler() is way too slow).
    i2c_set_slave_mode(i2c, true, address);

    i2c_hw_t *hw = i2c_get_hw(i2c);
    // unmask necessary interrupts
    hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_RD_REQ_BITS | I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS | I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_START_DET_BITS;

    // enable interrupt for current core
    uint num = I2C0_IRQ + i2c_hw_index(i2c);
    irq_set_exclusive_handler(num, i2c_slave_irq_handler);
    irq_set_enabled(num, true);
}

void i2c_slave_deinit() {
    i2c_slave_t *slave = (get_core_num() ? &i2c_core1_slave : &i2c_core0_slave);
    i2c_inst_t *i2c = slave->i2c;
    hard_assert_if(I2C, i2c == NULL); // should be called after i2c_slave_init()
    
    slave->i2c = NULL;
    slave->handler = NULL;
    slave->transfer_in_progress = false;

    uint num = I2C0_IRQ + i2c_hw_index(i2c);
    irq_set_enabled(num, false);
    irq_remove_handler(num, i2c_slave_irq_handler);

    i2c_hw_t *hw = i2c_get_hw(i2c);
    hw->intr_mask = I2C_IC_INTR_MASK_RESET;

    i2c_set_slave_mode(i2c, false, 0);
}
