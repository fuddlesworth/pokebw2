#include "types.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "system/bmp_menuwork.h"

// The options of list menus

static ListMenuOption *ListMenuCore_GetNextOptionPtr(ListMenuOption *options, u32 *heapId);

ListMenuOption *ListMenuCore_CreateOptionList(u32 count, u32 heapId) {
    ListMenuOption *options =
        GFL_HeapAllocate(heapId, (count + 1) * sizeof(ListMenuOption), TRUE, "bmp_menuwork.c", 41);

    if (options != NULL) {
        u32 i;

        for (i = 0; i < count; i++) {
            options[i].text = NULL;
            options[i].value = 0;
        }
        options[i].text = LIST_MENU_END;
        options[i].value = heapId;
    }
    return options;
}

void ListMenuCore_FreeOptionList(ListMenuOption *options) {
    ListMenuCore_FreeStrBufs(options);
    GFL_HeapFree(options);
}

void ListMenuCore_AppendMsgOption(ListMenuOption *options, MsgData *msgData, u32 messageId, sptr value, HeapID heapId) {
    u32 listHeapId;
    ListMenuOption *option = ListMenuCore_GetNextOptionPtr(options, &listHeapId);

    if (option != NULL) {
        option->text = GFL_MsgDataLoadStrbufNew(msgData, messageId);
        option->value = value;
    }
}

void ListMenuCore_AppendStrBufOption(ListMenuOption *options, const StrBuf *text, sptr value, HeapID heapId) {
    u32 listHeapId;
    ListMenuOption *option = ListMenuCore_GetNextOptionPtr(options, &listHeapId);

    if (option != NULL) {
        option->text = GFL_StrBufClone(text, heapId);
        option->value = value;
    }
}

// The first option without text, or NULL if the array is full. heapId gets the array's heap
static ListMenuOption *ListMenuCore_GetNextOptionPtr(ListMenuOption *options, u32 *heapId) {
    ListMenuOption *option;

    for (; options->text != NULL; options++) {
        if (options->text == LIST_MENU_END) {
            return NULL;
        }
    }
    option = options;
    while (options->text != LIST_MENU_END) {
        options++;
    }
    *heapId = options->value;
    return option;
}

void ListMenuCore_FreeStrBufs(ListMenuOption *options) {
    ListMenuOption *option;

    for (option = options; option->text != LIST_MENU_END; option++) {
        if (option->text == NULL) {
            break;
        }
        GFL_StrBufFree(option->text);
        option->text = NULL;
    }
}

u32 ListMenuCore_GetFirstFreeIndex(const ListMenuOption *options) {
    u32 count = 0;
    const ListMenuOption *option;

    for (option = options; option->text != LIST_MENU_END; option++) {
        if (option->text != NULL) {
            count++;
        }
    }
    return count;
}
