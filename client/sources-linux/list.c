/*!
 * @file list.c
 * @brief Definitions for functions that operate on lists.
 * @details An implementation of a simple thread safe double linked list structure. Can be used as either
 *          a stack (via pop/push), a queue (via push/shift) or an array (via get/add/insert/remove). If
 *          performing a group of actions on a list based on results from list actions, acquire the list
 *          lock before the group of actions and release lock when done.
 */
//#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#include "list.h"

/*!
 * @brief Create a thread-safe double linked list.
 * @returns A new instance of a linked list.
 * @retval NULL Indicates a memory allocation failure.
 */
PLIST list_create(void)
{
	PLIST pList = (PLIST)malloc(sizeof(LIST));

	if (pList != NULL)
	{
		pList->start = NULL;
		pList->end = NULL;
		pList->count = 0;
		pthread_mutex_init(&pList->lock, NULL);
	}
	return pList;
}

/*!
 * @brief Destroy an existing linked list.
 * @details This destroys all nodes and the list itself but not the data held in the
 *          linked list. This is the responsibility of the caller to destroy.
 * @param list The \c LIST instance to destroy.
 */
void list_destroy(PLIST pList)
{
	PNODE current_node;
	PNODE next_node;

	if (pList != NULL)
	{
		pthread_mutex_lock(&pList->lock);

		current_node = pList->start;

		while (current_node != NULL)
		{
			next_node = current_node->next;

			current_node->next = NULL;

			current_node->prev = NULL;

			free(current_node);

			current_node = next_node;
		}

		pList->count = 0;

		pthread_mutex_unlock(&pList->lock);

		pthread_mutex_destroy(&pList->lock);

		free(pList);
	}
}

/*!
 * @brief Get the number of items in the list.
 * @param pList The \c LIST to get a count of.
 * @returns The number of elements in the list.
 * @remark If using this coung value to itterate through the list with `list_get`, acquire
 *         the lists lock before the `list_count/list_get` block and release it afterwards.
 */
unsigned int list_count(PLIST pList)
{
	unsigned int count = 0;

	if (pList != NULL)
	{
		pthread_mutex_lock(&pList->lock);

		count = pList->count;

		pthread_mutex_unlock(&pList->lock);
	}

	return count;
}

/*!
 * @brief Get the data value held in the list and a specified index.
 * @param pList Pointer to the \c LIST to get the element from.
 * @param index Index of the element to get;
 * @returns Pointer to the item in the list.
 * @retval NULL Indicates the element doesn't exist in the list.
 * @remark This will perform a linear search from the beginning of the list.
 */
void * list_get(PLIST pList, unsigned int index)
{
	void * data = NULL;
	PNODE current_node = NULL;

	if (pList == NULL)
		return NULL;

	pthread_mutex_lock(&pList->lock);

	if (pList->count <= index)
	{
		pthread_mutex_unlock(&pList->lock);
		return NULL;
	}

	current_node = pList->start;

	while (current_node != NULL)
	{
		if (index == 0)
		{
			break;
		}

		current_node = current_node->next;

		index--;
	}

	if (current_node != NULL)
	{
		data = current_node->data;
	}

	pthread_mutex_unlock(&pList->lock);

	return data;
}

/*!
 * @brief Add a data item onto the end of the list.
 * @param pList Pointer to the \c LIST to add the item to.
 * @param data The data that is to be added to the list.
 * @returns Indication of success or failure.
 * @sa list_push
 */
bool list_add(PLIST pList, void * data)
{
	return list_push(pList, data);
}

/*!
 * @brief Internal function to remove a node from a list.
 * @param pList Pointer to the \c LIST containing \c node.
 * @param pNode Pointer to the \c NOTE to remove.
 * @returns Indication of success or failure.
 * @remark Assumes caller has aquired the appropriate lock first.
 */
bool list_remove_node(PLIST pList, PNODE pNode)
{
	if (pList == NULL || pNode == NULL)
	{
		return false;
	}

	if (pList->count - 1 == 0)
	{
		pList->start = NULL;
		pList->end = NULL;
	}
	else
	{
		if (pList->start == pNode)
		{
			pList->start = pList->start->next;
			pList->start->prev = NULL;
		}
		else if (pList->end == pNode)
		{
			pList->end = pList->end->prev;
			pList->end->next = NULL;
		}
		else
		{
			pNode->next->prev = pNode->prev;
			pNode->prev->next = pNode->next;
		}
	}

	pList->count -= 1;

	pNode->next = NULL;

	pNode->prev = NULL;

	free(pNode);

	return true;
}

