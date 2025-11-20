#ifndef _AXI_DMA_CONTROL_H_
#define _AXI_DMA_CONTROL_H_
#include "unistd.h"
#include "stdint.h"
#include "my-axi-dma-driver.h"

#define CUSTOME_XFFT_DMA_NODE "/dev/my-axi-dma-driver_00"
#define CUSTOME_IQ_DMA_NODE "/dev/my-axi-dma-driver_00"
// #define CUSTOME_DRONEID_DMA_NODE "/dev/my-axi-dma-driver_20"

#define BLOCK_SIZE_ONE_BURST 1024*1024*4

typedef struct axi_dma_control
{
    int32_t fd;
    volatile unsigned int *s2mm_buffers;
	volatile unsigned char *mm2s_buffers;
    uint32_t s2mm_len;
    uint32_t mm2s_len;
    int dma_s2mm_status;
	int dma_mm2s_status;
} axi_dma_control;


int axi_dma_control_init(axi_dma_control * device, char * node_path);
void axi_dma_control_exit(axi_dma_control * device);
int axi_dma_control_reset(axi_dma_control * device);
int axi_dma_control_s2mm_transfer(axi_dma_control * device, uint32_t len);
int axi_dma_control_mm2s_transfer(axi_dma_control * device, uint32_t len);

#endif