/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: serial flash operate functions by SFUD lib.
 * Created on: 2016-04-23
 */

/******************************************************************************
 * @file sfud.c
 * @brief SFUD integration documentation for the local APP1 project.
 * Original upstream copyright, license and file description are retained.
 * Local comment normalization does not change executable code.
 ******************************************************************************/

/****************************** Includes ******************************/

#include "sfud.h" /* 中间件层：SFUD 接口与配置 */

#include <string.h> /* C 标准库 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

/* 读取数据时发送的占位字节。 */
#define DUMMY_DATA 0xFF

#ifndef SFUD_FLASH_DEVICE_TABLE
#error "Please configure the flash device information table in (in sfud_cfg.h)."
#endif

/****************************** Defines ******************************/

/************************ Variable Definitions ************************/

/* 用户配置的闪存设备信息表。 */
static sfud_flash flash_table[] = SFUD_FLASH_DEVICE_TABLE;
/* 支持的厂商信息表。 */
static const sfud_mf mf_table[] = SFUD_MF_TABLE;

#ifdef SFUD_USING_FLASH_INFO_TABLE
/* 支持的闪存芯片参数表。 */
static const sfud_flash_chip flash_chip_table[] = SFUD_FLASH_CHIP_TABLE;
#endif

#ifdef SFUD_USING_QSPI
/**
 * 闪存读取模式。
 */
enum sfud_qspi_read_mode
{
    NORMAL_SPI_READ = 1 << 0, /**< 普通单线读取模式。 */
    DUAL_OUTPUT     = 1 << 1, /**< 双线输出快速读取。 */
    DUAL_IO         = 1 << 2, /**< 双线输入输出快速读取。 */
    QUAD_OUTPUT     = 1 << 3, /**< 四线输出快速读取。 */
    QUAD_IO         = 1 << 4, /**< 四线输入输出快速读取。 */
};

/* 四线总线芯片扩展参数表。 */
static const sfud_qspi_flash_ext_info qspi_flash_ext_info_table[] =
    SFUD_FLASH_EXT_INFO_TABLE;
#endif /* SFUD_USING_QSPI */

/************************ Variable Definitions ************************/

/****************************** Declaring ******************************/

static sfud_err software_init(const sfud_flash *flash);
static sfud_err hardware_init(sfud_flash *flash);
static sfud_err page256_or_1_byte_write(const sfud_flash *flash,
                                        uint32_t addr,
                                        size_t size,
                                        uint16_t write_gran,
                                        const uint8_t *data);
static sfud_err aai_write(const sfud_flash *flash,
                          uint32_t addr,
                          size_t size,
                          const uint8_t *data);
static sfud_err wait_busy(const sfud_flash *flash);
static sfud_err reset(const sfud_flash *flash);
static sfud_err read_jedec_id(sfud_flash *flash);
static sfud_err set_write_enabled(const sfud_flash *flash, bool enabled);
static sfud_err set_4_byte_address_mode(sfud_flash *flash, bool enabled);
static void make_address_byte_array(const sfud_flash *flash,
                                    uint32_t addr,
                                    uint8_t *array);

/* 端口与 SFDP 接口由对应模块实现。 */
extern void sfud_log_debug(const char *file,
                           const long line,
                           const char *format,
                           ...);
extern void sfud_log_info(const char *format, ...);

/****************************** Declaring ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 初始化指定设备的端口、芯片参数与软件状态。
 *
 * @param[in,out] flash : 待初始化的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_device_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    /* 初始化硬件与端口。 */
    result = hardware_init(flash);
    if (result == SFUD_SUCCESS)
    {
        result = software_init(flash);
    }
    if (result == SFUD_SUCCESS)
    {
        flash->init_ok = true;
        SFUD_INFO("%s flash device initialized successfully.", flash->name);
    }
    else
    {
        flash->init_ok = false;
        SFUD_INFO("Error: %s flash device initialization failed.", flash->name);
    }

    return result;
}

/**
 * @brief 初始化设备表中的全部闪存设备。
 *
 * @return 全部成功时返回 SFUD_SUCCESS；否则返回最后一次初始化错误。
 */
sfud_err sfud_init(void)
{
    sfud_err cur_flash_result = SFUD_SUCCESS, all_flash_result = SFUD_SUCCESS;
    size_t i;

    SFUD_DEBUG("Start initialize Serial Flash Universal Driver(SFUD) V%s.",
               SFUD_SW_VERSION);
    SFUD_DEBUG(
        "You can get the latest version on https://github.com/armink/SFUD .");
    /* 依次初始化设备表中的全部设备。 */
    for (i = 0; i < sizeof(flash_table) / sizeof(sfud_flash); i++)
    {
        /* 记录设备在信息表中的索引。 */
        flash_table[i].index = i;
        cur_flash_result     = sfud_device_init(&flash_table[i]);

        if (cur_flash_result != SFUD_SUCCESS)
        {
            all_flash_result = cur_flash_result;
        }
    }

    return all_flash_result;
}

