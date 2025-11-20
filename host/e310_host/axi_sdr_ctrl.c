#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "axi_sdr_ctrl.h"
#include <math.h>

int axi_sdr_ctrl_init(axi_sdr_ctrl * device, uint32_t base_addr, uint32_t reg_len){
    int32_t fd_axi = open("/dev/mem",O_RDWR | O_SYNC);
    uint32_t * map_base;
    if(fd_axi < 0)
    {
        perror("open /dev/mem failed");
        return -1;
    }
    map_base = (volatile uint32_t*)mmap(NULL, reg_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_axi, base_addr);
    if (map_base == MAP_FAILED) {
        perror("mmap failed");
        close(fd_axi);
        return -1;
    }
    device->fd_axi = fd_axi;
    device->mem_len = reg_len;
    device->mem_start = base_addr;
    device->map_base = map_base;

    return 0;
}

void axi_sdr_ctrl_exit(axi_sdr_ctrl * device){
    if (munmap((void *)device->map_base, device->mem_len) == -1) {
        perror("munmap 失败failed");
    }
    close(device->fd_axi);
}


uint32_t axi_sdr_ctrl_peek(axi_sdr_ctrl * device, uint32_t reg){
    uint32_t value = *(device->map_base + reg / 4);
    return value;
}


int32_t axi_sdr_ctrl_poke(axi_sdr_ctrl * device, uint32_t reg, uint32_t val){
    *(device->map_base + reg / 4) = val;

    uint32_t values = axi_sdr_ctrl_peek(device, reg);
    return 0;
}


int32_t axi_sdr_ctrl_set_tx_replay_len(axi_sdr_ctrl * device, uint32_t byte_len){

    axi_sdr_ctrl_poke(device, SET_REPLAY_LEN, byte_len);
    return 0;
}


int32_t axi_sdr_ctrl_set_tx_replay_start(axi_sdr_ctrl * device ){

    axi_sdr_ctrl_poke(device, SET_REPLAY_START, 1);
    usleep(1);
    axi_sdr_ctrl_poke(device, SET_REPLAY_START, 0);

    return 0;
}

int32_t axi_sdr_ctrl_set_tx_replay_stop(axi_sdr_ctrl * device ){
    axi_sdr_ctrl_poke(device, SET_REPLAY_STOP, 1);
    usleep(1);
    axi_sdr_ctrl_poke(device, SET_REPLAY_STOP, 0);

    return 0;
}

int32_t axi_sdr_ctrl_set_rx_sample_buffer_len(axi_sdr_ctrl * device, uint32_t byte_len){
printf("0\n");
    axi_sdr_ctrl_poke(device, SET_CAPTURE_LEN, byte_len);
    printf("2\n");
    return 0;
}


int32_t axi_sdr_ctrl_set_rx_sample_buffer_start(axi_sdr_ctrl * device ){

    axi_sdr_ctrl_poke(device, SET_CAPTURE_START, 1);
    usleep(1);
    axi_sdr_ctrl_poke(device, SET_CAPTURE_START, 0);

    return 0;
}

int32_t axi_sdr_ctrl_set_rx_sample_buffers_stop(axi_sdr_ctrl * device ){
    axi_sdr_ctrl_poke(device, SET_REPLAY_STOP, 1);
    usleep(1);
    axi_sdr_ctrl_poke(device, SET_REPLAY_STOP, 0);

    return 0;
}

uint32_t axi_sdr_ctrl_transfered_len_replay(axi_sdr_ctrl * device )
{
    uint32_t len =  axi_sdr_ctrl_peek(device, SET_TRANSFERED_LEN_RPLY);
    return len;
}
uint32_t axi_sdr_ctrl_dma_is_reday(axi_sdr_ctrl * device)
{
    uint32_t dmais_ready = axi_sdr_ctrl_peek(device, SET_DMA_IS_REDAY);
    return dmais_ready;
}
uint32_t axi_sdr_ctrl_transfered_len_sample(axi_sdr_ctrl * device)
{
    uint32_t len = axi_sdr_ctrl_peek(device, SET_TRANSFERED_LEN_SAMP);
    return len;
}
uint32_t axi_sdr_set_dma_blocks_size(axi_sdr_ctrl *device)
{
    axi_sdr_ctrl_poke(device,SET_DMA_BLOCK_SIZE, 1024*1024*4);
}