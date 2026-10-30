#ifndef POKEBW2_PC_FILESYSTEM_H
#define POKEBW2_PC_FILESYSTEM_H

// The caller keeps the extracted resource root alive until the filesystem is shut down.
void PcFs_SetRoot(const char *root);
// Close remaining game streams after all game callbacks have stopped.
void PcFs_Shutdown(void);

#endif
