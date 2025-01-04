/*
 * Copyright (c) 2017 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#include <stdio.h>

#include "stm32l4xx.h"
#include "stm32l4_iwdg.h"

#include "armv7m.h"

// Enable and configure the Independent Watchdog (IWDG) with a specified timeout
int stm32l4_iwdg_enable(uint32_t timeout)
{
    uint32_t iwdg_pr = 0; 
    uint32_t iwdg_rl;     
    
    if (timeout > 32000)
    {
        timeout = 32000;
    }

    // Set minimum timeout to 1 ms to avoid invalid configurations
    if (timeout < 1)
    {
        timeout = 1;
    }

    // Improved timeout calculation to avoid precision loss
    iwdg_rl = (timeout * 32000) / (4 * 1000);  

    // Scale reload value and increment prescaler if necessary
    while (iwdg_rl > 4095 && iwdg_pr < 6)
    {
        iwdg_pr++;
        iwdg_rl >>= 1;
    }

    __disable_irq(); 

    IWDG->KR = 0xCCCC;  // Start the watchdog
    IWDG->KR = 0x5555;  // Unlock registers for configuration

    // Wait for the IWDG to be ready, with a timeout to avoid indefinite blocking
    uint32_t timeout_counter = 10000;  // Arbitrary loop limit to prevent lock-up
    while ((IWDG->SR & (IWDG_SR_WVU | IWDG_SR_RVU | IWDG_SR_PVU)) && timeout_counter--)
    {
        if (timeout_counter == 0)
        {
            __enable_irq();
            return -1;       // Return error if watchdog configuration takes too long
        }
    }

    // Apply the calculated prescaler and reload value
    IWDG->PR = iwdg_pr;
    IWDG->RLR = iwdg_rl - 1;

    // Refresh the watchdog to prevent an immediate reset
    IWDG->KR = 0xAAAA;
    
    __enable_irq();

    return 0;  // Return 0 to indicate success
}
