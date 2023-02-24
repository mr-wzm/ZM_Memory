/*****************************************************************
* Copyright (C) 2021 zm. All rights reserved.                    *
******************************************************************
* zm_Memory.c
*
* DESCRIPTION:
*     zm memory management.
* AUTHOR:
*     zm
* CREATED DATE:
*     2023/2/24
* REVISION:
*     v0.1
*
* MODIFICATION HISTORY
* --------------------
* $Log:$
*
*****************************************************************/
 
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "ZM_Memory.h"

#if ZM_USE_MEM_MGR
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#define ZM_MEM_ALIGN_SIZE       ZM_ALIGN_SIZE
     
#define ZM_HEAP_MAGIC           0x1EA0

#define ZM_ALIGN_GET(size)      ZM_ALIGN(size, ZM_MEM_ALIGN_SIZE)

#define MIN_SIZE_ALIGNED        ZM_ALIGN(ZM_MIN_SIZE, ZM_MEM_ALIGN_SIZE)
#define MEM_STRUCT_SIZE         ZM_ALIGN(sizeof(zmMem_t), ZM_MEM_ALIGN_SIZE)


#define ZM_MEM_ASSERT(EX)       \
if(!(EX))                       \
{                               \
    while(1);                   \
}
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/

typedef struct zmMem
{
    zm_uint16_t magic;
    zm_uint16_t used;
    zm_size_t prev;
    zm_size_t next;
}zmMem_t;

typedef struct
{
    zm_size_t usedSize;
    zm_size_t maxSize;
}zmMemStats_t;
/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
#if ZM_MEM_USE_HEAP
     
#define ZM_MEM_HEAP_BEGIN         ZM_HEAP_BEGIN
#define ZM_MEM_HEAP_END           ZM_HEAP_END
     
/*#if (ZM_MEM_HEAP_END <= ZM_MEM_HEAP_BEGIN)
#error "Error heap addr"
#endif*/
     
#else
static zm_uint8_t zm_pool[ZM_MEM_SIZE];
#endif

/** pointer to the heap: for alignment, heap_ptr is now a pointer instead of an array */
static zm_uint8_t *zmMemHeap;
/** the last entry, always unused! */
static zmMem_t *zmMemEnd;
/** pointer to the lowest free block */
static zmMem_t *lfree;

static zm_size_t zmMemSize;

#if ZM_MEM_STATS
static zmMemStats_t memStats;
#endif
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void zm_mem_free(void *ptr);
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/

static void zm_putTogether(zmMem_t *pMem)
{
    zmMem_t *nextMem;
    zmMem_t *prevMem;
    
    nextMem = (zmMem_t *)&zmMemHeap[pMem->next];
    
    if(nextMem->magic == ZM_HEAP_MAGIC && nextMem != pMem &&
       nextMem->used == 0 && nextMem != zmMemEnd)
    {
        if(lfree == nextMem)
        {
            lfree = pMem;
        }
        pMem->next = nextMem->next;
        ((zmMem_t *)&zmMemHeap[nextMem->next])->prev = (zm_uint8_t *)pMem - zmMemHeap;
    }
    
    prevMem = (zmMem_t *)&zmMemHeap[pMem->prev];
    
    if(prevMem->magic == ZM_HEAP_MAGIC &&
       nextMem != pMem && prevMem->used == 0)
    {
        if(lfree == pMem)
        {
            lfree = prevMem;
        }
        prevMem->next = pMem->next;
        ((zmMem_t *)&zmMemHeap[pMem->next])->prev = (zm_uint8_t *)prevMem - zmMemHeap;
    }
}

