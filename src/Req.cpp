/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Req.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: claprand <claprand@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/16 16:37:41 by ymanchon          #+#    #+#             */
/*   Updated: 2025/02/28 13:11:12 by claprand         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Req.hpp"

const unsigned int
Req::nReqfun = REQ_COUNT;

Str Req::currentLine; 

const Str
Req::reqname[REQ_COUNT] = {"CAP", "INVITE", "JOIN", "KICK", "MODE", "NICK", "PASS", "TOPIC", "USER", "PRIVMSG"};

void
(*Req::reqfun[REQ_COUNT])(REQ_PARAMS) = {Req::__CAP, Req::__INVITE, Req::__JOIN, Req::__KICK, Req::__MODE, Req::__NICK, Req::__PASS, Req::__TOPIC, Req::__USER, Req::__PRIVMSG};

static Str	get_line(char** txt)
{
	Str	ret;

	for (int i = 0 ; **txt && **txt != '\n' ; ++i)
		ret.push_back(*((*txt)++));
	return (ret);
}

void 
Req::Check(REQ_PARAMS)
{
    while (!((currentLine = get_line(&req)).empty()))
    {
        size_t spacePos = currentLine.find_first_of(' ');

        Str cmd;
        if (spacePos == std::string::npos)
            cmd = currentLine;
        else
            cmd = Str(currentLine.begin(), currentLine.begin() + spacePos);

        Str input;
        if (spacePos != std::string::npos && spacePos + 1 < currentLine.length())
            input = Str(currentLine.begin() + spacePos + 1, currentLine.end());

        //std::cout << "Commande: " << cmd << " | Arguments: " << input << std::endl;

        bool found = false;
        for (int i = 0; i < REQ_COUNT; ++i)
        {
            if (!reqname[i].compare(cmd))
            {
                reqfun[i](REQ_DATA);
                found = true;
                break;
            }
        }

        if (!found)
            std::cout << RED << "Commande inconnue : " << cmd << RESET << std::endl;
    }
}

void sendWelcomeMessages(Client* client, Select& select) {
    if (!client)
        return;

    std::string welcomeMessage = ":localhost 001 " + client->GetNick() + 
        " :Welcome to the ft_irc Network, " + client->GetNick() + "!" + 
        client->GetUser() + "@localhost\r\n";
    
    std::string yourHostMessage = ":localhost 002 " + client->GetNick() + 
        " :Your host is localhost, running version 1.0\r\n";
    
    std::string createdMessage = ":localhost 003 " + client->GetNick() + 
        " :This server was created some time ago\r\n";

    int clientSocket = client->GetRemote()->Get();

    if (select.CanWrite(clientSocket)) {
        send(clientSocket, welcomeMessage.c_str(), welcomeMessage.size(), 0);
        send(clientSocket, yourHostMessage.c_str(), yourHostMessage.size(), 0);
        send(clientSocket, createdMessage.c_str(), createdMessage.size(), 0);
    }
}

/******************************************************/
/*                      CAP                           */
/******************************************************/

void
Req::__CAP(REQ_PARAMS)
{
	UNUSED_REQ_PARAMS
	server.SendTo(client->GetName(), "CAP END");
}

/******************************************************/
/*                      INVITE                        */
/******************************************************/

bool 
is_valid_channel_name(const std::string &name){
    return !name.empty() && name[0] == '#';
}

