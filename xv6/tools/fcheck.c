//Inode Addrs[]=x, where x is the stored the data block number 
//you can add uint to a char pointer variable. the uint represents bytes

//testcases 
//badinode : pass
//badaddr : pass
//badindir1 : pass
//badindir2 : pass
//badroot :pass
//badroot2 : pass 

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#include "../include/types.h"
#include "../include/fs.h"
#include "string.h"

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device
#define BLOCK_SIZE (BSIZE)

int check_inode_type(struct dinode *);
int check_valid_block_address(struct dinode *, uint,int,char*);
int check_root_dir_exists(struct dirent *);
//uint x= sizeof(struct dinode);
//printf("inode size is : %u \n",x);
// result : 64 bytes 
// so 1 block has 512/64 = 8 inodes xv6 has 200 inodes so need 25+1 extra blocks

//gcc fcheck.c -o fcheck -Wall -Werror -O

int
main(int argc, char *argv[])
{
  int totalblocks, datablocks, totalinodes;
  int i,fsfd;
  char *addr;
  char *blockaddr;
  struct dinode *dip;
  struct superblock *sb;
  struct dirent *de;
  uint bitmap_blocknumber;

  if(argc < 2){
    fprintf(stderr, "Usage: fcheck <file_system_image>\n");
    exit(1);
  }


  fsfd = open(argv[1], O_RDONLY);
  if(fsfd < 0){
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

  /* Dont hard code the size of file. Use fstat to get the size */
  struct stat object1;
  int check_stat_obj = fstat(fsfd,&object1);
  if(check_stat_obj<0) {printf("fstat failed.\n"); exit(1);}

  //to get the file size for an arbitrary FS .img
  int size_fstat = object1.st_size;
  int y = (int)object1.st_nlink;
  printf("\n%d\n",y);

  addr = mmap(NULL, size_fstat, PROT_READ, MAP_PRIVATE, fsfd, 0);
  if (addr == MAP_FAILED){
	perror("mmap failed");
	exit(1);
  }

  /* read the super block */
  sb = (struct superblock *) (addr + 1 * BLOCK_SIZE);
  totalblocks=sb->size;
  datablocks=sb->nblocks;
  totalinodes=sb->ninodes;
  printf("fs size %d, no. of blocks %d, no. of inodes %d \n", totalblocks, datablocks, totalinodes);
  
  /* read the inodes */ //disk inode pointer 
  dip = (struct dinode *) (addr + IBLOCK((uint)0)*BLOCK_SIZE); 
  printf("begin addr %p, begin inode %p , offset %ld \n", addr, dip, (char *)dip -addr);
  bitmap_blocknumber=totalinodes/IPB+3;
  printf("bitmap_blocknumber : %u\n",bitmap_blocknumber);

  //block starting address why 4? 1 unused, 1 super, 1 bit block, 1 extra for inode idk why
  blockaddr=  (char *) (addr + (totalinodes/IPB + 4)*BLOCK_SIZE);

  //Loop to check the error cases // return val 0 is everythings good 
  int x;
  //this value gets set to true if the exclusion to ERROR 3 conditions are met
  bool good_root_directory=false;
  int error3=2;
  for(i=ROOTINO;i<totalinodes;i++){
    //check for error 1
    x=check_inode_type(&(dip[i]));
    if(x==1){
      fprintf(stderr,"ERROR: bad inode.\n");
      close(fsfd);
      exit(1);
    }
    //check for error 2
    if(dip[i].type!=0){x=check_valid_block_address(&(dip[i]),bitmap_blocknumber,totalblocks,addr);}
    if(x==1){
      fprintf(stderr,"ERROR: bad direct address in inode.\n");
      close(fsfd);
      exit(1);
    }
    else if(x==2){
      fprintf(stderr,"ERROR: bad indirect address in inode.\n");
      close(fsfd);
      exit(1);
    }
    else;

    //check for error 3
    //check type and then call it
    if(dip[i].type==T_DIR && !good_root_directory) {
      de = (struct dirent *) (addr + (dip[i].addrs[0])*BLOCK_SIZE);
      error3=check_root_dir_exists(de);
      if(error3==0) good_root_directory=true;
      if(error3==1) {
        /*'badroot'	 'file system with a root directory in bad location'
        'badroot2'	 'file system with a bad root directory in good location'
        */
        //parent not right || inode number is wrong
        fprintf(stderr,"CASE FOR BAD ROOT DIRECTORY\n");
        fprintf(stderr,"ERROR: root directory does not exist.\n");
        close(fsfd);
        exit(1);
        }
    }
    if(i==(totalinodes-1)&& !good_root_directory){
      fprintf(stderr,"ERROR: root directory does not exist.\n");
      close(fsfd);
      exit(1);
    }
  }
  fprintf(stdout,"Good File System.\n");

  


  printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);


  // uint a =dip[ROOTINO].addrs[0]; //data block 0 block number :29
  // uint b =dip[ROOTINO].addrs[0]*BLOCK_SIZE;
  // char *c = (char *)(addr+b);
  // uint a2 =dip[2].addrs[0];
  // uint b2 =dip[2].addrs[0]*BLOCK_SIZE;
  // //uint c =(uint)(addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);
  // printf("a:%u b:%u a2:%u b2:%u addr:%p c:%p \n",a,b,a2,b2,addr,c);
  
  de = (struct dirent *) (addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);
  //i can get the name of the direcotry from dirent 
  
    // get the address of root dir 
  
  printf("root inode address stored in direct block 0 : %p\n",de);
  printf("root inode address stored in direct block 0 : %p\n",de);
  printf("value I calculated for starting block: %p\n",blockaddr);
  printf("value I calculated for starting block: %ld\n",(long int)blockaddr);
  printf("root directory name and inode number : %s,%u\n",de->name,de->inum);

  // need ptr to bitmap kernel/fs.c -> balloc()
  //mkfs balloc

  // print the entries in the first block of root dir 

 
  exit(0);

}