/*****************************************************************
* FUNCTION: zm_mem_init
*
* DESCRIPTION: 
*     zm dynamic memory init.
* INPUTS:
*     beginAddr : The beginning address of system heap memory.
*     endAddr   : The end address of system heap memory.
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
static void zm_mem_init(void *beginAddr, void *endAddr)
{
    zmMem_t *pMem;
    
    zm_size_t beginAlign = ZM_ALIGN((zm_size_t)beginAddr, ZM_MEM_ALIGN_SIZE);
    zm_size_t endAlign = ZM_ALIGN_DOWN((zm_size_t)endAddr, ZM_MEM_ALIGN_SIZE);
    
    if(endAlign > (2 * MEM_STRUCT_SIZE) &&
       (endAlign - 2 * MEM_STRUCT_SIZE) >= beginAlign)
    {
        zmMemSize = endAlign - beginAlign - 2 * MEM_STRUCT_SIZE;
    }
    else
    {
        //memory error begin address and end address.
        return;
    }
    
    zmMemHeap = (zm_uint8_t *)beginAlign;
    
    pMem = (zmMem_t *)zmMemHeap;
    pMem->magic = ZM_HEAP_MAGIC;
    pMem->used = 0;
    pMem->next = zmMemSize + MEM_STRUCT_SIZE;
    pMem->prev = 0;
    
    zmMemEnd = (zmMem_t *)&zmMemHeap[pMem->next];
    zmMemEnd->magic = ZM_HEAP_MAGIC;
    zmMemEnd->used = 1;
    zmMemEnd->next = zmMemSize + MEM_STRUCT_SIZE;
    zmMemEnd->prev = zmMemSize + MEM_STRUCT_SIZE;
    
    lfree = pMem;

#if ZM_MEM_STATS
    memStats.maxSize = 0;
    memStats.usedSize = 0;
#endif
}

/*****************************************************************
* FUNCTION: zm_mem_malloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
* INPUTS:
*     size : The number of bytes to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     null
*****************************************************************/
static void *zm_mem_malloc(zm_size_t size)
{
    zm_size_t idx;
    zmMem_t *pMem;
    
    if(size == 0) return NULL;
    
    size = ZM_ALIGN_GET(size);
    
    if(size > zmMemSize) return NULL;
    
    if(size < MIN_SIZE_ALIGNED) size = MIN_SIZE_ALIGNED;
    
    for(idx = (zm_uint8_t *)lfree - zmMemHeap;
        idx < (zmMemSize - size);
        idx = ((zmMem_t *)&zmMemHeap[idx])->next)
    {
        pMem = (zmMem_t *)&zmMemHeap[idx];
        
        if(!pMem->used && (pMem->next - idx - MEM_STRUCT_SIZE) >= size)
        {
            zmMem_t *mem;
            if((pMem->next - idx - MEM_STRUCT_SIZE) >= (size + MEM_STRUCT_SIZE + MIN_SIZE_ALIGNED))
            {
                zm_size_t ptr = idx + MEM_STRUCT_SIZE + size;
                
                mem = (zmMem_t *)&zmMemHeap[ptr];
                mem->magic = ZM_HEAP_MAGIC;
                mem->used = 0;
                mem->next = pMem->next;
                mem->prev = idx;
                
                pMem->next = ptr;
                pMem->used = 1;
                
                if(mem->next != (zmMemSize + MEM_STRUCT_SIZE))
                {
                    ((zmMem_t *)&zmMemHeap[mem->next])->prev = ptr;
                }
#if ZM_MEM_STATS
                memStats.usedSize += (size + MEM_STRUCT_SIZE);
                if(memStats.maxSize < memStats.usedSize)
                {
                    memStats.maxSize = memStats.usedSize;
                }
#endif
            }
            else
            {
                pMem->used = 1;
#if ZM_MEM_STATS
                memStats.usedSize += (pMem->next - idx);
                if(memStats.maxSize < memStats.usedSize)
                {
                    memStats.maxSize = memStats.usedSize;
                }
#endif
            }
            pMem->magic = ZM_HEAP_MAGIC;
            
            if(pMem == lfree)
            {
                while(lfree->used && lfree != zmMemEnd)
                {
                    lfree = (zmMem_t *)&zmMemHeap[lfree->next];
                }
                
                ZM_MEM_ASSERT(lfree == zmMemEnd || !lfree->used);
            }
            
            return (zm_uint8_t *)pMem + MEM_STRUCT_SIZE;
        }
    }
    return NULL;
}

