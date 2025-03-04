#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include <stddef.h> 

#pragma pack(push, 1)
struct SharedData {
    sem_t sem;
    int num_entries;
    char data[1024];
};
#pragma pack(pop)

int main() {
    // 打开共享内存
    int shm_fd = shm_open("/config_shm", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open 失败");
        return 1;
    }else {
        std::cout << "成功打开共享内存文件描述符: " << shm_fd << std::endl;
    }

    static_assert(sizeof(SharedData) == sizeof(sem_t) + sizeof(int) + 1024, "结构体大小不匹配");

    // 映射共享内存
    SharedData *shm_data = (SharedData*)mmap(nullptr, sizeof(SharedData), 
                                           PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_data == MAP_FAILED) {
        perror("mmap 失败");
        return 1;
    }else {
        //std::cout << "共享内存映射地址: " << shm_data << std::endl;
        std::cout << "共享内存映射地址有效性验证: "
              << (shm_data != nullptr ? "有效" : "无效") << std::endl;
    }
    
    std::cout << "信号量地址: " << &shm_data->sem << std::endl;

    // 读取数据 (带信号量保护)
    //sem_wait(&shm_data->sem);
    if (sem_wait(&shm_data->sem) == -1) {
        std::cout<<"error and this faild"<<std::endl;
        perror("sem_wait 失败");
        return 1;
    }

    std::cout << "获取信号量成功" << std::endl;
    std::cout << "条目数: " << shm_data->num_entries << std::endl;
    std::cout << "数据内容: " << shm_data->data << std::endl;
    sem_post(&shm_data->sem);

    // 清理资源
    munmap(shm_data, sizeof(SharedData));
    close(shm_fd);

    return 0;
}
