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
        int inUse;
        int size;
        int order;
        int index;
        int addr;
        
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
            temp.size = -1;
            temp.order = -1;
            temp.index = i;
            temp.addr = PAGE_TO_ADDR(i);
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
        //find correct size
        int sizeNeeded = 2;
        int orderNeeded = 1;
        while(size>sizeNeeded)
        {
            sizeNeeded *=2;
            orderNeeded +=1;
        }
        
	//Make sure size is within limits
	if( orderNeeded > MAX_ORDER || orderNeeded < MIN_ORDER)
            return NULL;
	
	//search for free space of size orderNeeded.
	for (int curOrder = orderNeeded; curOrder<= MAX_ORDER; curOrder++)
        { 
            
            //First available free page of smallest size possible
            if(!list_empty(&free_area[curOrder]))
            {
                //page to allocate
                page_t *allocPage = list_entry(free_area[curOrder].next,page_t,list);
                
                //once found, split blocks till we have the size we need
                list_del_init(&(allocPage->list));
                while(curOrder != orderNeeded)
                {
                    page_t* buddyPage = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(allocPage->addr,curOrder-1))];
                    buddyPage->order = curOrder-1;
                    list_add(&(buddyPage->list),&free_area[curOrder-1]);
                    curOrder--;
                }
                
                //update page values and return
                allocPage->order = orderNeeded;
                allocPage->size = sizeNeeded;
                return ((void*)(allocPage->addr));
            }
	}
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
        //page to free
        page_t * freePage = &g_pages[ADDR_TO_PAGE(addr)];
        
        //get order to look for
	int freePageOrder=freePage->order;
	if(freePageOrder<MIN_ORDER || freePageOrder>MAX_ORDER)
        {
            printf("ORDER OUT OF BOUNDS");
            while(1);
	}
	
	
	//start merging buddies until we cannot anymore.
	int curOrder;
	for(curOrder = freePageOrder; curOrder<MAX_ORDER; curOrder++)
        {
            //page of the buddy of the page we are freeing
            page_t* buddyPage = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(freePage->addr,curOrder))];
            
            
            struct list_head *traverseHead;
            int buddyIsFree = 0;
            list_for_each(traverseHead,&free_area[curOrder])
            {
                //search for buddy in free list
                if(list_entry(traverseHead,page_t,list)==buddyPage)
                    buddyIsFree = 1;
            }
            
            //if buddy is not free then we are done.
            if (buddyIsFree == 0)
                break;
            else //otherwise remove it from the list so we can merge.
            {
                list_del_init(&buddyPage->list);
                if(buddyPage<freePage)
                    freePage = buddyPage;
            }
	}
	
	//add freed page to free list
	freePage->order = curOrder;
	list_add(&freePage->list,&free_area[curOrder]);
	
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
