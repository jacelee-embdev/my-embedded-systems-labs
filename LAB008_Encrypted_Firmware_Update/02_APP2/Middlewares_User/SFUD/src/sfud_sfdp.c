/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016, Armink, <armink.ztl@gmail.com>
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
 * Function: Analyze the SFDP (Serial Flash Discoverable Parameters) which from
 *     JESD216/A/B (V1.X) standard.
 *           JESD216  (V1.0) document:
 *     http://www.jedec.org/sites/default/files/docs/JESD216.pdf
 *           JESD216A (V1.5) document:
 *     http://www.jedec.org/sites/default/files/docs/JESD216A.pdf
 *           JESD216B (V1.6) document:
 *     http://www.jedec.org/sites/default/files/docs/JESD216B.pdf
 *
 * Created on: 2016-05-26
 */

/******************************************************************************
 * @file sfud_sfdp.c
 * @brief SFUD integration documentation for the local APP2 project.
 * Original upstream copyright, license and file description are retained.
 * Local comment normalization does not change executable code.
 ******************************************************************************/

/****************************** Includes ******************************/

#include "sfud.h" /* 中间件层：SFUD 接口与配置 */

/****************************** Includes ******************************/

/**
 * JEDEC 参数术语：DWORD 为连续四个字节，共 32 位。
 * 扇区表示可擦除区域的最小尺寸与对齐粒度；不同区域可有不同粒度。
 */

#ifdef SFUD_USING_SFDP

/****************************** Defines ******************************/

/* 本驱动支持的最大 SFDP 主版本。 */
#define SUPPORT_MAX_SFDP_MAJOR_REV 1
/* 初版基本参数表使用 9 个 DWORD，共 36 字节。 */
#define BASIC_TABLE_LEN            9
/* 已排序擦除类型表中的最小擦除单元索引。 */
#define SMALLEST_ERASER_INDEX      0

/****************************** Defines ******************************/

/************************** Type Definitions **************************/

/**
 * SFDP 参数表头结构。
 */
typedef struct
{
    uint8_t id;        /**< 参数标识的低字节。 */
    uint8_t minor_rev; /**< 参数次版本。 */
    uint8_t major_rev; /**< 参数主版本。 */
    uint8_t len;       /**< 参数表长度，单位为 DWORD。 */
    uint32_t ptp;      /**< 参数表的 24 位字节地址。 */
} sfdp_para_header;

/************************** Type Definitions **************************/

/****************************** Declaring ******************************/

static sfud_err read_sfdp_data(const sfud_flash *flash,
                               uint32_t addr,
                               uint8_t *read_buf,
                               size_t size);
static bool read_sfdp_header(sfud_flash *flash);
static bool read_basic_header(const sfud_flash *flash,
                              sfdp_para_header *basic_header);
static bool read_basic_table(sfud_flash *flash, sfdp_para_header *basic_header);

/* 端口日志接口由 sfud_port.c 实现。 */
extern void sfud_log_debug(const char *file,
                           const long line,
                           const char *format,
                           ...);
extern void sfud_log_info(const char *format, ...);

/****************************** Declaring ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 读取并解析 SFDP 头与基本参数表。
 *
 * @param[in,out] flash : 接收 SFDP 参数的闪存设备。
 *
 * @return true 表示读取解析成功，false 表示失败或版本不受支持。
 */
bool sfud_read_sfdp(sfud_flash *flash)
{
    SFUD_ASSERT(flash);

    /* JEDEC 基本参数表头。 */
    sfdp_para_header basic_header;
    if (read_sfdp_header(flash) && read_basic_header(flash, &basic_header))
    {
        return read_basic_table(flash, &basic_header);
    }
    else
    {
        SFUD_INFO(
            "Warning: Read SFDP parameter header information failed. The %s "
            "does not support JEDEC SFDP.",
            flash->name);
        return false;
    }
}

/**
 * @brief 读取 SFDP 头并检查签名与主版本。
 *
 * @param[in,out] flash : 接收头信息的闪存设备。
 *
 * @return true 表示头有效，false 表示失败。
 */
