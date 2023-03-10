#include <rtos/glist.h>

void glist_push_front(glist_t *list, glist_node_t node)
{
    struct glist_hdr_t *ptr = (struct glist_hdr_t *)node;
    ptr->link_next = list->entry;

    if (! list->entry)
        list->extry = &ptr->link_next;
    list->entry = ptr;
}

void glist_push_back(glist_t *list, glist_node_t node)
{
    struct glist_hdr_t *ptr = (struct glist_hdr_t *)node;
    ptr->link_next = NULL;

    *(list->extry) = ptr;
    list->extry = &ptr->link_next;
}

glist_node_t glist_pop(glist_t *list)
{
    struct glist_hdr_t *ptr = list->entry;

    if (ptr)
    {
        list->entry = ptr->link_next;
        ptr->link_next = NULL;

        if (! list->entry)
            list->extry = &list->entry;
    }
    return ptr;
}

glist_iter_t glist_iter_next(glist_t *list, glist_iter_t iter)
{
    struct glist_hdr_t *cur = *(struct glist_hdr_t **)iter;

    if (cur->link_next)         // for safer check
        return &cur->link_next;
    else
        return list->extry;
}

void glist_iter_insert(glist_t *list, glist_iter_t iter, glist_node_t node)
{
    ARG_UNUSED(list);

    ((struct glist_hdr_t *)node)->link_next = *(struct glist_hdr_t **)iter;
    *(struct glist_hdr_t **)iter = node;
}

void glist_iter_insert_after(glist_t *list, glist_iter_t iter, glist_node_t node)
{
    if (iter == list->extry)
        list->extry = (struct glist_hdr_t **)&node;

    ((struct glist_hdr_t *)node)->link_next = (*(struct glist_hdr_t **)iter)->link_next;
    *(struct glist_hdr_t **)iter = node;
}

glist_node_t glist_iter_extract(glist_t *list, glist_iter_t iter)
{
    struct glist_hdr_t *ptr = *(struct glist_hdr_t **)iter;

    if (ptr)
    {
        *(struct glist_hdr_t **)iter = ptr->link_next;

        if (NULL == list->entry || list->extry == &ptr->link_next)
            list->extry = (struct glist_hdr_t **)iter;

        ptr->link_next = NULL;
    }
    return ptr;
}

glist_iter_t glist_find(glist_t *list, glist_node_t node)
{
    for (glist_iter_t iter = glist_iter_begin(list);
        iter != glist_iter_end(list);
        iter = glist_iter_next(list, iter))
    {
        if (node == *(struct glist_hdr_t **)iter)
            return iter;
    }
    return NULL;
}
