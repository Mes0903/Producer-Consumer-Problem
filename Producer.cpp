/** @brief Create three Producer Threads
 *  @date 2022/10/11
 *  @author Mes
 */

#include <chrono>
#include <thread>
#include <windows.h>
#include <iostream>
#include <random>
#include <atomic>
#include <iomanip>

constexpr int BUFFER_SIZE = 10;

struct Data {
  std::atomic<int> in = 0, out = 0;
  int buffer[BUFFER_SIZE];
};

void make_data(Data *data, int num, int pid, HANDLE *h_buf)
{
  // 拿一個 Full Semaphore，如果沒了，代表 Buffer 滿了，把 Producer 鎖住
  WaitForSingleObject(h_buf[0], INFINITE);    // Full Semaphore

  // 以下為 Critical section
  WaitForSingleObject(h_buf[2], INFINITE);    // Producer lock

  // making product
  std::cout << "Producer " << pid << " now producing product, index is: " << data->in << " Data is: " << num << '\n';
  data->buffer[data->in] = num;

  // 輸出
  for (const auto n : data->buffer)
    std::cout << "|" << std::setw(6) << n << " ";

  std::cout << "|\n";

  // 調整 index
  data->in = (data->in + 1) % BUFFER_SIZE;

  // 結束 Critical section
  ReleaseMutex(h_buf[2]);    // Free lock

  // 製造了一個物品，將 Empty Semaphore 減 1
  ReleaseSemaphore(h_buf[1], 1, NULL);    // Signal Empty Semaphore
}

void Producer(Data *data, int pid)
{
  // generate random device
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 99999);    // 生產的數字在 0~99999 之間
  std::uniform_int_distribution<int> dist2(100, 500);    // 生產的時間在 100~500 ms 間
  std::mt19937 gen(rd());

  // generate mutex
  TCHAR __Full[] = TEXT("Full_sem");
  TCHAR __Empty[] = TEXT("Empty_sem");
  TCHAR __synmutex[] = TEXT("syn_mutex");

  HANDLE Full = CreateSemaphore(NULL, 10, BUFFER_SIZE, __Empty);
  HANDLE Empty = CreateSemaphore(NULL, 0, BUFFER_SIZE, __Full);
  HANDLE synmutex = CreateMutex(NULL, FALSE, __synmutex);
  HANDLE h_buf[] = { Full, Empty, synmutex };

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(dist2(gen)));    // 模擬生產一個東西要很久的時間

    int num = dist(gen);
    make_data(data, num, pid, h_buf);
  }
}

int main()
{
  TCHAR memName[] = TEXT("shared_block");

  HANDLE hMap = CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    sizeof(Data),
    memName);

  Data *pBuffer = static_cast<Data *>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0));
  int pid = 0;
  std::thread t1(Producer, pBuffer, pid++);
  std::thread t2(Producer, pBuffer, pid++);
  std::thread t3(Producer, pBuffer, pid);

  t1.join();
  t2.join();
  t3.join();

  UnmapViewOfFile(pBuffer);
  CloseHandle(hMap);

  return 0;
}
