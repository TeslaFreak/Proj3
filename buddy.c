/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
        int inUse;
        
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
            page_t temp;
            temp.inUse = 0;
            g_pages[i] = temp;
	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
        int sizeNeeded = 2;
        int orderNeeded = 1;
        while(size>sizeNeeded)
        {
            sizeNeeded *=2;
            orderNeeded +=1;
        }
        
	//Make sure size is within limits
	if( orderNeeded > MAX_ORDER || orderNeeded < MIN_ORDER)
        {
            return NULL;
	}
	
	//see if there is a open block of (exact) size orderNeeded.
	for (int curOrder = orderNeeded; curOrder<= MAX_ORDER; curOrder++) //curOrder = current order we are looking at.
        { 
            if(!list_empty(&free_area[curOrder]))//smallest page entry that is free.
            {
                //found it
                page_t *pg = list_entry(free_area[curOrder].next,page_t,list);//get page., need to remove from free_area before breakdown.
                
                list_del_init(&(pg->list));
                breakdown(pg, curOrder, orderNeeded);//pg's of the right size. and its buddy's(if we broke any blocks down, are reallocated to free_area)
                pg->order = orderNeeded;
                return ((void*)(pg->mem));
            }

	}
	//else return null as there is not enough room
	return NULL;
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
        page_t * pg = &g_pages[ADDR_TO_PAGE(addr)];
	int i;
	int currentOrder=pg->order;
	if((currentOrder<MIN_ORDER) || (currentOrder>MAX_ORDER)){
		printf("Error: currentOrder out of bounds! currentOrder=%d",currentOrder);
		while(1);
	}   
	//if currentOrder == Maxorder, just add to free_area at top level.
	for(i = currentOrder; i<MAX_ORDER; i++){//for however far up we can go
		//get the buddy,(if there is one!(toplevel does not have a buddy, and just needs to go back to free.))
		page_t* buddy = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(pg->mem,i))];
		char freed = 0;
		struct list_head *pos;
		list_for_each(pos,&free_area[i]){

			if(list_entry(pos,page_t,list)==buddy){//if we find buddy in free area list,
			//if(pos == ){
				freed = 1;
			}
		}
		if (!freed){//if cant free buddy, break
			break;
		}

		list_del_init(&buddy->list);//remove from the free area list.
		//remove buudy from free list, 
		//go higher.
		if(buddy<pg){
			pg = buddy;
		}
	}
	pg->order = i;
	list_add(&pg->list,&free_area[i]);
	//now add page to free area at level i.
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