/**
 * @brief 按设备表索引获取闪存设备。
 *
 * @param[in] index : 设备表中的索引。
 *
 * @return 有效索引对应的设备指针；索引越界时返回 NULL。
 */
sfud_flash *sfud_get_device(size_t index)
{
    if (index < sfud_get_device_num())
    {
        return &flash_table[index];
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief 获取设备表中的闪存设备数量。
 *
 * @return 已配置的设备数量。
 */
size_t sfud_get_device_num(void)
{
    return sizeof(flash_table) / sizeof(sfud_flash);
}

/**
 * @brief 获取闪存设备表的首地址。
 *
 * @return 设备表首元素的指针。
 */
const sfud_flash *sfud_get_device_table(void)
{
    return flash_table;
}

#ifdef SFUD_USING_QSPI
/**
 * @brief 填写四线总线读取命令的指令、地址与数据阶段格式。
 *
 * @param[in,out] flash    : 接收命令格式的闪存设备。
 * @param[in] ins          : 读取指令。
 * @param[in] ins_lines    : 指令阶段的线数。
 * @param[in] addr_lines   : 地址阶段的线数。
 * @param[in] dummy_cycles : 占位时钟周期数。
 * @param[in] data_lines   : 数据阶段的线数。
 *
 * @return 无。
 */
static void qspi_set_read_cmd_format(sfud_flash *flash,
                                     uint8_t ins,
                                     uint8_t ins_lines,
                                     uint8_t addr_lines,
                                     uint8_t dummy_cycles,
                                     uint8_t data_lines)
{
    /* 容量大于 16 MiB 时使用四字节地址，并按表规则调整指令。 */
    if (flash->chip.capacity <= 0x1000000)
    {
        flash->read_cmd_format.instruction  = ins;
        flash->read_cmd_format.address_size = 24;
    }
    else
    {
        if (ins == SFUD_CMD_READ_DATA)
        {
            flash->read_cmd_format.instruction = ins + 0x10;
        }
        else
        {
            flash->read_cmd_format.instruction = ins + 1;
        }
        flash->read_cmd_format.address_size = 32;
    }

    flash->read_cmd_format.instruction_lines     = ins_lines;
    flash->read_cmd_format.address_lines         = addr_lines;
    flash->read_cmd_format.alternate_bytes_lines = 0;
    flash->read_cmd_format.dummy_cycles          = dummy_cycles;
    flash->read_cmd_format.data_lines            = data_lines;
}

/**
 * @brief 依据芯片扩展表与总线宽度配置快速读取格式。
 *
 * @param[in,out] flash       : 已配置或已初始化的闪存设备。
 * @param[in] data_line_width : 总线支持的最大数据线宽度：1、2 或 4。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 须在设备初始化成功后调用；未匹配时使用普通读取格式。
 */
sfud_err sfud_qspi_fast_read_enable(sfud_flash *flash, uint8_t data_line_width)
{
    size_t i          = 0;
    uint8_t read_mode = NORMAL_SPI_READ;
    sfud_err result   = SFUD_SUCCESS;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(data_line_width == 1 || data_line_width == 2 ||
                data_line_width == 4);

    /* 查找读取模式；未匹配时保留普通单线模式。 */
    for (i = 0; i < sizeof(qspi_flash_ext_info_table) /
                        sizeof(sfud_qspi_flash_ext_info);
         i++)
    {
        if ((qspi_flash_ext_info_table[i].mf_id == flash->chip.mf_id) &&
            (qspi_flash_ext_info_table[i].type_id == flash->chip.type_id) &&
            (qspi_flash_ext_info_table[i].capacity_id ==
             flash->chip.capacity_id))
        {
            read_mode = qspi_flash_ext_info_table[i].read_mode;
        }
    }

    /* 依据总线宽度选择读取模式并填写命令格式。 */
    switch (data_line_width)
    {
        case 1:
            qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
            break;
        case 2:
            if (read_mode & DUAL_IO)
            {
                qspi_set_read_cmd_format(
                    flash, SFUD_CMD_DUAL_IO_READ_DATA, 1, 2, 4, 2);
            }
            else if (read_mode & DUAL_OUTPUT)
            {
                qspi_set_read_cmd_format(
                    flash, SFUD_CMD_DUAL_OUTPUT_READ_DATA, 1, 1, 8, 2);
            }
            else
            {
                qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
            }
            break;
        case 4:
            if (read_mode & QUAD_IO)
            {
                qspi_set_read_cmd_format(
                    flash, SFUD_CMD_QUAD_IO_READ_DATA, 1, 4, 6, 4);
            }
            else if (read_mode & QUAD_OUTPUT)
            {
                qspi_set_read_cmd_format(
                    flash, SFUD_CMD_QUAD_OUTPUT_READ_DATA, 1, 1, 8, 4);
            }
            else
            {
                qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
            }
            break;
    }

    return result;
}
#endif /* SFUD_USING_QSPI */

/**
 * @brief 初始化端口并确定芯片容量、写入模式与擦除参数。
 *
 * @param[in,out] flash : 待初始化并填充参数的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
static sfud_err hardware_init(sfud_flash *flash)
{
    extern sfud_err sfud_spi_port_init(sfud_flash * flash);

    sfud_err result = SFUD_SUCCESS;
    size_t i;

    SFUD_ASSERT(flash);

    result = sfud_spi_port_init(flash);
    if (result != SFUD_SUCCESS)
    {
        return result;
    }

#ifdef SFUD_USING_QSPI
    /* 设置默认读取指令。 */
    flash->read_cmd_format.instruction = SFUD_CMD_READ_DATA;
#endif /* SFUD_USING_QSPI */

    /* 端口必须提供写后读回调。 */
    SFUD_ASSERT(flash->spi.wr);
    /* 用户未指定完整芯片参数时，通过 SFDP 或静态参数表识别。 */
    if (flash->chip.capacity == 0 || flash->chip.write_mode == 0 ||
        flash->chip.erase_gran == 0 || flash->chip.erase_gran_cmd == 0)
    {
        /* 读取厂商、存储类型与容量标识。 */
        result = read_jedec_id(flash);
        if (result != SFUD_SUCCESS)
        {
            return result;
        }

#ifdef SFUD_USING_SFDP
        extern bool sfud_read_sfdp(sfud_flash * flash);
        /* 读取 SFDP 参数。 */
        if (sfud_read_sfdp(flash))
        {
            flash->chip.name     = NULL;
            flash->chip.capacity = flash->sfdp.capacity;
            /* 当前 SFDP 写入粒度仅按 1 或 256 字节处理。 */
            if (flash->sfdp.write_gran == 1)
            {
                flash->chip.write_mode = SFUD_WM_BYTE;
            }
            else
            {
                flash->chip.write_mode = SFUD_WM_PAGE_256B;
            }
            /* 选择最小有效擦除单元作为默认擦除粒度。 */
            flash->chip.erase_gran     = flash->sfdp.eraser[0].size;
            flash->chip.erase_gran_cmd = flash->sfdp.eraser[0].cmd;
            for (i = 1; i < SFUD_SFDP_ERASE_TYPE_MAX_NUM; i++)
            {
                if (flash->sfdp.eraser[i].size != 0 &&
                    flash->chip.erase_gran > flash->sfdp.eraser[i].size)
                {
                    flash->chip.erase_gran     = flash->sfdp.eraser[i].size;
                    flash->chip.erase_gran_cmd = flash->sfdp.eraser[i].cmd;
                }
            }
        }
        else
        {
#endif

#ifdef SFUD_USING_FLASH_INFO_TABLE
            /* SFDP 读取失败时查找库内静态芯片参数表。 */
            for (i = 0; i < sizeof(flash_chip_table) / sizeof(sfud_flash_chip);
                 i++)
            {
                if ((flash_chip_table[i].mf_id == flash->chip.mf_id) &&
                    (flash_chip_table[i].type_id == flash->chip.type_id) &&
                    (flash_chip_table[i].capacity_id ==
                     flash->chip.capacity_id))
                {
                    flash->chip.name       = flash_chip_table[i].name;
                    flash->chip.capacity   = flash_chip_table[i].capacity;
                    flash->chip.write_mode = flash_chip_table[i].write_mode;
                    flash->chip.erase_gran = flash_chip_table[i].erase_gran;
                    flash->chip.erase_gran_cmd =
                        flash_chip_table[i].erase_gran_cmd;
                    break;
                }
            }
#endif

#ifdef SFUD_USING_SFDP
        }
#endif
    }

    if (flash->chip.capacity == 0 || flash->chip.write_mode == 0 ||
        flash->chip.erase_gran == 0 || flash->chip.erase_gran_cmd == 0)
    {
        SFUD_INFO("Warning: This flash device is not found or not supported.");
        return SFUD_ERR_NOT_FOUND;
    }
    else
    {
        const char *flash_mf_name = NULL;
        /* 查找厂商信息。 */
        for (i = 0; i < sizeof(mf_table) / sizeof(sfud_mf); i++)
        {
            if (mf_table[i].id == flash->chip.mf_id)
            {
                flash_mf_name = mf_table[i].name;
                break;
            }
        }
        /* 输出厂商和芯片名称。 */
        if (flash_mf_name && flash->chip.name)
        {
            SFUD_INFO("Found a %s %s flash chip. Size is %ld bytes.",
                      flash_mf_name,
                      flash->chip.name,
                      flash->chip.capacity);
        }
        else if (flash_mf_name)
        {
            SFUD_INFO("Found a %s flash chip. Size is %ld bytes.",
                      flash_mf_name,
                      flash->chip.capacity);
        }
        else
        {
            SFUD_INFO("Found a flash chip. Size is %ld bytes.",
                      flash->chip.capacity);
        }
    }

    /* 复位闪存设备。 */
    result = reset(flash);
    if (result != SFUD_SUCCESS)
    {
        return result;
    }

    /* 对需要处理保护位的器件，先取消块保护以允许写入和擦除。 */
    if (flash->chip.write_mode & SFUD_WM_AAI)
    {
        result = sfud_write_status(flash, true, 0x00);
    }
    else
    {
        /* MX25L3206E */
        if ((0xC2 == flash->chip.mf_id) && (0x20 == flash->chip.type_id) &&
            (0x16 == flash->chip.capacity_id))
        {
            result = sfud_write_status(flash, false, 0x00);
        }
    }
    if (result != SFUD_SUCCESS)
    {
        return result;
    }

    /* 容量大于 16 MiB 时进入四字节寻址模式。 */
    if (flash->chip.capacity > (1L << 24))
    {
        result = set_4_byte_address_mode(flash, true);
    }
    else
    {
        flash->addr_in_4_byte = false;
    }

    return result;
}

/**
 * @brief 保留软件初始化入口，当前仅检查设备指针。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 *
 * @return 固定返回 SFUD_SUCCESS。
 */
static sfud_err software_init(const sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    SFUD_ASSERT(flash);

    return result;
}

/**
 * @brief 从指定地址读取闪存数据。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[out] data : 接收读取数据的缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 设备须初始化成功，访问范围须在芯片容量内。
 */
sfud_err sfud_read(const sfud_flash *flash,
                   uint32_t addr,
                   size_t size,
                   uint8_t *data)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5 + SFUD_READ_DUMMY_BYTE_CNT];
    uint8_t cmd_size;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(data);
    /* 仅在设备初始化成功后执行。 */
    SFUD_ASSERT(flash->init_ok);
    /* 检查访问范围是否超出芯片容量。 */
    if (addr + size > flash->chip.capacity)
    {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* 存在锁回调时锁定总线。 */
    if (spi->lock)
    {
        spi->lock(spi);
    }

    result = wait_busy(flash);

    if (result == SFUD_SUCCESS)
    {
#ifdef SFUD_USING_QSPI
        if (flash->read_cmd_format.instruction != SFUD_CMD_READ_DATA)
        {
            result = spi->qspi_read(
                spi,
                addr,
                (sfud_qspi_read_cmd_format *)&flash->read_cmd_format,
                data,
                size);
        }
        else
#endif
        {
#ifdef SFUD_USING_FAST_READ
            cmd_data[0] = SFUD_CMD_FAST_READ_DATA;
#else
            cmd_data[0] = SFUD_CMD_READ_DATA;
#endif
            make_address_byte_array(flash, addr, &cmd_data[1]);
            cmd_size = flash->addr_in_4_byte ? 5 : 4;
#ifdef SFUD_USING_FAST_READ
            /* 仅快速读取模式追加占位字节。 */
            for (uint8_t i = 0; i < SFUD_READ_DUMMY_BYTE_CNT; i++)
            {
                cmd_data[cmd_size] = SFUD_DUMMY_DATA;
                cmd_size++;
            }
#endif
            result = spi->wr(spi, cmd_data, cmd_size, data, size);
        }
    }
    /* 存在解锁回调时释放总线。 */
    if (spi->unlock)
    {
        spi->unlock(spi);
    }

    return result;
}