// INVITE <nickname> <channel>
void 
Req::__INVITE(REQ_PARAMS)
{
    UNUSED_REQ_PARAMS;
    if (!client)
        return;
    
    std::stringstream ss(currentLine);
    std::string command, targetNick, channelName;
    ss >> command >> targetNick >> channelName;

    if (client->GetAuthenticated() == false || client->GetNick().empty() || client->GetUser().empty()){
        std::string errorMessage = ":localhost 421 " + client->GetName()  + " INVITE :Need to be logged in\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return ;
    }

    if (targetNick.empty() || channelName.empty()) {
        std::string errorMessage = ":localhost 461 " + client->GetNick() + " INVITE :Not enough parameters\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    Channel* channel = server.FindChannel(channelName);
    if (!is_valid_channel_name(channelName) || !channel) {
        std::string errorMessage = ":localhost 403 " + client->GetNick() + " " + channelName + " :No such channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
    if (!channel->isOperator(client)) {
        std::string errorMessage = ":localhost 482 " + client->GetNick() + " " + channelName + " :You're not channel operator\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
    Client* targetClient = server.getClientByNick(targetNick);
    if (!targetClient) {
        std::string errorMessage = ":localhost 401 " + client->GetNick() + " " + targetNick + " :No such nick/channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    if (channel->IsMember(targetClient)) {
        std::string errorMessage = ":localhost 443 " + client->GetNick() + " " + targetNick + " " + channelName + " :is already on channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
    channel->InviteUser(targetClient);
    // std::string inviteMessage = ":localhost 341 " + client->GetNick() + " " + targetNick + " " + channelName + "\r\n";
    // if (select.CanWrite(client->GetRemote()->Get()))
    //     send(client->GetRemote()->Get(), inviteMessage.c_str(), inviteMessage.size(), 0);
    
    std::string targetMessage = ":localhost 341 " + client->GetNick() + " " + targetNick + " " + channelName + "\r\n";
    if (select.CanWrite(targetClient->GetRemote()->Get()))
        send(targetClient->GetRemote()->Get(), targetMessage.c_str(), targetMessage.size(), 0);
}

/******************************************************/
/*                      JOIN                          */
/******************************************************/

void 
Req::__JOIN(REQ_PARAMS)
{
    UNUSED_REQ_PARAMS;
    if (currentLine.empty()) {
        std::cerr << "Error: currentLine is empty!" << std::endl;
        return;
    }
    
    if (client->GetAuthenticated() == false || client->GetNick().empty() || client->GetUser().empty()){
        std::string errorMessage = ":localhost 421 " + client->GetName()  + " JOIN :Need to be logged in\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return ;
    }
    
    std::istringstream iss(currentLine);
    std::string command, channelName, key;
    std::getline(iss, command, ' ');
    if (!std::getline(iss, channelName, ' ') || channelName.empty()) {
        std::cerr << "Error: Missing or invalid channel name!" << std::endl;
        return;
    }
    
    if (std::getline(iss, key)) {
        if (key.empty()) {
            key.clear();
        }
    }
    
    if (!client->GetAuthenticated()) {
        std::string errorMessage = ":localhost 462 " + client->GetNick() + " :You may not reregister\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
 
    if (channelName.empty()) {
        std::string errorMessage = ":localhost 461 " + client->GetNick() + " :Not enough parameters\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    if (!is_valid_channel_name(channelName)) {
        std::string errorMessage = ":localhost 403 " + client->GetNick() + " " + channelName + " :No such channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
    Channel* channel = server.FindChannel(channelName);
    if (!channel) {
        std::cout << MAGENTA << "Channel not found. Creating new channel: " << channelName << RESET << std::endl;
        server.CreateChannel(channelName);
        channel = server.FindChannel(channelName);
        
        if (!channel) {
            std::cerr << RED << "Error: Channel creation failed!" << RESET << std::endl;
            return;
        }
    std::cout << "DEBUG: Channel invite-only status = " << channel->isInviteOnly() << ", Client invited = " << channel->isInvited(client) << std::endl;

    try {
    channel->AddUser(client);
    } catch (const Channel::UserAlreadyInChannel& e){
        std::cerr << "Error: " << e.what() << std::endl;
    }
    std::string joinMessage = ":" + client->GetNick() + "!~" + client->GetNick() + "@localhost JOIN " + channelName + "\r\n";
    server.Broadcast(joinMessage, client, &select);
    try {
    channel->ElevateUser(client);
    } catch (const Channel::UserNotFound& e){
        std::cerr << "Error: " << e.what() << std::endl;
    }
        std::cout << GREEN << "Channel created successfully." << RESET << std::endl;
    } else {
        std::cout << MAGENTA << "Channel found: " << channelName << RESET << std::endl;
    }

    if (channel->isFull()) {
        std::string errorMessage = ":localhost 471 " + client->GetNick() + " " + channelName + " :Cannot join channel (+l)\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    if (channel->isInviteOnly() && !channel->isInvited(client)) {
        std::string errorMessage = ":localhost 473 " + client->GetNick() + " " + channelName + " :Cannot join channel (+i)\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
    if (channel->hasKey()) {
        if (key.empty() && channel->GetPass() != key) { 
            std::string errorMessage = ":localhost 475 " + client->GetNick() + " " + channelName + " :Cannot join channel (+k)\r\n";
            if (select.CanWrite(client->GetRemote()->Get()))
                send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
            return;
        }
    }
    std::cout << "DEBUG: Channel invite-only status = " << channel->isInviteOnly() << ", Client invited = " << channel->isInvited(client) << std::endl;
    try {
        channel->AddUser(client);
    } catch (const Channel::UserAlreadyInChannel& e) {
        //std::cerr << "Error: " << e.what() << std::endl;
    }
    std::string joinMessage = ":" + client->GetNick() + "!~" + client->GetNick() + "@localhost JOIN " + channelName + "\r\n";
    server.Broadcast(joinMessage, client, &select);
    server.sendChanInfos(client, channel);
}

/******************************************************/
/*                      KICK                          */
/******************************************************/
//Parameters: <channel> <user> [<comment>]
void 
Req::__KICK(REQ_PARAMS)
{
    UNUSED_REQ_PARAMS;
    std::istringstream iss(currentLine);
    std::string command, channelName, targetNick, reason;
    
    iss >> command >> channelName >> targetNick;
    std::getline(iss, reason);
    if (!reason.empty() && reason[0] == ':')
        reason = reason.substr(1);

        
    if (client->GetAuthenticated() == false || client->GetNick().empty() || client->GetUser().empty()){
        std::string errorMessage = ":localhost 421 " + client->GetNick()  + " KICK :Need to be logged in\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return ;
    }
        
    Channel* channel = server.FindChannel(channelName);
    if (!channel) {
        std::string errorMessage = ":localhost 403 " + client->GetNick() + " " + channelName + " :No such channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    if (!channel->IsMember(client)) {
        std::string errorMessage = ":localhost 442 " + client->GetNick() + " " + channelName + " :You're not on that channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    if (!channel->IsAdmin(client)) {
        std::string errorMessage = ":localhost 482 " + client->GetNick() + " " + channelName + " :You're not channel operator\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    Client* targetClient = server.FindClient(targetNick);
    if (!targetClient || !channel->IsMember(targetClient)) {
        std::string errorMessage = ":localhost 441 " + client->GetNick() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    channel->RevokeUser(targetClient);

    std::string kickMessage = ":" + client->GetNick() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
    std::cout << kickMessage << std::endl;

    std::string targetMessage = ":localhost 482 " + targetNick + " " + channelName + " :You have been kicked by " + client->GetNick() + " (" + reason + ")\r\n";
    if (select.CanWrite(client->GetRemote()->Get()))
        send(targetClient->GetRemote()->Get(), targetMessage.c_str(), targetMessage.size(), 0);
}

/******************************************************/
/*                      MODE                          */
/******************************************************/
void 
Req::__MODE(REQ_PARAMS)
{
    UNUSED_REQ_PARAMS;
    if (currentLine.empty()) {
        std::cerr << "Error: currentLine is empty!" << std::endl;
        return;
    }

    std::istringstream iss(currentLine);
    std::string command, channelName, modes, mdp;
    std::getline(iss, command, ' ');

    if (!std::getline(iss, channelName, ' ') || channelName.empty()) {
        std::cerr << "Error: Invalid or empty channel name!" << std::endl;
        return;
    }

    Channel* channel = server.FindChannel(channelName);
    if (!channel) {
        std::string errorMessage = ":localhost 403 " + client->GetNick() + " " + channelName + " :No such channel\r\n";
        std::cout << "DEBUG: Channel not found: " << errorMessage << std::endl;
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
    if (!std::getline(iss, modes, ' ')) {
        std::cerr << "Error: Modes string is empty!" << std::endl;
        return;
    }

    if (!channel->isOperator(client)) {
        std::string errorMessage = ":localhost 481 " + client->GetNick() + " :Permission Denied - You're not a channel operator\r\n";
        std::cout << "DEBUG: " << errorMessage << std::endl;
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    for (size_t i = 0; i < modes.size(); ++i) {
        char mode = modes[i];
        switch (mode) {
            case 'i':
            if (modes[i - 1] == '+') {
                channel->SetInviteOnly(true);
                std::cout << "DEBUG: Channel " << channel->GetName() << " invite-only mode enabled" << std::endl;
            } else if (modes[i - 1] == '-') {
                channel->SetInviteOnly(false);
                std::cout << "DEBUG: Channel " << channel->GetName() << " invite-only mode disabled" << std::endl;
            } else {
                std::cerr << "Error: Invalid mode argument for +i" << std::endl;
                return;
            }
            break;
        

            case 'k':  
                if (modes[i - 1] == '+') {
                    if (std::getline(iss, mdp)) {
                        channel->SetPass(mdp);
                        std::cout << "DEBUG: Channel " << channelName << " requires a password" << std::endl;
                    } else {
                        std::cerr << "Error: No password provided for channel " << channelName << std::endl;
                        return;
                    }
                } else if (modes[i - 1] == '-') {
                    channel->SetPass("");
                    std::cout << "DEBUG: Channel " << channelName << " no longer requires a password" << std::endl;
                }
                break;

            case 'l':
                if (modes[i - 1] == '+') {
                    int maxClients;
                    iss >> maxClients;
                    channel->setMaxClients(maxClients);
                    std::cout << "DEBUG: Channel " << channelName << " user limit set to " << maxClients << std::endl;
                } else if (modes[i - 1] == '-') {
                    channel->setMaxClients(50);
                    std::cout << "DEBUG: Channel " << channelName << " user limit removed" << std::endl;
                }
                break;

            case 't':
                if (modes[i - 1] == '+') {
                    channel->setTopicRestricted(true);
                    std::cout << "DEBUG: Channel " << channelName << " topic is now restricted" << std::endl;
                } else if (modes[i - 1] == '-') {
                    channel->setTopicRestricted(false);
                    std::cout << "DEBUG: Channel " << channelName << " topic is no longer restricted" << std::endl;
                }
                break;

            default:
                std::cerr << "Error: Unknown mode " << mode << " in the mode string!" << std::endl;
                break;
        }
    }
    std::string modeResponse = ":" + client->GetNick() + " MODE " + channelName + " :" + modes + "\r\n";
    std::cout << "DEBUG: MODE response: " << modeResponse << std::endl;
    if (select.CanWrite(client->GetRemote()->Get())) {
        send(client->GetRemote()->Get(), modeResponse.c_str(), modeResponse.size(), 0);
    }
}



/******************************************************/
/*                      NICK                          */
/******************************************************/

static bool	
isForbidden(char c)
{
	if (c == ' ' || c == ',' || c == '*' || c == '?' || c == '!'
		|| c == '@' || c == '.')
		return (true);
	else
		return (false);
}

static bool	
containsInvalidCharacters(std::string nickname)
{
	if (nickname[0] == '$' || nickname[0] == ':' || nickname[0] == '#')
		return (true);
	
	for (size_t i = 0; i < nickname.size(); i++) {
			if (isForbidden(nickname[i]) == true)
				return (true);
	}
	return (false);			
}

void 
Req::__NICK(REQ_PARAMS) 
{
    UNUSED_REQ_PARAMS;
    size_t spacePos = currentLine.find_first_of(' ');
    if (client->GetAuthenticated() == false) {
        std::string errorMessage = "You may start by command PASS to log in with the password\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    std::string newNick = currentLine.substr(spacePos + 1);
    if (containsInvalidCharacters(newNick)) {
        std::string errorMessage = ":localhost 432 " + newNick + " :Erroneous nickname\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
   
    if (spacePos == std::string::npos || spacePos + 1 >= currentLine.length()) {
        std::string errorMessage = ":localhost 431 " + client->GetName() + " :No nickname given\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    if (server.IsNicknameTaken(newNick)) {
        std::string errorMessage = ":localhost 433 " + newNick + " :Nickname is already in use\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    std::string oldNick = client->GetNick();
    if (oldNick.empty())
        oldNick = client->GetName();
    client->SetNick(newNick);
    std::cout << GREEN << oldNick + " changed his nickname to " + newNick << RESET << std::endl;

    if (client->GetAuthenticated() == true && !client->GetUser().empty()) {
        std::string welcomeMessage = ":localhost 001 " + client->GetName() + " :Welcome to the ft_irc : " + client->GetNick() + "!" + client->GetUser() + "@" + "localhost\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            sendWelcomeMessages(client, select);
        }
        // std::string joinMessage = ":" + client->GetNick() + " has joined";
        // std::cout << GREEN << joinMessage << RESET << std::endl;
}

/******************************************************/
/*                      PASS                          */
/******************************************************/

void 
Req::__PASS(REQ_PARAMS)
{
    UNUSED_REQ_PARAMS;
    if (client->GetAuthenticated()) {
        std::string errorMessage = ":localhost 462 " + client->GetName() + " :You may not reregister\r\n";
        //server.SendTo(client->GetName(), errorMessage, 512);
		if (select.CanWrite(client->GetRemote()->Get()))
			send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    size_t spacePos = currentLine.find_first_of(' ');
    if (spacePos == std::string::npos) {
        spacePos = currentLine.length();
        std::string errorMessage = ":localhost 461 " + client->GetName() + " :Not enough parameters\r\n";
        //server.SendTo(client->GetName(), errorMessage, 512);
		if (select.CanWrite(client->GetRemote()->Get()))
			send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    std::string password = currentLine.substr(spacePos + 1);
    if (password != server.GetPassword()) {
        std::string errorMessage = ":localhost 464 " + client->GetName() + " :Password incorrect\r\n";
        //server.SendTo(client->GetName(), errorMessage, 512);
		if (select.CanWrite(client->GetRemote()->Get()))
			send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        client->SetDisconnect(true); 
        return;
    }
    
    client->SetAuthenticated(true);
    if (client->GetAuthenticated() == true)
        std::cout << "\e[32m" << "New client connected! Socket FD: " << client->GetRemote()->Get() << "\e[0m" << std::endl;

}

/******************************************************/
/*                      TOPIC                         */
/******************************************************/

void
Req::__TOPIC(REQ_PARAMS)
{

    UNUSED_REQ_PARAMS;    
    if (client->GetAuthenticated() == false || client->GetNick().empty() || client->GetUser().empty()){
        std::string errorMessage = ":localhost 421 " + client->GetName()  + " TOPIC :Need to be logged in\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return ;
    }

    size_t spacePos = currentLine.find(' '); 
    if (spacePos == std::string::npos || spacePos + 1 >= currentLine.length()) {
        std::string errorMessage = ":localhost 461 " + client->GetName() + " TOPIC :Not enough parameters\r\n";
        send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }
    
        std::string remaining = currentLine.substr(spacePos + 1);
        size_t nextSpacePos = remaining.find(' ');
    
        std::string channel = (nextSpacePos == std::string::npos) ? remaining : remaining.substr(0, nextSpacePos);
        std::string topic = (nextSpacePos == std::string::npos) ? "" : remaining.substr(nextSpacePos + 1);
    
        Channel* currentChannel = server.GetChannel(channel);
        if (!currentChannel) {
            std::string errorMessage = ":localhost 403 " + channel + " :No such channel\r\n";
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
            return;
        }
    
        if (!currentChannel->IsMember(client)) {
            std::string errorMessage = ":localhost 442 " + channel + " :You're not on that channel\r\n";
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
            return;
        }
    
        if (topic.empty()) {
            if (currentChannel->GetTopic().empty()) {
                std::string noTopicMessage = ":localhost 331 " + client->GetName() + " " + channel + " :No topic is set\r\n";
                send(client->GetRemote()->Get(), noTopicMessage.c_str(), noTopicMessage.size(), 0);
            } else {
                std::string topicMessage = ":localhost 332 " + client->GetName() + " " + channel + " :" + topic + "\r\n";
                send(client->GetRemote()->Get(), topicMessage.c_str(), topicMessage.size(), 0);
            }
        } else { 
            if (topic[0] == ':') {
                topic = topic.substr(1); 
            }
            currentChannel->SetTopic(topic);
            std::string topicChangeMessage = ":" + client->GetName() + " TOPIC " + channel + " :" + topic + "\r\n";
            server.BroadcastToChannel(currentChannel, topicChangeMessage, &select);
        }
}

/******************************************************/
/*                      USER                          */
/******************************************************/

// Parameters: <username> <hostname> <servername> <realname>
void 
Req::__USER(REQ_PARAMS)
{
    UNUSED_REQ_PARAMS;
    if (client->GetAuthenticated() == false) {
        std::string errorMessage = "You may start by command PASS to log in with the password\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return ;
    }

    size_t spacePos = currentLine.find_first_of(' ');
    if (spacePos == std::string::npos) {
        std::string errorMessage = ":localhost 461 " + client->GetName() + " :Not enough parameters\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    size_t secondSpacePos = currentLine.find_first_of(' ', spacePos + 1);
    if (secondSpacePos == std::string::npos) {
        std::string errorMessage = ":localhost 461 " + client->GetName() + " :Not enough parameters\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    size_t thirdSpacePos = currentLine.find_first_of(' ', secondSpacePos + 1);
    if (thirdSpacePos == std::string::npos) {
        std::string errorMessage = ":localhost 461 " + client->GetName() + " :Not enough parameters\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    size_t realnamePos = currentLine.find_first_of(':', thirdSpacePos + 1);
    if (realnamePos == std::string::npos) {
        std::string errorMessage = ":localhost 461 " + client->GetName() + " :Not enough parameters\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    Str username = Str(currentLine.begin() + spacePos + 1, currentLine.begin() + secondSpacePos);
    Str hostname = Str(currentLine.begin() + secondSpacePos + 1, currentLine.begin() + thirdSpacePos);
    Str servername = Str(currentLine.begin() + thirdSpacePos + 1, currentLine.begin() + realnamePos);
    Str realname = Str(currentLine.begin() + realnamePos + 1, currentLine.end());

    if (client->GetUser() != "") {
        std::string errorMessage = ":localhost 462 " + client->GetName() + " :You may not reregister\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return;
    }

    client->SetUser(username);
    client->SetRealname(realname);
    client->SetHostname(hostname);
    client->SetServername(servername);

    if (client->GetAuthenticated() == true && !client->GetNick().empty()) {
        if (select.CanWrite(client->GetRemote()->Get()))
            sendWelcomeMessages(client, select);;
        
        std::string joinMessage = ":" + client->GetNick() + " has joined";
        std::cout << GREEN << joinMessage << RESET << std::endl;
    }
}

/******************************************************/
/*                      PRIVMSG                       */
/******************************************************/

void 
Req::__PRIVMSG(REQ_PARAMS) 
{
    UNUSED_REQ_PARAMS;
    if (!client)
        return;

    std::stringstream ss(currentLine);
    std::string command, target, message;

    ss >> command >> target;
    std::getline(ss, message);

    if (!message.empty() && message[0] == ':')
        message = message.substr(1);

    if (target.empty() || message.empty()) {
        std::string errorMessage = ":localhost 411 " + client->GetNick() + " :No recipient or text to send\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
        return ;
    }

    std::cout << message << std::endl; 

    if (target[0] == '#') {
        Channel* channel = server.FindChannel(target);
        if (!channel) {
            std::string errorMessage = ":localhost 403 " + client->GetNick() + " " + target + " :No such channel\r\n";
            if (select.CanWrite(client->GetRemote()->Get()))
                send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
            return;
        }

    server.Broadcast(":" + client->GetNick() + " PRIVMSG " + target + " " + message + "\r\n", client, &select);
    } else {
        Client* recipient = server.getClientByNick(target);
        if (!recipient) {
            std::string errorMessage = ":localhost 401 " + client->GetNick() + " " + target + " :No such nick\r\n";
            if (select.CanWrite(client->GetRemote()->Get()))
                send(client->GetRemote()->Get(), errorMessage.c_str(), errorMessage.size(), 0);
            return;
        }

        std::string msgToSend = ":" + client->GetNick() + "!" + client->GetUser() + "@localhost " + client->GetNick() + " PRIVMSG " + target + message + "\r\n";
        if (select.CanWrite(client->GetRemote()->Get()))
            send(recipient->GetRemote()->Get(), msgToSend.c_str(), msgToSend.size(), 0);
    }
}


//#define REQ_PARAMS const char* line, Server& server, Client* client
