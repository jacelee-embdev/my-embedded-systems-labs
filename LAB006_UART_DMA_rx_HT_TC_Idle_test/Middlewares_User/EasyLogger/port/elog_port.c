/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
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
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <elog.h>

/*  用于声明 printf()、snprintf() 等标准格式化输出函数。
elog_port_output() 使用 printf()，时间接口使用 snprintf()。*/
#include <stdio.h>               

// 用于声明 HAL_GetTick()，并提供 STM32 HAL/CMSIS 相关类型和接口。
#include "stm32f4xx_hal.h" // 用于支持日志时间计算
/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
    
    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {

    /* add your code here */

}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    
    /* add your code here */
    /*将 EasyLogger 已格式化好的日志交给 printf()，再通过已完成的串口重定向发送到 USART。
    %.*s 可以按        EasyLogger 给出的长度输出，不要求字符串必须以 \\0 结尾。*/
    printf("%.*s",size,log); //新增内容
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    
    /* add your code here */
      __disable_irq(); // 新增内容,关闭全局中断，防止日志并发输出造成内容交叉
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    
    /* add your code here */
    __enable_irq();// 新增内容,开启全局中断，结束日志输出临界区
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    
    /* add your code here */
     /*******************新增代码开始 ************* */
    /*原因:将 HAL_GetTick() 得到的毫秒计数转换成 EasyLogger 所需的字符串，
    并作为日志时间字段返回。静态    缓冲区用于保证函数返回后字符串仍然有效。*/
    static char time_buf[16];
    uint32_t tick = HAL_GetTick();

    /* 格式：启动后的 秒.毫秒，例如 12.345 */
    (void)snprintf(time_buf,
                   sizeof(time_buf),
                   "%lu.%03lu",
                   (unsigned long)(tick / 1000U),
                   (unsigned long)(tick % 1000U));

    return time_buf;
     /*******************新增代码结束 ************* */
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    /* add your code here */
    
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    /* add your code here */
    
}
