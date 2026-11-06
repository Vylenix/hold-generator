#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>
#include <slc/slc.hpp>

using namespace geode::prelude;

#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(EditorPauseLayerMod, EditorPauseLayer)
{
	bool init(LevelEditorLayer *layer)
	{
		if (!EditorPauseLayer::init(layer))
		{
			return false;
		}

		auto importMacroSpr = ButtonSprite::create(
			"Import\nMacro", 30, 0, 0.4f, true,
			"bigFont.fnt", "GJ_button_04.png", 30.f);
		importMacroSpr->setScale(.8f);
		auto importMacroBtn = CCMenuItemSpriteExtra::create(
			importMacroSpr, this, menu_selector(EditorPauseLayerMod::onMyButton));
		importMacroBtn->setID("import-macro"_spr);

		auto menu = this->getChildByID("small-actions-menu");
		menu->insertBefore(importMacroBtn, nullptr);
		menu->updateLayout();

		return true;
	}

	void onMyButton(CCObject *)
	{
		file::FilePickOptions options;
		options.defaultPath = std::filesystem::current_path();

		async::spawn(
			file::pick(file::PickMode::OpenFile, options),
			[](Result<std::optional<std::filesystem::path>> result)
			{
				if (!result.isOk())
				{
					FLAlertLayer::create("Import Macro", "Failed to open file select dialog", "OK")->show();
					return;
				}

				auto pathOpt = result.unwrap();
				if (!pathOpt.has_value())
				{
					FLAlertLayer::create("Import Macro", "Macro import was cancelled", "OK")->show();
					return;
				}

				std::filesystem::path selectedPath = pathOpt.value();
				std::ifstream in(selectedPath, std::ios::binary);
				auto res = slc::v3::Replay<>::read(in);
				if (!res)
				{
					FLAlertLayer::create("Import Macro", "Macro import failed", "OK")->show();
					return;
				}
				auto editor = LevelEditorLayer::get();
				auto placeControl = [&](int frame, bool disable)
				{
					float x = editor->posForTime(frame / res->m_meta.m_tps).x;
					auto str = fmt::format("1,2899,2,{},3,105,165,{};", x, disable ? 1 : -1);
					editor->createObjectsFromString(str, true, true);
				};
				placeControl(0, 1);

				for (auto &atom : res->m_atoms.m_atoms)
				{
					if (auto *a = std::get_if<slc::v3::ActionAtom>(&atom))
					{
						for (auto &act : a->m_actions)
						{
							placeControl(act.m_frame, !act.m_holding);
						}
					}
				}
			});
	}
};