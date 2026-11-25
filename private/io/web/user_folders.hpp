#pragma once

extern "C"
{
// Implemented in user_folders.js. Both strings are the caller's to free.
char* TgeGetUserFoldersRoot();
void TgeStoreUserFolders();
void TgeFinishUserFolders(void (*pOnStored)());
char* TgeTakeUserFoldersProblem();
} // extern "C"
