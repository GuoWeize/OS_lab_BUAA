#ifndef _SYS_QUEUE_H_
#define _SYS_QUEUE_H_

/*********************************List declarations*********************************/

/* 一个 list 以一个 LIST_HEAD 宏定义的结构开头, 这个结构包括一个指向 list 首元素的简单指针。
元素是双向链接的, 因此删除随意一个元素不用遍历整个 list。
新的元素可以添加到一个元素的后面 或 list开头。
LIST_HEAD 结构被声明为: LIST_HEAD (name, type) head;
其中 name 是被定义结构的名字, type是元素的类型。*/
#define LIST_HEAD(name, type)                                           \
        struct name {                                                           \
                struct type *lh_first;  /* 指向首元素的指针 */                     \
        }

/* 设置 list 开头变量为 LIST_HEAD_INITIALIZER(head) 来清空 list */
#define LIST_HEAD_INITIALIZER(head)                                     \
        { NULL }

/* 使用 LIST_ENTRY(type) field 作为 list 的迭代工具。
le_prev 指向一个指向这个 LIST_ENTRY 结构的指针, 可以用 *le_prev = le_next 来删除这个 entry */
#define LIST_ENTRY(type)                                                \
        struct {                                                                \
                struct type *le_next;   /* 指向下一个元素 */                      \
                struct type **le_prev;  /* 指向前一个元素的 le_next */  \
        }


/*********************************List functions*********************************/

/* 判断名为 head 的 list 是否为空 */
#define LIST_EMPTY(head)        ((head)->lh_first == NULL)

/*返回名为 head 的 list 的首元素 */
#define LIST_FIRST(head)        ((head)->lh_first)

/* 遍历名为 head 的 list。
在循环中, list 元素为变量 var, 使用 LIST_ENTRY 结构成员 field 作为 link field */
#define LIST_FOREACH(var, head, field)                                  \
        for ((var) = LIST_FIRST((head));                                \
                 (var);                                                 \
                 (var) = LIST_NEXT((var), field))

/* 清空名为 head 的 list */
#define LIST_INIT(head) do {                                            \
                LIST_FIRST((head)) = NULL;                                      \
        } while (0)

/* 将元素 elm 插入到元素 listelm 后面, field 是如上的 link 元素 */
#define LIST_INSERT_AFTER(listelm, elm, field) do {                     \
                if ((LIST_NEXT((elm), field) = LIST_NEXT((listelm), field)) != NULL)\
                        LIST_NEXT((listelm), field)->field.le_prev =            \
                                        &LIST_NEXT((elm), field);                               \
                LIST_NEXT((listelm), field) = (elm);                            \
                (elm)->field.le_prev = &LIST_NEXT((listelm), field);            \
        } while (0)

/* 将元素 elm 插入到元素 listelm 前面, field 是如上的 link 元素 */
#define LIST_INSERT_BEFORE(listelm, elm, field) do {                    \
                (elm)->field.le_prev = (listelm)->field.le_prev;                \
                LIST_NEXT((elm), field) = (listelm);                            \
                *(listelm)->field.le_prev = (elm);                              \
                (listelm)->field.le_prev = &LIST_NEXT((elm), field);            \
        } while (0)

/* 将元素 elm 插入到名为 head 的 list 的开头, field 是如上的 link 元素 */
#define LIST_INSERT_HEAD(head, elm, field) do {                         \
                if ((LIST_NEXT((elm), field) = LIST_FIRST((head))) != NULL)     \
                        LIST_FIRST((head))->field.le_prev = &LIST_NEXT((elm), field);\
                LIST_FIRST((head)) = (elm);                                     \
                (elm)->field.le_prev = &LIST_FIRST((head));                     \
        } while (0)

/* elm 的下一个元素 */
#define LIST_NEXT(elm, field)   ((elm)->field.le_next)

/* 删除元素 elm , field是如上的 link 元素 */
#define LIST_REMOVE(elm, field) do {                                    \
                if (LIST_NEXT((elm), field) != NULL)                            \
                        LIST_NEXT((elm), field)->field.le_prev =                \
                                        (elm)->field.le_prev;                           \
                *(elm)->field.le_prev = LIST_NEXT((elm), field);                \
        } while (0)




/*
* Tail queue definitions.
*/
#define TAILQ_HEAD(name, type)                                          \
struct name {                                                           \
        struct type *tqh_first; /* first element */                     \
        struct type **tqh_last; /* addr of last next element */         \
}

#define TAILQ_ENTRY(type)                                               \
struct {                                                                \
        struct type *tqe_next;  /* next element */                      \
        struct type **tqe_prev; /* address of previous next element */  \
}

/*
* Tail queue functions.
*/
#define TAILQ_INIT(head) {                                              \
        (head)->tqh_first = NULL;                                       \
        (head)->tqh_last = &(head)->tqh_first;                          \
}

