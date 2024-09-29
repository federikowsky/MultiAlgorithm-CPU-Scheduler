#pragma once

typedef struct ListItem {
  struct ListItem* prev;
  struct ListItem* next;
} ListItem;

typedef struct ListHead {
  ListItem* first;
  ListItem* last;
  int size;
} ListHead;

void List_init(ListHead* head);
int List_empty(ListHead *head);
ListItem *List_getAt(ListHead *head, int index);
ListItem *List_find(ListHead* head, ListItem* item);
ListItem *List_insert(ListHead* head, ListItem* previous, ListItem* item);
ListItem *List_detach(ListHead* head, ListItem* item);
ListItem *List_pushBack(ListHead* head, ListItem* item);
ListItem *List_pushFront(ListHead* head, ListItem* item);
ListItem *List_popFront(ListHead* head);
void List_sort(ListHead* head, int (*cmp)(ListItem*, ListItem*));
void List_print(ListHead *head, void (*print)(ListItem *));