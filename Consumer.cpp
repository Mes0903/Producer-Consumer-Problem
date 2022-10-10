/** @brief Create three Consumer Threads
 *  @date 2022/10/11
 *  @author Mes
 */

#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <iomanip>

constexpr int BUFFER_SIZE = 10;

struct Data {
  std::atomic<int> in = 0, out = 0;
  int buffer[BUFFER_SIZE];
};

void consume_data(Data *data, int pid, HANDLE *h_buf)
{
  // 拿一個 Empty Semaphore，如果沒了，代表 Buffer 空了，把 Consumer 鎖住
  WaitForSingleObject(h_buf[1], INFINITE);

  // 以下為 Critical section
  WaitForSingleObject(h_buf[2], INFINITE);

  // Consuming data
  std::cout << "Consumer " << pid << " now buying product, index is: " << data->out << " ,data is: " << data->buffer[data->out] << '\n';
  data->buffer[data->out] = 0;

  // 輸出
  for (const auto n : data->buffer)
    std::cout << "|" << std::setw(6) << n << " ";

  std::cout << "|\n";

  // 調整 index
  data->out = (data->out + 1) % BUFFER_SIZE;

  // 結束 Critical section
  ReleaseMutex(h_buf[2]);

  // 消耗了一個物品，將 Full Semaphore 減 1
  ReleaseSemaphore(h_buf[0], 1, NULL);
}

void Consumer(Data *data, int pid)
{
  // generate random device
  std::random_device rd;
  std::uniform_int_distribution<int> dist2(100, 500);    // 消費的時間在 100~500ms 間
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
    std::this_thread::sleep_for(std::chrono::milliseconds(dist2(gen)));    // 模擬消費一個東西花費的時間
    consume_data(data, pid, h_buf);
  }
}

int main()
{
  TCHAR memName[] = TEXT("shared_block");

  HANDLE hMap = OpenFileMapping(
    FILE_MAP_ALL_ACCESS,
    FALSE,
    memName);

  Data *pBuffer = static_cast<Data *>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0));
  int pid = 0;
  std::thread t1(Consumer, pBuffer, pid++);
  std::thread t2(Consumer, pBuffer, pid++);
  std::thread t3(Consumer, pBuffer, pid);

  t1.join();
  t2.join();
  t3.join();

  UnmapViewOfFile(pBuffer);
  CloseHandle(hMap);

  return 0;
}
