
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h> 		//inet_addr
#include <unistd.h>    		//write
#include <time.h>
#include <sys/ioctl.h>		/* ioctl */
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/select.h>
#include "my-axi-dma-driver.h"
#include "axi_dma_control.h"
#include "axi_sdr_ctrl.h"
#include "no_os_gpio.h"
#include "linux_gpio.h"

#define DATA_SIZE (32768 * 4)
#define DRONEID_DMA_SIZE 1024*1024*4
#define TEST_CAPTURE_LEN 1024*1024*16
#define DMA_BLOCK_SIZE DRONEID_DMA_SIZE
#define PORT 12345
#define PSMIO20_BUTTON_GPIO_NUM 920  // psmio20按键编号
#define DATA_SIZE_512M (512 * 1024 * 1024)  // 512M数据大小

volatile int running = 1;
volatile int button_pressed = 0;  // 按键按下标志

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    running = 0;
}

// 按键检测函数
int check_button_pressed(struct no_os_gpio_desc *button_gpio) {
    uint8_t button_value;
    int ret = no_os_gpio_get_value(button_gpio, &button_value);
    if (ret == 0) {
        return (button_value == NO_OS_GPIO_LOW);
    } else {
        printf("Failed to read button value, error: %d\n", ret);
    }
    return 0;
}
int32_t replay_one_block_data(int sock_fd, axi_dma_control *device, char* rx_buffer)
{
	uint64_t command=0;
	uint32_t one_block_sent = 0;
	uint32_t dma_mm2s_status = 0;
	size_t received = recv(sock_fd, &command, sizeof(command), 0);
	if (received < 0)
	{
		printf("recv error\r\n");
		return -1;
	}
	
	printf("command:%llx\r\n", command);
	if ((command >> 32) == 0x5555550A){
		
		while (one_block_sent < DRONEID_DMA_SIZE)
		{
			received = recv(sock_fd, rx_buffer+one_block_sent, 4096, 0);
			//printf("one_block_sent:%d\r\n", one_block_sent);
			one_block_sent += received;
		}

		memcpy(device->mm2s_buffers, rx_buffer, DRONEID_DMA_SIZE);
		int dma_transfer_status = axi_dma_control_mm2s_transfer(device , BLOCK_SIZE_ONE_BURST);
		if(dma_transfer_status == 0 )
		{
			return one_block_sent;
		}
		else
		{
			return -1;
		}
	}
}

