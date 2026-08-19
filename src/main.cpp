#include "SimpleIni.h"

using namespace RE;

namespace
{
	CSimpleIniA ini(true, false, false);
	BGSSoundCategory* playerSoundCategory = nullptr;
	float playerVoiceFrequency = 1.0F;

	REL::Relocation<float*> globalTimeMultiplier{ REL::ID{ 1266509, 2666308 } };

	void SetPlayerVoiceFrequency(float)
	{
		ini.LoadFile("Data\\F4SE\\Plugins\\PlayerVoiceFrequency.ini");
		playerVoiceFrequency = std::stof(ini.GetValue("General", "Frequency", "1.0"));
		playerVoiceFrequency = (std::max)(playerVoiceFrequency, 0.05F);

		if (playerSoundCategory) {
			playerSoundCategory->frequencyMult = playerVoiceFrequency;
			playerSoundCategory->minFrequencyMult = playerVoiceFrequency * 0.1F;
		}
	}

	void SetGlobalTimeMultiplier(std::uint64_t a_timer, float a_multiplier)
	{
		using func_t = void(std::uint64_t, float);
		REL::Relocation<func_t> func{ REL::ID{ 1419977, 2267970 } };
		func(a_timer, a_multiplier);
		SetPlayerVoiceFrequency(a_multiplier);
	}

	struct MenuWatcher final : BSTEventSink<MenuOpenCloseEvent>
	{
		BSEventNotifyControl ProcessEvent(
			const MenuOpenCloseEvent& a_event,
			BSTEventSource<MenuOpenCloseEvent>*) override
		{
			if (a_event.menuName == BSFixedString("LoadingMenu") && !a_event.opening) {
				SetPlayerVoiceFrequency(*globalTimeMultiplier);
			}
			return BSEventNotifyControl::kContinue;
		}
	};

	void OnF4SEMessage(F4SE::MessagingInterface::Message* a_message)
	{
		if (a_message->type != F4SE::MessagingInterface::kGameDataReady) {
			return;
		}

		static MenuWatcher menuWatcher;
		UI::GetSingleton()->GetEventSource<MenuOpenCloseEvent>()->RegisterSink(&menuWatcher);

		auto* dataHandler = TESDataHandler::GetSingleton();
		for (auto* category : dataHandler->GetFormArray<BGSSoundCategory>()) {
			if (category && std::strcmp(category->fullName.c_str(), "AudioCategoryVOCPlayer") == 0) {
				playerSoundCategory = category;
				REX::INFO("AudioCategoryVOCPlayer {:p}", static_cast<void*>(category));
				break;
			}
		}
	}
}

F4SEPluginLoad(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se, {
		.log = true,
		.logName = "PlayerVoiceFrequency",
		.trampoline = true,
		.trampolineSize = 32,
	});

	const auto isOG = REX::FModule::IsRuntimeOG();
	const auto changeTime = REL::Relocation<std::uintptr_t>{ REL::ID{ 249054, 2237201 } }.address() +
		(isOG ? 0x2B : 0x2B);
	const auto revertTime = REL::Relocation<std::uintptr_t>{ REL::ID{ 249054, 2237201 } }.address() +
		(isOG ? 0x6B : 0x6B);
	const auto overrideJump = REL::Relocation<std::uintptr_t>{ REL::ID{ 157156, 2267454 } }.address() +
		(isOG ? 0x97 : 0x97);

	auto& trampoline = REL::GetTrampoline();
	trampoline.write_call<5>(changeTime, &SetGlobalTimeMultiplier);
	trampoline.write_call<5>(revertTime, &SetGlobalTimeMultiplier);
	REL::WriteSafeFill(overrideJump, 0x90, 2);

	const auto executableVersion = REX::FModule::GetExecutingModule().GetFileVersion();
	REX::INFO("detected Fallout 4 runtime={} f4seRuntimeVersion={} executableVersion={}",
		isOG ? "OG" : "AE", a_f4se->RuntimeVersion().string(), executableVersion.string());

	F4SE::GetMessagingInterface()->RegisterListener(OnF4SEMessage);
	return true;
}

extern "C"
{
	F4SE_EXPORT bool F4SEPlugin_Query(const F4SE::QueryInterface*, F4SE::PluginInfo* a_info)
	{
		const auto* versionData = F4SE::PluginVersionData::GetSingleton();
		if (!versionData) {
			return false;
		}

		a_info->name = versionData->GetPluginName().data();
		a_info->infoVersion = F4SE::PluginInfo::kVersion;
		a_info->version = versionData->pluginVersion;
		return true;
	}
}
