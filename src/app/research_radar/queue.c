#include "types.h"
#include "app/research_radar/queue.h"
#include "gfl/heap.h"

struct Queue {
    u32 *buffer;
    int size;
    int head;
    int tail;
};

static void Queue_Init(Queue *queue);
static void Queue_AllocBuffer(Queue *queue, int size, HeapID heapId);
static void Queue_FreeBuffer(Queue *queue);
static void Queue_PushCore(Queue *queue, u32 value);
static u32 Queue_PopCore(Queue *queue);
static void Queue_ClearCore(Queue *queue);
static BOOL Queue_IsEmptyCore(Queue *queue);

Queue *Queue_Create(int size, HeapID heapId) {
    Queue *queue = GFL_HeapAllocate(heapId, sizeof(Queue), FALSE, "queue.c", 60);

    Queue_Init(queue);
    Queue_AllocBuffer(queue, size, heapId);
    return queue;
}

void Queue_Delete(Queue *queue) {
    Queue_FreeBuffer(queue);
    GFL_HeapFree(queue);
}

void Queue_Push(Queue *queue, u32 value) {
    Queue_PushCore(queue, value);
}

u32 Queue_Pop(Queue *queue) {
    return Queue_PopCore(queue);
}

void Queue_Clear(Queue *queue) {
    Queue_ClearCore(queue);
}

BOOL Queue_IsEmpty(Queue *queue) {
    return Queue_IsEmptyCore(queue);
}

static void Queue_Init(Queue *queue) {
    queue->buffer = NULL;
    queue->head = 0;
    queue->tail = 0;
}

static void Queue_AllocBuffer(Queue *queue, int size, HeapID heapId) {
    queue->buffer = GFL_HeapAllocate(heapId, sizeof(u32) * size, FALSE, "queue.c", 211);
    queue->size = size;
}

static void Queue_FreeBuffer(Queue *queue) {
    GFL_HeapFree(queue->buffer);
}

static void Queue_PushCore(Queue *queue, u32 value) {
    queue->buffer[queue->tail] = value;
    queue->tail = (queue->tail + 1) % queue->size;
}

static u32 Queue_PopCore(Queue *queue) {
    u32 value = queue->buffer[queue->head];

    queue->head = (queue->head + 1) % queue->size;
    return value;
}

static void Queue_ClearCore(Queue *queue) {
    queue->head = 0;
    queue->tail = 0;
}

static BOOL Queue_IsEmptyCore(Queue *queue) {
    if (queue->head == queue->tail) {
        return TRUE;
    }
    return FALSE;
}
