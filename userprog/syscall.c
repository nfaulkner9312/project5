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

struct lock write_lock;
static void syscall_handler (struct intr_frame *);
void get_arg (struct intr_frame *f, int *arg, int n);
void is_valid_ptr(const void *ptr); 
void is_valid_buf(void *buf, int len);
int write(int fd, const void *buffer, unsigned size);
void is_valid_buf(void *buf, int len);
void is_valid_ptr(const void *ptr);

void halt(void);
void exit(int status);
int exec (const char* cmd_line);
int wait(int pid);/*pid_t replaced with int, unsure where 'pid_t' type is defined*/
bool create(const char* fileName, unsigned initial_size);
bool remove(const char* file);
int open(const char* file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

void syscall_init (void) {
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init(&write_lock);
}

static void syscall_handler (struct intr_frame *f UNUSED) {
  
    int arg[3];
    
    /* TODO check if stack pointer is out of bounds */
    is_valid_ptr(f->esp); 

    get_arg(f, &arg[0], 3);

    switch(*(int*)f->esp) {
	/*each argument is just the next thing on the stack after the syctemcall#, esp+1 esp+2 etc*/
        case SYS_HALT: {
            halt();
            break;
        } case SYS_EXIT: {
            exit(arg[0]);
            break;
        } case SYS_EXEC: {
            is_valid_ptr((const void *) arg[0]);
            const char* buf = pagedir_get_page(thread_current()->pagedir, (const void *)arg[0]);
            if (buf == NULL) {
                exit(-1);
            }
            f->eax = exec(buf);
            break;
        } case SYS_WAIT: {
            f->eax = process_wait(arg[0]);
            break;
        } case SYS_CREATE: {
            is_valid_ptr((const void *) arg[0]);
            f->eax=create((const char*)arg[0],(unsigned)arg[1]);
            break;
        } case SYS_REMOVE: {
            is_valid_ptr((const void *) arg[0]);
            f->eax=remove((const char*)arg[0]);
            break;
        } case SYS_OPEN: {
            is_valid_ptr((const void *) arg[0]);
		    f->eax = open((const char*)arg[0]);
            break;
        } case SYS_FILESIZE: {
            f->eax = filesize((int)arg[0]); 
            break;
        } case SYS_READ: {
            is_valid_buf((void *) arg[1], arg[2]);
            void* buf = pagedir_get_page(thread_current()->pagedir, (const void *)arg[1]);
            if (buf == NULL) {
                exit(-1);
            }
            f->eax = read(arg[0], buf, (unsigned) arg[2]);
            break;
        } case SYS_WRITE: {
            is_valid_buf((void *) arg[1], arg[2]);
            const void* buf = pagedir_get_page(thread_current()->pagedir, (const void *)arg[1]);
            if (buf == NULL) {
                exit(-1);
            }
            f->eax = write(arg[0], buf, (unsigned) arg[2]);
            break;
        } case SYS_SEEK: {
                seek(arg[0],arg[1]);
            break;
        } case SYS_TELL: {
            f->eax=tell(arg[0]);
            break;
        } case SYS_CLOSE: {
            close((int)arg[0]);
            break;
        }
	
    }
}

void is_valid_ptr(const void *ptr) {
    if (ptr == NULL) {
        exit(-1);
    }
    if (!is_user_vaddr(ptr)) {
        exit(-1); 
    }
}

void is_valid_buf(void *buf, int len) {
    if (buf == NULL) {
        exit(-1);
    }
    int i;
    char* tmp_buf = (char *) buf;
    for (i=0; i<len; i++) {
        is_valid_ptr( (const void *)tmp_buf );
        tmp_buf++;
    }
}

/* get arguments for the system call off of the stack */
void get_arg (struct intr_frame *f, int *arg, int n){
    int i;
    int *ptr;
    for (i = 0; i < n; i++) {
        ptr = (int *) f->esp + i + 1;
        arg[i] = *ptr;
    }
}


int write(int fd, const void *buffer, unsigned size) {
    /* TODO: synchronization */
    if (fd == STDOUT_FILENO) {
        putbuf(buffer, size);
        return size;
    } else if(fd == STDIN_FILENO) {
        return -1;
    } else {
        struct thread* cur=thread_current();
        struct list_elem* e;
        struct filehandle* fh;
        bool hasFH=false;
        for(e=list_begin(&cur->fd_list); e!= list_end(&cur->fd_list); e=list_next(e)) {
            fh=list_entry(e,struct filehandle, elem);
            if(fh->fd==fd){
                hasFH=true;
                break;
            }
        }
        if(!hasFH){
            return -1;
        }
        lock_acquire(&write_lock);/* NOTE: replace with lock from specific filesystem sector being written to, currently unsure about the granularity of the locking on the filesystem but I know this current implementation is not robust enough*/
        unsigned int ret = file_write(fh->fp,buffer,size);
        lock_release(&write_lock);
        return ret;
    }
}

void halt(void){
/*Terminates Pintos by calling shutdown_power_off() (declared in
‘devices/shutdown.h’). This should be seldom used, because you lose
some information about possible deadlock situations, etc.*/
shutdown_power_off();
}

void exit(int status){
    struct thread *cur = thread_current();
    cur->c->exit_status = status;
    /* print name of thread and exit status */
    printf ("%s: exit(%d)\n", cur->name, status);
    thread_exit();
}

int exec (const char* cmd_line){
/*Runs the executable whose name is given in cmd line, passing any given arguments,
and returns the new process’s program id (pid). Must return pid -1, which otherwise
should not be a valid pid, if the program cannot load or run for any reason. Thus,
the parent process cannot return from the exec until it knows whether the child
process successfully loaded its executable. You must use appropriate synchronization
to ensure this.*/
  
    /* execute the command and return the new process's pid */ 
    pid_t pid = process_execute(cmd_line);

    struct thread* cur=thread_current(); /*current parent thread*/
    struct list_elem* CLE; /*child list element*/
    struct child* c;  /*pointer to child with tid == child_tid*/

    bool found = false;
    for(CLE=list_begin(&(cur->child_list)); CLE != list_end(&(cur->child_list)); CLE=list_next(CLE)){
        c = list_entry(CLE, struct child, elem); 

        if(c->tid == pid){
            found = true;
            break;
        }
    }

    /* if pid is not in the parent's child_list for some reason */
    if(!found) {
        return -1; 
    }
   
    while(!c->load_status){
        barrier();
    }
    /* if load status = -1 then load of executable failed */
    if (c->load_status < 0) {
        return c->load_status;
    } else {
        return pid;
    }
}

bool create(const char* fileName, unsigned initial_size){
/*Creates a new file called file initially initial size bytes in size. Returns true if successful,
false otherwise. Creating a new file does not open it: opening the new file is
a separate operation which would require a open system call.*/
    if(fileName){
    return filesys_create(fileName, initial_size);
    }else{
    exit(-1);
    return 0;
    }
}
bool remove(const char* file){
/*Deletes the file called file. Returns true if successful, false otherwise. A file may be
removed regardless of whether it is open or closed, and removing an open file does
not close it. See [Removing an Open File], page 35, for details.*/
    return filesys_remove(file);
}
int open(const char* file){
    
	struct thread* cur=thread_current();
	    /* add to file descriptor list here */
	struct file* fd_p=filesys_open(file);
	if(fd_p==NULL){
	    return -1;
	} else {
        struct filehandle* new_fh=malloc(sizeof(struct child));
        new_fh->fp=fd_p;
        new_fh->fd=cur->fd_count;/*accounting for STDIN and STDOUT*/
        list_push_back(&cur->fd_list,&new_fh->elem);
	    return cur->fd_count++;
    }
}
int filesize(int fd){
/*Returns the size, in bytes, of the file open as fd*/
	struct thread* cur=thread_current();
    struct list_elem* e;
    struct filehandle* fh;
    bool hasFH=false;
    for(e=list_begin(&cur->fd_list); e!= list_end(&cur->fd_list); e=list_next(e))
    {
        fh=list_entry(e,struct filehandle, elem);
        if(fh->fd==fd){
        hasFH=true;
        break;
        }
    }
    if(!hasFH){
        return -1;
    }
    
	unsigned ret=file_length(fh->fp);
	return ret;
}

int read(int fd, void *buffer, unsigned size){
    unsigned int i;
	if(fd == 0){
	    /*Reading from keyboard (STDIN) using input_getc()*/
	    /*TO-DO*/
        uint8_t *tmp_buf = (uint8_t *) buffer;
        for (i=0; i<size; i++) {
            tmp_buf[i] = input_getc();
        }
	    return size;
	}
	else if (fd==1){
	    /*not sure if this makes sense really, should be STDOUT*/
	    return -1;
    }

    struct thread* cur=thread_current();
    struct list_elem* e;
    struct filehandle* fh;
    bool hasFH=false;
    for(e=list_begin(&cur->fd_list); e!= list_end(&cur->fd_list); e=list_next(e)) {
        fh=list_entry(e,struct filehandle, elem);
        if(fh->fd==fd){
            hasFH=true;
            break;
        }
    }
    if(!hasFH){
        return -1;
    }
    
	unsigned ret=file_read(fh->fp,buffer,size);
	return ret;
}

void seek(int fd, unsigned position){
    struct thread* cur=thread_current();
    struct list_elem* e;
    struct filehandle* fh;
    bool hasFH=false;
    for(e=list_begin(&cur->fd_list); e!= list_end(&cur->fd_list); e=list_next(e))
    {
        fh=list_entry(e,struct filehandle, elem);
        if(fh->fd==fd){
        hasFH=true;
        break;
        }
    }
    if(!hasFH){
        return;
    }
    
	file_seek(fh->fp,position);	
    return;
}
unsigned tell(int fd){
    /*Returns the position of the next byte to be read or written in open file fd, expressed
      in bytes from the beginning of the file.*/

    struct thread* cur=thread_current();
    struct list_elem* e;
    struct filehandle* fh;
    bool hasFH=false;
    for(e=list_begin(&cur->fd_list); e!= list_end(&cur->fd_list); e=list_next(e))
    {
        fh=list_entry(e,struct filehandle, elem);
        if(fh->fd==fd){
        hasFH=true;
        break;
        }
    }
    if(!hasFH){
        return -1;
    }
    
	unsigned ret=file_tell(fh->fp);
	return ret;
}
void close(int fd){
    /*Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open
      file descriptors, as if by calling this function for each one.*/
    struct thread* cur=thread_current();
    struct list_elem* e;
    struct filehandle* fh;
    bool hasFH=false;
    for(e=list_begin(&cur->fd_list); e!= list_end(&cur->fd_list); e=list_next(e))
    {
        fh=list_entry(e,struct filehandle, elem);
        if(fh->fd==fd){
        hasFH=true;
        break;
        }
    }
    if(!hasFH){
        return;
    }
    
	file_close(fh->fp);
    list_remove(e);
    free(fh);
}