/**
 * @brief 执行整片闪存擦除并等待完成。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_chip_erase(const sfud_flash *flash)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[4];

    SFUD_ASSERT(flash);
    /* 仅在设备初始化成功后执行。 */
    SFUD_ASSERT(flash->init_ok);
    /* 存在锁回调时锁定总线。 */
    if (spi->lock)
    {
        spi->lock(spi);
    }

    /* 设置写使能。 */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS)
    {
        goto __exit;
    }

    cmd_data[0] = SFUD_CMD_ERASE_CHIP;
    /* 双缓冲器件的整片擦除命令与普通器件不同。 */
    if (flash->chip.write_mode & SFUD_WM_DUAL_BUFFER)
    {
        cmd_data[1] = 0x94;
        cmd_data[2] = 0x80;
        cmd_data[3] = 0x9A;
        result      = spi->wr(spi, cmd_data, 4, NULL, 0);
    }
    else
    {
        result = spi->wr(spi, cmd_data, 1, NULL, 0);
    }
    if (result != SFUD_SUCCESS)
    {
        SFUD_INFO("Error: Flash chip erase SPI communicate error.");
        goto __exit;
    }
    result = wait_busy(flash);

__exit:
    /* 关闭写使能。 */
    set_write_enabled(flash, false);
    /* 存在解锁回调时释放总线。 */
    if (spi->unlock)
    {
        spi->unlock(spi);
    }

    return result;
}

