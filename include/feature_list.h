#ifndef FEATURE_LIST_H
#define FEATURE_LIST_H

#include <stdbool.h>
#include <stddef.h>

#define FEATURE_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define feature_container_of(ptr, type, member) \
    ((type*)((uintptr_t)(ptr)-offsetof(type, member)))

typedef struct feature_list_node {
    struct feature_list_node* prev;
    struct feature_list_node* next;
} feature_list_node;

#define feature_list_in_list(item) ((item)->prev != NULL)
#define feature_list_is_empty(list) ((list)->next == (list) && (list)->prev == (list))
#define feature_list_is_singular(list) ((list)->next == (list)->prev)

#define feature_list_initialize(list)              \
    do {                                           \
        struct feature_list_node* __list = (list); \
        __list->prev = __list->next = __list;      \
    } while (0)

#define feature_list_delete(item)                  \
    do {                                           \
        struct feature_list_node* __item = (item); \
        __item->next->prev = __item->prev;         \
        __item->prev->next = __item->next;         \
        __item->prev = __item->next = NULL;        \
    } while (0)

#define feature_list_add_tail(list, item)          \
    do {                                           \
        struct feature_list_node* __list = (list); \
        struct feature_list_node* __item = (item); \
        __item->prev = __list->prev;               \
        __item->next = __list;                     \
        __list->prev->next = __item;               \
        __list->prev = __item;                     \
    } while (0)

#define feature_list_for_every_safe(list, node, temp) \
    for (node = (list)->next, temp = node->next;      \
         node != (list); node = temp, temp = node->next)

#define feature_list_for_every_entry_safe(list, entry, temp, type, member) \
    for (entry = feature_container_of((list)->next, type, member),         \
        temp = feature_container_of(entry->member.next, type, member);     \
         &entry->member != (list); entry = temp,                           \
        temp = feature_container_of(temp->member.next, type, member))

#define weakref_list_node feature_list_node
#define weakref_container_of(ptr, type, member) feature_container_of(ptr, type, member)
#define weakref_list_initialize(list) feature_list_initialize(list)
#define weakref_list_delete(item) feature_list_delete(item)
#define weakref_list_add_tail(list, item) feature_list_add_tail(list, item)
#define weakref_list_for_every_entry_safe(list, entry, temp, type, member) \
    feature_list_for_every_entry_safe(list, entry, temp, type, member)

#endif // FEATURE_LIST_H
