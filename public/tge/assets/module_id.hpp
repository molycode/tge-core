#pragma once

namespace Tge
{
class IModuleId;

// The identity the module providing IAssetProvider reports, so its consumers order after it without naming it.
namespace Assets
{
extern IModuleId* const gModuleId;
} // namespace Assets
} // namespace Tge
