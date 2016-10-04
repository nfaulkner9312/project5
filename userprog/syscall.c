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

static void syscall_handler (struct intr_frame *);
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
	/*each argument is just the next thing on the stack after the syctemcall#, esp+1 esp+2 etc*/
        case SYS_HALT: {halt();
            break;
        } case SYS_EXIT: {
            struct thread *cur = thread_current();
            cur->c->exit_status = arg[0];
            printf ("%s: exit(%d)\n", cur->name, arg[0]);
            thread_exit();
            break;
        } case SYS_EXEC: {
            exec((const char*)arg[0]);/*maybe this one is correct? The first thing on the stack is a char*, so I want to cast the void* to a const char*(*?) and dereference it to pass the value at that location which is then a const char*   I am super confused by all these stars*/
            break;
        } case SYS_WAIT: {
            process_wait(arg[0]);
            break;
        } case SYS_CREATE: {
            break;
        } case SYS_REMOVE: {
            break;
        } case SYS_OPEN: {
		    int k = open((const char*)arg[0]);
            
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
        //int ret = file_write(cur->fd_list[fd], buffer, size);
        int ret = 0;
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
void halt(void){
/*Terminates Pintos by calling shutdown_power_off() (declared in
‘devices/shutdown.h’). This should be seldom used, because you lose
some information about possible deadlock situations, etc.*/

}
void exit(int status){
/*Terminates the current user program, returning status to the kernel. If the process’s
parent waits for it (see below), this is the status that will be returned. Conventionally,
a status of 0 indicates success and nonzero values indicate errors.*/

}
int exec (const char* cmd_line){
/*Runs the executable whose name is given in cmd line, passing any given arguments,
and returns the new process’s program id (pid). Must return pid -1, which otherwise
should not be a valid pid, if the program cannot load or run for any reason. Thus,
the parent process cannot return from the exec until it knows whether the child
process successfully loaded its executable. You must use appropriate synchronization
to ensure this.*/

return 0;
}

int wait(int pid){
/*See documentation, this one is long.*/

return 0;
}
bool create(const char* fileName, unsigned initial_size){
/*Creates a new file called file initially initial size bytes in size. Returns true if successful,
false otherwise. Creating a new file does not open it: opening the new file is
a separate operation which would require a open system call.*/
return filesys_create(fileName, initial_size);
}
bool remove(const char* file){
/*Deletes the file called file. Returns true if successful, false otherwise. A file may be
removed regardless of whether it is open or closed, and removing an open file does
not close it. See [Removing an Open File], page 35, for details.*/
return filesys_remove(file);
}
int open(const char* file){
/*Opens the file called file. Returns a nonnegative integer handle called a “file descriptor”
(fd), or -1 if the file could not be opened.
File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is
standard input, fd 1 (STDOUT_FILENO) is standard output. The open system call will
never return either of these file descriptors, which are valid as system call arguments
only as explicitly described below.
Each process has an independent set of file descriptors. File descriptors are not
inherited by child processes.
When a single file is opened more than once, whether by a single process or different
processes, each open returns a new file descriptor. Different file descriptors for a single
file are closed independently in separate calls to close and they do not share a file
position.*/
    
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
/*Reads size bytes from the file open as fd into buffer. Returns the number of bytes
actually read (0 at end of file), or -1 if the file could not be read (due to a condition
other than end of file). Fd 0 reads from the keyboard using input_getc().*/
	struct thread *cur=thread_current(); 
	if(fd == 0){
	/*Reading from keyboard (STDIN) using input_getc()*/
	/*TODO*/	
	return 0;
	}
	else if (fd==1){
	/*not sure if this makes sense really, should be STDOUT*/
	return 0;
	}
	//unsigned ret = (unsigned)file_read (cur->fd_list[fd], buffer, size);
    unsigned int ret = 0;
	return ret;
}


void seek(int fd, unsigned position){
/*Changes the next byte to be read or written in open file fd to position, expressed in
bytes from the beginning of the file. (Thus, a position of 0 is the file’s start.)
A seek past the current end of a file is not an error. A later read obtains 0 bytes,
indicating end of file. A later write extends the file, filling any unwritten gap with
zeros. (However, in Pintos files have a fixed length until project 4 is complete, so
writes past end of file will return an error.) These semantics are implemented in the
file system and do not require any special effort in system call implementation.*/
	
return;
}
unsigned tell(int fd){
/*Returns the position of the next byte to be read or written in open file fd, expressed
in bytes from the beginning of the file.*/
	struct thread* cur=thread_current();
	//unsigned ret=file_tell(cur->fd_list[fd]);
    unsigned ret = 0;
	return ret;
}
void close(int fd){
/*Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open
file descriptors, as if by calling this function for each one.*/
	struct thread* cur=thread_current();
	//file_close(cur->fd_list[fd]);
}


