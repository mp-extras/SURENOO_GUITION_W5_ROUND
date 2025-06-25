/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Mark Grosen <mark@grosen.org>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "py/runtime.h"

#include "mphalport.h"
#include "driver/spi_master.h"
#include "soc/gpio_sig_map.h"
#include "soc/spi_pins.h"

typedef struct _qspi_qspi_t {
    mp_obj_base_t base;
    spi_host_device_t host;
    int8_t sck;
    int8_t d0;
    int8_t d1;
    int8_t d2;
    int8_t d3;
    uint32_t baudrate;
    uint32_t polarity;
    uint32_t phase;
    spi_device_handle_t io_handle;
} qspi_qspi_t;

static mp_obj_t qspi_qspi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_dev_id, MP_ARG_REQUIRED | MP_ARG_INT, },
        { MP_QSTR_sck, MP_ARG_REQUIRED | MP_ARG_OBJ, },
        { MP_QSTR_data_pins, MP_ARG_REQUIRED | MP_ARG_OBJ, },
        { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT, },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_phase, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    enum { ARG_dev_id, ARG_sck, ARG_data_pins, ARG_baudrate, ARG_polarity, ARG_phase, };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    qspi_qspi_t *self = m_new_obj(qspi_qspi_t);
    self->base.type = type;
    self->host = args[ARG_dev_id].u_int;

    self->sck = machine_pin_get_id(args[ARG_sck].u_obj);

    mp_obj_t *items;
    mp_obj_get_array_fixed_n(args[ARG_data_pins].u_obj, 4, &items);
    self->d0 = machine_pin_get_id(items[0]);
    self->d1 = machine_pin_get_id(items[1]);
    self->d2 = machine_pin_get_id(items[2]);
    self->d3 = machine_pin_get_id(items[3]);

    self->baudrate = args[ARG_baudrate].u_int;
    self->polarity = args[ARG_polarity].u_int;
    self->phase = args[ARG_phase].u_int;

    spi_bus_config_t buscfg = {
        .data0_io_num = self->d0,
        .data1_io_num = self->d1,
        .data2_io_num = self->d2,
        .data3_io_num = self->d3,
        .sclk_io_num = self->sck,
        .max_transfer_sz = (0x4000 * 16) + 8,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
    };
    esp_err_t ret = spi_bus_initialize(self->host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != 0) {
         mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(spi_bus_initialize)"), ret);
    }

    spi_device_interface_config_t devcfg = {
        .command_bits = 8,
        .address_bits = 24,
        .mode = self->phase | (self->polarity << 1),
        .clock_speed_hz = self->baudrate,
        .spics_io_num = -1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 10,
    };

    ret = spi_bus_add_device(self->host, &devcfg, &self->io_handle);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(spi_bus_add_device)"), ret);
    }

    return MP_OBJ_FROM_PTR(self);
}

static void qspi_qspi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    qspi_qspi_t *self = self_in;
    mp_printf(print, "<QSPI.%p id: %d, sck: %d, d0: %d, d1: %d, d2: %d, d3: %d, baudrate: %d>",
              self, self->host, self->sck, self->d0, self->d1, self->d2, self->d3, self->baudrate);
}

