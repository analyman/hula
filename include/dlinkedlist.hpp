#pragma once

template<typename T>
struct DLinkedList {
    DLinkedList* next;
    DLinkedList* prev;
    T value;

    DLinkedList(const T& value): value(value) {}
};

template<typename T>
void DLinkedList_insert(DLinkedList<T>** cbl, T value) //{
{
    DLinkedList<T>* new_entry = new DLinkedList<T>(value);

    if(*cbl == nullptr) {
        *cbl = new_entry;
        return;
    }

    DLinkedList<T>* cbl_next = (*cbl)->next;

    (*cbl)->next = new_entry;
    new_entry->prev = *cbl;

    if(cbl_next != nullptr) {
        cbl_next->prev = new_entry;
        new_entry->next = cbl_next;
    }
    return;
} //}
template<typename T>
void DLinkedList_delete(DLinkedList<T>** cbl) //{
{
    assert(*cbl != nullptr);

    DLinkedList<T>* cbl_prev = (*cbl)->prev;
    DLinkedList<T>* cbl_next = (*cbl)->next;

    delete *cbl;

    *cbl = nullptr;

    if(cbl_prev != nullptr) {
        cbl_prev->next = cbl_next;
        *cbl = cbl_prev;
    }
    if(cbl_next != nullptr) {
        cbl_next->prev = cbl_prev;
        if(*cbl == nullptr) *cbl = cbl_next;
    }
    return;
} //}
template<typename T>
void DLinkedList_head(DLinkedList<T>** cbl) //{
{
    DLinkedList<T>* c = *cbl;
    if(c == nullptr) return;
    while(c->prev != nullptr)
        c = c->prev;
    *cbl = c;
} //}
template<typename T>
void DLinkedList_tail(DLinkedList<T>** cbl) //{
{
    DLinkedList<T>* c = *cbl;
    if(c == nullptr) return;
    while(c->next != nullptr)
        c = c->next;
    *cbl = c;
} //}
template<typename T>
void DLinkedList_insert_to_head(DLinkedList<T>** cbl, T value) //{
{
    DLinkedList_head(cbl);
    DLinkedList<T>* new_entry = new DLinkedList<T>(value);
    
    DLinkedList<T>* old_head = *cbl;
    *cbl = new_entry;
    new_entry->next = old_head;
    if(old_head != nullptr)
        old_head->prev = new_entry;
} //}
template<typename T>
void DLinkedList_insert_to_tail(DLinkedList<T>** cbl, T value) //{
{
    DLinkedList_tail(cbl);
    DLinkedList<T>* new_entry = new DLinkedList<T>();
    
    DLinkedList<T>* old_tail = *cbl;
    *cbl = new_entry;
    new_entry->prev = old_tail;
    if(old_tail != nullptr)
        old_tail->next = new_entry;
} //}

