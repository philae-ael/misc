#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

struct pool {
  size_t block_size;

  struct freelist {
    freelist *next;
  };

  // tail is null iff start is **considered** null
  freelist *start = nullptr;
  freelist *tail = nullptr;

  void *allocate() {
    if (tail == nullptr) {
      return malloc(block_size);
    }

    void *out = tail;
    tail = tail->next;
    return out;
  }
  void deallocate(void *block) {
    auto l = (freelist *)block;
    l->next = nullptr;

    if (tail == nullptr) {
      tail = start = l;
    } else {
      start->next = l;
    }
  }
};

struct skiplist {
  struct node_head {
    int data;
    size_t size;
    node_head get(size_t level) {
      node_head *nexts =
          (node_head *)((unsigned char *)this + sizeof(node_head));

      assert(level < size);
      return nexts[level];
    }
  };

  std::vector<pool> pools;
  node_head *lists;
  std::vector<int> *v;

  void insert(int i) {
    size_t level = 1;
    while (rand() % 2 == 0) {
      level++;
    }

    for (usize i = 0; i < level; i++) {
    }
  }

  bool find(int i) {
    size_t level = head->size;
    node_head *n = head;
    while () {
      if (i == n->data) {
        return
      }
      if (i > n->data) {
        level--;
      } else {
        n = n->next;
      }
    }
  }
};

int main() {
  void *p1, *p2, *p3, *p4, *p5, *p6;
  pool p{8};
  p1 = p.allocate();
  p2 = p.allocate();
  p3 = p.allocate();
  p.deallocate(p1);
  p.deallocate(p2);
  p4 = p.allocate();
  p5 = p.allocate();
  p6 = p.allocate();
  printf("%p %p %p %p %p %p", p1, p2, p3, p4, p5, p6);
}
