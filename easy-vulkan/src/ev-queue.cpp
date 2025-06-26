#include "ev-queue.h"

using namespace std;

namespace ev {

ev::Queue::Queue(
    std::shared_ptr<Device> _device,
    uint32_t _queue_family_index,
    uint32_t _queue_index
) : device(std::move(_device)), queue_family_index(_queue_family_index), queue_index(_queue_index) {
    vkGetDeviceQueue(*device, queue_family_index, queue_index, &queue); 
}

VkResult ev::Queue::submit(shared_ptr<CommandBuffer> buffer) {
    VkSubmitInfo submit_info = {};
    VkCommandBuffer command_buffer = *buffer;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    return vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
}

VkResult ev::Queue::waitIdle(uint64_t timeout) {
    return vkQueueWaitIdle(queue);
}

VkResult ev::Queue::present(std::shared_ptr<Swapchain> swapchain, uint32_t image_index) {
    VkPresentInfoKHR present_info = {};
    const VkSwapchainKHR handle = *swapchain;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &handle;
    present_info.pImageIndices = &image_index;

    return vkQueuePresentKHR(queue, &present_info);
}

ev::Queue::~Queue() {
    // No explicit cleanup needed, as the queue is managed by the Vulkan device
    queue = VK_NULL_HANDLE;
}

}