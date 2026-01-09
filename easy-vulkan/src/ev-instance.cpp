#include "ev-instance.h"
#include "debugger/ev-debug-messenger.h"
#include "ev-macro.h"

using namespace ev;

ev::Instance::Instance(
    VkInstance instance,
    const bool enable_debug_messenger
) : enable_debug_messenger(enable_debug_messenger) {
    ev_log_info("Creating Vulkan instance from existing instance...");
    this->instance = instance;
    if (instance == VK_NULL_HANDLE) {
        ev_log_error("Invalid Vulkan instance provided.");
        exit(EXIT_FAILURE);
    }
    
    this->instance_extensions = ev::utility::list_instance_extensions();
    this->instance_layers = ev::utility::list_instance_layers();

    if (enable_debug_messenger) {
        create_debug_messenger();
    }
}

ev::Instance::Instance(
    const vector<const char*>& required_extensions,
    const vector<const char*>& required_layers,
    const bool enable_debug_messenger
): enable_debug_messenger(enable_debug_messenger) {
    ev_log_info("Creating Vulkan instance...");
    this->instance_extensions = ev::utility::list_instance_extensions();
    this->instance_layers = ev::utility::list_instance_layers();

    #ifdef __APPLE__
    if (std::find(required_extensions.begin(), required_extensions.end(), VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == required_extensions.end()) {
        enabled_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }
    #endif

    for (const auto& ext_name : required_extensions) {
        if (!is_support_extension(ext_name)) {
            ev_log_error("Required instance extension not supported: %s", ext_name);
            exit(EXIT_FAILURE);
        }
        ev_log_debug("Extension supported: %s", ext_name);
        enabled_extensions.push_back(ext_name);
    }

    for (const auto& layer_name : required_layers) {
        if (!is_support_layer(layer_name)) {
            ev_log_error("Required instance layer not supported: %s", layer_name);
            exit(EXIT_FAILURE);
        }
        ev_log_debug("Layer supported: %s", layer_name);
        enabled_layers.push_back(layer_name);
    }

    VkApplicationInfo app_info = ev::initializer::create_application_info(
        "easy-vulkan-project",
        VK_MAKE_VERSION(1, 0, 0),
        "easy-vulkan-engine",
        VK_MAKE_VERSION(1, 0, 0),
#if defined(VK_API_VERSION_1_4)
        VK_API_VERSION_1_4
#elif defined(VK_API_VERSION_1_3)
	VK_API_VERSION_1_3
#else 
	VK_API_VERSION_1_0
#endif
    );

    VkInstanceCreateInfo create_info = ev::initializer::instance_create_info(
        &app_info,
        static_cast<uint32_t>(enabled_layers.size()),
        enabled_layers.data(),
        static_cast<uint32_t>(enabled_extensions.size()),
        enabled_extensions.data()
    );
    create_info.pNext = nullptr; // No additional structures
    #ifdef __APPLE__
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    CHECK_RESULT(vkCreateInstance(&create_info, nullptr, &instance));
    ev_log_info("Vulkan instance created successfully.");
    if ( enable_debug_messenger ) {
        create_debug_messenger();
    }
}

void ev::Instance::create_debug_messenger() {
    if (!enable_debug_messenger) {
        return;
    }
    ev_log_info("Creating debug messenger...");

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = ev::debugger::debug_callback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func) {
        ev_log_error("Failed to retrieve vkCreateDebugUtilsMessengerEXT function pointer.");
        exit(EXIT_FAILURE);
    }

    VkResult result = func(instance, &debug_create_info, nullptr, &debug_messenger);
    if (result != VK_SUCCESS) {
        ev_log_error("Failed to create debug messenger: %d", result);
        exit(EXIT_FAILURE);
    } else {
        ev_log_info("Debug messenger created successfully.");
    }
}

bool ev::Instance::is_support_extension(const char* extension_name) const {
    for (const auto& ext : instance_extensions) {
        if (strcmp(ext.extensionName, extension_name) == 0) {
            return true;
        }
    }
    return false;
}

bool ev::Instance::is_support_layer(const char* layer_name) const {
    for (const auto& layer : instance_layers) {
        if (strcmp(layer.layerName, layer_name) == 0) {
            return true;
        }
    }
    ev_log_warn("Layer not supported: %s", layer_name);
    return false;
}

vector<VkPhysicalDevice> ev::Instance::get_physical_devices() const {
    return ev::utility::list_physical_devices(instance);
}

void ev::Instance::destroy() {
    if (  !is_valid() ) {
        return;
    }

    if (debug_messenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(instance, debug_messenger, nullptr);
            debug_messenger = VK_NULL_HANDLE;
            ev_log_info("Debug messenger destroyed.");
        } else {
            ev_log_error("Failed to get vkDestroyDebugUtilsMessengerEXT function pointer.");
        }
    }

    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
    ev_log_info("Vulkan instance destroyed.");
}

Instance::~Instance() {
    destroy();
}
