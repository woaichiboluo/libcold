#include "cold/coro/IoServicePool.h"

#include "third_party/fmt/include/fmt/format.h"

using namespace Cold;

Base::IoServicePool::IoServicePool(size_t poolSize, std::string nameArg)
    : poolSize_(poolSize), nameArg_(std::move(nameArg)) {
  if (nameArg_ == "") {
    nameArg_ = "Pool";
  }
  for (size_t i = 0; i < poolSize; ++i) {
    auto ioService = std::make_unique<IoService>();
    auto thread = std::make_unique<Thread>(
        [service = ioService.get()]() { service->Start(); },
        fmt::format("{}-{}", nameArg_, i + 1));
    threads_.push_back(std::move(thread));
    serviceVector_.push_back(std::move(ioService));
  }
}

Base::IoServicePool::~IoServicePool() {
  for (auto& thread : threads_) {
    if (thread->Started()) thread->Join();
  }
}

Base::IoService& Base::IoServicePool::GetMainIoService() {
  return mainIoService_;
}

Base::IoService& Base::IoServicePool::GetNextIoService() {
  if (serviceVector_.empty()) return mainIoService_;
  if (index_ >= serviceVector_.size()) index_ = 0;
  return *serviceVector_[index_++].get();
}

Base::IoService& Base::IoServicePool::GetIoServiceForHash(size_t hashCode) {
  if (serviceVector_.empty()) return mainIoService_;
  return *serviceVector_[hashCode % serviceVector_.size()].get();
}

void Base::IoServicePool::Start() {
  for (auto& thread : threads_) {
    thread->Start();
  }
  mainIoService_.Start();
}