//Inode Addrs[]=x, where x is the stored the data block number 
//you can add uint to a char pointer variable. the uint represents bytes

//testcases 
//badinode : pass
//badaddr : pass
//badindir1 : pass
//badindir2 : pass
//badroot :pass
//badroot2 : pass
//badfmt: pass
//mrkfree: pass 
//indirfree: pass
//mrkused : pass
//addronce : pass
//addronce2: pass

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

uint blocks_used[1025];
int direct_address_counter[1025];
int indirect_address_counter[1025];
//int inodes_marked_inuse[200];
//int inodes_found_in_directory[200];
int check_inode_type(struct dinode *);
int check_valid_block_address(struct dinode *, uint,int,char*);
int check_root_dir_exists(struct dinode *,struct dirent *,int);
int check_dir_format(struct dinode *,struct dirent *,int);
int check_inuseinodes_bitmap(uint *,struct dinode *,char*);
int check_inusebitmap_inodes(uint*);
int check_direct_address_use();
int check_indirect_address_use();
void find_directory_entries(struct dinode * ,struct dirent *,int,int*);
int check_inode_exists_in_directory(int*,int*);
//uint x= sizeof(struct dinode);
//printf("inode size is : %u \n",x);
// result : 64 bytes 
// so 1 block has 512/64 = 8 inodes xv6 has 200 inodes so need 25+1 extra blocks

//gcc fcheck.c -o fcheck -Wall -Werror -O