/**
 * @brief 按器件擦除粒度擦除指定范围。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 擦除覆盖完整擦除单元，可能影响请求范围边界外的数据。
 */
sfud_err sfud_erase(const sfud_flash *flash, uint32_t addr, size_t size)
{
    extern size_t sfud_sfdp_get_suitable_eraser(
        const sfud_flash *flash, uint32_t addr, size_t erase_size);

    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5], cmd_size, cur_erase_cmd;
    size_t cur_erase_size;

    SFUD_ASSERT(flash);
    /* 仅在设备初始化成功后执行。 */
    SFUD_ASSERT(flash->init_ok);
    /* 检查访问范围是否超出芯片容量。 */
    if (addr + size > flash->chip.capacity)
    {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }

    if (addr == 0 && size == flash->chip.capacity)
    {
        return sfud_chip_erase(flash);
    }

    /* 存在锁回调时锁定总线。 */
    if (spi->lock)
    {
        spi->lock(spi);
    }

    /* 按选定擦除单元循环擦除。 */
    while (size)
    {
        /* 支持 SFDP 时使用参数表提供的擦除类型。 */
#ifdef SFUD_USING_SFDP
        size_t eraser_index;
        if (flash->sfdp.available)
        {
            /* 从 SFDP 表中选择当前范围适用的擦除单元。 */
            eraser_index   = sfud_sfdp_get_suitable_eraser(flash, addr, size);
            cur_erase_cmd  = flash->sfdp.eraser[eraser_index].cmd;
            cur_erase_size = flash->sfdp.eraser[eraser_index].size;
        }
        else
        {
#else
        {
#endif
            cur_erase_cmd  = flash->chip.erase_gran_cmd;
            cur_erase_size = flash->chip.erase_gran;
        }
        /* 设置写使能。 */
        result = set_write_enabled(flash, true);
        if (result != SFUD_SUCCESS)
        {
            goto __exit;
        }

        cmd_data[0] = cur_erase_cmd;
        make_address_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;
        result   = spi->wr(spi, cmd_data, cmd_size, NULL, 0);
        if (result != SFUD_SUCCESS)
        {
            SFUD_INFO("Error: Flash erase SPI communicate error.");
            goto __exit;
        }
        result = wait_busy(flash);
        if (result != SFUD_SUCCESS)
        {
            goto __exit;
        }
        /* 按擦除单元对齐，并计算下一擦除地址。 */
        if (addr % cur_erase_size != 0)
        {
            if (size > cur_erase_size - (addr % cur_erase_size))
            {
                size -= cur_erase_size - (addr % cur_erase_size);
                addr += cur_erase_size - (addr % cur_erase_size);
            }
            else
            {
                goto __exit;
            }
        }
        else
        {
            if (size > cur_erase_size)
            {
                size -= cur_erase_size;
                addr += cur_erase_size;
            }
            else
            {
                goto __exit;
            }
        }
    }

__exit:
    /* 关闭写使能。 */
    set_write_enabled(flash, false);
    /* 存在解锁回调时释放总线。 */
    if (spi->unlock)
    {
        spi->unlock(spi);
    }

    return result;
}

