#include <iostream>
#include <sqlite3.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include <stddef.h>

#pragma pack(push, 1)
// 共享内存数据结构
struct SharedData {
    sem_t sem;                  // 信号量用于同步
    int num_entries;            // 数据条目数
    char data[1024];            // 存储实际数据 (格式: key1=value1;key2=value2;...)
};
#pragma pack(pop)

int main() {
    sqlite3 *db;
    char *err_msg = nullptr;

    // 打开数据库
    if (sqlite3_open("config.db", &db) != SQLITE_OK) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    static_assert(sizeof(SharedData) == sizeof(sem_t) + sizeof(int) + 1024, "结构体大小不匹配");

    // 创建或打开共享内存
    int shm_fd = shm_open("/config_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open 失败");
        return 1;
    }
    std::cout << "共享内存结构体大小: " << sizeof(SharedData) << std::endl;
    // 调整共享内存大小
    ftruncate(shm_fd, sizeof(SharedData));

    // 映射共享内存
    SharedData *shm_data = (SharedData*)mmap(nullptr, sizeof(SharedData), 
                                           PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_data == MAP_FAILED) {
        perror("mmap 失败");
        return 1;
    }

    // 初始化信号量
    //sem_init(&shm_data->sem, 1, 1); // 进程间共享信号量
    if (sem_init(&shm_data->sem, 1, 1) == -1) {
        perror("sem_init 失败");
        return 1;
    } else {
        //std::cout << "信号量初始化成功" << std::endl;
        std::cout << "信号量初始化成功，地址: " << &shm_data->sem << std::endl;
    }

    // 从数据库读取数据
    std::string query = "SELECT key, value FROM config_data;";
    std::string data_str;

    if (sqlite3_exec(db, query.c_str(), [](void *data, int argc, char **argv, char **col) -> int {
        std::string *str = (std::string*)data;
        *str += std::string(argv[0]) + "=" + argv[1] + ";";
        return 0;
    }, &data_str, &err_msg) != SQLITE_OK) {
        std::cerr << "数据库查询失败: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return 1;
    }

    // 写入共享内存 (带信号量保护)
    sem_wait(&shm_data->sem);
    shm_data->num_entries = 3; // 实际应根据查询结果动态计算
    strncpy(shm_data->data, data_str.c_str(), sizeof(shm_data->data) - 1);
    shm_data->data[sizeof(shm_data->data) - 1] = '\0'; // 确保终止符
    sem_post(&shm_data->sem);

    std::cout << "数据已写入共享内存!" << std::endl;

    // 清理资源
    sqlite3_close(db);
    munmap(shm_data, sizeof(SharedData));
    close(shm_fd);

    return 0;
}
