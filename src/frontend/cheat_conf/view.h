#pragma once

#include "frontend/tool_base.h"
#include "retro/fwd.h"
#include "sdl/fwd.h"

#include <memory>
#include <string>

struct SDL_Window;
struct ImFont;

namespace CheatConf {

class Edit;

class View : public Frontend::ToolBase {
public:
	View(RETRO::Core& core, RETRO::Cheat::Block& block, RETRO::Cheat::Block& user_block);
    ~View();

	void Show(SDL::Window& main_win, ImFont* mono_font);
    void DisableAllEntries();

private:
	RETRO::Core* m_core{};
	RETRO::Cheat::Block* m_block{};
	RETRO::Cheat::Block* m_user_block{};
	std::string m_user_entry_to_delete;
	RETRO::Cheat::Entry* m_user_parent_entry_to_delete{};
	std::string m_new_name;
	std::size_t m_new_id{1'000'000};
	std::unique_ptr<Edit> m_edit{};

    void ShowTree(RETRO::Cheat::Block* block, bool can_edit);
	void ShowTreeNode(RETRO::Cheat::Entry& entry, RETRO::Cheat::Entry* parent_entry, bool can_edit);
    void ShowTreeNodeGroup(RETRO::Cheat::Entry& entry, RETRO::Cheat::Entry* parent_entry, bool can_edit);
	void ShowTreeNodeOptions(RETRO::Cheat::Entry& entry, RETRO::Cheat::Entry* parent_entry, bool can_edit);
	void ShowTreeNodeSingle(RETRO::Cheat::Entry& entry, RETRO::Cheat::Entry* parent_entry, bool can_edit);
	void ShowItemContextMenu(RETRO::Cheat::Entry* parent_entry, RETRO::Cheat::Entry* entry, bool delete_only = false);
	void NewEntry();
	void DisableEntry(RETRO::Cheat::Entry& entry);
	void DisableTreeNode(RETRO::Cheat::Entry& entry);
};

}
