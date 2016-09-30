#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

file* fd_list[128];


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  printf("syscall init here");
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
}

static void syscall_handler (struct intr_frame *f UNUSED) {
   
    printf ("system call = %d\n", *(int*)f->esp);
    
    switch(*(int*)f->esp) {
        case SYS_HALT: {
            break;
        } case SYS_EXIT: {
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
		int k = open();
            break;
        } case SYS_FILESIZE: {
            break;
        } case SYS_READ: {
            break;
        } case SYS_WRITE: {
            break;
        } case SYS_SEEK: {
            break;
        } case SYS_TELL: {
            break;
        } case SYS_CLOSE: {
            break;
        }
    }:

    thread_exit ();
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
pid_t exec (const char* cmd_line){
/*Runs the executable whose name is given in cmd line, passing any given arguments,
and returns the new process’s program id (pid). Must return pid -1, which otherwise
should not be a valid pid, if the program cannot load or run for any reason. Thus,
the parent process cannot return from the exec until it knows whether the child
process successfully loaded its executable. You must use appropriate synchronization
to ensure this.*/

return 0;
}

int wait(pid_t pid){
/*See documentation, this one is long.*/

return 0;
}
bool create(const char* fileName, unsigned initial_size){
/*Creates a new file called file initially initial size bytes in size. Returns true if successful,
false otherwise. Creating a new file does not open it: opening the new file is
a separate operation which would require a open system call.*/
return false;
}
bool remove(const char* file){
/*Deletes the file called file. Returns true if successful, false otherwise. A file may be
removed regardless of whether it is open or closed, and removing an open file does
not close it. See [Removing an Open File], page 35, for details.*/
return false;
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

return 0;
}
int filesize(int fd){
/*Returns the size, in bytes, of the file open as fd*/

return 0;
}

int read(int fd, void *buffer, unsigned size){
/*Reads size bytes from the file open as fd into buffer. Returns the number of bytes
actually read (0 at end of file), or -1 if the file could not be read (due to a condition
other than end of file). Fd 0 reads from the keyboard using input_getc().*/

	if(fd == 0){
	/*Reading from keyboard (STDIN) using input_getc()*/
	/*TO-DO*/	
	return 0;

	/*not sure if this makes sense really, should be STDOUT*/
	return 0;
	}
	int ret=0;
	file* op= fd_list[fd];
	op=(char*)op;
	
	
	




	return ret;
}
int write(int fd, void *buffer, unsigned size){
/*Writes size bytes from buffer to the open file fd. Returns the number of bytes actually
written, which may be less than size if some bytes could not be written.
Writing past end-of-file would normally extend the file, but file growth is not implemented
by the basic file system. The expected behavior is to write as many bytes as
possible up to end-of-file and return the actual number written, or 0 if no bytes could
be written at all.
Fd 1 writes to the console. Your code to write to the console should write all of buffer
in one call to putbuf(), at least as long as size is not bigger than a few hundred
bytes. (It is reasonable to break up larger buffers.) Otherwise, lines of text output
by different processes may end up interleaved on the console, confusing both human
readers and our grading scripts.*/
return 0;
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
return 0;
}
void close(int fd){
/*Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open
file descriptors, as if by calling this function for each one.*/
return;
}