int
main(int argc, char *argv[])
{
  int totalblocks, totalinodes;
  int i,fsfd;
  char *addr;
  int x;
  bool good_root_directory=false;
  int error3=2;
  int error4=0;
  int error5=0;
  int error6=0;
  int error7=0;
  int error8=0;
  int error9=0;
  //char *blockaddr;
  uint *bitblockstartaddr;
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
 
  //printf("\n%d\n",y);

  addr = mmap(NULL, size_fstat, PROT_READ, MAP_PRIVATE, fsfd, 0);
  if (addr == MAP_FAILED){
	perror("mmap failed");
	exit(1);
  }

  /* read the super block */
  sb = (struct superblock *) (addr + 1 * BLOCK_SIZE);
  totalblocks=sb->size;
  //datablocks=sb->nblocks;
  totalinodes=sb->ninodes;
  // int inodes_marked_inuse[totalinodes];
  // int inodes_found_in_directory[totalinodes];
  //printf("fs size %d, no. of blocks %d, no. of inodes %d \n", totalblocks, datablocks, totalinodes);
  
  /* read the inodes */ //disk inode pointer 
  dip = (struct dinode *) (addr + IBLOCK((uint)0)*BLOCK_SIZE); 
  //printf("begin addr %p, begin inode %p , offset %ld \n", addr, dip, (char *)dip -addr);
  bitmap_blocknumber=totalinodes/IPB+3;
 
  bitblockstartaddr=(uint *)(addr + (totalinodes/IPB + 3)*BLOCK_SIZE);
  
  
  //printf("bit block is %d, bit block start address is %p ",)

  //Loop to check the error cases // return val 0 is everythings good 
  
  for(i=ROOTINO;i<totalinodes;i++){
    //Error 1
    x=check_inode_type(&(dip[i]));
    if(x==1){
      fprintf(stderr,"ERROR: bad inode.\n");
      close(fsfd);
      exit(1);
    }
    //Error 2
    if(dip[i].type!=0){
      x=check_valid_block_address(&(dip[i]),bitmap_blocknumber,totalblocks,addr);
      //inodes_marked_inuse[i]= inodes_marked_inuse[i]+1;
    }


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

    //Error 4 
    if(dip[i].type==T_DIR){
      de = (struct dirent *) (addr + (dip[i].addrs[0])*BLOCK_SIZE);
      error4=check_dir_format(&dip[i],de,i);
      if(error4==1){
        fprintf(stderr,"ERROR: directory not properly formatted.\n");
        close(fsfd);
        exit(1);
      }
    }

    //Error 3
    //check type and then call it
    if(dip[i].type==T_DIR && !good_root_directory) {
      de = (struct dirent *) (addr + (dip[i].addrs[0])*BLOCK_SIZE);
      error3=check_root_dir_exists(&dip[i],de,i);
      if(error3==0) good_root_directory=true;
      if(error3==1) {
        //parent not right || inode number is wrong
        //fprintf(stderr,"CASE FOR BAD ROOT DIRECTORY\n");
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
    //To Handle Case 9
    //de = (struct dirent *) (addr + (dip[i].addrs[0])*BLOCK_SIZE);
    //find_directory_entries(&dip[i],de,i,inodes_found_in_directory);

    //Error 5 bitblockstartaddr
    error5 = check_inuseinodes_bitmap(bitblockstartaddr,&dip[i],addr);
    if(error5==1){
      fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
      close(fsfd);
      exit(1);
    }

  }
  //outside the loop so to reduce 
  error6=check_inusebitmap_inodes(bitblockstartaddr);
  if(error6==1){
      fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
      close(fsfd);
      exit(1);
  }
  error7= check_direct_address_use();
  if(error7==1){
    fprintf(stderr,"ERROR: direct address used more than once.\n");
    close(fsfd);
    exit(1);
  }
  error8= check_indirect_address_use();
  if(error8==1){
    fprintf(stderr,"ERROR: indirect address used more than once.\n");
    close(fsfd);
    exit(1);
  }
  //error9=check_inode_exists_in_directory(inodes_marked_inuse,inodes_found_in_directory);
  if(error9==1){
    fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
    close(fsfd);
    exit(1);
  }
  fprintf(stdout,"Good File System.\n");
  exit(0);
}

int check_inode_type(struct dinode *ptr){
  int t=ptr->type;
  if(t==0 || t==T_FILE || t ==T_DEV || t==T_DIR){/*printf("inode type: %d\n",t);*/ return 0;}
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

int check_root_dir_exists(struct dinode * dip,struct dirent *de,int inumber){
  int i;
  char name[]=".";
  char parentdir[]="..";
  bool foundone=false;
  bool foundtwo=false;
  int n = dip->size/sizeof(struct dirent);
  //handles the case the inumber we are looking at is not 1 but everything
  //else matches with conditions for it to be the root inode 
  for (i = 0; i < n; i++,de++){
    if(de->inum ==1){
      if (strcmp(name,de->name)==0){ if(de->inum==inumber) foundone=true;}
    }
    if(de->inum ==1){
      if (strcmp(parentdir,de->name)==0){ if(de->inum==inumber) foundtwo=true;}
    }
    if(foundone && foundtwo){/*printf("found root inode!");*/return 0;}
  }
  if(!foundone && !foundtwo) return 2;
  return 1;
  
}

int check_dir_format(struct dinode * dip,struct dirent *de,int inodenumber){
  int i;
  char dirname[]=".";
  char parentdir[]="..";
  bool foundone=false;
  bool foundtwo=false;
  int n = dip->size/sizeof(struct dirent);
  for (i = 0; i < n; i++,de++){
    if(strcmp(dirname,de->name)==0){
      if(de->inum==inodenumber)foundone=true;
      else return 1;
    } 
    if(strcmp(parentdir,de->name)==0) foundtwo=true;
    if(foundone && foundtwo) {/* printf("foundone and foundtwo works\n"); */return 0;}
  }
  return 1;
}

//global variable to handle case 5 instead of having to pass in
//ill check only from 29 onwards 


int check_inuseinodes_bitmap(uint *bitblockstartaddr,struct dinode *ptr,char *addr){
  //uint bitblock=28;
  //uint uintsperblock = BLOCK_SIZE/sizeof(uint); //128//size of uint=4 bytes=32 bits
  //subtract amount = 29 //29 is the first datablock. 
  //so block 29 needs to be stored in the first bit of the bitmap 
  // hence 29-29 = 0th bit
  //each uint represents 32 bits 
  //uint starting_block =29;
  int i;
  uint set_to_starting;
  uint unit = sizeof(uint)*8;
  //printf("unit size : %u \n",unit);
  uint which_uint;
  uint value;
  uint which_bit;
  //uint lastblock = totalblocks-1;
  //+1 to check the ndirect block data block
  for (i=0; i<NDIRECT+1;i++){
    //used and out of range 
    if(ptr->addrs[i]!=0 ){
      set_to_starting=ptr->addrs[i];
      blocks_used[set_to_starting]=1;
      //printf("direct set : %u \n ", set_to_starting);
      direct_address_counter[set_to_starting] +=1;
      which_uint=set_to_starting/unit; //0
      which_bit=set_to_starting%unit; //0
      //think if i got exactly 32 then 32/32 =1 but i still need to look at zero index
      value = bitblockstartaddr[which_uint];
      //printf("value read as the integer : %u\n",value);
      if(value & (1<<which_bit)){/*bit set to high we good*/}
      else{
        return 1;
      }
    }
  }
  //check Indirect block if its not empty 

 
  uint indirectblocknum = ptr->addrs[NDIRECT];
  uint *IDB=(uint*)(addr+indirectblocknum*BLOCK_SIZE);
  for(i=0; i < NINDIRECT; i++){
    //printf("IDB[i] : %p ",IDB);
    //printf("*TDB[i]:%u \n",IDB[i]);
    if(IDB[i]!=0){
      set_to_starting=IDB[i];
      blocks_used[set_to_starting]=1;
      //printf("indirect set : %u \n ", set_to_starting);
      indirect_address_counter[set_to_starting]+=1;
      //printf("IDB[%d]:%u   set_to_starting = %u\n",i,IDB[i],set_to_starting);
      which_uint=set_to_starting/unit;
      which_bit=set_to_starting%unit;
      value = bitblockstartaddr[which_uint];
      if(value & (1<<which_bit)){
      //bit set to high we good
      }
      else{
        //printf ("value for failure : %u\n", value);
        //printf("bitblock value not correct INDIRECT BLOCK.\n");
        return 1;
      }
    }  
  }
  //printf("end of bitmap check for loop. means bitmap is good!\n");
  return 0;
}
//	printf(" inum %d, name %s ", de->inum, de->name);
//	printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);

int check_inusebitmap_inodes(uint*bitblockstartaddr){
  int numentries = BLOCK_SIZE/sizeof(uint);
  int i,j;
  uint offset =29;
  uint actual;
  uint value;
  for( i =0; i < numentries-1; i++){
    value = bitblockstartaddr[i];
    //printf("value at start : %u \n",value);
    for( j=0; j<sizeof(uint)*8; j++){
      if(i==0){
        if (j>=offset){
          if(value & (1<<j)){
            actual = j;
            if(blocks_used[actual]==1);
            else {
            return 1;
            }
          }

        }
      } // i know its horrible, but my brain was losing convolutions at this point
      // smooth like butter :(
      else{
        //i greater than zero
        if(value & (1<<j)){
            actual = j+i*32;
            if(blocks_used[actual]==1);
            else {
            return 1;
            }
        }
      }
      
    }
  }
  return 0;
}


int check_direct_address_use(){
  int i;
  for(i=0;i<1025;i++){
    if(direct_address_counter[i]>1) return 1;
  }
  return 0;
}

int check_indirect_address_use(){
  int i;
  for(i=0;i<1025;i++){
    if(indirect_address_counter[i]>1) return 1;
  }
  return 0;
}


void find_directory_entries(struct dinode * dip,struct dirent *de,int inumber,int*inodes_found_in_directory){
  int i;
  int n = dip->size/sizeof(struct dirent);
  //handles the case the inumber we are looking at is not 1 but everything
  //else matches with conditions for it to be the root inode 
  for (i = 0; i < n;i++,de++){
    inodes_found_in_directory[de->inum] =inodes_found_in_directory[de->inum]+1;
  }
}

int check_inode_exists_in_directory(int *inodes_marked_inuse,int *inodes_found_in_directory){
  int i;
  for(i=0;i<200;i++){
    if(inodes_marked_inuse[i]!=inodes_found_in_directory[i]) return 1;
  }
  return 0;
}