static bool read_sfdp_header(sfud_flash *flash)
{
    sfud_sfdp *sfdp = &flash->sfdp;
    /* SFDP 头位于参数空间地址零，包含签名、参数表头数量与版本。 */
    /* SFDP 头的起始地址。 */
    uint32_t header_addr = 0;
    /* SFDP 头占两个 DWORD，共 8 字节。 */
    uint8_t header[2 * 4] = { 0 };

    SFUD_ASSERT(flash);

    sfdp->available = false;
    /* 读取 SFDP 头。 */
    if (read_sfdp_data(flash, header_addr, header, sizeof(header)) !=
        SFUD_SUCCESS)
    {
        SFUD_INFO("Error: Can't read SFDP header.");
        return false;
    }
    /* 检查 SFDP 签名与版本。 */
    if (!(header[0] == 'S' && header[1] == 'F' && header[2] == 'D' &&
          header[3] == 'P'))
    {
        SFUD_DEBUG(
            "Error: Check SFDP signature error. It's must be 50444653h('S' "
            "'F' 'D' 'P').");
        return false;
    }
    sfdp->minor_rev = header[4];
    sfdp->major_rev = header[5];
    if (sfdp->major_rev > SUPPORT_MAX_SFDP_MAJOR_REV)
    {
        SFUD_INFO("Error: This reversion(V%d.%d) of SFDP is not supported.",
                  sfdp->major_rev,
                  sfdp->minor_rev);
        return false;
    }
    SFUD_DEBUG("Check SFDP header is OK. The reversion is V%d.%d, NPN is %d.",
               sfdp->major_rev,
               sfdp->minor_rev,
               header[6]);

    return true;
}

/**
 * @brief 读取基本参数表头并解析表地址与长度。
 *
 * @param[in] flash         : 已配置或已初始化的闪存设备。
 * @param[out] basic_header : 接收基本参数表头信息的结构体。
 *
 * @return true 表示表头有效，false 表示失败。
 */
static bool read_basic_header(const sfud_flash *flash,
                              sfdp_para_header *basic_header)
{
    /* 基本参数表头是必需项，从字节偏移 0x08 开始。 */
    uint32_t header_addr = 8;
    /* 每个参数头占两个 DWORD，共 8 字节。 */
    uint8_t header[2 * 4] = { 0 };

    SFUD_ASSERT(flash);
    SFUD_ASSERT(basic_header);

    /* 读取基本参数表头。 */
    if (read_sfdp_data(flash, header_addr, header, sizeof(header)) !=
        SFUD_SUCCESS)
    {
        SFUD_INFO("Error: Can't read JEDEC basic flash parameter header.");
        return false;
    }
    basic_header->id        = header[0];
    basic_header->minor_rev = header[1];
    basic_header->major_rev = header[2];
    basic_header->len       = header[3];
    basic_header->ptp =
        (long)header[4] | (long)header[5] << 8 | (long)header[6] << 16;
    /* 检查基本参数表头版本与长度。 */
    if (basic_header->major_rev > SUPPORT_MAX_SFDP_MAJOR_REV)
    {
        SFUD_INFO(
            "Error: This reversion(V%d.%d) of JEDEC basic flash parameter "
            "header is not supported.",
            basic_header->major_rev,
            basic_header->minor_rev);
        return false;
    }
    if (basic_header->len < BASIC_TABLE_LEN)
    {
        SFUD_INFO(
            "Error: The JEDEC basic flash parameter table length (now is %d) "
            "error.",
            basic_header->len);
        return false;
    }
    SFUD_DEBUG(
        "Check JEDEC basic flash parameter header is OK. The table id is %d, "
        "reversion is V%d.%d,"
        " length is %d, parameter table pointer is 0x%06lX.",
        basic_header->id,
        basic_header->major_rev,
        basic_header->minor_rev,
        basic_header->len,
        basic_header->ptp);

    return true;
}

/**
 * @brief 解析基本参数表中的容量、写入与擦除参数。
 *
 * @param[in,out] flash    : 接收解析结果的闪存设备。
 * @param[in] basic_header : 基本参数表的地址与长度信息。
 *
 * @return true 表示解析成功，false 表示失败。
 */