/**
 * @brief 使用单字节或最多 256 字节的页编程方式写入。
 *
 * @param[in] flash      : 已配置或已初始化的闪存设备。
 * @param[in] addr       : 闪存访问起始字节地址。
 * @param[in] size       : 本次访问的字节数。
 * @param[in] write_gran : 写入粒度，仅支持 1 或 256 字节。
 * @param[in] data       : 待写入的数据缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 不执行擦除；按页边界分段发送编程命令。
 */
static sfud_err page256_or_1_byte_write(const sfud_flash *flash,
                                        uint32_t addr,
                                        size_t size,
                                        uint16_t write_gran,
                                        const uint8_t *data)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    static uint8_t cmd_data[5 + SFUD_WRITE_MAX_PAGE_SIZE];
    uint8_t cmd_size;
    size_t data_size;

    SFUD_ASSERT(flash);
    /* 仅支持 1 或 256 字节写入粒度。 */
    SFUD_ASSERT(write_gran == 1 || write_gran == 256);
    /* 仅在设备初始化成功后执行。 */
    SFUD_ASSERT(flash->init_ok);
    /* 检查访问范围是否超出芯片容量。 */
    if (addr + size > flash->chip.capacity)
    {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* 存在锁回调时锁定总线。 */
    if (spi->lock)
    {
        spi->lock(spi);
    }

    /* 按写入粒度与页边界分段编程。 */
    while (size)
    {
        /* 设置写使能。 */
        result = set_write_enabled(flash, true);
        if (result != SFUD_SUCCESS)
        {
            goto __exit;
        }
        cmd_data[0] = SFUD_CMD_PAGE_PROGRAM;
        make_address_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;

        /* 按写入单元对齐，并计算下一写入地址。 */
        if (addr % write_gran != 0)
        {
            if (size > write_gran - (addr % write_gran))
            {
                data_size = write_gran - (addr % write_gran);
            }
            else
            {
                data_size = size;
            }
        }
        else
        {
            if (size > write_gran)
            {
                data_size = write_gran;
            }
            else
            {
                data_size = size;
            }
        }
        size -= data_size;
        addr += data_size;

        memcpy(&cmd_data[cmd_size], data, data_size);

        result = spi->wr(spi, cmd_data, cmd_size + data_size, NULL, 0);
        if (result != SFUD_SUCCESS)
        {
            SFUD_INFO("Error: Flash write SPI communicate error.");
            goto __exit;
        }
        result = wait_busy(flash);
        if (result != SFUD_SUCCESS)
        {
            goto __exit;
        }
        data += data_size;
    }

__exit:
    /* 关闭写使能。 */
    set_write_enabled(flash, false);
    /* 存在解锁回调时释放总线。 */
    if (spi->unlock)
    {
        spi->unlock(spi);
    }

    return result;
}

