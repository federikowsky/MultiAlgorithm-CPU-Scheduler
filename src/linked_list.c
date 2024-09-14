#include <assert.h>
#include <stdio.h>

#include "../include/linked_list.h"

void List_init(ListHead *head)
{
	head->first = 0;
	head->last = 0;
	head->size = 0;
}

ListItem *List_find(ListHead *head, ListItem *item)
{
	// linear scanning of list
	ListItem *aux = head->first;
	while (aux)
	{
		if (aux == item)
			return item;
		aux = aux->next;
	}
	return 0;
}

ListItem *List_insert(ListHead *head, ListItem *prev, ListItem *item)
{
	if (item->next || item->prev)
		return 0;

#ifdef _LIST_DEBUG_
	// we check that the element is not in the list
	ListItem *instance = List_find(head, item);
	assert(!instance);

	// we check that the previous is inthe list

	if (prev)
	{
		ListItem *prev_instance = List_find(head, prev);
		assert(prev_instance);
	}
	// we check that the previous is inthe list
#endif

	ListItem *next = prev ? prev->next : head->first;
	if (prev)
	{
		item->prev = prev;
		prev->next = item;
	}
	if (next)
	{
		item->next = next;
		next->prev = item;
	}
	if (!prev)
		head->first = item;
	if (!next)
		head->last = item;
	++head->size;
	return item;
}

ListItem *List_detach(ListHead *head, ListItem *item)
{

#ifdef _LIST_DEBUG_
	// we check that the element is in the list
	ListItem *instance = List_find(head, item);
	assert(instance);
#endif

	ListItem *prev = item->prev;
	ListItem *next = item->next;
	if (prev)
	{
		prev->next = next;
	}
	if (next)
	{
		next->prev = prev;
	}
	if (item == head->first)
		head->first = next;
	if (item == head->last)
		head->last = prev;
	head->size--;
	item->next = item->prev = 0;
	return item;
}

ListItem *List_pushBack(ListHead *head, ListItem *item)
{
	return List_insert(head, head->last, item);
};

ListItem *List_pushFront(ListHead *head, ListItem *item)
{
	return List_insert(head, 0, item);
};

ListItem *List_popFront(ListHead *head)
{
	return List_detach(head, head->first);
}

/**
 * @brief sort the list using merge sort algorithm with complexity O(n log n) 
 * 
 * @param head pointer to the list to sort 
 * @param cmp function that takes two ListItem and returns an integer negative if the first is less than the second, 0 if they are equal, 
 * 			and a positive integer if the first is greater than the second
 */
void List_sort(ListHead *head, int (*cmp)(ListItem *, ListItem *))
{
	assert(head && "null pointer to list");
	assert(cmp && "null pointer to compare function");

	// if the list is empty or has only one element, it is already sorted
	if (head->size <= 1)
	{
		return;
	}

	// split the list in two parts and sort them recursively 
	ListHead left, right;
	List_init(&left);
	List_init(&right);

	int middle = head->size / 2;
	ListItem *aux = head->first;
	// iterate over the first half of the list and push the elements in the left list
	for (int i = 0; i < middle; i++)
	{
		// detach the element from the list and push it in the left list 
		List_pushBack(&left, List_detach(head, aux));
		aux = head->first;
	}
	// since head->size decreases at each iteration due to List_detach
	// iterate over the second half of the list and push the elements in the right list 
	// until the head list is empty 
	for (;head->size > 0;)
	{
		// detach the element from the list and push it in the right list
		List_pushBack(&right, List_detach(head, aux));
		aux = head->first;
	}

	List_sort(&left, cmp);
	List_sort(&right, cmp);
	List_init(head);

	// merge the two sorted lists until one of them is empty 
	while (left.size > 0 && right.size > 0)
	{
		if (cmp(left.first, right.first) <= 0)
		{
			List_pushBack(head, List_popFront(&left));
		}
		else
		{
			List_pushBack(head, List_popFront(&right));
		}
	}

	// at this point, one of the two lists is empty and the other is sorted 
	// we can append the remaining elements to the head list 
	while (left.size > 0)
	{
		List_pushBack(head, List_popFront(&left));
	}

	while (right.size > 0)
	{
		List_pushBack(head, List_popFront(&right));
	}
}

void List_print(ListHead *head, void (*print)(ListItem *))
{
	ListItem *aux = head->first;
	while (aux)
	{
		print(aux);
		aux = aux->next;
	}
	printf("\n");
}