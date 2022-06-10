/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#include "otpch.h"

#include "creatures/interactions/chat.h"
#include "game/game.h"
#include "game/scheduling/scheduler.h"

bool PrivateChatChannel::isInvited(uint32_t guid) const
{
	if (guid == getOwner()) {
		return true;
	}
	return invites.contains(guid);
}

bool PrivateChatChannel::removeInvite(uint32_t guid)
{
	return invites.erase(guid) != 0;
}

void PrivateChatChannel::invitePlayer(const Player& player, Player& invitePlayer)
{
	auto result = invites.emplace(invitePlayer.getGUID(), &invitePlayer);
	if (!result.second) {
		return;
	}

	std::ostringstream ss;
	ss << player.getName() << " invites you to " << (player.getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " private chat channel.";
	invitePlayer.sendTextMessage(MESSAGE_PARTY_MANAGEMENT, ss.str());

	ss.str(std::string());
	ss << invitePlayer.getName() << " has been invited.";
	player.sendTextMessage(MESSAGE_PARTY_MANAGEMENT, ss.str());

	for (const auto& it : users) {
		it.second->sendChannelEvent(id, invitePlayer.getName(), CHANNELEVENT_INVITE);
	}
}

void PrivateChatChannel::excludePlayer(const Player& player, Player& excludePlayer)
{
	if (!removeInvite(excludePlayer.getGUID())) {
		return;
	}

	removeUser(excludePlayer);

	std::ostringstream ss;
	ss << excludePlayer.getName() << " has been excluded.";
	player.sendTextMessage(MESSAGE_PARTY_MANAGEMENT, ss.str());

	excludePlayer.sendClosePrivate(id);

	for (const auto& it : users) {
		it.second->sendChannelEvent(id, excludePlayer.getName(), CHANNELEVENT_EXCLUDE);
	}
}

void PrivateChatChannel::closeChannel() const
{
	for (const auto& it : users) {
		it.second->sendClosePrivate(id);
	}
}

bool ChatChannel::addUser(Player& player)
{
	if (users.contains(player.getID())) {
		return false;
	}

	if (!executeOnJoinEvent(player)) {
		return false;
	}

	// TODO: Move to script when guild channels can be scripted
	if (id == CHANNEL_GUILD) {
		Guild* guild = player.getGuild();
		if (guild && !guild->getMotd().empty()) {
			g_scheduler().addEvent(createSchedulerTask(150, std::bind_front(&Game::sendGuildMotd, &g_game(), player.getID())));
		}
	}

	if (!publicChannel) {
		for (const auto& it : users) {
			it.second->sendChannelEvent(id, player.getName(), CHANNELEVENT_JOIN);
		}
	}

	users[player.getID()] = &player;
	return true;
}

bool ChatChannel::removeUser(const Player& player)
{
	auto iter = users.find(player.getID());
	if (iter == users.end()) {
		return false;
	}

	users.erase(iter);

	if (!publicChannel) {
		for (const auto& it : users) {
			it.second->sendChannelEvent(id, player.getName(), CHANNELEVENT_LEAVE);
		}
	}

	executeOnLeaveEvent(player);
	return true;
}

bool ChatChannel::hasUser(const Player& player) const {
	return users.contains(player.getID());
}

void ChatChannel::sendToAll(const std::string& message, SpeakClasses type) const
{
	for (const auto& [playerId, player] : users) {
		player->sendChannelMessage("", message, type, static_cast<uint16_t>(playerId));
	}
}

bool ChatChannel::talk(const Player& fromPlayer, SpeakClasses type, const std::string& text) const
{
	if (!users.contains(fromPlayer.getID())) {
		return false;
	}

	for (const auto& [playerId, player] : users) {
		player->sendToChannel(&fromPlayer, type, text, static_cast<uint16_t>(playerId));
	}
	return true;
}

bool ChatChannel::executeCanJoinEvent(const Player& player)
{
	if (canJoinEvent == -1) {
		return true;
	}

	//canJoin(player)
	LuaScriptInterface* scriptInterface = g_chat().getScriptInterface();
	if (!scriptInterface->reserveScriptEnv()) {
		SPDLOG_ERROR("[CanJoinChannelEvent::execute - Player {}, on channel {}] "
                     "Call stack overflow. Too many lua script calls being nested.",
                     player.getName(), getName());
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(canJoinEvent, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(canJoinEvent);
	LuaScriptInterface::pushUserdata(L, &player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	return scriptInterface->callFunction(1);
}

bool ChatChannel::executeOnJoinEvent(const Player& player)
{
	if (onJoinEvent == -1) {
		return true;
	}

	//onJoin(player)
	LuaScriptInterface* scriptInterface = g_chat().getScriptInterface();
	if (!scriptInterface->reserveScriptEnv()) {
		SPDLOG_ERROR("[OnJoinChannelEvent::execute - Player {}, on channel {}] "
									"Call stack overflow. Too many lua script calls being nested",
									player.getName(), getName());
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(onJoinEvent, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(onJoinEvent);
	LuaScriptInterface::pushUserdata(L, &player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	return scriptInterface->callFunction(1);
}

bool ChatChannel::executeOnLeaveEvent(const Player& player)
{
	if (onLeaveEvent == -1) {
		return true;
	}

	//onLeave(player)
	LuaScriptInterface* scriptInterface = g_chat().getScriptInterface();
	if (!scriptInterface->reserveScriptEnv()) {
		SPDLOG_ERROR("[OnLeaveChannelEvent::execute - Player {}, on channel {}] "
                     "Call stack overflow. Too many lua script calls being nested.",
                     player.getName(), getName());
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(onLeaveEvent, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(onLeaveEvent);
	LuaScriptInterface::pushUserdata(L, &player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	return scriptInterface->callFunction(1);
}

bool ChatChannel::executeOnSpeakEvent(const Player& player, SpeakClasses& type, const std::string& message)
{
	if (onSpeakEvent == -1) {
		return true;
	}

	//onSpeak(player, type, message)
	LuaScriptInterface* scriptInterface = g_chat().getScriptInterface();
	if (!scriptInterface->reserveScriptEnv()) {
		SPDLOG_ERROR("[OnSpeakChannelEvent::execute - Player {}, type {}] "
                     "Call stack overflow. Too many lua script calls being nested.",
                     player.getName(), type);
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(onSpeakEvent, scriptInterface);

	lua_State* L = scriptInterface->getLuaState();

	scriptInterface->pushFunction(onSpeakEvent);
	LuaScriptInterface::pushUserdata(L, &player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushnumber(L, type);
	LuaScriptInterface::pushString(L, message);

	bool result = false;
	int size0 = lua_gettop(L);
	int ret = scriptInterface->protectedCall(L, 3, 1);
	if (ret != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else if (lua_gettop(L) > 0) {
		if (lua_isboolean(L, -1)) {
			result = LuaScriptInterface::getBoolean(L, -1);
		} else if (lua_isnumber(L, -1)) {
			result = true;
			type = LuaScriptInterface::getNumber<SpeakClasses>(L, -1);
		}
		lua_pop(L, 1);
	}

	if ((lua_gettop(L) + 4) != size0) {
		LuaScriptInterface::reportError(nullptr, "Stack size changed!");
	}
	scriptInterface->resetScriptEnv();
	return result;
}

Chat::Chat():
	scriptInterface("Chat Interface"),
	dummyPrivate(CHANNEL_PRIVATE, "Private Chat Channel")
{
	scriptInterface.initState();
}

bool Chat::load()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/chatchannels/chatchannels.xml");
	if (!result) {
		printXMLError("[Chat::load]", "data/chatchannels/chatchannels.xml", result);
		return false;
	}

	for (auto channelNode : doc.child("channels").children()) {
		auto channelId = static_cast<uint16_t>(channelNode.attribute("id").as_int());
		std::string channelName = channelNode.attribute("name").as_string();
		bool isPublic = channelNode.attribute("public").as_bool();
		pugi::xml_attribute scriptAttribute = channelNode.attribute("script");

		auto it = normalChannels.find(channelId);
		if (it != normalChannels.end()) {
			ChatChannel& channel = it->second;
			channel.publicChannel = isPublic;
			channel.name = channelName;

			if (scriptAttribute) {
				if (scriptInterface.loadFile("data/chatchannels/scripts/" + std::string(scriptAttribute.as_string())) == 0) {
					channel.onSpeakEvent = scriptInterface.getEvent("onSpeak");
					channel.canJoinEvent = scriptInterface.getEvent("canJoin");
					channel.onJoinEvent = scriptInterface.getEvent("onJoin");
					channel.onLeaveEvent = scriptInterface.getEvent("onLeave");
				} else {
					SPDLOG_WARN("[Chat::load] - Can not load script: {}",
                                scriptAttribute.as_string());
				}
			}

			for (UsersMap tempUserMap = std::move(channel.users);
			const auto& [playerId, player] : tempUserMap)
			{
				channel.addUser(*player);
			}
			continue;
		}

		ChatChannel channel(channelId, channelName);
		channel.publicChannel = isPublic;

		if (scriptAttribute) {
			if (scriptInterface.loadFile("data/chatchannels/scripts/" + std::string(scriptAttribute.as_string())) == 0) {
				channel.onSpeakEvent = scriptInterface.getEvent("onSpeak");
				channel.canJoinEvent = scriptInterface.getEvent("canJoin");
				channel.onJoinEvent = scriptInterface.getEvent("onJoin");
				channel.onLeaveEvent = scriptInterface.getEvent("onLeave");
			} else {
				SPDLOG_WARN("[Chat::load] Can not load script: {}",
                            scriptAttribute.as_string());
			}
		}

		normalChannels[channel.id] = channel;
	}
	return true;
}

ChatChannel* Chat::createChannel(const Player& player, uint16_t channelId)
{
	if (getChannel(player, channelId) != nullptr) {
		return nullptr;
	}

	switch (channelId) {
		case CHANNEL_GUILD: {
			Guild* guild = player.getGuild();
			if (guild != nullptr) {
				auto ret = guildChannels.emplace(std::make_pair(guild->getId(), ChatChannel(channelId, guild->getName())));
				return &ret.first->second;
			}
			break;
		}

		case CHANNEL_PARTY: {
			Party* party = player.getParty();
			if (party != nullptr) {
				auto ret = partyChannels.emplace(std::make_pair(party, ChatChannel(channelId, "Party")));
				return &ret.first->second;
			}
			break;
		}

		case CHANNEL_PRIVATE: {
			//only 1 private channel for each premium player
			if (!player.isPremium() || (getPrivateChannel(player) != nullptr)) {
				return nullptr;
			}

			//find a free private channel slot
			for (uint16_t i = 100; i < 10000; ++i) {
				auto ret = privateChannels.emplace(std::make_pair(i, PrivateChatChannel(i, player.getName() + "'s Channel")));
				if (ret.second) { //second is a bool that indicates that a new channel has been placed in the map
					auto& newChannel = (*ret.first).second;
					newChannel.setOwner(player.getGUID());
					return &newChannel;
				}
			}
			break;
		}

		default:
			break;
	}
	return nullptr;
}

bool Chat::deleteChannel(const Player& player, uint16_t channelId)
{
	switch (channelId) {
		case CHANNEL_GUILD: {
			Guild* guild = player.getGuild();
			if (guild == nullptr) {
				return false;
			}

			auto it = guildChannels.find(guild->getId());
			if (it == guildChannels.end()) {
				return false;
			}

			guildChannels.erase(it);
			break;
		}

		case CHANNEL_PARTY: {
			Party* party = player.getParty();
			if (party == nullptr) {
				return false;
			}

			auto it = partyChannels.find(party);
			if (it == partyChannels.end()) {
				return false;
			}

			partyChannels.erase(it);
			break;
		}

		default: {
			auto it = privateChannels.find(channelId);
			if (it == privateChannels.end()) {
				return false;
			}

			it->second.closeChannel();

			privateChannels.erase(it);
			break;
		}
	}
	return true;
}

ChatChannel* Chat::addUserToChannel(Player& player, uint16_t channelId)
{
	ChatChannel* channel = getChannel(player, channelId);
	if ((channel != nullptr) && channel->addUser(player)) {
		return channel;
	}
	return nullptr;
}

bool Chat::removeUserFromChannel(const Player& player, uint16_t channelId)
{
	ChatChannel* channel = getChannel(player, channelId);
	if ((channel == nullptr) || !channel->removeUser(player)) {
		return false;
	}

	if (channel->getOwner() == player.getGUID()) {
		deleteChannel(player, channelId);
	}
	return true;
}

void Chat::removeUserFromAllChannels(const Player& player)
{
	for (auto& [id, channel] : normalChannels) {
		channel.removeUser(player);
	}

	for (auto& [id, channel] : partyChannels) {
		channel.removeUser(player);
	}

	for (auto& [id, channel] : guildChannels) {
		channel.removeUser(player);
	}

	for (auto [id, channel] : privateChannels) {
		PrivateChatChannel* privateChannel = &channel;
		privateChannel->removeInvite(player.getGUID());
		privateChannel->removeUser(player);
		// Close and delete the channel if the owner logout
		if (privateChannel->getOwner() == player.getGUID()) {
			privateChannel->closeChannel();
			privateChannels.clear();
		}
	}
}

bool Chat::talkToChannel(const Player& player, SpeakClasses type, const std::string& text, uint16_t channelId)
{
	ChatChannel* channel = getChannel(player, channelId);
	if (channel == nullptr) {
		return false;
	}

	if (channelId == CHANNEL_GUILD) {
		GuildRank_ptr rank = player.getGuildRank();
		if (rank && rank->level > 1) {
			type = TALKTYPE_CHANNEL_O;
		} else if (type != TALKTYPE_CHANNEL_Y) {
			type = TALKTYPE_CHANNEL_Y;
		}
	} else if (type != TALKTYPE_CHANNEL_Y && (channelId == CHANNEL_PRIVATE || channelId == CHANNEL_PARTY)) {
		type = TALKTYPE_CHANNEL_Y;
	}

	if (!channel->executeOnSpeakEvent(player, type, text)) {
		return false;
	}

	return channel->talk(player, type, text);
}

ChannelList Chat::getChannelList(const Player& player)
{
	ChannelList list;
	if (player.getGuild()) {
		ChatChannel* channel = getChannel(player, CHANNEL_GUILD);
		if (channel) {
			list.push_back(channel);
		} else {
			channel = createChannel(player, CHANNEL_GUILD);
			if (channel) {
				list.push_back(channel);
			}
		}
	}

	if (player.getParty()) {
		ChatChannel* channel = getChannel(player, CHANNEL_PARTY);
		if (channel) {
			list.push_back(channel);
		} else {
			channel = createChannel(player, CHANNEL_PARTY);
			if (channel) {
				list.push_back(channel);
			}
		}
	}

	for (const auto& it : normalChannels) {
		ChatChannel* channel = getChannel(player, it.first);
		if (channel) {
			list.push_back(channel);
		}
	}

	bool hasPrivate = false;
	for (auto& it : privateChannels) {
		if (PrivateChatChannel* channel = &it.second) {
			uint32_t guid = player.getGUID();
			if (channel->isInvited(guid)) {
				list.push_back(channel);
			}

			if (channel->getOwner() == guid) {
				hasPrivate = true;
			}
		}
	}

	if (!hasPrivate && player.isPremium()) {
		list.push_front(&dummyPrivate);
	}
	return list;
}

ChatChannel* Chat::getChannel(const Player& player, uint16_t channelId)
{
	switch (channelId) {
		case CHANNEL_GUILD: {
			Guild* guild = player.getGuild();
			if (guild != nullptr) {
				auto it = guildChannels.find(guild->getId());
				if (it != guildChannels.end()) {
					return &it->second;
				}
			}
			break;
		}

		case CHANNEL_PARTY: {
			Party* party = player.getParty();
			if (party != nullptr) {
				auto it = partyChannels.find(party);
				if (it != partyChannels.end()) {
					return &it->second;
				}
			}
			break;
		}

		default: {
			auto it = normalChannels.find(channelId);
			if (it != normalChannels.end()) {
				ChatChannel& channel = it->second;
				if (!channel.executeCanJoinEvent(player)) {
					return nullptr;
				}
				return &channel;
			}

			auto it2 = privateChannels.find(channelId);
			if (it2 != privateChannels.end() && it2->second.isInvited(player.getGUID())) {
				return &it2->second;
			}
			break;
		}
	}
	return nullptr;
}

ChatChannel* Chat::getGuildChannelById(uint32_t guildId)
{
	auto it = guildChannels.find(guildId);
	if (it == guildChannels.end()) {
		return nullptr;
	}
	return &it->second;
}

ChatChannel* Chat::getChannelById(uint16_t channelId)
{
	auto it = normalChannels.find(channelId);
	if (it == normalChannels.end()) {
		return nullptr;
	}
	return &it->second;
}

PrivateChatChannel* Chat::getPrivateChannel(const Player& player)
{
	for (auto& it : privateChannels) {
		if (it.second.getOwner() == player.getGUID()) {
			return &it.second;
		}
	}
	return nullptr;
}
