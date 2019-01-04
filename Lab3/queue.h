#ifndef _SYS_QUEUE_H_
#define _SYS_QUEUE_H_

/*********************************List declarations*********************************/

/* һ�� list ��һ�� LIST_HEAD �궨��Ľṹ��ͷ, ����ṹ����һ��ָ�� list ��Ԫ�صļ�ָ�롣
Ԫ����˫�����ӵ�, ���ɾ������һ��Ԫ�ز��ñ������� list��
�µ�Ԫ�ؿ�����ӵ�һ��Ԫ�صĺ��� �� list��ͷ��
LIST_HEAD �ṹ������Ϊ: LIST_HEAD (name, type) head;
���� name �Ǳ�����ṹ������, type��Ԫ�ص����͡�*/
#define LIST_HEAD(name, type)                                           \
        struct name {                                                           \
                struct type *lh_first;  /* ָ����Ԫ�ص�ָ�� */                     \
        }

/* ���� list ��ͷ����Ϊ LIST_HEAD_INITIALIZER(head) ����� list */
#define LIST_HEAD_INITIALIZER(head)                                     \
        { NULL }

/* ʹ�� LIST_ENTRY(type) field ��Ϊ list �ĵ������ߡ�
le_prev ָ��һ��ָ����� LIST_ENTRY �ṹ��ָ��, ������ *le_prev = le_next ��ɾ����� entry */
#define LIST_ENTRY(type)                                                \
        struct {                                                                \
                struct type *le_next;   /* ָ����һ��Ԫ�� */                      \
                struct type **le_prev;  /* ָ��ǰһ��Ԫ�ص� le_next */  \
        }


/*********************************List functions*********************************/

/* �ж���Ϊ head �� list �Ƿ�Ϊ�� */
#define LIST_EMPTY(head)        ((head)->lh_first == NULL)

/*������Ϊ head �� list ����Ԫ�� */
#define LIST_FIRST(head)        ((head)->lh_first)

/* ������Ϊ head �� list��
��ѭ����, list Ԫ��Ϊ���� var, ʹ�� LIST_ENTRY �ṹ��Ա field ��Ϊ link field */
#define LIST_FOREACH(var, head, field)                                  \
        for ((var) = LIST_FIRST((head));                                \
                 (var);                                                 \
                 (var) = LIST_NEXT((var), field))

/* �����Ϊ head �� list */
#define LIST_INIT(head) do {                                            \
                LIST_FIRST((head)) = NULL;                                      \
        } while (0)

/* ��Ԫ�� elm ���뵽Ԫ�� listelm ����, field �����ϵ� link Ԫ�� */
#define LIST_INSERT_AFTER(listelm, elm, field) do {                     \
                if ((LIST_NEXT((elm), field) = LIST_NEXT((listelm), field)) != NULL)\
                        LIST_NEXT((listelm), field)->field.le_prev =            \
                                        &LIST_NEXT((elm), field);                               \
                LIST_NEXT((listelm), field) = (elm);                            \
                (elm)->field.le_prev = &LIST_NEXT((listelm), field);            \
        } while (0)

/* ��Ԫ�� elm ���뵽Ԫ�� listelm ǰ��, field �����ϵ� link Ԫ�� */
#define LIST_INSERT_BEFORE(listelm, elm, field) do {                    \
                (elm)->field.le_prev = (listelm)->field.le_prev;                \
                LIST_NEXT((elm), field) = (listelm);                            \
                *(listelm)->field.le_prev = (elm);                              \
                (listelm)->field.le_prev = &LIST_NEXT((elm), field);            \
        } while (0)

/* ��Ԫ�� elm ���뵽��Ϊ head �� list �Ŀ�ͷ, field �����ϵ� link Ԫ�� */
#define LIST_INSERT_HEAD(head, elm, field) do {                         \
                if ((LIST_NEXT((elm), field) = LIST_FIRST((head))) != NULL)     \
                        LIST_FIRST((head))->field.le_prev = &LIST_NEXT((elm), field);\
                LIST_FIRST((head)) = (elm);                                     \
                (elm)->field.le_prev = &LIST_FIRST((head));                     \
        } while (0)

/* elm ����һ��Ԫ�� */
#define LIST_NEXT(elm, field)   ((elm)->field.le_next)

/* ɾ��Ԫ�� elm , field�����ϵ� link Ԫ�� */
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