#define TAILQ_INSERT_HEAD(head, elm, field) {                           \
        if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)        \
                (head)->tqh_first->field.tqe_prev =                     \
                    &(elm)->field.tqe_next;                             \
        else                                                            \
                (head)->tqh_last = &(elm)->field.tqe_next;              \
        (head)->tqh_first = (elm);                                      \
        (elm)->field.tqe_prev = &(head)->tqh_first;                     \
}

#define TAILQ_INSERT_TAIL(head, elm, field) {                           \
        (elm)->field.tqe_next = NULL;                                   \
        (elm)->field.tqe_prev = (head)->tqh_last;                       \
        *(head)->tqh_last = (elm);                                      \
        (head)->tqh_last = &(elm)->field.tqe_next;                      \
}

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) {                 \
        if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)\
                (elm)->field.tqe_next->field.tqe_prev =                 \
                    &(elm)->field.tqe_next;                             \
        else                                                            \
                (head)->tqh_last = &(elm)->field.tqe_next;              \
        (listelm)->field.tqe_next = (elm);                              \
        (elm)->field.tqe_prev = &(listelm)->field.tqe_next;             \
}

#define TAILQ_INSERT_BEFORE(listelm, elm, field) {                      \
        (elm)->field.tqe_prev = (listelm)->field.tqe_prev;              \
        (elm)->field.tqe_next = (listelm);                              \
        *(listelm)->field.tqe_prev = (elm);                             \
        (listelm)->field.tqe_prev = &(elm)->field.tqe_next;             \
}

#define TAILQ_REMOVE(head, elm, field) {                                \
        if (((elm)->field.tqe_next) != NULL)                            \
                (elm)->field.tqe_next->field.tqe_prev =                 \
                    (elm)->field.tqe_prev;                              \
        else                                                            \
                (head)->tqh_last = (elm)->field.tqe_prev;               \
        *(elm)->field.tqe_prev = (elm)->field.tqe_next;                 \
}

/*
* Circular queue definitions.
*/
#define CIRCLEQ_HEAD(name, type)                                        \
struct name {                                                           \
        struct type *cqh_first;         /* first element */             \
        struct type *cqh_last;          /* last element */              \
}

#define CIRCLEQ_ENTRY(type)                                             \
struct {                                                                \
        struct type *cqe_next;          /* next element */              \
        struct type *cqe_prev;          /* previous element */          \
}

/*
* Circular queue functions.
*/
#define CIRCLEQ_INIT(head) {                                            \
        (head)->cqh_first = (void *)(head);                             \
        (head)->cqh_last = (void *)(head);                              \
}

#define CIRCLEQ_INSERT_AFTER(head, listelm, elm, field) {               \
        (elm)->field.cqe_next = (listelm)->field.cqe_next;              \
        (elm)->field.cqe_prev = (listelm);                              \
        if ((listelm)->field.cqe_next == (void *)(head))                \
                (head)->cqh_last = (elm);                               \
        else                                                            \
                (listelm)->field.cqe_next->field.cqe_prev = (elm);      \
        (listelm)->field.cqe_next = (elm);                              \
}

#define CIRCLEQ_INSERT_BEFORE(head, listelm, elm, field) {              \
        (elm)->field.cqe_next = (listelm);                              \
        (elm)->field.cqe_prev = (listelm)->field.cqe_prev;              \
        if ((listelm)->field.cqe_prev == (void *)(head))                \
                (head)->cqh_first = (elm);                              \
        else                                                            \
                (listelm)->field.cqe_prev->field.cqe_next = (elm);      \
        (listelm)->field.cqe_prev = (elm);                              \
}

#define CIRCLEQ_INSERT_HEAD(head, elm, field) {                         \
        (elm)->field.cqe_next = (head)->cqh_first;                      \
        (elm)->field.cqe_prev = (void *)(head);                         \
        if ((head)->cqh_last == (void *)(head))                         \
                (head)->cqh_last = (elm);                               \
        else                                                            \
                (head)->cqh_first->field.cqe_prev = (elm);              \
        (head)->cqh_first = (elm);                                      \
}

#define CIRCLEQ_INSERT_TAIL(head, elm, field) {                         \
        (elm)->field.cqe_next = (void *)(head);                         \
        (elm)->field.cqe_prev = (head)->cqh_last;                       \
        if ((head)->cqh_first == (void *)(head))                        \
                (head)->cqh_first = (elm);                              \
        else                                                            \
                (head)->cqh_last->field.cqe_next = (elm);               \
        (head)->cqh_last = (elm);                                       \
}

#define CIRCLEQ_REMOVE(head, elm, field) {                              \
        if ((elm)->field.cqe_next == (void *)(head))                    \
                (head)->cqh_last = (elm)->field.cqe_prev;               \
        else                                                            \
                (elm)->field.cqe_next->field.cqe_prev =                 \
                    (elm)->field.cqe_prev;                              \
        if ((elm)->field.cqe_prev == (void *)(head))                    \
                (head)->cqh_first = (elm)->field.cqe_next;              \
        else                                                            \
                (elm)->field.cqe_prev->field.cqe_next =                 \
                    (elm)->field.cqe_next;                              \
}


#endif  /* !_SYS_QUEUE_H_ */
