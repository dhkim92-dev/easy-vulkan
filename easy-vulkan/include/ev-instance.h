#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>
#include "initializer/ev-initializer.h"
#include "ev-utility.h"
#include "ev-logger.h"

using namespace std;

namespace ev {

class Instance {

private:
    vector<VkExtensionProperties> instance_extensions;

    vector<VkLayerProperties> instance_layers;

    vector<const char *> enabled_layers;

    vector<const char *> enabled_extensions;

    VkInstance instance = VK_NULL_HANDLE; 

    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    const bool enable_debug_messenger;

    void create_debug_messenger();
    
public: 

    explicit Instance(
        VkInstance instance,
        const bool enable_debug_messenger = false
    );

    explicit Instance ( 
        const vector<const char*> &required_extensions, 
        const vector<const char*> &required_layers,
        const bool enable_debug_messenger = false
    );

    Instance& operator=(const Instance&) = delete;

    Instance(const Instance&) = delete;

    ~Instance();

    void destroy();

    vector<VkPhysicalDevice> get_physical_devices() const;

    bool is_support_extension(const char* extension_name) const;

    bool is_support_layer(const char* layer_name) const;

    operator VkInstance() const {
        return instance;
    }

    const VkInstance get_instance() const {
        return instance;
    }

    const vector<const char*>  get_enabled_extensions() const {
        return enabled_extensions;
    }

    const vector<const char*> get_enabled_layers() const{
        return enabled_layers;
    }

    bool is_valid() const {
        return instance != VK_NULL_HANDLE;
    }

};

}