/**
 * @brief 使用地址自动递增模式按双字节连续编程。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[in] data  : 待写入的数据缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 不执行擦除；奇数起始地址和尾部单字节使用单字节编程处理。
 */
static sfud_err aai_write(const sfud_flash *flash,
                          uint32_t addr,
                          size_t size,
                          const uint8_t *data)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[8], cmd_size;
    bool first_write = true;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(flash->init_ok);
    /* 检查访问范围是否超出芯片容量。 */
    if (addr + size > flash->chip.capacity)
    {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* 存在锁回调时锁定总线。 */
    if (spi->lock)
    {
        spi->lock(spi);
    }
    /* 双字节编程要求偶数地址；奇数起始地址先写入一个字节。 */
    if (addr % 2 != 0)
    {
        result = page256_or_1_byte_write(flash, addr++, 1, 1, data++);
        if (result != SFUD_SUCCESS)
        {
            goto __exit;
        }
        size--;
    }
    /* 设置写使能。 */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS)
    {
        goto __exit;
    }
    /* 循环执行双字节连续编程。 */
    cmd_data[0] = SFUD_CMD_AAI_WORD_PROGRAM;
    while (size >= 2)
    {
        if (first_write)
        {
            make_address_byte_array(flash, addr, &cmd_data[1]);
            cmd_size               = flash->addr_in_4_byte ? 5 : 4;
            cmd_data[cmd_size]     = *data;
            cmd_data[cmd_size + 1] = *(data + 1);
            first_write            = false;
        }
        else
        {
            cmd_size    = 1;
            cmd_data[1] = *data;
            cmd_data[2] = *(data + 1);
        }

        result = spi->wr(spi, cmd_data, cmd_size + 2, NULL, 0);
        if (result != SFUD_SUCCESS)
        {
            SFUD_INFO("Error: Flash write SPI communicate error.");
            goto __exit;
        }

        result = wait_busy(flash);
        if (result != SFUD_SUCCESS)
        {
            goto __exit;
        }

        size -= 2;
        addr += 2;
        data += 2;
    }
    /* 关闭写使能以退出地址自动递增模式。 */
    result = set_write_enabled(flash, false);
    /* 剩余一个字节时使用单字节编程。 */
    if (result == SFUD_SUCCESS && size == 1)
    {
        result = page256_or_1_byte_write(flash, addr, 1, 1, data);
    }