static mp_obj_t qspi_qspi_write_cmd(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t params_in) {
    qspi_qspi_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    mp_buffer_info_t params_info;
    if (params_in == mp_const_none) {
        params_info.buf = NULL;
        params_info.len = 0;
    }
    else {
        mp_get_buffer_raise(params_in, &params_info, MP_BUFFER_READ);
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.flags = (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR);
    t.cmd = 0x02;
    t.addr = cmd << 8;
    t.tx_buffer = params_info.buf;
    t.length = 8 * params_info.len;

    spi_device_polling_transmit(self->io_handle, &t);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_3(qspi_qspi_write_cmd_obj, qspi_qspi_write_cmd);

static mp_obj_t qspi_qspi_write_data(mp_obj_t self_in, mp_obj_t data_in) {
    qspi_qspi_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t data_info;
    mp_get_buffer_raise(data_in, &data_info, MP_BUFFER_READ);

    spi_transaction_ext_t t;
    memset(&t, 0, sizeof(t));
    t.base.flags = SPI_TRANS_MODE_QIO;
    t.base.cmd = 0x32;
    t.base.addr = 0x002C00;

    spi_device_polling_transmit(self->io_handle, (spi_transaction_t *)&t);

    memset(&t, 0, sizeof(t));
    t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | \
                   SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;

    const uint8_t *p_color = (uint8_t *)data_info.buf;
    size_t len = data_info.len;
    do {
        size_t chunk_size;

        if (len > 0x8000) {
            chunk_size = 0x8000;
        } else {
            chunk_size = len;
        }
        t.base.tx_buffer = p_color;
        t.base.length = chunk_size * 8;
        spi_device_polling_transmit(self->io_handle, (spi_transaction_t *)&t);
        len -= chunk_size;
        p_color += chunk_size;
    } while (len > 0);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(qspi_qspi_write_data_obj, qspi_qspi_write_data);

static mp_obj_t qspi_qspi_write_data_rotate(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t swap_in) {
    qspi_qspi_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t data_info;
    mp_get_buffer_raise(data_in, &data_info, MP_BUFFER_READ);
    mp_buffer_info_t swap_info;
    mp_get_buffer_raise(swap_in, &swap_info, MP_BUFFER_WRITE);

    if (swap_info.len == data_info.len) {
        // TODO - generalize dimensions - extra (optional) params
        uint16_t *p_color = (uint16_t *)data_info.buf;
        uint16_t *buf = (uint16_t *)swap_info.buf;
        for (int i = 0; i < 640; i++) {
            uint16_t *pix = p_color + i;
            for (int j = 0; j < 180; j++) {
              *buf++ = *pix;
              pix += 640;
            }
        }
        qspi_qspi_write_data(self_in, swap_in);
    } else {
        spi_transaction_ext_t t;
        memset(&t, 0, sizeof(t));
        t.base.flags = SPI_TRANS_MODE_QIO;
        t.base.cmd = 0x32;
        t.base.addr = 0x002C00;
        spi_device_polling_transmit(self->io_handle, (spi_transaction_t *)&t);

        memset(&t, 0, sizeof(t));
        t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD |
                       SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;

        t.base.tx_buffer = swap_info.buf;
        t.base.length = swap_info.len * 8;

        uint32_t rows = data_info.len / swap_info.len;
        uint32_t cols = swap_info.len / 2;
        uint16_t *p_color = (uint16_t *)data_info.buf;
        for (int i = 0; i < rows; i++) {
            uint16_t *pix = p_color + i;
            uint16_t *buf = (uint16_t *)swap_info.buf;
            for (int j = 0; j < cols; j++) {
              *buf++ = *pix;
              pix += rows;
            }
            spi_device_polling_transmit(self->io_handle, (spi_transaction_t *)&t);
        }
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_3(qspi_qspi_write_data_rotate_obj, qspi_qspi_write_data_rotate);

static mp_obj_t qspi_qspi_deinit(mp_obj_t self_in) {
    qspi_qspi_t *self = MP_OBJ_TO_PTR(self_in);

    spi_bus_remove_device(self->io_handle);
    spi_bus_free(self->host);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(qspi_qspi_deinit_obj, qspi_qspi_deinit);

static const mp_rom_map_elem_t qspi_qspi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write_cmd), MP_ROM_PTR(&qspi_qspi_write_cmd_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_data), MP_ROM_PTR(&qspi_qspi_write_data_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_data_rotate), MP_ROM_PTR(&qspi_qspi_write_data_rotate_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&qspi_qspi_deinit_obj) },
};

static MP_DEFINE_CONST_DICT(qspi_qspi_locals_dict, qspi_qspi_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    qspi_qspi_type,
    MP_QSTR_QSPI,
    MP_TYPE_FLAG_NONE,
    make_new, qspi_qspi_make_new,
    print, qspi_qspi_print,
    locals_dict, &qspi_qspi_locals_dict
);

static const mp_rom_map_elem_t qspi_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_qspi) },
    { MP_ROM_QSTR(MP_QSTR_QSPI), MP_ROM_PTR(&qspi_qspi_type) },
};

static MP_DEFINE_CONST_DICT(qspi_module_globals, qspi_module_globals_table);

const mp_obj_module_t qspi_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&qspi_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_qspi, qspi_module);