int check_inode_type(struct dinode *ptr){
  int t=ptr->type;
  if(t==0 || t==T_FILE || t ==T_DEV || t==T_DIR){printf("inode type: %d\n",t); return 0;}
  else{return 1;}
}

int check_valid_block_address(struct dinode *ptr,uint bitblock_num,int totalblocks,char *addr){
  int i;
  uint lastblock = totalblocks-1;
  //+1 to check the ndirect block data block
  for (i=0; i<NDIRECT;i++){
    //used and out of range 
    if(ptr->addrs[i]!=0 && (ptr->addrs[i]<(bitblock_num+1) ||ptr->addrs[i]>lastblock))return 1;
  }
  //check Indirect block if its not empty 
  if(ptr->addrs[NDIRECT]!=0){
    if(ptr->addrs[NDIRECT]<(bitblock_num+1) ||ptr->addrs[NDIRECT]>lastblock)return 2;
    uint indirectblocknum = ptr->addrs[NDIRECT];
    uint *IDB=(uint*)(addr+indirectblocknum*BLOCK_SIZE);
    for(i=0; i < NINDIRECT; i++){
      //printf("IDB[i] : %p ",IDB);
      //printf("*TDB[i]:%u \n",IDB[i]);
      if(IDB[i]!=0 && (IDB[i]<(bitblock_num+1) ||IDB[i]>lastblock))return 2;
    }
  }
  return 0;
}

int check_root_dir_exists(struct dirent *de){
  char name[]=".";
  if(de->inum ==1){
    if (strcmp(name,de->name)==0){
      printf("ROOT DIRECTORY inode number and name match !\n");
      de++;
      if(de->inum ==1) {printf("ROOT DIRECTORY parent good too !\n");return 0;}
    }
    return 1; //returns 1 if root dir name dont match with inum 1 or parent dir
    //dont point to itself
  }
  if(strcmp(name,de->name)==0) return 1; // root directory exists but has a bad inumber(!1)
  return 2;
}

//	printf(" inum %d, name %s ", de->inum, de->name);
//	printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);
  