static bool read_basic_table(sfud_flash *flash, sfdp_para_header *basic_header)
{
    sfud_sfdp *sfdp = &flash->sfdp;
    /* 参数表的字节地址。 */
    uint32_t table_addr = basic_header->ptp;
    /* 保存读取到的参数表数据。 */
    uint8_t table[BASIC_TABLE_LEN * 4] = { 0 }, i, j;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(basic_header);

    /* 读取基本参数表。 */
    if (read_sfdp_data(flash, table_addr, table, sizeof(table)) != SFUD_SUCCESS)
    {
        SFUD_INFO("Warning: Can't read JEDEC basic flash parameter table.");
        return false;
    }
    /* 输出基本参数表内容供调试。 */
    SFUD_DEBUG("JEDEC basic flash parameter table info:");
    SFUD_DEBUG("MSB-LSB  3    2    1    0");
    for (i = 0; i < BASIC_TABLE_LEN; i++)
    {
        SFUD_DEBUG("[%04d] 0x%02X 0x%02X 0x%02X 0x%02X",
                   i + 1,
                   table[i * 4 + 3],
                   table[i * 4 + 2],
                   table[i * 4 + 1],
                   table[i * 4]);
    }

    /* 解析 4 KiB 擦除支持情况与命令。 */
    sfdp->erase_4k_cmd = table[1];
    switch (table[0] & 0x03)
    {
        case 1:
            sfdp->erase_4k = true;
            SFUD_DEBUG(
                "4 KB Erase is supported throughout the device. Command is "
                "0x%02X.",
                sfdp->erase_4k_cmd);
            break;
        case 3:
            sfdp->erase_4k = false;
            SFUD_DEBUG("Uniform 4 KB erase is unavailable for this device.");
            break;
        default:
            SFUD_INFO("Error: Uniform 4 KB erase supported information error.");
            return false;
    }
    /* 解析写入粒度。 */
    /* TODO：当前采用 V1.0 的解析方式，尚未支持 V1.5 及以上的页大小字段。 */
    switch ((table[0] & (0x01 << 2)) >> 2)
    {
        case 0:
            sfdp->write_gran = 1;
            SFUD_DEBUG("Write granularity is 1 byte.");
            break;
        case 1:
            sfdp->write_gran = 256;
            SFUD_DEBUG("Write granularity is 64 bytes or larger.");
            break;
    }
    /* 解析状态寄存器块保护位的易失属性。 */
    switch ((table[0] & (0x01 << 3)) >> 3)
    {
        case 0:
            /* 保护位支持非易失写入，或通过 0x50/0x06 分别进行易失/非易失写入。
             */
            sfdp->sr_is_non_vola = true;
            SFUD_DEBUG("Target flash status register is non-volatile.");
            break;
        case 1:
            /* 状态寄存器的块保护位仅支持易失写入。 */
            sfdp->sr_is_non_vola = false;
            SFUD_DEBUG(
                "Block Protect bits in device's status register are solely "
                "volatile.");
            /* 选择易失状态寄存器写使能命令。 */
            switch ((table[0] & (0x01 << 4)) >> 4)
            {
                case 0:
                    sfdp->vola_sr_we_cmd = SFUD_VOLATILE_SR_WRITE_ENABLE;
                    SFUD_DEBUG(
                        "Flash device requires instruction 50h as the write "
                        "enable prior "
                        "to performing a volatile write to the status "
                        "register.");
                    break;
                case 1:
                    sfdp->vola_sr_we_cmd = SFUD_CMD_WRITE_ENABLE;
                    SFUD_DEBUG(
                        "Flash device requires instruction 06h as the write "
                        "enable prior "
                        "to performing a volatile write to the status "
                        "register.");
                    break;
            }
            break;
    }
    /* 解析数组读写和擦除所需的地址字节数。 */
    switch ((table[2] & (0x03 << 1)) >> 1)
    {
        case 0:
            sfdp->addr_3_byte = true;
            sfdp->addr_4_byte = false;
            SFUD_DEBUG("3-Byte only addressing.");
            break;
        case 1:
            sfdp->addr_3_byte = true;
            sfdp->addr_4_byte = true;
            SFUD_DEBUG("3- or 4-Byte addressing.");
            break;
        case 2:
            sfdp->addr_3_byte = false;
            sfdp->addr_4_byte = true;
            SFUD_DEBUG("4-Byte only addressing.");
            break;
        default:
            sfdp->addr_3_byte = false;
            sfdp->addr_4_byte = false;
            SFUD_INFO("Error: Read address bytes error!");
            return false;
    }
    /* 解析存储容量，并转换为字节数。 */
    uint32_t table2_temp = ((long)table[7] << 24) | ((long)table[6] << 16) |
                           ((long)table[5] << 8) | (long)table[4];
    switch ((table[7] & (0x01 << 7)) >> 7)
    {
        case 0:
            sfdp->capacity = 1 + (table2_temp >> 3);
            break;
        case 1:
            table2_temp &= 0x7FFFFFFF;
            if (table2_temp > sizeof(sfdp->capacity) * 8 + 3)
            {
                sfdp->capacity = 0;
                SFUD_INFO(
                    "Error: The flash capacity is grater than 32 Gb/ 4 GB! "
                    "Not Supported.");
                return false;
            }
            sfdp->capacity = 1L << (table2_temp - 3);
            break;
    }
    SFUD_DEBUG("Capacity is %ld Bytes.", sfdp->capacity);
    /* 解析各擦除类型的尺寸和命令。 */
    for (i = 0, j = 0; i < SFUD_SFDP_ERASE_TYPE_MAX_NUM; i++)
    {
        if (table[28 + 2 * i] != 0x00)
        {
            sfdp->eraser[j].size = 1L << table[28 + 2 * i];
            sfdp->eraser[j].cmd  = table[28 + 2 * i + 1];
            SFUD_DEBUG(
                "Flash device supports %ldKB block erase. Command is 0x%02X.",
                sfdp->eraser[j].size / 1024,
                sfdp->eraser[j].cmd);
            j++;
        }
    }
    /* 按擦除单元尺寸从小到大排序。 */
    for (i = 0, j = 0; i < SFUD_SFDP_ERASE_TYPE_MAX_NUM; i++)
    {
        if (sfdp->eraser[i].size)
        {
            for (j = i + 1; j < SFUD_SFDP_ERASE_TYPE_MAX_NUM; j++)
            {
                if (sfdp->eraser[j].size != 0 &&
                    sfdp->eraser[i].size > sfdp->eraser[j].size)
                {
                    /* 交换较小的有效擦除类型到当前位置。 */
                    uint32_t temp_size   = sfdp->eraser[i].size;
                    uint8_t temp_cmd     = sfdp->eraser[i].cmd;
                    sfdp->eraser[i].size = sfdp->eraser[j].size;
                    sfdp->eraser[i].cmd  = sfdp->eraser[j].cmd;
                    sfdp->eraser[j].size = temp_size;
                    sfdp->eraser[j].cmd  = temp_cmd;
                }
            }
        }
    }

    sfdp->available = true;
    return true;
}

