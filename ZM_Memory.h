/*****************************************************************
* Copyright (C) 2021 zm. All rights reserved.                    *
******************************************************************
* ZM_Memory.h
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
#ifndef __ZM_MEMORY_H__
#define __ZM_MEMORY_H__
 
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/

/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/ 
#define ZM_USE_MEM_MGR          1
#define ZM_MEM_USE_HEAP         1
#define ZM_MEM_STATS            1

#define ZM_ALIGN_SIZE           4
#define ZM_MIN_SIZE             12

#define __ZM_WEAK               __weak

/**
 * ZM_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. ZM_ALIGN(13, 4)
 * would return 16.
 */
#define ZM_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * ZM_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. ZM_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define ZM_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#if ZM_MEM_USE_HEAP

#define ZM_HEAP_BEGIN           (0)
#define ZM_HEAP_END             (0)
/*
#if defined(__CC_ARM) || defined(__CLANG_ARM)
extern int Image$RW_IRAM1$ZI$Limit;
#define ZM_HEAP_BEGIN      ((void *)&Image$RW_IRAM1$ZI$Limit)
#elif __ICCARM__
#pragma section="CSTACK"
#define ZM_HEAP_BEGIN      (__segment_end("CSTACK"))
#else
extern int __bss_end;
#define ZM_HEAP_BEGIN      ((void *)&__bss_end)
#endif
*/

#else

#define  ZM_MEM_SIZE            (8192)

#endif

/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
typedef signed char zm_int8_t;         //!< Signed 8 bit integer
typedef unsigned char zm_uint8_t;      //!< Unsigned 8 bit integer

typedef signed short zm_int16_t;       //!< Signed 16 bit integer
typedef unsigned short zm_uint16_t;    //!< Unsigned 16 bit integer

typedef signed int zm_int32_t;         //!< Signed 32 bit integer
typedef unsigned int zm_uint32_t;      //!< Unsigned 32 bit integer

typedef zm_uint32_t zm_size_t;
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/

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
void zm_memoryMgrInit(void);
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
void *zm_malloc(zm_size_t size);
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
void *zm_realloc(void *ptr, zm_size_t newsize);
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
void *zm_calloc(zm_size_t count, zm_size_t size);
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
void zm_free(void *ptr);
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
zm_size_t zm_getMemTotal(void);
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
*     If no set zm_MEM_STATS to 1, It always returns 0.
*****************************************************************/
zm_size_t zm_getMemUsed(void);
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
*     If no set zm_MEM_STATS to 1, It always returns 0.
*****************************************************************/
zm_size_t zm_getMemMaxUsed(void);



#ifdef __cplusplus
}
#endif
#endif /* ZM_Memory.h */
