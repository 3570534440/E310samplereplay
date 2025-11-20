#include <iostream>
#include "antsdrDevice.h"
#include "unistd.h"
#include "vector"
#include "common.h"
#include "math_utils.h"
#include <fstream>
#define SAMPLES_BUFFER_KEY 1
#define REPLAT_DATA 0
#define SAMPLES_BUFFER 0
using namespace std;
#define SAMPLES 2000000
#define TOTAL_SIZE (1024 * 1024 * 512) // 512MB
#define CHUNK_SIZE (4 * 1024 * 1024) // 4MB
std::vector<int> x_data,recv1_i,recv1_q;
std::vector<int> recv2_i,recv2_q;
pthread_mutex_t lock;

CF32 iq_data[SAMPLES];

void get_rx_data(sdr_transfer *trans){
    pthread_mutex_lock(&lock);
    recv1_i.clear();
    recv1_q.clear();
    if(trans->channels == 2){
        recv2_i.clear();
        recv2_q.clear();
    }
    for(int i=0;i<trans->length / trans->channels;i++){
        recv1_i.push_back(trans->data[(2*trans->channels)*i]);
        recv1_q.push_back(trans->data[(2*trans->channels)*i+1]);
        iq_data[i].real(recv1_i[i]);
        iq_data[i].imag(recv1_q[i]);
        if(trans->channels == 2){
            recv2_i.push_back(trans->data[(2*trans->channels)*i+2]);
            recv2_q.push_back(trans->data[(2*trans->channels)*i+3]);
        }
    }
    pthread_mutex_unlock(&lock);
}

#include "/home/wu/work/board_test/e200_test_src/matplotlib-cpp/matplotlibcpp.h"

namespace plt = matplotlibcpp;
// 保存数据到.cs16文件

int main() {

    size_t bytes_read;
    size_t one_block_sent = 0;
    size_t total_sent = 0;
    antsdrDevice device;
    double gain = 65;
    
    if(device.open(true) < 0){
        printf("Failed to open device\n");
        return -1;
    }
    device.set_rx_samprate(61440000);
    device.set_tx_freq(5766.5e6);
    device.set_rx_freq(5766.5e6);
    device.set_rx_gain(gain, 1); // 1 ，2 ，3
    device.set_tx_samprate(61440000);
    device.set_tx_attenuation(20);
    
    //// 录制----
#if SAMPLES_BUFFER
    device.config_recorder_data(CHUNK_SIZE*32);
    FILE *file = fopen("recorder_data.cs16", "wb");
    if (!file) {
        perror("Failed to open file for writing");
        device.release_socket();
        return -1;
    }
    device.recorder_data(file, CHUNK_SIZE*32);
    fclose(file);
    device.release_socket();
#endif
//回放--------------------------------------
#if REPLAT_DATA
    device.sample_stop_replay();
    device.release_socket();
    device.config_replay_data();
    
    FILE *file_r = fopen("recorder_data.cs16", "rb"); 
    if (!file_r) {
        perror("Failed to open file");
        device.release_socket();
        exit(EXIT_FAILURE); 
    }
    char *tx_buffer = (char*)malloc(CHUNK_SIZE);
    if (!tx_buffer) {
        printf("Failed to allocate memory for tx buffer\n");
        fclose(file_r);
        device.release_socket();
        exit(EXIT_FAILURE);
    }

    fseek(file_r, 0, SEEK_END);
    uint64_t file_size = ftell(file_r);
    rewind(file_r);
    int16_t blockcnt = file_size /CHUNK_SIZE;
    file_size= blockcnt*CHUNK_SIZE;
    printf("File size: %lu bytes\n", file_size);

    fread(tx_buffer, 1, 32, file_r);
    for (size_t i = 0; i < 32; i++)
    {
        printf("tx_buffer[%d]=%x\r\n", i, tx_buffer[i]);
    }
    rewind(file_r);
    device.send_replay_len(file_size);
    printf("Replay length sent: %lu bytes\n", file_size);
    while ((bytes_read = fread(tx_buffer, 1, CHUNK_SIZE, file_r)) > 0) {
        one_block_sent = device.send_once_block_data(tx_buffer);
        total_sent += one_block_sent;
        printf("total_sent %lu/%lu bytes\n", total_sent, file_size);
        // 如果返回0，可能表示发送失败
        if (one_block_sent == 0) {
            printf("Warning: send_once_block_data returned 0, this might indicate an error\n");
            break;
        }
        if(total_sent == file_size)
        {
            break;
        }
    }
    printf("All data sent, closing file and freeing buffer\n");
    fclose(file_r);
    free(tx_buffer);
    printf("Starting replay...\n");
    device.sample_start_replay();

#endif
//按键录制-----------------------------
#if SAMPLES_BUFFER_KEY
    device.config_recorder_data_key(TOTAL_SIZE);
    FILE *file = fopen("recorder_data.cs16", "wb");
    if (!file) {
        perror("Failed to open file for writing");
        device.release_socket();
        return -1;
    }
    device.recorder_data(file, TOTAL_SIZE);
    fclose(file);
    device.release_socket();
    printf("Replay completed successfully.\n");
//stop--------------------------------
#endif
     device.release_socket();
    while(1)
    {}
    fprintf(stdout,"Exit Program.\n");
return 0;
}