__exit:
    if (result != SFUD_SUCCESS)
    {
        set_write_enabled(flash, false);
    }
    /* 存在解锁回调时释放总线。 */
    if (spi->unlock)
    {
        spi->unlock(spi);
    }

    return result;
}

/**
 * @brief 依据器件写入模式执行闪存编程。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[in] data  : 待写入的数据缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 不执行擦除；双缓冲写入分支尚未实现。
 */
sfud_err sfud_write(const sfud_flash *flash,
                    uint32_t addr,
                    size_t size,
                    const uint8_t *data)
{
    sfud_err result = SFUD_SUCCESS;

    if (flash->chip.write_mode & SFUD_WM_PAGE_256B)
    {
        result = page256_or_1_byte_write(flash, addr, size, 256, data);
    }
    else if (flash->chip.write_mode & SFUD_WM_AAI)
    {
        result = aai_write(flash, addr, size, data);
    }
    else if (flash->chip.write_mode & SFUD_WM_DUAL_BUFFER)
    {
        /* TODO：双缓冲写入模式尚未实现。 */
    }

    return result;
}

/**
 * @brief 先擦除指定范围，再写入数据。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[in] data  : 待写入的数据缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 擦除影响完整擦除单元，不保存请求范围边界外的原有数据。
 */
sfud_err sfud_erase_write(const sfud_flash *flash,
                          uint32_t addr,
                          size_t size,
                          const uint8_t *data)
{
    sfud_err result = SFUD_SUCCESS;

    result = sfud_erase(flash, addr, size);

    if (result == SFUD_SUCCESS)
    {
        result = sfud_write(flash, addr, size, data);
    }

    return result;
}

/**
 * @brief 发送复位使能和复位命令，并等待器件就绪。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
static sfud_err reset(const sfud_flash *flash)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    cmd_data[0] = SFUD_CMD_ENABLE_RESET;
    result      = spi->wr(spi, cmd_data, 1, NULL, 0);
    if (result == SFUD_SUCCESS)
    {
        result = wait_busy(flash);
    }
    else
    {
        SFUD_INFO("Error: Flash device reset failed.");
        return result;
    }

    cmd_data[1] = SFUD_CMD_RESET;
    result      = spi->wr(spi, &cmd_data[1], 1, NULL, 0);

    if (result == SFUD_SUCCESS)
    {
        result = wait_busy(flash);
    }

    if (result == SFUD_SUCCESS)
    {
        SFUD_DEBUG("Flash device reset success.");
    }
    else
    {
        SFUD_INFO("Error: Flash device reset failed.");
    }

    return result;
}

/**
 * @brief 读取厂商、存储类型与容量标识并保存到芯片参数。
 *
 * @param[in,out] flash : 接收芯片标识信息的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
static sfud_err read_jedec_id(sfud_flash *flash)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[1], recv_data[3];

    SFUD_ASSERT(flash);

    cmd_data[0] = SFUD_CMD_JEDEC_ID;
    result =
        spi->wr(spi, cmd_data, sizeof(cmd_data), recv_data, sizeof(recv_data));
    if (result == SFUD_SUCCESS)
    {
        flash->chip.mf_id       = recv_data[0];
        flash->chip.type_id     = recv_data[1];
        flash->chip.capacity_id = recv_data[2];
        SFUD_DEBUG(
            "The flash device manufacturer ID is 0x%02X, memory type ID is "
            "0x%02X, capacity ID is 0x%02X.",
            flash->chip.mf_id,
            flash->chip.type_id,
            flash->chip.capacity_id);
    }
    else
    {
        SFUD_INFO("Error: Read flash device JEDEC ID error.");
    }

    return result;
}

/**
 * @brief 设置写使能状态并回读写使能锁存位。
 *
 * @param[in] flash   : 已配置或已初始化的闪存设备。
 * @param[in] enabled : true 表示使能，false 表示禁止。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
static sfud_err set_write_enabled(const sfud_flash *flash, bool enabled)
{
    sfud_err result = SFUD_SUCCESS;
    uint8_t cmd, register_status;

    SFUD_ASSERT(flash);

    if (enabled)
    {
        cmd = SFUD_CMD_WRITE_ENABLE;
    }
    else
    {
        cmd = SFUD_CMD_WRITE_DISABLE;
    }

    result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

    if (result == SFUD_SUCCESS)
    {
        result = sfud_read_status(flash, &register_status);
    }

    if (result == SFUD_SUCCESS)
    {
        if (enabled && (register_status & SFUD_STATUS_REGISTER_WEL) == 0)
        {
            SFUD_INFO("Error: Can't enable write status.");
            return SFUD_ERR_WRITE;
        }
        else if (!enabled && (register_status & SFUD_STATUS_REGISTER_WEL) != 0)
        {
            SFUD_INFO("Error: Can't disable write status.");
            return SFUD_ERR_WRITE;
        }
    }

    return result;
}

/**
 * @brief 发送四字节寻址模式切换命令并更新设备状态。
 *
 * @param[in,out] flash : 待切换寻址模式的闪存设备。
 * @param[in] enabled   : true 表示进入，false 表示退出。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 初始化流程仅对容量大于 16 MiB 的器件调用进入操作。
 */
