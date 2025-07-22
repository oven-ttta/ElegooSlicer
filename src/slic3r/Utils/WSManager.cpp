#include "WSManager.hpp"
#include "WSClient.hpp"
#include <iostream>

namespace Slic3r {

WSManager::WSManager() {
}

WSManager::~WSManager() {
    close();
}

bool WSManager::addClient(const std::string& clientId, const std::string& url, 
                          std::shared_ptr<MessageHandler> msgHandler,
                          std::shared_ptr<ConnectionStatusHandler> statusHandler) {
    std::lock_guard<std::mutex> lock(mClientsMutex);
    
    if (mClients.find(clientId) != mClients.end()) {
        std::cerr << "Client " << clientId << " already exists" << std::endl;
        return false;
    }
    
    try {
        auto client = std::make_shared<WSClient>(clientId, url, msgHandler, statusHandler);

        if (!client->connect()) {
            std::cerr << "Failed to connect client " << clientId << " to " << url << std::endl;
            return false;
        }
        mClients[clientId] = client;
        
        std::cout << "Added client: " << clientId << " -> " << url << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to add client " << clientId << ": " << e.what() << std::endl;
        return false;
    }
}

void WSManager::removeClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(mClientsMutex);
    
    auto it = mClients.find(clientId);
    if (it != mClients.end()) {
        it->second->disconnect();
        mClients.erase(it);
        
        std::cout << "Removed client: " << clientId << std::endl;
    }
}

std::shared_ptr<WSClient> WSManager::getClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(mClientsMutex);
    
    auto it = mClients.find(clientId);
    return (it != mClients.end()) ? it->second : nullptr;
}

std::string WSManager::send(const std::string& clientId, const std::string& data, int timeoutMs) {
    auto client = getClient(clientId);
    if (!client) {
        throw std::runtime_error("Client not found: " + clientId);
    }
    
    if (!client->isConnected()) {
        throw std::runtime_error("Client not connected: " + clientId);
    }
    
    return client->send(data, timeoutMs);
}

void WSManager::close() {
    std::lock_guard<std::mutex> lock(mClientsMutex);
    
    for (auto& pair : mClients) {
        pair.second->disconnect();
    }
    mClients.clear();
    
    std::cout << "WSManager closed" << std::endl;
}

} // namespace Slic3r 