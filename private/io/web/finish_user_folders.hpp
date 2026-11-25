#pragma once

namespace Tge::IO
{
// The run's last flush: calls back once nothing is left to store, always after this has returned.
void FinishUserFolders(void (*pOnFinished)());
} // namespace Tge::IO
