#ifndef _AXI_SDR_CTRL_H_
#define _AXI_SDR_CTRL_H_
#include "unistd.h"
#include "stdint.h"

#define AXI_SDR_CTRL_BASE_ADDR		    (0x43C30000)
#define AXI_SDR_CTRL_RANGE              0x10000
#define AXI_SDR_CTRL_S00_AXI_SLV_REG0_OFFSET 0
#define AXI_SDR_CTRL_S00_AXI_SLV_REG1_OFFSET 4
#define AXI_SDR_CTRL_S00_AXI_SLV_REG2_OFFSET 8
#define AXI_SDR_CTRL_S00_AXI_SLV_REG3_OFFSET 12
#define AXI_SDR_CTRL_S00_AXI_SLV_REG4_OFFSET 16
#define AXI_SDR_CTRL_S00_AXI_SLV_REG5_OFFSET 20
#define AXI_SDR_CTRL_S00_AXI_SLV_REG6_OFFSET 24
#define AXI_SDR_CTRL_S00_AXI_SLV_REG7_OFFSET 28
#define AXI_SDR_CTRL_S00_AXI_SLV_REG8_OFFSET 32



#define SET_REPLAY_LEN                  AXI_SDR_CTRL_S00_AXI_SLV_REG0_OFFSET
#define SET_REPLAY_START                AXI_SDR_CTRL_S00_AXI_SLV_REG1_OFFSET
#define SET_REPLAY_STOP                 AXI_SDR_CTRL_S00_AXI_SLV_REG2_OFFSET
#define SET_TRANSFERED_LEN_RPLY         AXI_SDR_CTRL_S00_AXI_SLV_REG3_OFFSET
#define SET_CAPTURE_LEN                 AXI_SDR_CTRL_S00_AXI_SLV_REG4_OFFSET
#define SET_DMA_BLOCK_SIZE              AXI_SDR_CTRL_S00_AXI_SLV_REG5_OFFSET
#define SET_CAPTURE_START               AXI_SDR_CTRL_S00_AXI_SLV_REG6_OFFSET
#define SET_DMA_IS_REDAY                AXI_SDR_CTRL_S00_AXI_SLV_REG7_OFFSET
#define SET_TRANSFERED_LEN_SAMP         AXI_SDR_CTRL_S00_AXI_SLV_REG8_OFFSET


typedef struct axi_sdr_ctrl
{
    uint32_t mem_start;
    int32_t fd_axi;
    uint32_t mem_len;
    volatile uint32_t *map_base;
} axi_sdr_ctrl;


int axi_sdr_ctrl_init(axi_sdr_ctrl * device, uint32_t base_addr, uint32_t reg_len);
void axi_sdr_ctrl_exit(axi_sdr_ctrl * device);
uint32_t axi_sdr_ctrl_peek(axi_sdr_ctrl * device, uint32_t reg);
int32_t axi_sdr_ctrl_poke(axi_sdr_ctrl * device, uint32_t reg, uint32_t val);

int32_t axi_sdr_ctrl_set_tx_source(axi_sdr_ctrl * device, uint32_t val);

int32_t axi_sdr_ctrl_set_noise_cfg(axi_sdr_ctrl * device, uint32_t idx_start, uint32_t idx_stop);

int32_t axi_sdr_ctrl_set_tx_dds_freq(axi_sdr_ctrl * device, uint32_t sample_rate, int32_t freq);

int32_t axi_sdr_ctrl_set_enable_tx_mixer(axi_sdr_ctrl * device, uint8_t enable);

int32_t axi_sdr_ctrl_set_tx_mixer_freq(axi_sdr_ctrl * device, uint32_t sample_rate, int32_t freq);

int32_t axi_sdr_ctrl_set_tx_replay_len(axi_sdr_ctrl * device, uint32_t byte_len);
int32_t axi_sdr_ctrl_set_tx_replay_start(axi_sdr_ctrl * device );
int32_t axi_sdr_ctrl_set_tx_replay_stop(axi_sdr_ctrl * device );

int32_t axi_sdr_ctrl_set_rx_sample_buffer_len(axi_sdr_ctrl * device, uint32_t byte_len);
int32_t axi_sdr_ctrl_set_rx_sample_buffer_start(axi_sdr_ctrl * device );
int32_t axi_sdr_ctrl_set_rx_sample_buffers_stop(axi_sdr_ctrl * device );

uint32_t axi_sdr_ctrl_transfered_len_replay(axi_sdr_ctrl * device );
uint32_t axi_sdr_ctrl_dma_is_reday(axi_sdr_ctrl * device);
uint32_t axi_sdr_ctrl_transfered_len_sample(axi_sdr_ctrl * device);
uint32_t axi_sdr_set_dma_blocks_size(axi_sdr_ctrl *device);
#endif