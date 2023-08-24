#pragma once

#include <cstdint>

namespace libesp {
/*
 * @author cmdc0de
 * @date 8-23-2023
 *
 * @brief this is a fixed size allocator allowing us to use new and delete but get memory from a fixed
 * size array
 */
template<typename T, size_t N>
class FixedClassAllocator {
   public:
      struct Node {
         Node *next;
      };

      FixedClassAllocator() : StaticMemory() {
         Node *n = (Node *)StaticMemory;
         for (size_t i = 0; i < N - 1; i++) {
            n->next = (Node *)&StaticMemory[i + 1];
            n = n->next;
         }
         n->next = 0;
      }
      ~FixedClassAllocator() {}
      T *allocate() {
         T *t = (T *)StaticMemory;
         if(!t) return 0;
         StaticMemory = StaticMemory->next;
         return t;
      }
      void free(T *t) {
         Node *n = (Node *)t;
         n->next = StaticMemory;
         StaticMemory = n;
      }
   private:
      T StaticMemory[N];
};

}

