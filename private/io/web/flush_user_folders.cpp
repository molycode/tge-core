#include "io/flush_user_folders.hpp"
#include "io/web/finish_user_folders.hpp"
#include "io/web/user_folders.hpp"
#include <tge/logging/loggers.hpp>
#include <cstdlib>

namespace Tge::IO
{
namespace
{
void (*gpOnFinished)(){ nullptr };

//////////////////////////////////////////////////////////////////////////
void SayProblem()
{
	char* const pProblem{ TgeTakeUserFoldersProblem() };

	if (pProblem != nullptr)
	{
		gLog.Warning("The browser does not keep what is written to the user folders: {}", pProblem);
		std::free(pProblem);
	}
}

//////////////////////////////////////////////////////////////////////////
void OnStored()
{
	SayProblem();
	gpOnFinished();
}
} // namespace

//////////////////////////////////////////////////////////////////////////
void FlushUserFolders()
{
	TgeStoreUserFolders();
	SayProblem();
}

//////////////////////////////////////////////////////////////////////////
void FinishUserFolders(void (*pOnFinished)())
{
	gpOnFinished = pOnFinished;
	TgeFinishUserFolders(OnStored);
}
} // namespace Tge::IO