static sfud_err set_4_byte_address_mode(sfud_flash *flash, bool enabled)
{
    sfud_err result = SFUD_SUCCESS;
    uint8_t cmd;

    SFUD_ASSERT(flash);

    /* 设置写使能。 */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS)
    {
        return result;
    }

    if (enabled)
    {
        cmd = SFUD_CMD_ENTER_4B_ADDRESS_MODE;
    }
    else
    {
        cmd = SFUD_CMD_EXIT_4B_ADDRESS_MODE;
    }

    result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

    if (result == SFUD_SUCCESS)
    {
        flash->addr_in_4_byte = enabled ? true : false;
        SFUD_DEBUG("%s 4-Byte addressing mode success.",
                   enabled ? "Enter" : "Exit");
    }
    else
    {
        SFUD_INFO("Error: %s 4-Byte addressing mode failed.",
                  enabled ? "Enter" : "Exit");
    }

    return result;
}

/**
 * @brief 读取闪存状态寄存器。
 *
 * @param[in] flash   : 已配置或已初始化的闪存设备。
 * @param[out] status : 状态寄存器值的输出地址。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_read_status(const sfud_flash *flash, uint8_t *status)
{
    uint8_t cmd = SFUD_CMD_READ_STATUS_REGISTER;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(status);

    return flash->spi.wr(&flash->spi, &cmd, 1, status, 1);
}

/**
 * @brief 轮询忙状态，按配置延时并限制重试次数。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
static sfud_err wait_busy(const sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;
    uint8_t status;
    size_t retry_times = flash->retry.times;

    SFUD_ASSERT(flash);

    while (true)
    {
        result = sfud_read_status(flash, &status);
        if (result == SFUD_SUCCESS &&
            ((status & SFUD_STATUS_REGISTER_BUSY)) == 0)
        {
            break;
        }
        /* 消耗一次重试次数，并按配置执行延时。 */
        SFUD_RETRY_PROCESS(flash->retry.delay, retry_times, result);
    }

    if (result != SFUD_SUCCESS || ((status & SFUD_STATUS_REGISTER_BUSY)) != 0)
    {
        SFUD_INFO("Error: Flash wait busy has an error.");
    }

    return result;
}

/**
 * @brief 将地址转换为高字节在前的命令地址数组。
 *
 * @param[in] flash  : 已配置或已初始化的闪存设备。
 * @param[in] addr   : 闪存访问起始字节地址。
 * @param[out] array : 接收三字节或四字节地址的缓冲区。
 *
 * @return 无。
 */
static void make_address_byte_array(const sfud_flash *flash,
                                    uint32_t addr,
                                    uint8_t *array)
{
    uint8_t len, i;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(array);

    len = flash->addr_in_4_byte ? 4 : 3;

    for (i = 0; i < len; i++)
    {
        array[i] = (addr >> ((len - (i + 1)) * 8)) & 0xFF;
    }
}

/**
 * @brief 按易失或非易失方式写入状态寄存器。
 *
 * @param[in] flash       : 已配置或已初始化的闪存设备。
 * @param[in] is_volatile : true 使用易失写使能命令，false 使用常规写使能。
 * @param[in] status      : 待写入的状态寄存器值。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_write_status(const sfud_flash *flash,
                           bool is_volatile,
                           uint8_t status)
{
    sfud_err result     = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    if (is_volatile)
    {
        cmd_data[0] = SFUD_VOLATILE_SR_WRITE_ENABLE;
        result      = spi->wr(spi, cmd_data, 1, NULL, 0);
    }
    else
    {
        result = set_write_enabled(flash, true);
    }

    if (result == SFUD_SUCCESS)
    {
        cmd_data[0] = SFUD_CMD_WRITE_STATUS_REGISTER;
        cmd_data[1] = status;
        result      = spi->wr(spi, cmd_data, 2, NULL, 0);
    }

    if (result != SFUD_SUCCESS)
    {
        SFUD_INFO("Error: Write_status register failed.");
    }

    return result;
}

/************************ Function Definitions ************************/
