#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
extern struct lock write_lock;
void exit(int);

#endif /* userprog/syscall.h */
