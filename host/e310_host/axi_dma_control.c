
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>    		//write
#include <time.h>
#include <sys/time.h>  // for gettimeofday
#include <sys/ioctl.h>		/* ioctl */
#include <sys/sendfile.h>
#include <sys/mman.h>

#include "axi_dma_control.h"
#include "conversion.h"

#define TIMEOUT_S 0.1  // 超时时间设置为 3000 毫秒

int axi_dma_control_init(axi_dma_control * device, char * node_path){

    int fd = open (node_path, O_RDWR); 
    
    if (fd < 0) {
        printf ("user level: %s can not be opened.\n", node_path);
        exit(-1); 
    }
    device->fd = fd;
    // printf("----------------------------------------------------\r\n");
    // printf("open device node ok \r\n");

	usleep (10000); 
	int ret_val = ioctl (fd, IOCTL_MY_DMA_RESET, 1); 
    // printf("----------------------------------------------------\r\n");
    // printf(" ioctl IOCTL_MY_DMA_RESET ok \r\n");

	// configure dma engines
    ret_val = ioctl (fd, IOCTL_MY_DMA_SET_S2MM_TRANSFER_BLOCK_SIZE, BLOCK_SIZE_ONE_BURST); 
    ret_val = ioctl (fd, IOCTL_MY_DMA_SET_MM2S_TRANSFER_BLOCK_SIZE, BLOCK_SIZE_ONE_BURST); 
	ret_val = ioctl (fd, IOCTL_MY_DMA_SET_NUMBER_OF_BUFFERS_S2MM, 1); 
	ret_val = ioctl (fd, IOCTL_MY_DMA_SET_NUMBER_OF_BUFFERS_MM2S, 1); 
	// for all three dmas do the coherent memory allocation 
	// printf ("user level: dma allocate memory blocks...\n"); 
    ret_val = ioctl (fd, IOCTL_MY_DMA_ALLOCATE_COHERENT_MEMORY, 1); 
		
	usleep (10000); 

	//mmap for dma buffers
	// do mmap for s2mm 
    ret_val = ioctl (fd, IOCTL_MY_DMA_SET_S2MM_MMAP, 1); 
    
    // do it for all of the s2mm buffers 
        
	// set the buffer number for mmap to map 
	ret_val = ioctl (fd, IOCLT_MY_DMA_SET_BUFFER_NO_MMAP, 0); 
	
	// do the mmap operation 
	device->s2mm_buffers = (volatile unsigned int*) mmap (NULL, BLOCK_SIZE_ONE_BURST , PROT_READ | PROT_WRITE , MAP_SHARED, fd, 0);

	usleep (10000); 
	ioctl (fd, IOCTL_MY_DMA_SET_S2MM_MMAP, 0); 


	// do mmap for mm2s 
    ret_val = ioctl (fd, IOCTL_MY_DMA_SET_MM2S_MMAP, 1); 
        
	// set the buffer number for mmap to map 
	ret_val = ioctl (fd, IOCLT_MY_DMA_SET_BUFFER_NO_MMAP, 0); 
	
	// do the mmap operation 
	device->mm2s_buffers = (volatile unsigned char*) mmap (NULL, BLOCK_SIZE_ONE_BURST , PROT_READ | PROT_WRITE , MAP_SHARED, fd, 0);

	usleep (10000); 
	ioctl (fd, IOCTL_MY_DMA_SET_MM2S_MMAP, 0); 

	ioctl (fd, IOCTL_MY_DMA_RESET , 1); 
    ioctl ( device->fd, IOCTL_MY_DMA_SET_NUMBER_OF_BLOCKS_TO_S2MM, 1) ;
    ioctl ( device->fd, IOCTL_MY_DMA_SET_NUMBER_OF_BLOCKS_TO_MM2S, 1) ;
    device->dma_mm2s_status = 0;
    device->dma_s2mm_status = 0;
    device->mm2s_len = BLOCK_SIZE_ONE_BURST;
    device->s2mm_len = BLOCK_SIZE_ONE_BURST;

    return 0;
	// printf("dma set done! press enter to go on...\n");
}