/*****************************************************************
* FUNCTION: zm_mem_realloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
* INPUTS:
*     ptr : pointer to memory allocated by zm_mem_malloc.
*     newsize : The number of new size to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     null
*****************************************************************/
static void *zm_mem_realloc(void *ptr, zm_size_t newsize)
{
    zm_size_t idx;
    zm_size_t size;
    zmMem_t *pMem;
    void *newMem;
    
    newsize = ZM_ALIGN_GET(newsize);
    
    if(newsize > zmMemSize) return NULL;
    
    if(newsize == 0)
    {
        zm_mem_free(ptr);
        return NULL;
    }

    if(newsize < MIN_SIZE_ALIGNED) newsize = MIN_SIZE_ALIGNED;
    
    if(ptr == NULL) return zm_mem_malloc(newsize);
    
    if((zm_uint8_t *)ptr < (zm_uint8_t *)zmMemHeap ||
       (zm_uint8_t *)ptr >= (zm_uint8_t *)zmMemEnd)
    {
        //illegal memory
        return ptr;
    }
    
    pMem = (zmMem_t *)((zm_uint8_t *)ptr - MEM_STRUCT_SIZE);
    
    idx = (zm_uint8_t *)pMem - zmMemHeap;
    size = pMem->next - idx - MEM_STRUCT_SIZE;
    
    if(size == newsize)
    {
        return ptr;
    }
    
    if((newsize + MEM_STRUCT_SIZE + MIN_SIZE_ALIGNED) < size)
    {
        zm_size_t idx2;
        zmMem_t *mem;
        
        idx2 = idx + MEM_STRUCT_SIZE + size;
        mem = (zmMem_t *)&zmMemHeap[idx2];
        mem->magic = ZM_HEAP_MAGIC;
        mem->used = 0;
        mem->next = pMem->next;
        mem->prev = idx;
        
        pMem->next = idx2;
        
        if(mem->next != (zmMemSize + MEM_STRUCT_SIZE))
        {
            ((zmMem_t *)&zmMemHeap[mem->next])->prev = idx2;
        }
#if ZM_MEM_STATS
        memStats.usedSize -= (size - newsize);
#endif
        if(mem < lfree) lfree = mem;
        
        zm_putTogether(mem);
        
        return ptr;
    }
    
    newMem = zm_mem_malloc(newsize);
    
    if(newMem)
    {
        memcpy(newMem, ptr, size < newsize ? size : newsize);
        zm_mem_free(ptr);
    }
        
    return newMem;
}
/*****************************************************************
* FUNCTION: zm_mem_calloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
*     This function will contiguously allocate enough space for count objects
*     that are size bytes of memory each and returns a pointer to the allocated.
* memory.
* INPUTS:
*     count : Number of objects to allocate.
*     size : The number of size to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     null
*****************************************************************/
void *zm_mem_calloc(zm_size_t count, zm_size_t size)
{
    void *ptr;
    
    ptr = zm_mem_malloc(count * size);
    
    if(ptr) memset(ptr, 0, count * size);
    
    return ptr;
}
/*****************************************************************
* FUNCTION: zm_mem_free
*
* DESCRIPTION: 
*       zm dynamic memory de-allocation.
* INPUTS:
*     ptr : The first address assigned by zm_mem_malloc().
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
static void zm_mem_free(void *ptr)
{
    zmMem_t *pMem;
    
    if(ptr == NULL) return;
    
    if((zm_uint8_t *)ptr < (zm_uint8_t *)zmMemHeap ||
       (zm_uint8_t *)ptr >= (zm_uint8_t *)zmMemEnd)
    {
        //illegal memory
        return;
    }
    
    pMem = (zmMem_t *)((zm_uint8_t *)ptr - MEM_STRUCT_SIZE);
    
    if(pMem->magic != ZM_HEAP_MAGIC || !pMem->used)
    {
        ZM_MEM_ASSERT(0);
        //return;
    }
    pMem->used = 0;
    
    if(pMem < lfree) lfree = pMem;
    
#if ZM_MEM_STATS
    memStats.usedSize -= (pMem->next - ((zm_uint8_t *)pMem - zmMemHeap));
#endif
    
    zm_putTogether(pMem);
}

/*****************************************************************
* FUNCTION: zm_memoryMgrInit
*
* DESCRIPTION: 
*     zm dynamic memory allocation init.
* INPUTS:
*     null
* RETURNS:
*     null.
* NOTE:
*     null
*****************************************************************/
void zm_memoryMgrInit(void)
{
#if ZM_MEM_USE_HEAP
    zm_mem_init((void *)ZM_MEM_HEAP_BEGIN, (void *)ZM_MEM_HEAP_END);
#else
    zm_mem_init((void *)&zm_pool[0], (void *)((zm_uint8_t *)&zm_pool[ZM_MEM_SIZE - 1]));
#endif
}
/*****************************************************************
* FUNCTION: zm_malloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
* INPUTS:
*     size : The number of bytes to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     null
*****************************************************************/
void *zm_malloc(zm_size_t size)
{
    return zm_mem_malloc(size);
}
/*****************************************************************
* FUNCTION: zm_realloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
*     This function will change the previously allocated memory block.
* INPUTS:
*     ptr : pointer to memory allocated by zm_mem_malloc.
*     newsize : The number of new size to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     null
*****************************************************************/
void *zm_realloc(void *ptr, zm_size_t newsize)
{
    return zm_mem_realloc(ptr, newsize);
}
/*****************************************************************
* FUNCTION: zm_mem_calloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
*     This function will contiguously allocate enough space for count objects
*     that are size bytes of memory each and returns a pointer to the allocated.
* memory.
* INPUTS:
*     count : Number of objects to allocate.
*     size : The number of size to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     null
*****************************************************************/
void *zm_calloc(zm_size_t count, zm_size_t size)
{
    return zm_mem_calloc(count, size);
}
/*****************************************************************
* FUNCTION: zm_free
*
* DESCRIPTION: 
*       zm dynamic memory de-allocation.
* INPUTS:
*     ptr : The first address assigned by zm_malloc().
* RETURNS:
*     null
* NOTE:
*     null
*****************************************************************/
void zm_free(void *ptr)
{
    zm_mem_free(ptr);
}
/*****************************************************************
* FUNCTION: zm_getMemTotal
*
* DESCRIPTION: 
*       Get zm memory management total size.
* INPUTS:
*     null
* RETURNS:
*     memory total size.
* NOTE:
*     null
*****************************************************************/
zm_size_t zm_getMemTotal(void)
{
    return zmMemSize;
}
/*****************************************************************
* FUNCTION: zm_getMemUsed
*
* DESCRIPTION: 
*       Get zm memory management used size.
* INPUTS:
*     null
* RETURNS:
*     memory used size.
* NOTE:
*     If no set ZM_MEM_STATS to 1, It always returns 0.
*****************************************************************/
zm_size_t zm_getMemUsed(void)
{
#if ZM_MEM_STATS
    return memStats.usedSize;
#else
    return 0;
#endif
}
/*****************************************************************
* FUNCTION: zm_getMemMaxUsed
*
* DESCRIPTION: 
*       Get zm memory management max used size.
* INPUTS:
*     null
* RETURNS:
*     memory max used size.
* NOTE:
*     If no set ZM_MEM_STATS to 1, It always returns 0.
*****************************************************************/
zm_size_t zm_getMemMaxUsed(void)
{
#if ZM_MEM_STATS
    return memStats.maxSize;
#else
    return 0;
#endif
}