/**
 * @brief 使用三字节地址与占位字节读取 SFDP 数据。
 *
 * @param[in] flash     : 已配置或已初始化的闪存设备。
 * @param[in] addr      : 闪存访问起始字节地址。
 * @param[out] read_buf : 接收读取数据的缓冲区。
 * @param[in] size      : 本次访问的字节数。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
static sfud_err read_sfdp_data(const sfud_flash *flash,
                               uint32_t addr,
                               uint8_t *read_buf,
                               size_t size)
{
    uint8_t cmd[] = {
        SFUD_CMD_READ_SFDP_REGISTER, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF,
        (addr >> 0) & 0xFF,          SFUD_DUMMY_DATA,
    };

    SFUD_ASSERT(flash);
    SFUD_ASSERT(addr < 1L << 24);
    SFUD_ASSERT(read_buf);
    SFUD_ASSERT(flash->spi.wr);

    return flash->spi.wr(&flash->spi, cmd, sizeof(cmd), read_buf, size);
}

/**
 * @brief 按起始地址和剩余长度选择合适的擦除类型。
 *
 * @param[in] flash      : 已配置或已初始化的闪存设备。
 * @param[in] addr       : 闪存访问起始字节地址。
 * @param[in] erase_size : 本次待擦除的剩余字节数。
 *
 * @return SFDP 擦除类型表中的索引。
 *
 * @note 优先选择满足对齐与长度要求的较大擦除单元。
 */
size_t sfud_sfdp_get_suitable_eraser(const sfud_flash *flash,
                                     uint32_t addr,
                                     size_t erase_size)
{
    size_t index = SMALLEST_ERASER_INDEX, i;
    /* 仅在 SFDP 参数有效时使用。 */
    SFUD_ASSERT(flash->sfdp.available);
    /* 起始地址未按最小擦除单元对齐时，选择最小单元。 */
    if (addr % flash->sfdp.eraser[SMALLEST_ERASER_INDEX].size)
    {
        return SMALLEST_ERASER_INDEX;
    }
    /* 擦除类型已按尺寸升序排列；从末尾查找较大单元以减少命令次数。 */
    for (i = SFUD_SFDP_ERASE_TYPE_MAX_NUM - 1;; i--)
    {
        if ((flash->sfdp.eraser[i].size != 0) &&
            (erase_size >= flash->sfdp.eraser[i].size) &&
            (addr % flash->sfdp.eraser[i].size == 0))
        {
            index = i;
            break;
        }
        if (i == SMALLEST_ERASER_INDEX)
        {
            break;
        }
    }
    return index;
}

/************************ Function Definitions ************************/

#endif /* SFUD_USING_SFDP */
