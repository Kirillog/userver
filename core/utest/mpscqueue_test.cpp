
#include <cassert>
#include <deque>
#include <map>
#include <optional>
#include "runtime/include/value_wrapper.h"
#include "runtime/include/verifying.h"
#include "userver/concurrent/impl/intrusive_hooks.hpp"
#include "userver/concurrent/impl/intrusive_mpsc_queue.hpp"
#include <runtime/include/verifying_macro.h>
#include <runtime/include/verifying.h>


static constexpr size_t SIZE = 100;

USERVER_NAMESPACE_BEGIN

struct Node final : public concurrent::impl::SinglyLinkedBaseHook {
    explicit Node(std::size_t x = 0) : x(x) {}

    std::size_t x{0};
};

using MpscQueue = concurrent::impl::IntrusiveMpscQueue<Node>;


struct IntrusiveMPSCQueue {
    public:
        IntrusiveMPSCQueue() {
            for (size_t i = 0; i < SIZE; ++i) {
                nodes.emplace_back(i);
            }
        }
    
        non_atomic void Push(size_t index) {
            queue.Push(nodes[index]);
        }
    
        non_atomic int TryPopBlocking() {
            auto res = queue.TryPopBlocking();
            if (res) {
                return res->x;
            } else {
                return -1;
            }
        }
    
    private:
        std::deque<Node> nodes{};
        MpscQueue queue;
    };
    
USERVER_NAMESPACE_END

namespace spec {
    struct IntrusiveMPSCQueue {
        std::vector<int> values;
        std::deque<int> deq;
        IntrusiveMPSCQueue() {
            for (size_t i = 0; i < SIZE; ++i) {
                values.emplace_back(i);
            }
        }
        void Push(size_t index) {
            deq.push_back(values[index]);
        }
    
        int TryPopBlocking() {
            if (deq.empty()) {
                return -1;
            }
            int value = deq.front();
            deq.pop_front();
            return value;
        }
    
        using method_t = std::function<ValueWrapper(IntrusiveMPSCQueue *l, void *args)>;
        static auto GetMethods() {
          method_t push_func = [](IntrusiveMPSCQueue *l, void *args) -> ValueWrapper {
            auto real_args = reinterpret_cast<std::tuple<int> *>(args);
            l->Push(std::get<0>(*real_args));
            return void_v;
          };
      
          method_t pop_func = [](IntrusiveMPSCQueue *l, void *args) -> int { return l->TryPopBlocking(); };
      
          return std::map<std::string, method_t>{
              {"Push", push_func},
              {"TryPopBlocking", pop_func},
          };
        }
      };
      
      struct IntrusiveMPSCQueueHash {
        size_t operator()(const IntrusiveMPSCQueue &r) const {
          int res = 0;
          for (int elem : r.deq) {
            res += elem;
          }
          return res;
        }
      };
      
struct IntrusiveMPSCQueueEquals {
template <typename PushArgTuple, int ValueIndex>
bool operator()(const IntrusiveMPSCQueue &lhs, const IntrusiveMPSCQueue &rhs) const {
    return lhs.deq == rhs.deq;
}
};
struct MPSCQueueVerifier {
    bool Verify(const std::string& task_name, size_t thread_id) {
        if (task_name == "Push") {
            return thread_id != 0;
        } else if (task_name == "TryPopBlocking") {
            receiver = thread_id == 0; 
            return thread_id == 0;
        } else {
            assert(false);
        }
    }

    void OnFinished(Task& task, size_t thread_id) {
        auto task_name = task->GetName();
        if (task_name == "Push") {
            return;
        } else if (task_name == "TryPopBlocking") {
            receiver = false;
            return;
        } else {
            assert(false);
        }
    }

    std::optional<std::string> ReleaseTask(size_t thread_id) {
        if (receiver > 0) {
            return {"Push"};
        }
        return std::nullopt;
    }

    bool receiver;
    };
} // namespace spec

static int a = 0;
auto generateInt(size_t unused_param) {
    if (a == SIZE) {
        a = 0;
    }
    return ltest::generators::makeSingleArg(a++);
}
    
using spec_t =
    ltest::Spec<userver::IntrusiveMPSCQueue, spec::IntrusiveMPSCQueue, spec::IntrusiveMPSCQueueHash, spec::IntrusiveMPSCQueueEquals>;

LTEST_ENTRYPOINT_CONSTRAINT(spec_t, spec::MPSCQueueVerifier);

// Targets.
target_method(generateInt, void, userver::IntrusiveMPSCQueue, Push, int);
target_method(ltest::generators::genEmpty, int, userver::IntrusiveMPSCQueue, TryPopBlocking);