int axi_dma_control_reset(axi_dma_control * device){
    int ret_val = ioctl (device->fd, IOCTL_MY_DMA_RESET, 1);
    if (ret_val < 0) {
        printf ("user level: %s axi_dma_control_reset failed.\n");
        exit(-1); 
    } 
    return 0;
}

void axi_dma_control_exit(axi_dma_control * device){
    int ret_val = ioctl (device->fd, IOCTL_MY_DMA_RELEASE_COHERENT_MEMORY, 1); 
    if (ret_val < 0) {
        printf ("user level: %s IOCTL_MY_DMA_RELEASE_COHERENT_MEMORY failed.\n");
        exit(-1); 
        close(device->fd);
    } 
    close(device->fd);
}

int axi_dma_control_s2mm_transfer(axi_dma_control * device, uint32_t len){
    int ret_val=0;


    if (len > BLOCK_SIZE_ONE_BURST){
        printf ("user level: IOCTL_MY_DMA_RELEASE_COHERENT_MEMORY failed.\n");
        len = BLOCK_SIZE_ONE_BURST;
    } else if (len != device->s2mm_len)
    {
        device->s2mm_len = len;
        ret_val = ioctl (device->fd, IOCTL_MY_DMA_SET_S2MM_TRANSFER_BLOCK_SIZE, len); 
    }

    // start a dma transfer
    // ret_val = ioctl ( device->fd, IOCTL_MY_DMA_SET_NUMBER_OF_BLOCKS_TO_S2MM, 1) ;

    ret_val = ioctl ( device->fd, IOCTL_MY_DMA_START_S2MM, 1 ); 
    struct timeval start, now;
    gettimeofday(&start, NULL);

    do
    {
        ioctl(device->fd, IOCTL_MY_DMA_GET_S2MM_STATUS, &device->dma_s2mm_status);
        gettimeofday(&now, NULL);
        double elapsed_s = TVAL_TO_SEC(now) - TVAL_TO_SEC(start);
        if (elapsed_s > TIMEOUT_S) {
            fprintf(stderr, "DMA S2MM transfer timeout after %f s\n", elapsed_s);
            return -1;
        }

    } while (device->dma_s2mm_status); 
    // printf("user level: dma_s2mm done...\n");
    return 0;
}

int axi_dma_control_mm2s_transfer(axi_dma_control * device, uint32_t len){
    int ret_val = 0;
    if (len > BLOCK_SIZE_ONE_BURST){
        printf ("user level: %s IOCTL_MY_DMA_RELEASE_COHERENT_MEMORY failed.\n");
        len = BLOCK_SIZE_ONE_BURST;
    } else if (len != device->mm2s_len)
    {
        device->mm2s_len = len;
        ret_val = ioctl (device->fd, IOCTL_MY_DMA_SET_MM2S_TRANSFER_BLOCK_SIZE, len); 
    }

    // ret_val = ioctl ( device->fd, IOCTL_MY_DMA_SET_NUMBER_OF_BLOCKS_TO_MM2S, 1 ) ;

    // printf("user level: start write operations ...\n");
    ret_val = ioctl ( device->fd, IOCTL_MY_DMA_START_MM2S, 1 ); 
    struct timeval start, now;
    gettimeofday(&start, NULL);
    do
    {
        ioctl(device->fd, IOCTL_MY_DMA_GET_MM2S_STATUS, &device->dma_mm2s_status);
        gettimeofday(&now, NULL);
        double elapsed_s = TVAL_TO_SEC(now) - TVAL_TO_SEC(start);
        if (elapsed_s > TIMEOUT_S) {
            fprintf(stderr, "DMA MM2S transfer timeout after %f s\n", elapsed_s);
            return -1;
        }

    } while (device->dma_mm2s_status);
    return 0;
}