#else

/*****************************************************************
* FUNCTION: zm_malloc
*
* DESCRIPTION: 
*     zm dynamic memory allocation.
* INPUTS:
*     size : The number of bytes to allocate from the HEAP.
* RETURNS:
*     The first address of the allocated memory space.
*     NULL : faild, It may be out of memory.
* NOTE:
*     It's weak functions, you can redefine it.
*****************************************************************/
__ZM_WEAK void *zm_malloc(zm_size_t size)
{
    return malloc(size);
}
/*****************************************************************
* FUNCTION: zm_free
*
* DESCRIPTION: 
*       zm dynamic memory de-allocation.
* INPUTS:
*     ptr : The first address assigned by zm_mem_malloc().
* RETURNS:
*     null
* NOTE:
*     It's weak functions, you can redefine it.
*****************************************************************/
__ZM_WEAK void zm_free(void *ptr)
{
    free(ptr);
}
/*****************************************************************
* FUNCTION: zm_getMemTotal
*
* DESCRIPTION: 
*       Get zm memory management total size.
* INPUTS:
*     null
* RETURNS:
*     memory total size.
* NOTE:
*     null
*****************************************************************/
__ZM_WEAK zm_size_t zm_getMemTotal(void)
{
    return 0;
}
/*****************************************************************
* FUNCTION: zm_getMemUsed
*
* DESCRIPTION: 
*       Get zm memory management used size.
* INPUTS:
*     null
* RETURNS:
*     memory used size.
* NOTE:
*     
*****************************************************************/
__ZM_WEAK zm_size_t zm_getMemUsed(void)
{
    return 0;
}
/*****************************************************************
* FUNCTION: zm_getMemMaxUsed
*
* DESCRIPTION: 
*       Get zm memory management max used size.
* INPUTS:
*     null
* RETURNS:
*     memory max used size.
* NOTE:
*     
*****************************************************************/
__ZM_WEAK zm_size_t zm_getMemMaxUsed(void)
{
    return 0;
}
#endif
/****************************************************** END OF FILE ******************************************************/
