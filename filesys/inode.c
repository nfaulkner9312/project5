#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Define number of direct, indirect and doubly indirect blocks per inode */
#define NUM_DIRECT 4
#define NUM_INDIRECT 9
#define NUM_DOUBLY_INDIRECT 1

/* starting index in the inode structure
   for direct, indirect, and doubly indirect blocks */
#define START_DIRECT 0
#define START_INDIRECT 4
#define START_DOUBLY_INDIRECT 13

/* Define the total number of block pointers for an inode */
#define NUM_DIRECT_BLOCK_PTRS 14

/* Total number of block pointers for an indirect block */
#define NUM_INDIRECT_BLOCK_PTRS 128


/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44




/* the meat of the inode */
struct inode_data {
    off_t length;                               /* File size in bytes. */

    uint32_t direct_index;                      /* index to the direct blocks */
    uint32_t indirect_index;                    /* index to the indrect blocks */
    uint32_t doubly_indirect_index;             /* index to the double indirect blocks */

    block_sector_t block_ptr[NUM_DIRECT_BLOCK_PTRS];   /* pointers to the direct blocks */
};

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk {
    //block_sector_t start;             /* First data sector. (old system) */
    unsigned magic;                     /* Magic number. */
    struct inode_data data;

    uint32_t unused[109];               /* Not used. */
};

struct indirect_block {
    block_sector_t ptr[NUM_INDIRECT_BLOCK_PTRS];
};

/* Returns the number of sectors to allocate for an inode SIZE bytes long. */
static inline size_t bytes_to_sectors(off_t size) {
    return DIV_ROUND_UP(size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_data data;             /* Inode content */
    /* struct inode_disk data; */       /* Inode content. can't do this anymore 
                                           the unused array takes up too much space */
};


/* functions for expanding size */
off_t inode_expand(struct inode *inode, off_t length);
bool inode_release(struct inode *inode);
bool inode_allocate(struct inode_disk *disk_inode);

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
/* NOTE: we will rewrite this to search through the direct and indirect and double indirect blocks and return the sector found*/

static block_sector_t byte_to_sector (const struct inode *inode, off_t pos) {
    ASSERT (inode != NULL);
    if (pos < inode->data.length) {
        
        if (pos < BLOCK_SECTOR_SIZE*NUM_DIRECT) {
            return inode->data.block_ptr[pos / BLOCK_SECTOR_SIZE];
        } else {
            uint32_t indirect_index;
            struct indirect_block indirect_block;
            /* sector is in indirect block */
            if (pos < BLOCK_SECTOR_SIZE*(NUM_DIRECT + NUM_INDIRECT*NUM_INDIRECT_BLOCK_PTRS)) {
                pos = pos - BLOCK_SECTOR_SIZE*NUM_DIRECT;
                /* calculate the index to the indirect block */
                indirect_index = NUM_DIRECT + (pos / (BLOCK_SECTOR_SIZE*NUM_INDIRECT_BLOCK_PTRS));
                /* read the indirect block */
                block_read(fs_device, inode->data.block_ptr[indirect_index], &indirect_block.ptr); 

            /* sector is in a doubly indirect block */
            } else {
            /* read the only double indirect block */
                block_read(fs_device, inode->data.block_ptr[START_DOUBLY_INDIRECT], &indirect_block.ptr);
                /* offset position to where indirect blocks are */
                pos = pos - BLOCK_SECTOR_SIZE*(NUM_DIRECT + NUM_INDIRECT*NUM_INDIRECT_BLOCK_PTRS);
                /* calculate the index to the indirect block */
                indirect_index = pos / (BLOCK_SECTOR_SIZE*NUM_INDIRECT_BLOCK_PTRS);
                /* read the indrect block */
                block_read(fs_device, indirect_block.ptr[indirect_index], &indirect_block.ptr);
            }
            /* calculate index to the block pointed to from the indirect block */
            pos = pos % BLOCK_SECTOR_SIZE*NUM_INDIRECT_BLOCK_PTRS;
            return indirect_block.ptr[pos / BLOCK_SECTOR_SIZE];
        }
    } 
    return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      // size_t sectors = bytes_to_sectors (length);
      disk_inode->data.length = length;
      disk_inode->magic = INODE_MAGIC;

        
      if (inode_allocate(disk_inode)) {
          block_write(fs_device, sector, disk_inode); 
          success = true;
      }
      /* this needs to change to a allocate_inode(disk_inode) */
      /*
      if (free_map_allocate (sectors, &disk_inode->start)) 
        {
          block_write (fs_device, sector, disk_inode);
          if (sectors > 0) 
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;
              
              for (i = 0; i < sectors; i++) 
                block_write (fs_device, disk_inode->start + i, zeros);
            }
          success = true; 
        }
        */ 
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  /* had to change this up because there is no longer a inode_disk field for inode */
  struct inode_disk disk_inode;
  block_read (fs_device, inode->sector, &disk_inode);
  inode->data.length = disk_inode.data.length;
  inode->data.direct_index = disk_inode.data.direct_index;
  inode->data.indirect_index = disk_inode.data.indirect_index;
  inode->data.doubly_indirect_index = disk_inode.data.doubly_indirect_index;
  memcpy(&inode->data.block_ptr, &disk_inode.data.block_ptr, NUM_DIRECT_BLOCK_PTRS*sizeof(block_sector_t));

  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void inode_close (struct inode *inode) {
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

    /* Release resources if this was the last opener. */
    if (--inode->open_cnt == 0) {
        /* Remove from inode list and release lock. */
        list_remove (&inode->elem);

        /* Deallocate blocks if removed. */
        if (inode->removed) {
            free_map_release (inode->sector, 1);
            inode_release(inode);
            // TODO: going to need a new version of this because of indirect blocks
            // release_inode(inode);
            // free_map_release (inode->data.start, bytes_to_sectors (inode->data.length)); 
        } else {
            // TODO: write inode back to disk
        }
        free (inode);  
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;


  // expand if you want to write beyond the current length
  if (offset + size > inode_length(inode)) {
      inode->data.length = inode_expand(inode, offset + size);
  }

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

off_t inode_expand(struct inode *inode, off_t length) {
}

bool inode_allocate(struct inode_disk *disk_inode) {

    /* initialize an empty inode with size 0 */
    struct inode inode;
    inode.data.length = 0;
    inode.data.direct_index = 0;
    inode.data.indirect_index = 0;
    inode.data.doubly_indirect_index = 0;

    /* expand it until it is large enough */
    inode_expand(&inode, disk_inode->data.length);

    /* copy over indicies and pointers */
    disk_inode->data.direct_index = inode.data.direct_index;
    disk_inode->data.indirect_index = inode.data.indirect_index;
    disk_inode->data.doubly_indirect_index = inode.data.doubly_indirect_index;
    memcpy(&disk_inode->data.block_ptr, &inode.data.block_ptr, NUM_DIRECT_BLOCK_PTRS*sizeof(block_sector_t));

    return true;
}

bool inode_release(struct inode *inode) {
    return true;
}

