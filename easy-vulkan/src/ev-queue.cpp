#include "ev-queue.h"
#include <cstdlib>

using namespace std;

namespace ev {

ev::Queue::Queue(
    std::shared_ptr<Device> _device,
    uint32_t _queue_family_index,
    uint32_t _queue_index
) : device(std::move(_device)), queue_family_index(_queue_family_index), queue_index(_queue_index) {
    vkGetDeviceQueue(*device, queue_family_index, queue_index, &queue);
    vector<const char*> enabled_extensions = device->get_enabled_extensions();

    bool swapchain_extension_found = false;
    for(const char* ext : enabled_extensions) {
        if ( strcmp(ext, "VK_KHR_swapchain") == 0 ){
            swapchain_extension_found = true;
            break;
        }
    }

    if ( swapchain_extension_found ) {
        logger::Logger::getInstance().error("[Queue::Queue] VK_KHR_swapchain extension is enabled.");
        logger::Logger::getInstance().debug("[Queue::Queue] Loading vkQueuePresentKHR function pointer. before loading : " + std::to_string((uintptr_t)pfn.vkQueuePresentKHR)   
);
        pfn.vkQueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(*device, "vkQueuePresentKHR");
        logger::Logger::getInstance().debug("[Queue::Queue] Loaded vkQueuePresentKHR function pointer: " + std::to_string((uintptr_t)pfn.vkQueuePresentKHR));
        if (!pfn.vkQueuePresentKHR) {
            // std::cout << pfn.vkQueuePresentKHR << std::endl;
            logger::Logger::getInstance().error("[Queue::Queue] Failed to load vkQueuePresentKHR function pointer.");
            exit(EXIT_FAILURE);
        }
    } else if (device->is_swapchain_enabled() && !swapchain_extension_found) {
        logger::Logger::getInstance().error("[Queue::Queue] VK_KHR_swapchain extension is not enabled. Cannot use present functionality.");
        exit(EXIT_FAILURE);
    }
}

VkResult ev::Queue::submit(shared_ptr<CommandBuffer> buffer,
    vector<std::shared_ptr<ev::Semaphore>> wait_semaphores,
    vector<std::shared_ptr<ev::Semaphore>> signal_semaphores,
    VkPipelineStageFlags* wait_dst_stage_mask,
    shared_ptr<Fence> fence,
    void* next
) {
    VkSubmitInfo submit_info = {};
    VkCommandBuffer command_buffer = *buffer;

    vector<VkSemaphore> wait_semaphores_vk;
    for (const auto& semaphore : wait_semaphores) {
        if (semaphore) {
            wait_semaphores_vk.push_back(*semaphore);
        }
    }

    vector<VkSemaphore> signal_semaphores_vk;
    for (const auto& semaphore : signal_semaphores) {
        if (semaphore) {
            signal_semaphores_vk.push_back(*semaphore);
        }
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    if ( wait_semaphores_vk.empty() ) {
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
    } else {
        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores_vk.size());
        submit_info.pWaitSemaphores = wait_semaphores_vk.data();
    }

    if( signal_semaphores_vk.empty() ) {
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
    } else {
        submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores_vk.size());
        submit_info.pSignalSemaphores = signal_semaphores_vk.data();
    }
    submit_info.pWaitDstStageMask = wait_dst_stage_mask;
    submit_info.pNext = next;

    VkFence fence_vk = fence ? *fence : VK_NULL_HANDLE;

    return vkQueueSubmit(queue, 1, &submit_info, fence_vk);
}

VkResult ev::Queue::submits( std::vector<std::shared_ptr<ev::CommandBuffer>> buffers,
    vector<std::shared_ptr<ev::Semaphore>> wait_semaphores,
    vector<std::shared_ptr<ev::Semaphore>> signal_semaphores,
    VkPipelineStageFlags* wait_dst_stage_mask,
    shared_ptr<Fence> fence,
    void* next
) {
    vector<VkCommandBuffer> command_buffers;

    for (const auto& buffer : buffers) {
        if (buffer) {
            command_buffers.push_back(*buffer);
        }
    }

    VkSubmitInfo submit_info = {};
    vector<VkSemaphore> wait_semaphores_vk;
    for (const auto& semaphore : wait_semaphores) {
        if (semaphore) {
            wait_semaphores_vk.push_back(*semaphore);
        }
    }

    vector<VkSemaphore> signal_semaphores_vk;
    for (const auto& semaphore : signal_semaphores) {
        if (semaphore) {
            signal_semaphores_vk.push_back(*semaphore);
        }
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
    submit_info.pCommandBuffers = command_buffers.data();
    submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores_vk.size());
    submit_info.pWaitSemaphores = wait_semaphores_vk.data();
    submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores_vk.size());
    submit_info.pSignalSemaphores = signal_semaphores_vk.data();
    submit_info.pWaitDstStageMask = wait_dst_stage_mask;
    submit_info.pNext = next;

    return vkQueueSubmit(queue, 1, &submit_info, fence ? *fence : VK_NULL_HANDLE);
}

VkResult ev::Queue::wait_idle(uint64_t timeout) {
    return vkQueueWaitIdle(queue);
}

VkResult ev::Queue::present(std::shared_ptr<Swapchain> swapchain, 
    uint32_t image_index,
    vector<std::shared_ptr<ev::Semaphore>> wait_semaphores
) {
    ev::logger::Logger::getInstance().debug("[Queue::present] : Presenting image index " + std::to_string(image_index) + " to swapchain.");
    VkPresentInfoKHR present_info = {};
    const VkSwapchainKHR handle = *swapchain;

    vector<VkSemaphore> wait_semaphores_vk;
    for (const auto& semaphore : wait_semaphores) {
        if (semaphore) {
            wait_semaphores_vk.push_back(*semaphore);
        }
    }
    logger::Logger::getInstance().debug("[Queue::present] : Number of wait semaphores: " + std::to_string(wait_semaphores_vk.size()));

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &handle;
    present_info.pImageIndices = &image_index;
    present_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores_vk.size());
    present_info.pWaitSemaphores = wait_semaphores_vk.data();

    return pfn.vkQueuePresentKHR(queue, &present_info);
}

ev::Queue::~Queue() {
    // No explicit cleanup needed, as the queue is managed by the Vulkan device
    queue = VK_NULL_HANDLE;
}

}