/**
 * Contains top-level routines to manage the in-order core
 *
 * MARSS-RISCV : Micro-Architectural System Simulator for RISC-V
 *
 * Copyright (c) 2017-2019 Gaurav Kothari {gkothar1@binghamton.edu}
 * State University of New York at Binghamton
 *
 * Copyright (c) 2018-2019 Parikshit Sarnaik {psarnai1@binghamton.edu}
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../cutils.h"
#include "../riscv_cpu_priv.h"
#include "circular_queue.h"
#include "inorder.h"
#include "riscv_sim_cpu.h"

INCore *
in_core_init(const SimParams *p, struct RISCVSIMCPUState *simcpu)
{
    INCore *core;

    core = calloc(1, sizeof(INCore));
    assert(core);

    /* Create execution units */
    core->ialu = (CPUStage *)calloc(p->num_alu_stages, sizeof(CPUStage));
    assert(core->ialu);

    core->imul = (CPUStage *)calloc(p->num_mul_stages, sizeof(CPUStage));
    assert(core->imul);

    core->imul32 = (CPUStage *)calloc(p->num_mul32_stages, sizeof(CPUStage));
    assert(core->imul32);

    core->idiv = (CPUStage *)calloc(p->num_div_stages, sizeof(CPUStage));
    assert(core->idiv);

    core->idiv32 = (CPUStage *)calloc(p->num_div32_stages, sizeof(CPUStage));
    assert(core->idiv32);

    core->fpu_alu = (CPUStage *)calloc(p->num_fpu_alu_stages, sizeof(CPUStage));
    assert(core->fpu_alu);

    core->fpu_alu2 = (CPUStage *)calloc(p->num_fpu_alu2_stages, sizeof(CPUStage));
    assert(core->fpu_alu2);

    core->fpu_alu3 = (CPUStage *)calloc(p->num_fpu_alu3_stages, sizeof(CPUStage));
    assert(core->fpu_alu3);

    core->fpu_fma = (CPUStage *)calloc(p->num_fpu_fma_stages, sizeof(CPUStage));
    assert(core->fpu_fma);


    /* Create FU to Memory selection queue */
    cq_init(&core->ins_dispatch_queue.cq, INCORE_NUM_INS_DISPATCH_QUEUE_ENTRY);
    memset((void *)core->ins_dispatch_queue.data, 0,
           sizeof(uint64_t) * INCORE_NUM_INS_DISPATCH_QUEUE_ENTRY);

    /* Set pointer to 5 or 6 stage run() function */
    switch (p->num_cpu_stages)
    {
        case 5:
        {
            core->pfn_incore_run_internal = &in_core_run_5_stage;
            break;
        }
        case 6:
        {
            core->pfn_incore_run_internal = &in_core_run_6_stage;
            break;
        }
    }

    core->simcpu = simcpu;
    return core;
}

void
in_core_reset(void *core_type)
{
    int i;
    INCore *core;

    core = (INCore *)core_type;

    /* Reset stages */
    cpu_stage_flush(&core->pcgen);
    cpu_stage_flush(&core->fetch);
    cpu_stage_flush(&core->decode);
    cpu_stage_flush(&core->memory);
    cpu_stage_flush(&core->commit);

    /* To start fetching */
    core->pcgen.has_data = TRUE;

    /* Reset register valid bits */
    for (i = 0; i < NUM_INT_REG; ++i)
    {
        core->int_reg_status[i] = TRUE;
    }

    for (i = 0; i < NUM_FP_REG; ++i)
    {
        core->fp_reg_status[i] = TRUE;
    }

    /* Reset execution units */
    exec_unit_flush(core->ialu, core->simcpu->params->num_alu_stages);
    exec_unit_flush(core->imul, core->simcpu->params->num_mul_stages);
    exec_unit_flush(core->imul32, core->simcpu->params->num_mul32_stages);
    exec_unit_flush(core->idiv, core->simcpu->params->num_div_stages);
    exec_unit_flush(core->idiv32, core->simcpu->params->num_div32_stages);
    exec_unit_flush(core->fpu_alu, core->simcpu->params->num_fpu_alu_stages);
    exec_unit_flush(core->fpu_alu2, core->simcpu->params->num_fpu_alu2_stages);
    exec_unit_flush(core->fpu_alu3, core->simcpu->params->num_fpu_alu3_stages);
    exec_unit_flush(core->fpu_fma, core->simcpu->params->num_fpu_fma_stages);

    /* Reset EX to Memory queue */
    core->ins_dispatch_id = 0;
    cq_reset(&core->ins_dispatch_queue.cq);

    /* Reset Data FWD latches */
    memset((void *)core->fwd_latch, 0, sizeof(DataFWDLatch) * NUM_FWD_BUS);
}

