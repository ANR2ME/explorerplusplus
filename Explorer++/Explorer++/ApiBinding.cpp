// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "APIBinding.h"
#include "CommandInvoked.h"
#include "MenuApi.h"
#include "TabsAPI.h"
#include "TabCreated.h"
#include "TabMoved.h"
#include "TabRemoved.h"
#include "TabUpdated.h"
#include "UiApi.h"

void BindTabsAPI(sol::state &state, TabContainerInterface *tabContainer, TabInterface *tabInterface);
void BindMenuApi(sol::state &state, Plugins::PluginMenuManager *pluginMenuManager);
void BindUiApi(sol::state &state, UiTheming *uiTheming);
void BindCommandApi(int pluginId, sol::state &state, Plugins::PluginCommandManager *pluginCommandManager);
template<typename T>
void BindObserverMethods(sol::state &state, sol::table &parentTable, const std::string &observerTableName, const std::shared_ptr<T> &object);
sol::table MarkTableReadOnly(sol::state &state, sol::table &table);
int deny(lua_State *state);

void Plugins::BindAllApiMethods(int pluginId, sol::state &state, PluginInterface *pluginInterface)
{
	BindTabsAPI(state, pluginInterface->GetTabContainer(), pluginInterface->GetTabInterface());
	BindMenuApi(state, pluginInterface->GetPluginMenuManager());
	BindUiApi(state, pluginInterface->GetUiTheming());
	BindCommandApi(pluginId, state, pluginInterface->GetPluginCommandManager());
}

void BindTabsAPI(sol::state &state, TabContainerInterface *tabContainer, TabInterface *tabInterface)
{
	std::shared_ptr<Plugins::TabsApi> tabsApi = std::make_shared<Plugins::TabsApi>(tabContainer, tabInterface);

	sol::table tabsTable = state.create_named_table("tabs");
	sol::table tabsMetaTable = MarkTableReadOnly(state, tabsTable);

	tabsMetaTable.set_function("getAll", &Plugins::TabsApi::getAll, tabsApi);
	tabsMetaTable.set_function("get", &Plugins::TabsApi::get, tabsApi);
	tabsMetaTable.set_function("create", &Plugins::TabsApi::create, tabsApi);
	tabsMetaTable.set_function("update", &Plugins::TabsApi::update, tabsApi);
	tabsMetaTable.set_function("refresh", &Plugins::TabsApi::refresh, tabsApi);
	tabsMetaTable.set_function("move", &Plugins::TabsApi::move, tabsApi);
	tabsMetaTable.set_function("close", &Plugins::TabsApi::close, tabsApi);

	std::shared_ptr<Plugins::TabCreated> tabCreated = std::make_shared<Plugins::TabCreated>(tabContainer);
	BindObserverMethods(state, tabsMetaTable, "onCreated", tabCreated);

	std::shared_ptr<Plugins::TabMoved> tabMoved = std::make_shared<Plugins::TabMoved>(tabContainer);
	BindObserverMethods(state, tabsMetaTable, "onMoved", tabMoved);

	std::shared_ptr<Plugins::TabUpdated> tabUpdated = std::make_shared<Plugins::TabUpdated>(tabContainer);
	BindObserverMethods(state, tabsMetaTable, "onUpdated", tabUpdated);

	std::shared_ptr<Plugins::TabRemoved> tabRemoved = std::make_shared<Plugins::TabRemoved>(tabContainer);
	BindObserverMethods(state, tabsMetaTable, "onRemoved", tabRemoved);

	tabsMetaTable.new_usertype<Plugins::TabsApi::FolderSettings>("FolderSettings",
		"viewMode", &Plugins::TabsApi::FolderSettings::viewMode,
		"__tostring", &Plugins::TabsApi::FolderSettings::toString);

	tabsMetaTable.new_usertype<Plugins::TabsApi::Tab>("Tab",
		"id", &Plugins::TabsApi::Tab::id,
		"location", &Plugins::TabsApi::Tab::location,
		"name", &Plugins::TabsApi::Tab::name,
		"locked", &Plugins::TabsApi::Tab::locked,
		"addressLocked", &Plugins::TabsApi::Tab::addressLocked,
		"folderSettings", &Plugins::TabsApi::Tab::folderSettings,
		"__tostring", &Plugins::TabsApi::Tab::toString);

	tabsMetaTable.new_enum("ViewMode",
		"details", ViewMode::VM_DETAILS,
		"extraLargeIcons", ViewMode::VM_EXTRALARGEICONS,
		"icons", ViewMode::VM_ICONS,
		"largeIcons", ViewMode::VM_LARGEICONS,
		"list", ViewMode::VM_LIST,
		"smallIcons", ViewMode::VM_SMALLICONS,
		"thumbnails", ViewMode::VM_THUMBNAILS,
		"tiles", ViewMode::VM_TILES);
}

void BindMenuApi(sol::state &state, Plugins::PluginMenuManager *pluginMenuManager)
{
	std::shared_ptr<Plugins::MenuApi> menuApi = std::make_shared<Plugins::MenuApi>(pluginMenuManager);

	sol::table menuTable = state.create_named_table("menu");
	sol::table metaTable = MarkTableReadOnly(state, menuTable);

	metaTable.set_function("create", &Plugins::MenuApi::create, menuApi);
	metaTable.set_function("remove", &Plugins::MenuApi::remove, menuApi);
}

void BindUiApi(sol::state &state, UiTheming *uiTheming)
{
	std::shared_ptr<Plugins::UiApi> uiApi = std::make_shared<Plugins::UiApi>(uiTheming);

	sol::table uiTable = state.create_named_table("ui");
	sol::table metaTable = MarkTableReadOnly(state, uiTable);

	metaTable.set_function("setListViewColors", &Plugins::UiApi::setListViewColors, uiApi);
	metaTable.set_function("setTreeViewColors", &Plugins::UiApi::setTreeViewColors, uiApi);
}

void BindCommandApi(int pluginId, sol::state &state, Plugins::PluginCommandManager *pluginCommandManager)
{
	sol::table commandsTable = state.create_named_table("commands");
	sol::table commandsMetaTable = MarkTableReadOnly(state, commandsTable);

	std::shared_ptr<Plugins::CommandInvoked> commandInvoked = std::make_shared<Plugins::CommandInvoked>(pluginCommandManager, pluginId);
	BindObserverMethods(state, commandsMetaTable, "onCommand", commandInvoked);
}

template<typename T>
void BindObserverMethods(sol::state &state, sol::table &parentTable, const std::string &observerTableName, const std::shared_ptr<T> &object)
{
	static_assert(std::is_base_of<Plugins::Event, T>::value, "T must inherit from Plugins::Event");

	sol::table observerTable = parentTable.create_named(observerTableName);
	sol::table observerMetaTable = MarkTableReadOnly(state, observerTable);

	observerMetaTable.set_function("addListener", &T::addObserver, object);
	observerMetaTable.set_function("removeListener", &T::removeObserver, object);
}

sol::table MarkTableReadOnly(sol::state &state, sol::table &table)
{
	sol::table metaTable = state.create_table();

	metaTable[sol::meta_function::new_index] = deny;
	metaTable[sol::meta_function::index] = metaTable;

	table[sol::metatable_key] = metaTable;

	return metaTable;
}

int deny(lua_State *state)
{
	return luaL_error(state, "Attempt to modify read-only table");
}