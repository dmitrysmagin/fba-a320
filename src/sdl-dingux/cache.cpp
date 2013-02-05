#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "burner.h"
#include "cache.h"

#ifndef WIN32
#include <sys/mman.h>
#include <errno.h>
#else

// emulate mmap/munmap on windows, just replace with malloc/free
#define PROT_READ 0
#define MAP_PRIVATE 0 

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	int fsize;
	char *p;

	fsize = lseek( fd, 0, SEEK_END );
	p = (char *)malloc(fsize);
	if(!p) return (void *)(-1);

	lseek( fd, 0, SEEK_SET );
	read( fd, p, fsize );
	return (void *)p;
}

int munmap(void *addr, size_t length)
{
	if(addr) free(addr);
	return 0;
}

#endif

int bBurnUseRomCache = 0;
static int pBurnCacheFile = 0;

static struct BurnCacheHeader {
	unsigned int ver;		// min fba version
	char name[12];				// ROM Name
	struct BurnCacheBlock {
		unsigned int offset;	// block offset in cache file
		char desc[12];			// describe of this block
	} blocks[15];
} bcHeader;

//static void * blocks_map[14];
static void * BurnCacheBase = 0;
static int BurnCacheSize = 0;

void show_rom_loading_text(char * szText, int nSize, int nTotalSize);	// fba_player.cpp

void DisableReadAhead()
{
#ifndef WIN32
	char * value = "0\r";
	int fReadAhead = open("/proc/sys/vm/max-readahead",O_RDWR|O_TRUNC);
	if (fReadAhead)
	{
		write(fReadAhead,value,2);
		close(fReadAhead);
	}
	fReadAhead = open("/proc/sys/vm/min-readahead",O_RDWR|O_TRUNC);
	if (fReadAhead)
	{
		write(fReadAhead,value,2);
		close(fReadAhead);
	}
#endif
}

int BurnCacheInit(const char * cfname, char *rom_name)
{
	pBurnCacheFile = 0;
	BurnCacheBase = 0;
	
	strcpy(szAppRomPaths[0], cfname);
	char * p = strrchr(szAppRomPaths[0], '/');
	if (p) {
		p++;
		strcpy(rom_name, p);
		
		*p = 0;
		p = strrchr(rom_name, '.');
		if (p) {
			
			if ( strcmp( p, ".zip" ) == 0 ) {
				*p = 0;
				return 0;
			} else {
				// cache file 
				pBurnCacheFile = open(cfname, O_RDONLY);
				if ( pBurnCacheFile ) {
					
					lseek( pBurnCacheFile, 0, SEEK_SET );
					read( pBurnCacheFile, &bcHeader, sizeof(bcHeader) );
					strcpy(rom_name, bcHeader.name);
					
					for (int i=0;i<15;i++)
						if ( bcHeader.blocks[i].offset ) 
							BurnCacheSize = bcHeader.blocks[i].offset;
						else break;
					
					//show_rom_loading_text("Cache", 0x100, BurnCacheSize);
					
					BurnCacheBase = mmap(0, BurnCacheSize, PROT_READ, MAP_PRIVATE, pBurnCacheFile, 0);
					if ((int)BurnCacheBase == -1) {
						BurnCacheBase = 0;
						return -3;
					}
					bBurnUseRomCache = 1;
					DisableReadAhead();
					return 0;
					
				} else return -2;
			}
		}
	}
	return -1;
}

unsigned int BurnCacheBlockSize(int blockid)
{
	return  bcHeader.blocks[blockid+1].offset - bcHeader.blocks[blockid].offset;
}

int BurnCacheRead(unsigned char * dst, int blockid)
{
	if ( pBurnCacheFile ) {
		show_rom_loading_text(bcHeader.blocks[blockid].desc, bcHeader.blocks[blockid+1].offset-bcHeader.blocks[blockid].offset, BurnCacheSize);
		lseek( pBurnCacheFile, bcHeader.blocks[blockid].offset, SEEK_SET );
		read( pBurnCacheFile, dst, bcHeader.blocks[blockid+1].offset - bcHeader.blocks[blockid].offset );
		return 0;
	} 
	return 1;
}

