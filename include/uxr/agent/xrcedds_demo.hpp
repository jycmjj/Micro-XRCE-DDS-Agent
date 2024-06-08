#ifndef XRCE_QUEUE_HPP_
#define XRCE_QUEUE_HPP_
#include <ucdr/microcdr.h>
#include <atomic>
#include <memory>
#include <vector>
typedef struct student
{
  int32_t gradle;
  std::string hobby[3];
  std::string namee;
  int32_t number;
  void des(ucdrBuffer ucr)
  {
    char temp[255];
    if (ucdr_deserialize_string(&ucr, temp, 255))
    {
      namee = std::string(temp);
    }
    ucdr_deserialize_int32_t(&ucr, &number);
    ucdr_deserialize_int32_t(&ucr, &gradle);

    for (int i = 0; i < 3; i++)
    {

      if (ucdr_deserialize_string(&ucr, temp, 255))
      {
        hobby[i] = std::string(temp);
      }
      else
      {
        throw std::runtime_error("反序列化失败");
      }
    }
  }
};

// 单入单出 无锁队列

template <typename Type>

class XRCEQueue
{

public:
  XRCEQueue();

  ~XRCEQueue();

  XRCEQueue(const XRCEQueue &other) = delete;

  XRCEQueue &operator=(const XRCEQueue &other) = delete;

  bool IsEmpty() const;

  void Push(const Type &data);

  std::shared_ptr<Type> Pop();

private:
  struct LockFreeXrceNode
  {

    // std::make_shared does not throw an exception.

    LockFreeXrceNode() : data(nullptr), next(nullptr) {}

    std::shared_ptr<Type> data;

    LockFreeXrceNode *next;
  };

private:
  LockFreeXrceNode *PopHead();

private:
  std::atomic<LockFreeXrceNode *> head_;

  std::atomic<LockFreeXrceNode *> tail_;
};

template <typename Type>

XRCEQueue<Type>::XRCEQueue() : head_(new LockFreeXrceNode), tail_(head_.load()){};

template <typename Type>

XRCEQueue<Type>::~XRCEQueue()
{
  while (LockFreeXrceNode *old_head = head_.load())
  {
    head_.store(old_head->next);
    delete old_head;
  }
};

template <typename Type>

bool XRCEQueue<Type>::IsEmpty() const
{

  return head_.load() == tail_.load();
}

template <typename Type>

void XRCEQueue<Type>::Push(const Type &data)
{
  auto new_data = std::make_shared<Type>(data);
  LockFreeXrceNode *p = new LockFreeXrceNode; // 3
  LockFreeXrceNode *old_tail = tail_.load(); // 4
  old_tail->data.swap(new_data); // 5
  old_tail->next = p; // 6
  tail_.store(p); // 7
}
template <typename Type>
std::shared_ptr<Type> XRCEQueue<Type>::Pop()
{

  LockFreeXrceNode *old_head = PopHead();
  if (old_head == nullptr)
  {
    return std::shared_ptr<Type>();
  }
  const std::shared_ptr<Type> res(old_head->data); // 2
  delete old_head;
  return res;
}
template <typename Type>
typename XRCEQueue<Type>::LockFreeXrceNode *XRCEQueue<Type>::PopHead()
{
  LockFreeXrceNode *old_head = head_.load();
  if (old_head == tail_.load())
  { // 1
    return nullptr;
  }
  head_.store(old_head->next);
  return old_head;
}
// 全局队列 单例模式
class Agent2CentorQueue
{
public:
  static Agent2CentorQueue &instance()
  {
    static Agent2CentorQueue _imp;
    return _imp;
  };
  XRCEQueue<student> center_read_queue;
private:
  // 将其构造和析构成为私有的, 禁止外部构造和析构
  Agent2CentorQueue() = default;
  ~Agent2CentorQueue() = default;
  // 将其拷贝构造和赋值构造成为私有函数, 禁止外部拷贝和赋值
  Agent2CentorQueue(const Agent2CentorQueue &Agent2CentorQueue);
  const Agent2CentorQueue &operator=(const Agent2CentorQueue &Agent2CentorQueue);
};
class Center2AgentQueue
{
public:
  // 获取单实例
  static Center2AgentQueue &instance()
  {
    static Center2AgentQueue _imp;
    return _imp;
  };
  XRCEQueue<student> center_write_queue;
private:
  // 将其构造和析构成为私有的, 禁止外部构造和析构
  Center2AgentQueue() = default;

  ~Center2AgentQueue() = default;

  // 将其拷贝构造和赋值构造成为私有函数, 禁止外部拷贝和赋值

  Center2AgentQueue(const Center2AgentQueue &Center2AgentQueue);

  const Center2AgentQueue &operator=(const Center2AgentQueue &Center2AgentQueue);
};

#endif /* XRCE_QUEUE_HPP_ */
