#pragma once

#include <stddef.h>
#include <stdint.h>

struct list_head
{
    struct list_head* next;
    struct list_head* prev;
};

typedef struct list_head list_head_t;

#define LIST_INIT(list)     { &(list), &(list) }
#define LIST_DECLARE(name)  list_head_t name = LIST_INIT(name)
#define ADDR(x)             (uintptr_t)(x)

static inline void M_ListInit(list_head_t* list)
{
    list->next = list->prev = list;
}

static inline void __M_ListAdd(list_head_t* new, list_head_t* prev, list_head_t* next)
{
    next->prev = new;
    prev->next = new;
    new->next = next;
    new->prev = prev;
}

static inline void __M_ListDel(list_head_t* prev, list_head_t* next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void M_ListAddHead(list_head_t* new, list_head_t* head)
{
    __M_ListAdd(new, head, head->next);
}

static inline void M_ListAddTail(list_head_t* new, list_head_t* head)
{
    __M_ListAdd(new, head->prev, head);
}

static inline int M_ListEmpty(list_head_t* entry)
{
    return (entry->next == entry);
}

static inline void M_ListDel(list_head_t* entry)
{
    __M_ListDel(entry->prev, entry->next);
    entry->next = (void*)entry;
    entry->prev = (void*)entry;
}

#define M_ListEntry(ptr, type, member) \
    ({ \
       ((type*)(ADDR(ptr) - ADDR(offsetof(type, member)))); \
    })

#define M_ListNextEntry(ptr, type, member) \
    ({ \
       ((type*)(ADDR((ptr)->next) - ADDR(offsetof(type, member)))); \
    })

#define M_ListPrevEntry(ptr, type, member) \
    ({ \
       ((type*)(ADDR((ptr)->prev) - ADDR(offsetof(type, member)))); \
    })

#define M_ListFront(head, type, member) \
    M_ListNextEntry(head, type, member)

#define M_ListBack(head, type, member) \
    M_ListPrevEntry(head, type, member)

#define M_ListForEach(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define M_ListForEach_entry(pos, head, member) \
    for (pos = M_ListEntry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = M_ListEntry(pos->member.next, typeof(*pos), member))
