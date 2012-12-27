#ifndef BURN_CACHE_H
#define BURN_CACHE_H

extern int bBurnUseRomCache;

int BurnCacheInit(const char * cfname, char *rom_name);

unsigned int BurnCacheBlockSize(int blockid);
int BurnCacheRead(unsigned char * dst, int blockid);
void * BurnCacheMap(int blockid);

void BurnCacheExit();

#ifdef __cplusplus
extern "C" {
#endif

#define BurnMalloc CachedMalloc
#define BurnFree CachedFree

void InitMemPool();
void DestroyMemPool();
void *CachedMalloc(size_t size);
void CachedFree(void* mem);
int GetCachedSize(void* mem);

#ifdef __cplusplus
}
#endif

#endif	// BURN_CACHE_H
