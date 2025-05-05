#include <deque>
#include <functional>
#include <random>
#include <map>
#include <userver/concurrent/impl/intrusive_stack.hpp>
#include "runtime/include/value_wrapper.h"
#include <runtime/include/verifying_macro.h>
#include <runtime/include/verifying.h>

static constexpr size_t SIZE = 100;

USERVER_NAMESPACE_BEGIN

struct BoxInt {
    BoxInt(int x) : x(x) {}

    concurrent::impl::SinglyLinkedHook<BoxInt> stack_hook;
    int x;
};

using BoxIntStack =
    concurrent::impl::IntrusiveStack<BoxInt, concurrent::impl::MemberHook<&BoxInt::stack_hook>>;
    
struct IntrusiveStack {
public:
    IntrusiveStack() {
        for (size_t i = 0; i < SIZE; ++i) {
            nodes.emplace_back(i);
        }
    }

    non_atomic void Push(size_t index) {
        stack.Push(nodes[index]);
    }

    non_atomic int TryPop() {
        auto res = stack.TryPop();
        if (res) {
            return res->x;
        } else {
            return -1;
        }
    }

private:
    std::deque<BoxInt> nodes{};
    BoxIntStack stack;
};

USERVER_NAMESPACE_END

namespace spec {
struct IntrusiveStackSpec {
    std::vector<int> values;
    std::deque<int> deq;
    IntrusiveStackSpec() {
        for (size_t i = 0; i < SIZE; ++i) {
            values.emplace_back(i);
        }
    }
    void Push(size_t index) {
        deq.push_back(values[index]);
    }

    int TryPop() {
        if (deq.empty()) {
            return -1;
        }
        int value = deq.back();
        deq.pop_back();
        return value;
    }

    using method_t = std::function<ValueWrapper(IntrusiveStackSpec *l, void *args)>;
    static auto GetMethods() {
      method_t push_func = [](IntrusiveStackSpec *l, void *args)  {
        auto real_args = reinterpret_cast<std::tuple<int> *>(args);
        l->Push(std::get<0>(*real_args));
        return void_v;
      };
  
      method_t pop_func = [](IntrusiveStackSpec *l, void *args) -> int { return l->TryPop(); };
  
      return std::map<std::string, method_t>{
          {"Push", push_func},
          {"TryPop", pop_func},
      };
    }
  };
  
  struct IntrusiveStackHash {
    size_t operator()(const IntrusiveStackSpec &r) const {
      int res = 0;
      for (int elem : r.deq) {
        res += elem;
      }
      return res;
    }
  };
  
  struct IntrusiveStackEquals {
    template <typename PushArgTuple, int ValueIndex>
    bool operator()(const IntrusiveStackSpec &lhs, const IntrusiveStackSpec &rhs) const {
      return lhs.deq == rhs.deq;
    }
  };
} // namespace spec
// Arguments generator.
static int a = 0;
auto generateInt(size_t unused_param) {
    if (a == SIZE) {
        a = 0;
    }
    return ltest::generators::makeSingleArg(a++);
}
  
// Specify target structure and it's sequential specification.
using spec_t =
    ltest::Spec<userver::IntrusiveStack, spec::IntrusiveStackSpec, spec::IntrusiveStackHash, spec::IntrusiveStackEquals>;

LTEST_ENTRYPOINT(spec_t);

// Targets.
target_method(generateInt, void, userver::IntrusiveStack, Push, int);
target_method(ltest::generators::genEmpty, int, userver::IntrusiveStack, TryPop);
