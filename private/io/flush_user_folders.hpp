#pragma once

namespace Tge::IO
{
// Once a frame: makes what was written in the user folders since the last call outlive the run.
void FlushUserFolders();
} // namespace Tge::IO