int main(int argc, char **argv)
{
    printf("Hello World!\n");
    unsigned int ret_val = 0; 
	unsigned char my_axi_dma_driver_node[128] = "/dev/my-axi-dma-driver_00";

	uint32_t sample_transfered = 0;
	

	int socket_desc = 0, client_sock = 0, c = 0, read_size = 0;
	struct sockaddr_in server , client;

    axi_dma_control axi_dma_dev;
	axi_sdr_ctrl axi_sdr_dev;
	struct no_os_gpio_desc *button_gpio = NULL;
	
	/* GPIO初始化 - psmio20按键 */
	struct no_os_gpio_init_param button_gpio_param = {
		.number = PSMIO20_BUTTON_GPIO_NUM,
		.platform_ops = &linux_gpio_ops,
		.pull = NO_OS_PULL_UP,  // 上拉电阻
	};
	
	if (no_os_gpio_get(&button_gpio, &button_gpio_param) != 0) {
		printf("Failed to initialize psmio20 button GPIO\n");
	} else {
		if (no_os_gpio_direction_input(button_gpio) != 0) {
			printf("Failed to set psmio20 button as input\n");
		} else {
			printf("psmio20 button GPIO initialized successfully\n");
		}
	}
	
	/* axi dma module init */


	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);
    printf("Press Ctrl+C to exit gracefully.\n");

    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

	axi_dma_control_init(&axi_dma_dev ,my_axi_dma_driver_node );
	axi_sdr_ctrl_init(&axi_sdr_dev , AXI_SDR_CTRL_BASE_ADDR ,AXI_SDR_CTRL_RANGE);
	//ioctl ( my_axil_sample_buffer_fd, IOCTL_SET_DMA_BLOCK_SIZE, DMA_BLOCK_SIZE);


	struct sockaddr_in client_addr;
	uint32_t length;
	uint32_t total_received = 0;
	uint64_t ack = 0x5555550300000000; // Acknowledgment
	socklen_t client_len = sizeof(client_addr);


	char *rx_buffer = malloc(DMA_BLOCK_SIZE);
    if (!rx_buffer) {
        printf("Memory allocation failed");
    }

	printf("Server is ready. Waiting for connections and button presses...\n");
	while (running) {
		// Use select to make accept non-blocking for graceful shutdown
		fd_set read_fds;
		struct timeval timeout;
		
		FD_ZERO(&read_fds);
		FD_SET(server_socket, &read_fds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;  // 100ms timeout for responsive button checking
		
		int select_result = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);
		
		if (select_result < 0) {
			if (errno != EINTR) {
				perror("select failed");
			}
			continue;
		}
		
		if (select_result == 0) {
			// Timeout, continue to check button
			continue;
		}
		
		int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
		if (client_socket < 0) {
			perror("Accept failed");
			continue;
		}

        uint64_t command;
		ssize_t received = recv(client_socket, &command, sizeof(command), 0);
		if (received <= 0) {
			perror("Failed to receive data");
			close(client_socket);
		}

		if ((command >> 32) == 0x55555501) {
			length = command & 0xFFFFFFFF;
			printf("Received collection length: %u bytes\n", length);
			
			send(client_socket, &ack, sizeof(ack), 0);
			received = recv(client_socket, &command, sizeof(command), 0);
			if ((command >> 32) == 0x55555502) {
				printf("Received start command\n");
				axi_sdr_ctrl_set_rx_sample_buffer_len(&axi_sdr_dev,length);
				usleep(100);
				axi_sdr_ctrl_set_rx_sample_buffer_start(&axi_sdr_dev);
				
				usleep(3000000);
				while(sample_transfered != length ){
					uint32_t dram_is_ready =  axi_sdr_ctrl_dma_is_reday(&axi_sdr_dev);
					
					//ioctl(my_axil_sample_buffer_fd, IOCTL_GET_BUFFER_STATUS, &dram_is_ready);
					axi_sdr_set_dma_blocks_size(&axi_sdr_dev);
					if (dram_is_ready > 0) {
						axi_dma_control_s2mm_transfer(&axi_dma_dev,DRONEID_DMA_SIZE);
						send(client_socket, axi_dma_dev.s2mm_buffers, DMA_BLOCK_SIZE, 0);
						sample_transfered = axi_sdr_ctrl_transfered_len_sample(&axi_sdr_dev);
					} 
					printf("sample is %d \n ",sample_transfered);
				}
				printf("Data transmission complete\n");
				sample_transfered=0;
			}
			close(client_socket);
		} else if( (command >> 32) == 0x55555510 ){
			length = command & 0xFFFFFFFF;
			printf("key Received collection length: %u bytes\n", length);
			axi_sdr_ctrl_set_rx_sample_buffer_len(&axi_sdr_dev,length);

			while(!check_button_pressed(button_gpio) )
			{
			}
			usleep(10);
			axi_sdr_ctrl_set_rx_sample_buffer_start(&axi_sdr_dev);
			send(client_socket, &ack, sizeof(ack), 0);

			while(sample_transfered != length ){
					uint32_t dram_is_ready =  axi_sdr_ctrl_dma_is_reday(&axi_sdr_dev);
					
					//ioctl(my_axil_sample_buffer_fd, IOCTL_GET_BUFFER_STATUS, &dram_is_ready);
					axi_sdr_set_dma_blocks_size(&axi_sdr_dev);
					if (dram_is_ready > 0) {
						axi_dma_control_s2mm_transfer(&axi_dma_dev,DRONEID_DMA_SIZE);
						send(client_socket, axi_dma_dev.s2mm_buffers, DMA_BLOCK_SIZE, 0);
						sample_transfered = axi_sdr_ctrl_transfered_len_sample(&axi_sdr_dev);
					} 
					printf("sample is %d \n ",sample_transfered);
				}
			sample_transfered = 0 ;
			close(client_socket);

		} else if ((command >> 32) == 0x55555504) { // set replay length command
			length = command & 0xFFFFFFFF;
			printf("Received Relpay length: %u bytes\n", length);
			send(client_socket, &ack, sizeof(ack), 0);
			axi_sdr_ctrl_set_tx_replay_len(&axi_sdr_dev,length);
			axi_sdr_ctrl_set_tx_replay_start(&axi_sdr_dev);
			uint32_t one_block_sent=0;
			while (total_received < length) {
				//if(length - total_received > DRONEID_DMA_SIZE)
				received = replay_one_block_data(client_socket, &axi_dma_dev, rx_buffer);
				if(received < 0 )
				{
					printf("replay one block data filed\n");
				}
				total_received += received;
				printf("Received %lu/%u bytes\n", total_received, length);

				if (send(client_socket, &ack, sizeof(ack), 0) < 0) {
					printf( "Failed to send chunk ACK\n");
					break;
				}
			}
			printf("Data transmission complete\n");
			total_received=0;
			close(client_socket);
		} else if ((command >> 32) == 0x55555505) { // stop replay command
			printf("Received Relpay stop command\n");
			axi_sdr_ctrl_set_tx_replay_stop(&axi_sdr_dev);
			if (send(client_socket, &ack, sizeof(ack), 0) < 0) {
				printf( "Failed to send chunk ACK\n");
			}
			printf("stop relpay\n");
			close(client_socket);
		} else if ((command >> 32) == 0x55555506 ) { // get the replay status
			printf("Received Get replay status command\n");
			uint32_t sample_replayed = axi_sdr_ctrl_transfered_len_replay(&axi_sdr_dev);
			printf("dma transfer done, sample_replayed:%d \r\n", sample_replayed);
			uint64_t status_ack = ((uint64_t)0x55555507<<32 |sample_replayed ); // Send collection length
			send(client_socket, &status_ack, sizeof(status_ack), 0);
			close(client_socket);
		} 	
    }
    close(server_socket);

	printf("all samples transfer done\r\n");
	free(rx_buffer);
	axi_dma_control_exit(&axi_dma_dev);
	axi_sdr_ctrl_exit(&axi_sdr_dev);
	
	// 清理GPIO资源
	if (button_gpio) {
		no_os_gpio_remove(button_gpio);
	}
	
    return 0;
}
