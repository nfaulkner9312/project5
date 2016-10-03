#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);
void get_arg (struct intr_frame *f, int *arg, int n);
int write(int fd, const void *buffer, unsigned size);

void syscall_init (void) {
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f UNUSED) {
  
    int arg[3]; 
    
    get_arg(f, &arg[0], 3);
    /*int i = 0;
    for (i = 0; i< 3; i++) {
        printf("arg_%d = %d\n", i, arg[i]);
    }
    */
    /*
    int *arg_ptr = (int*)f->esp;
    printf ("system call = %d\n", *arg_ptr);
    */ 
    switch(*(int*)f->esp) {
        case SYS_HALT: {
            break;
        } case SYS_EXIT: {
            struct thread *cur = thread_current();
            printf ("%s: exit(%d)\n", cur->name, arg[0]);
            thread_exit();
            break;
        } case SYS_EXEC: {
            break;
        } case SYS_WAIT: {
            break;
        } case SYS_CREATE: {
            break;
        } case SYS_REMOVE: {
            break;
        } case SYS_OPEN: {
            break;
        } case SYS_FILESIZE: {
            break;
        } case SYS_READ: {
            break;
        } case SYS_WRITE: {
            const void* buf = pagedir_get_page(thread_current()->pagedir, (const void *)arg[1]);
            f->eax = write(arg[0], buf, (unsigned) arg[2]);
            /*printf("done writing\n");*/
            break;
        } case SYS_SEEK: {
            break;
        } case SYS_TELL: {
            break;
        } case SYS_CLOSE: {
            break;
        }
    }

    /*thread_exit ();*/
}

int write(int fd, const void *buffer, unsigned size) {
    /* TODO: synchronization */
    if (fd == STDOUT_FILENO) {
        putbuf(buffer, size);
        return size;
    } else {
        struct thread* cur = thread_current();
        int ret = file_write(cur->fd_list[fd], buffer, size);
        return ret;
    }
}

void get_arg (struct intr_frame *f, int *arg, int n){
    int i;
    int *ptr;
    for (i = 0; i < n; i++) {
        ptr = (int *) f->esp + i + 1;
        arg[i] = *ptr;
    }
}


