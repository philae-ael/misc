#include <errno.h>
#include <linux/io_uring.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/mman.h>

#include <sys/syscall.h>
#include <unistd.h>

noreturn void panic(const char *msg, ...) {
  fprintf(stderr, "PANIC: ");

  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);

  putc('\n', stderr);

  exit(1);
}

int io_uring_setup(unsigned entries, struct io_uring_params *p) {
  long int ret;
  ret = syscall((long)__NR_io_uring_setup, entries, p);
  return (ret < 0) ? -errno : ret;
}

int io_uring_enter(int ring_fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags) {
  int ret;
  ret = syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete, flags,
                NULL, 0);
  return (ret < 0) ? -errno : ret;
}

int ringfd;

_Atomic unsigned *sring_tail;
unsigned *sring_mask;
unsigned *sring_array;
struct io_uring_sqe *sqes;

_Atomic unsigned *cring_head;
unsigned *cring_tail;
unsigned *cring_mask;
struct io_uring_cqe *cqes;

off_t offset = 0;

char buff[4096];
int submit_(int fd, int op) {
  unsigned index, tail;
  tail = *sring_tail;
  index = tail & *sring_mask;
  struct io_uring_sqe *sqe = &sqes[index];

  sqe->opcode = op;
  sqe->fd = fd;
  sqe->addr = (unsigned long)buff;
  switch (op) {
  case IORING_OP_READ:
    sqe->len = sizeof(buff);
    sqe->off = 0;
    break;
  case IORING_OP_WRITE:
    sqe->len = strlen(buff);
    sqe->off = 0;
    break;
  default:
    panic("unsupported op %d", op);
  }
  sqe->off = offset;
  sring_array[index] = index;
  tail++;

  atomic_store_explicit(sring_tail, tail, memory_order_release);

  int ret = io_uring_enter(ringfd, 1, 1, IORING_ENTER_GETEVENTS);
  if (ret < 0)
    panic("io_uring_enter failed %s", strerror(-ret));

  return ret;
}

int read_() {
  struct io_uring_cqe *cqe;
  unsigned head;

  head = atomic_load_explicit(cring_head, memory_order_acquire);
  if (head == *cring_tail)
    return -1;

  cqe = &cqes[head & *cring_mask];
  if (cqe->res < 0)
    panic("cqe res error %s", strerror(-cqe->res));
  head++;
  atomic_store_explicit(cring_head, head, memory_order_release);

  return cqe->res;
}

int main() {
  struct io_uring_params p = {0};
  ringfd = io_uring_setup(8, &p);
  if (ringfd < 0)
    panic("io_uring_setup failed %s", strerror(-ringfd));

  int sring_sz = p.sq_off.array + p.sq_entries * sizeof(unsigned);
  int cring_sz = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);

  if (p.features & IORING_FEAT_SINGLE_MMAP) {
    if (sring_sz > cring_sz)
      cring_sz = sring_sz;
    else
      sring_sz = cring_sz;
  }

  void *sq_ptr = mmap(0, sring_sz, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE, ringfd, IORING_OFF_SQ_RING);

  if (sq_ptr == MAP_FAILED)
    panic("mmap sq_ring failed %s", strerror(errno));

  void *cq_ptr;
  if (p.features & IORING_FEAT_SINGLE_MMAP) {
    cq_ptr = sq_ptr;
  } else {
    cq_ptr = mmap(0, cring_sz, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_POPULATE, ringfd, IORING_OFF_CQ_RING);
    if (cq_ptr == MAP_FAILED)
      panic("mmap cq_ring failed %s", strerror(errno));
  }

  sring_tail = sq_ptr + p.sq_off.tail;
  sring_mask = sq_ptr + p.sq_off.ring_mask;
  sring_array = sq_ptr + p.sq_off.array;

  sqes = mmap(0, p.sq_entries * sizeof(struct io_uring_sqe),
              PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ringfd,
              IORING_OFF_SQES);
  if (sqes == MAP_FAILED)
    panic("mmap sqes failed %s", strerror(errno));

  cring_head = cq_ptr + p.cq_off.head;
  cring_tail = cq_ptr + p.cq_off.tail;
  cring_mask = cq_ptr + p.cq_off.ring_mask;
  cqes = cq_ptr + p.cq_off.cqes;

  while (1) {
    submit_(STDIN_FILENO, IORING_OP_READ);
    int n = read_();
    if (n > 0) {
      buff[n] = 0;
      fprintf(stderr, "read %d bytes: %s", n, buff);
      submit_(STDOUT_FILENO, IORING_OP_WRITE);
      read_();
      offset += n;
    } else if (n == 0) {
      fprintf(stderr, "EOF\n");
      break;
    } else if (n < 0) {
      panic("read_ failed");
    }
    offset += n;
  }

  return 0;
}