/*!
 * @brief Remove a given data item from the list.
 * @param pList Pointer to the \c LIST to remove the item from.
 * @param data The data that is to be removed from the list.
 * @remark Assumes data items are unqique as only the first occurrence is removed.
 * @returns Indication of success or failure.
 * @sa list_remove_node
 */
bool list_remove(PLIST pList, void * data)
{
	bool result = false;
	PNODE current_node = NULL;

	if (pList == NULL || data == NULL)
	{
		return false;
	}

	pthread_mutex_lock(&pList->lock);

	current_node = pList->start;

	while (current_node != NULL)
	{
		if (current_node->data == data)
		{
			break;
		}

		current_node = current_node->next;
	}

	result = list_remove_node(pList, current_node);

	pthread_mutex_unlock(&pList->lock);

	return result;
}

/*!
 * @brief Remove a list item at the specified index.
 * @param pList Pointer to the \c LIST to remove the item from.
 * @param index Index of the item to remove.
 * @returns Indication of success or failure.
 */
bool list_delete(PLIST pList, unsigned int index)
{
	bool result = false;
	void * data = NULL;
	PNODE current_node = NULL;

	if (pList == NULL)
	{
		return false;
	}

	pthread_mutex_lock(&pList->lock);

	if (pList->count > index)
	{
		current_node = pList->start;

		while (current_node != NULL)
		{
			if (index == 0)
			{
				result = list_remove_node(pList, current_node);
				break;
			}

			current_node = current_node->next;

			index--;
		}
	}

	pthread_mutex_unlock(&pList->lock);

	return result;
}

/*!
 * @brief Push a data item onto the end of the list.
 * @param pList Pointer to the \c LIST to append the data to.
 * @param data Pointer to the data to append.
 * @returns Indication of success or failure.
 */
bool list_push(PLIST pList, void * data)
{
	PNODE pNode = NULL;

	if (pList == NULL)
		return false;

	pNode = (PNODE)malloc(sizeof(NODE));
	if (pNode == NULL)
	{
		return false;
	}

	pNode->data = data;
	pNode->next = NULL;
	pNode->prev = NULL;

	pthread_mutex_lock(&pList->lock);

	if (pList->end != NULL)
	{
		pList->end->next = pNode;

		pNode->prev = pList->end;

		pList->end = pNode;
	}
	else
	{
		pList->start = pNode;
		pList->end = pNode;
	}

	pList->count += 1;

	pthread_mutex_unlock(&pList->lock);

	return true;
}

/*!
 * @brief Pop a data value off the end of the list.
 * @param pList Pointer to the \c LIST to pop the value from.
 * @returns The popped value.
 * @retval NULL Indicates no data in the list.
 */
void * list_pop(PLIST pList)
{
	void * data = NULL;

	if (pList == NULL)
	{
		return NULL;
	}

	pthread_mutex_lock(&pList->lock);

	if (pList->end != NULL)
	{
		data = pList->end->data;

		list_remove_node(pList, pList->end);
	}

	pthread_mutex_unlock(&pList->lock);

	return data;
}

/*!
 * @brief Pop a data value off the start of the list.
 * @param pList Pointer to the \c LIST to shift the value from.
 * @returns The shifted value.
 * @retval NULL Indicates no data in the list.
 */
void * list_shift(PLIST pList)
{
	void * data = NULL;

	if (pList == NULL)
	{
		return NULL;
	}

	pthread_mutex_lock(&pList->lock);

	if (pList->start != NULL)
	{
		data = pList->start->data;

		list_remove_node(pList, pList->start);
	}

	pthread_mutex_unlock(&pList->lock);

	return data;
}

/*!
 * @brief Iterate over the list and call a function callback on each element.
 * @param pList Pointer to the \c LIST to enumerate.
 * @param pCallback Callback function to invoke for each element in the list.
 * @param pState Pointer to the state to pass with each function call.
 */
bool list_enumerate(PLIST pList, PLISTENUMCALLBACK pCallback, void * pState)
{
	PNODE pCurrent;
	bool bResult;
	if (pList == NULL || pCallback == NULL)
	{
		return false;
	}

	pthread_mutex_lock(&pList->lock);


	pCurrent=pList->start;
	bResult = false;

	while (pCurrent != NULL)
	{
		bResult = pCallback(pState, pCurrent->data) || bResult;
		pCurrent = pCurrent->next;
	}

	pthread_mutex_unlock(&pList->lock);
	return bResult;
}