void * BurnCacheMap(int blockid)
{
	if ( BurnCacheBase ) {
		if ( (bcHeader.blocks[blockid+1].offset - bcHeader.blocks[blockid].offset) > 0 ) {
			show_rom_loading_text(bcHeader.blocks[blockid].desc, bcHeader.blocks[blockid+1].offset-bcHeader.blocks[blockid].offset, BurnCacheSize);
			return (unsigned char *)BurnCacheBase + bcHeader.blocks[blockid].offset;
		} else return 0;
	} else
		return 0;
}

void BurnCacheExit()
{	
	if ( BurnCacheBase ) {
		munmap( BurnCacheBase, BurnCacheSize );
		BurnCacheBase = 0;
		BurnCacheSize = 0;
	}

	if (pBurnCacheFile) {
		close(pBurnCacheFile);
		pBurnCacheFile = 0;
	}

	bBurnUseRomCache = 0;
}

#define BLOCKSIZE 1024
#define MEMSIZE 0x8000000

static int fd = -1;
void *CachedMem = NULL;
int TakenSize[MEMSIZE / BLOCKSIZE];
char SwapPath[512] = "./";

void InitMemPool()
{
	if(!config_options.option_useswap) return;

#ifndef USE_SWAP
	CachedMem = (void *)malloc(MEMSIZE);
#else
	char *home = getenv("HOME");
	if(home) sprintf(SwapPath, "%s/.fba", home); else sprintf(SwapPath, "./.fba", home);
	mkdir(SwapPath, 0777);

	if(errno == EROFS || errno == EACCES || errno == EPERM) {
		getcwd(SwapPath, 512);
		strcat(SwapPath, "/.fba");
		mkdir(SwapPath, 0777);
	}

	strcat(SwapPath, "/fba.swp");
	fd = open(SwapPath, O_CREAT | O_RDWR | O_SYNC);
	if(fd < 0) {
		printf("Error creating swap\n");
		exit(-1);
	}
	lseek(fd, MEMSIZE, SEEK_SET);
	write(fd, " ", 1);

	CachedMem = mmap(0, MEMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif
	memset(TakenSize, 0, sizeof(TakenSize));
}

void DestroyMemPool()
{
	if(!config_options.option_useswap) return;

#ifndef USE_SWAP
	free(CachedMem);
#else
	munmap (CachedMem, MEMSIZE);
	if(fd >= 0) close(fd);
#endif
	CachedMem = NULL;
}

// Allocates memory
void *CachedMalloc(size_t size)
{
	if(!config_options.option_useswap) return malloc(size);

	if(size < BLOCKSIZE) size = BLOCKSIZE;
	int i = 0; printf("CachedMalloc: %x\n", size);
ReDo:
	for(; TakenSize[i]; i += TakenSize[i]);
	if(i >= MEMSIZE / BLOCKSIZE) {
		printf("CachedMalloc out of mem!");
		return NULL;
	}

	int BSize = (size - 1) / BLOCKSIZE + 1;
	for(int j = 1; j < BSize; j++) {
		if (TakenSize[i + j]) {
			i += j;
			goto ReDo; //OMG Goto, kill me.
		}
	}
  
	TakenSize[i] = BSize;
	void *mem = ((char*)CachedMem) + i * BLOCKSIZE; //printf("%x, %x, %i\n", CachedMem, mem, i);
	memset(mem, 0, size);
	return mem;
}

// Releases CachedMalloc'ed memory
void CachedFree(void* mem)
{
	if(!config_options.option_useswap) { free(mem); return; }

	int i = (((int)mem) - ((int)CachedMem));
	if (i < 0 || i >= MEMSIZE) {
		printf("CachedFree of not CachedMalloced mem: %p\n", mem);
	} else {
		if (i % BLOCKSIZE)
			printf("delete error: %p\n", mem);
		TakenSize[i / BLOCKSIZE] = 0;
	}
}

// Returns the size of a CachedMalloced block.
int GetCachedSize(void* mem)
{
	if(!config_options.option_useswap) return 0;

	int i = (((int)mem) - ((int)CachedMem));
	if (i < 0 || i >= MEMSIZE) {
		printf("GetCachedSize of not CachedMalloced mem: %p\n", mem);
		return -1;
	}
	return TakenSize[i / BLOCKSIZE] * BLOCKSIZE;
}