void
in_core_free(void *core_type)
{
    INCore *core;

    core = (INCore *)(*(INCore **)core_type);
    free(core->fpu_fma);
    core->fpu_fma = NULL;
    free(core->fpu_alu3);
    core->fpu_alu3 = NULL;
    free(core->fpu_alu2);
    core->fpu_alu2 = NULL;
    free(core->fpu_alu);
    core->fpu_alu = NULL;
    free(core->idiv32);
    core->idiv32 = NULL;
    free(core->idiv);
    core->idiv = NULL;
    free(core->imul32);
    core->imul32 = NULL;
    free(core->imul);
    core->imul = NULL;
    free(core->ialu);
    core->ialu = NULL;
    free(core);
}

static int
in_core_pipeline_drained(INCore *core)
{
    int i;
    RISCVSIMCPUState *simcpu = core->simcpu;

    if (core->pcgen.has_data || core->fetch.has_data || core->decode.has_data
        || core->memory.has_data || core->commit.has_data)
    {
        return PIPELINE_NOT_DRAINED;
    }

    for (i = 0; i < simcpu->params->num_alu_stages; ++i)
    {
        if (core->ialu[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_mul_stages; ++i)
    {
        if (core->imul[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_mul32_stages; ++i)
    {
        if (core->imul32[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_div_stages; ++i)
    {
        if (core->idiv[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_div32_stages; ++i)
    {
        if (core->idiv32[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_fpu_alu_stages; ++i)
    {
        if (core->fpu_alu[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_fpu_alu2_stages; ++i)
    {
        if (core->fpu_alu2[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_fpu_alu3_stages; ++i)
    {
        if (core->fpu_alu3[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    for (i = 0; i < simcpu->params->num_fpu_fma_stages; ++i)
    {
        if (core->fpu_fma[i].has_data)
        {
            return PIPELINE_NOT_DRAINED;
        }
    }

    return PIPELINE_DRAINED;
}

int
in_core_run(void *core_type)
{
    INCore *core = (INCore *)core_type;
    RISCVCPUState *s = core->simcpu->emu_cpu_state;

    while (1)
    {
        /* Advance DRAM clock */
        s->simcpu->mmu->mem_controller->mem_controller_update_internal(
            s->simcpu->mmu->mem_controller);

        /* For 5-stage pipeline calls in_core_run_5_stage(), For 6-stage
         * pipeline calls in_core_run_6_stage() */
        if (core->pfn_incore_run_internal(core))
        {
            return s->sim_exception_cause;
        }

        /* If an exception occurred and pipeline is drained, safely exit from
         * simulation */
        if (s->sim_exception && in_core_pipeline_drained(core))
        {
            return s->sim_exception_cause;
        }

        /* Advance simulation cycle */
        ++s->simcpu->clock;
        ++s->simcpu->stats[s->priv].cycles;
    }
}

int
in_core_run_6_stage(INCore *core)
{
    if (in_core_commit(core))
    {
        /* Timeout */
        return -1;
    }

    in_core_memory(core);
    in_core_execute_all(core);
    in_core_decode(core);

    /* After the instruction in decode reads forwarded value, clear
     * forwarding latches. This keeps the data on forwarding latches valid
     * for exactly one cycle */
    memset((void *)core->fwd_latch, 0, sizeof(DataFWDLatch) * NUM_FWD_BUS);
    in_core_fetch(core);
    in_core_pcgen(core);
    return 0;
}

int
in_core_run_5_stage(INCore *core)
{
    if (in_core_commit(core))
    {
        /* Timeout */
        return -1;
    }

    in_core_memory(core);
    in_core_execute_all(core);
    in_core_decode(core);

    /* After the instruction in decode reads forwarded value, clear
     * forwarding latches. This keeps the data on forwarding latches valid
     * for exactly one cycle */
    memset((void *)core->fwd_latch, 0, sizeof(DataFWDLatch) * NUM_FWD_BUS);
    in_core_pcgen(core);
    in_core_fetch(core);
    return 0;
}
