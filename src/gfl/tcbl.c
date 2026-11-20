#include "types.h"
#include "gfl/heap.h"
#include "gfl/tcbl.h"

// Tasks like tcb.c's, each with data of its own: in the task when it fits, or allocated

struct TCBEx {
    TCBExManager *manager;
    TCBEx *prev;
    TCBEx *next;
    u32 priority;
    // Allocated data, or NULL for the data that follows the task
    void *data;
    TCBExFunc func;
};

struct TCBExManager {
    u32 count;
    // The data each task holds
    u32 dataSize;
    HeapID dataHeapId;
    u32 paddingC;
    BOOL removeRequested;
    TCBEx *tasks;
    // The head of the circular list of running tasks
    TCBEx head;
    TCBEx *current;
    // The tasks not running, as a list through next
    TCBEx *ready;
};

static TCBEx *GFL_TCBExMgrGetTask(TCBExManager *manager, u32 index);
static TCBEx *GFL_TCBExMgrGetReadyTask(TCBExManager *manager);
static void GFL_TCBExMgrNotifyReadyTask(TCBExManager *manager, TCBEx *task);
static void GFL_TCBExMgrInitTask(TCBExManager *manager, TCBEx *task);
static void GFL_TCBExRemove(TCBEx *task);
static void *GFL_TCBExGetDataCore(TCBEx *task);
static void GFL_TCBExMgrInitTasks(TCBExManager *manager);

static TCBEx *GFL_TCBExMgrGetTask(TCBExManager *manager, u32 index) {
    return (TCBEx *)((u8 *)manager->tasks + index * (manager->dataSize + sizeof(TCBEx)));
}

static TCBEx *GFL_TCBExMgrGetReadyTask(TCBExManager *manager) {
    TCBEx *task = manager->ready;

    if (task == NULL) {
        return NULL;
    }
    manager->ready = task->next;
    if (task->next != NULL) {
        task->next->prev = NULL;
    }
    return task;
}

static void GFL_TCBExMgrNotifyReadyTask(TCBExManager *manager, TCBEx *task) {
    TCBEx *ready = manager->ready;

    task->prev = NULL;
    task->next = ready;
    if (ready != NULL) {
        ready->prev = task;
    }
    manager->ready = task;
}

static void GFL_TCBExMgrInitTask(TCBExManager *manager, TCBEx *task) {
    task->manager = manager;
    task->prev = &manager->head;
    task->next = &manager->head;
    task->priority = 0;
    task->data = NULL;
    task->func = NULL;
}

static void GFL_TCBExRemove(TCBEx *task) {
    task->func = NULL;
    if (task->data != NULL) {
        GFL_HeapFree(task->data);
    }
    task->prev->next = task->next;
    task->next->prev = task->prev;
    GFL_TCBExMgrNotifyReadyTask(task->manager, task);
}

static void *GFL_TCBExGetDataCore(TCBEx *task) {
    void *data = task->data;

    if (data == NULL) {
        data = task + 1;
    }
    return data;
}

static void GFL_TCBExMgrInitTasks(TCBExManager *manager) {
    u32 i;

    for (i = 0; i < manager->count; i++) {
        TCBEx *task = GFL_TCBExMgrGetTask(manager, i);

        GFL_TCBExMgrInitTask(manager, task);
        GFL_TCBExMgrNotifyReadyTask(manager, task);
    }
}

TCBExManager *GFL_TCBExMgrCreate(HeapID heapId, HeapID dataHeapId, u32 count, u32 dataSize) {
    TCBExManager *manager = GFL_HeapAllocate(heapId, sizeof(TCBExManager), FALSE, "tcbl.c", 199);

    manager->count = count;
    manager->dataSize = dataSize;
    manager->dataHeapId = dataHeapId;
    manager->paddingC = 0;
    manager->removeRequested = FALSE;
    dataSize = (dataSize + 3) & ~3;
    manager->tasks = GFL_HeapAllocate(heapId, (dataSize + sizeof(TCBEx)) * count, FALSE, "tcbl.c", 207);
    manager->ready = NULL;
    GFL_TCBExMgrInitTasks(manager);
    GFL_TCBExMgrInitTask(manager, &manager->head);
    manager->current = NULL;
    return manager;
}

void GFL_TCBExMgrUpdate(TCBExManager *manager) {
    TCBEx *head = &manager->head;

    manager->current = head->next;
    while (manager->current != head) {
        manager->removeRequested = FALSE;
        manager->current->func(manager->current, GFL_TCBExGetDataCore(manager->current));
        if (manager->removeRequested) {
            TCBEx *next = manager->current->next;

            GFL_TCBExRemove(manager->current);
            manager->current = next;
        } else {
            manager->current = manager->current->next;
        }
    }
    manager->current = NULL;
}

void GFL_TCBExMgrFree(TCBExManager *manager) {
    GFL_HeapFree(manager->tasks);
    GFL_HeapFree(manager);
}

void GFL_TCBExMgrFreeTasks(TCBExManager *manager) {
    TCBEx *head = &manager->head;

    manager->current = head->next;
    while (manager->current != head) {
        TCBEx *next = manager->current->next;

        GFL_TCBExRemove(manager->current);
        manager->current = next;
    }
}

// Removes a task, at the end of its function if it is running
void GFL_TCBExRequestEnd(TCBEx *task) {
    if (task->manager->current == task) {
        task->manager->removeRequested = TRUE;
    } else {
        GFL_TCBExRemove(task);
    }
}

TCBEx *GFL_TCBExMgrAddTask(TCBExManager *manager, TCBExFunc func, u32 dataSize, u32 priority) {
    TCBEx *task;
    TCBEx *it;
    TCBEx *head = &manager->head;

    task = GFL_TCBExMgrGetReadyTask(manager);
    if (task == NULL) {
        return NULL;
    }
    task->priority = priority;
    task->func = func;
    if (dataSize <= manager->dataSize) {
        task->data = NULL;
    } else {
        task->data = GFL_HeapAllocate(manager->dataHeapId, dataSize, FALSE, "tcbl.c", 325);
    }
    for (it = head->next; it != head; it = it->next) {
        if (it->priority > task->priority) {
            task->prev = it->prev;
            task->next = it;
            it->prev->next = task;
            it->prev = task;
            return task;
        }
    }
    task->prev = manager->head.prev;
    task->next = &manager->head;
    manager->head.prev->next = task;
    manager->head.prev = task;
    return task;
}

void *GFL_TCBExGetData(TCBEx *task) {
    return GFL_TCBExGetDataCore(task);
}
