/**
 * DRAMSim2 wrapper C connector
 *
 * MARSS-RISCV : Micro-Architectural System Simulator for RISC-V
 *
 * Copyright (c) 2020 Gaurav Kothari {gkothar1@binghamton.edu}
 * State University of New York at Binghamton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <cstdlib>

#include "dramsim_wrapper.h"
#include "dramsim_wrapper_c_connector.h"

#ifdef __cplusplus
extern "C" {
#endif

static dramsim_wrapper *dramsim_wrapper_obj = NULL;

void
dramsim_wrapper_init(const char *dram_ini_file, const char *system_ini_file,
                     const char *stats_dir, const char *app_name, int size_mb,
                     StageMemAccessQueue *frontend_mem_access_queue,
                     StageMemAccessQueue *backend_mem_access_queue)
{
    if (dramsim_wrapper_obj == NULL)
    {
        dramsim_wrapper_obj = new dramsim_wrapper(
            dram_ini_file, system_ini_file, stats_dir, app_name, size_mb,
            frontend_mem_access_queue, backend_mem_access_queue);
    }
}

void
dramsim_wrapper_destroy()
{
    delete dramsim_wrapper_obj;
    dramsim_wrapper_obj = NULL;
}

int
dramsim_wrapper_can_add_transaction(target_ulong addr)
{
    return dramsim_wrapper_obj->can_add_transaction(addr);
}

int
dramsim_wrapper_add_transaction(target_ulong addr, int isWrite)
{
    return dramsim_wrapper_obj->add_transaction(addr, (bool)isWrite);
}

void
dramsim_wrapper_update()
{
    dramsim_wrapper_obj->update();
}

void
dramsim_wrapper_print_stats()
{
    dramsim_wrapper_obj->print_stats();
}

int
dramsim_get_burst_size()
{
    return dramsim_wrapper_obj->get_burst_size();
}

#ifdef __cplusplus
}
#endif