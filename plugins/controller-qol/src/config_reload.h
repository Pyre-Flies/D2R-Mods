#pragma once
#include "live_config.h"
#include <D2RLPlugin/api.h>
#include <memory>
namespace QolReload {
using Idle=bool(*)() noexcept;
using Apply=void(*)(const QolConfig::Settings&) noexcept;
bool Start(const D2RL::PluginContext*,const QolConfig::Settings&,Idle,Apply) noexcept;
void Stop() noexcept;
std::shared_ptr<const QolConfig::Settings> Snapshot() noexcept;
bool Request() noexcept;
// The root plugin owns /qol even if navigation/native-hook admission refuses.
D2RL::ConsoleCommandResult Command(D2R::Game::Client*,const D2RL::ConsoleCommandContext*,void*) noexcept;
}
