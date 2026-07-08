/*

    This program is based off of the GLFW-Vulkan example for ImGui.

*/
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "implot_internal.h"
#include "NotoSansMathRegular.hpp"


#include <cmath>
#include <stdio.h>          
#include <stdlib.h>         
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "lesset.hpp"
#include <fstream>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// #define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
#endif

// Data
static VkAllocationCallbacks*   g_Allocator = nullptr;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

struct Instance
{
    std::string name;
    std::vector<lessetB::Variable> userVariables;
    std::vector<lessetB::Alias> userAliases;
    std::string lastScriptOutput;
};



#define TRIMMEDDECIMALPLACES 3
#define HIGHPRECISIONDRAWDELAY 50
#define MANYGRAPHS 100
#define MAX_COMMANDS 100000
#define MAXGRAPHPOINTSBASE 8000





bool isNoisy(const std::vector<double> &pointsX, const std::vector<double> &pointsY, size_t i, int maxIndividualGraphPointsMultiplier);
bool addIdentifier(Instance &data,const lessetB::Alias &newAlias);
bool addIdentifier(Instance &data,const lessetB::Variable &newVariable);
bool replaceAliases(std::string &equation, Instance &instance);







static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static void SetupVulkan(ImVector<const char*> instance_extensions)
{
    VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(err);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_Instance);
#endif

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif
    }

    // Select Physical Device (GPU)
    g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
    IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);

    // Select graphics queue family
    g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
    IM_ASSERT(g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    // Create Descriptor Pool
    // If you wish to load e.g. additional textures you may need to falter pools sizes and maxSets.
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->Surface = surface;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_COUNTOF(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_COUNTOF(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount, 0);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow(ImGui_ImplVulkanH_Window* wd)
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, wd, g_Allocator);
    vkDestroySurfaceKHR(g_Instance, wd->Surface, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "LessetG", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(err);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha=0.9f;
    style.FrameRounding=3.f;
    style.PopupRounding=3.f;
    style.WindowRounding=3.f;
    style.GrabRounding=3.f;
    //style.FontScaleMain=1.5;
    
    //ImGui::StyleColorsLight();

    // Setup scaling
    // ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.Allocator = g_Allocator;
    init_info.PipelineInfoMain.RenderPass = wd->RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    // io.Fonts->AddFontDefaultVector();
    // io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    io.Fonts->AddFontFromMemoryCompressedTTF(NotoSansMathRegular_compressed_data,NotoSansMathRegular_compressed_size);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 0.f);

    std::string result;
    std::string newIdentifierNameVariable{};
    std::string newIdentifierValueVariable{};
    std::string newIdentifierNameAlias{};
    std::string newIdentifierValueAlias{};
    std::string nothing;
    std::string lastNonEmptyEquation;
    std::vector<std::string> graphsEquations{};
    std::string graphEquation{};
    std::string nonEmptyGraphEquation{};
    std::string equation{};
    std::string nonEmptyEquation{};
    float pastPrecisionDivisors[100]{};
    bool graph{};
    bool previewGraph{};
    bool updatePreviewGraph{};
    bool markSpecialPoints{true};
    size_t timeStationary{};
    ImPlotRect limits{};
    size_t functionCount{};
    size_t selectedInstance{};
    std::string newInstanceName;
    std::string filePath;
    std::string resultHistory;
    std::string assignVariableReport;
    std::string assignAliasReport;
    bool showFps{};
    std::pair<std::vector<double>,std::vector<double>> liveGraphPoints;

    bool drawManyGraphs{true};

    int maxIndividualGraphPoints {8000}; // Maximum number of points per graph calculated for one screen. (There may be more because of memoization when zooming out)

    bool hasRunScriptInMain{};

    std::vector<Instance> instances;
    instances.emplace_back("main"); 

    std::pair<std::vector<std::vector<double>>,std::vector<std::vector<double>>> graphsPoints{};

    lessetB::Options options{0,0,0,0.1,2,true};
    float xMinFloat{};
    float xMaxFloat{};
    float xStepFloat{0.1};
    int aroundTruthinessLeniency{2};
    bool followImplicitMultiplicationPriorityConvention{true};

    bool interpolateDiscontinuities{};
    bool recalculateGraphs{};
    int maxIndividualGraphPointsMultiplier{2};

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Resize swap chain?
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount, 0);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            nothing.clear();
            auto io = ImGui::GetIO();
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::SetNextWindowPos(ImVec2(0,0));
            style.WindowRounding=0.f;

            ImGui::Begin("LessetG says hello!",__null,ImGuiWindowFlags_MenuBar+ImGuiWindowFlags_NoTitleBar+ImGuiWindowFlags_NoResize+ImGuiWindowFlags_NoMove);                          // Create a window called "Hello, world!" and append into it.
            style.WindowRounding=5.f;
            if (ImGui::BeginMenuBar())
            {
                if(ImGui::BeginMenu("Scripting"))
                {
                    //ImGui::MenuItem("Everything about automation.",NULL,false,false);
                    if(ImGui::BeginMenu("Instances"))
                    {
                        if(ImGui::BeginMenu("Add instances"))
                        {
                            ImGui::InputText("New instance name",&newInstanceName);

                            if(ImGui::Button("Add instance"))
                            {
                                bool nameOkay{};
                                if(newInstanceName!="") nameOkay=true;
                                for(size_t i{}; i<instances.size(); i++)
                                {
                                    if(newInstanceName==instances.at(i).name) nameOkay=false;
                                }
                                if(nameOkay) instances.emplace_back(newInstanceName);
                            }  
                            ImGui::EndMenu(); 
                        }     
                        
                        if(instances.size()>1)if(ImGui::BeginMenu("Select instance"))
                        {
                            for(size_t i{0}; i<instances.size(); i++)
                            {
                                if(ImGui::Button(instances.at(i).name.c_str()))
                                {
                                    selectedInstance=i;
                                    recalculateGraphs=true;
                                }
                            }    
                            ImGui::EndMenu();
                        }

                        if(instances.size()>1) if(ImGui::BeginMenu("Remove instances"))
                        {
                            ImGui::MenuItem("Click an instance to remove it.",NULL,false,false);
                            for(size_t i{1}; i<instances.size(); i++)
                            {
                                if(ImGui::Button(instances.at(i).name.c_str()))
                                {
                                    if(selectedInstance==i) selectedInstance=0;
                                    instances.erase(instances.begin()+i);
                                }
                            }  
                            ImGui::EndMenu();
                        }

                        ImGui::EndMenu();
                    }
                    ImGui::SetItemTooltip("Encapsulate calculator data.");
                    if (ImGui::BeginMenu("Run"))
                    {
                        if(selectedInstance==0) ImGui::Text("You are using the main instance.");
                        if(selectedInstance==0 && hasRunScriptInMain)
                        {
                            if(ImGui::Button("Clear main instance's data"))
                            {
                                instances.at(0)=Instance();
                                hasRunScriptInMain=false;
                            }
                        }
                        ImGui::InputText("File path to script",&filePath);

                        if(ImGui::Button("Run"))
                        {
                            size_t iterations{};
                            std::ifstream calculationsFile;
                            calculationsFile.open(filePath);
                            if(!calculationsFile.fail())
                            {
                                if(instances.at(selectedInstance).lastScriptOutput!="")
                                {
                                    instances.at(selectedInstance).lastScriptOutput="";
                                }
                                if(selectedInstance==0) hasRunScriptInMain=true;
                                std::string scriptEquation;
                                bool skip{};
                                bool conditionTrue{};
                                std::string conditionValue;
                                std::string jumpValue;
                                
                                while(std::getline(calculationsFile,scriptEquation) && iterations<MAX_COMMANDS)
                                {
                                    std::string conditionValue;
                                    std::string jumpValue;
                                    iterations++;
                                    for(size_t i{}; i<scriptEquation.length() && scriptEquation.at(i)!='#'; i++)
                                    {
                                        if(std::isspace(scriptEquation.at(i))) scriptEquation.erase(i--,1);
                                    }


                                    if(scriptEquation.find("endif")==0)
                                    {
                                        skip=false;
                                        instances.at(selectedInstance).lastScriptOutput+="endif\n";
                                        continue;
                                    }

                                    if(scriptEquation.find("graph")==0 &&(!skip || conditionTrue==true))
                                    {
                                        for(size_t j{}; j<instances.at(selectedInstance).userVariables.size(); j++)
                                        {
                                            for(size_t i{}; i<scriptEquation.length(); i++)
                                            {
                                                if(scriptEquation.find("numof",i)==i)
                                                {
                                                    if(scriptEquation.find(instances.at(selectedInstance).userVariables.at(j).name)<=i+6)
                                                    {
                                                        scriptEquation.replace(i,5+instances.at(selectedInstance).userVariables.at(j).name.length(),instances.at(selectedInstance).userVariables.at(j).value);
                                                    }
                                                }
                                            }
                                        }

                                        if(0==graphsEquations.size()) 
                                        {
                                            instances.at(selectedInstance).lastScriptOutput+="Graphing "+scriptEquation.substr(5)+'\n';
                                            graphsEquations.push_back(scriptEquation.substr(5));
                                            recalculateGraphs=true;
                                        }
                                        else for(size_t i{}; i<graphsEquations.size(); i++)
                                        {
                                            if(graphsEquations.at(i)==scriptEquation.substr(5)) break;
                                            else if(i>=graphsEquations.size()-1) 
                                            {
                                                instances.at(selectedInstance).lastScriptOutput+="Graphing "+scriptEquation.substr(5)+'\n';
                                                graphsEquations.push_back(scriptEquation.substr(5));
                                                recalculateGraphs=true;
                                            }
                                        }
                                        continue;
                                    }


                                    if(scriptEquation.find("if")==0)
                                    {
                                        std::string subEquation=scriptEquation.substr(2);
                                        if(subEquation!="")
                                        {
                                            for(size_t i{}; i<subEquation.length(); i++)
                                            {
                                                if(std::isspace(subEquation.at(i)))
                                                {
                                                    subEquation.erase(i--,1);
                                                }
                                            }
                                            size_t subEquationLength=subEquation.length();
                                            mainLoop(options, true, true, subEquation, nothing,conditionValue,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);

                                            if(conditionValue.substr(subEquationLength+3).find("true")!=std::string::npos) conditionTrue=true;

                                            else if(conditionValue.substr(subEquationLength+3).find("false")!=std::string::npos) conditionTrue=false;
                                            else if(conditionValue.substr(subEquationLength+3).find("Not a Number")!=std::string::npos) conditionTrue=false;

                                            
                                            else if(std::stold(conditionValue.substr(subEquationLength+3))>=1)
                                            {
                                                conditionTrue=true;
                                            }
                                            if(calculationsFile.peek()=='\n') for(; calculationsFile.peek()=='\n'; calculationsFile.seekg(static_cast<size_t>(calculationsFile.tellg())+1));
                                            skip=true;
                                            if(conditionTrue) instances.at(selectedInstance).lastScriptOutput+="beginif\n";
                                            continue;
                                        }
                                    }
                                    if(scriptEquation.find("endif")==0 && conditionTrue)
                                    {
                                        conditionTrue=false;
                                    }

                                    if(scriptEquation.find("jump")==0&&(!skip || conditionTrue==true))
                                    {
                                        std::string subEquation=scriptEquation.substr(4);
                                        if(subEquation!="")
                                        {
                                            long long jumpDestination{};
                                            for(size_t i{}; i<subEquation.length(); i++)
                                            {
                                                if(std::isspace(subEquation.at(i)))
                                                {
                                                    subEquation.erase(i--,1);
                                                }
                                            }

                                            // I am gonna be honest, this code sucks. The .substr() is merely there to work around Lesset returning more than just the result.

                                            size_t subEquationLength=subEquation.length();
                                            mainLoop(options, true, true, subEquation, nothing,jumpValue,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false); // Lines with # are comments
                                            if(jumpValue.substr(subEquationLength+3).find("true")!=std::string::npos) jumpDestination=1;
                                            else if(jumpValue.substr(subEquationLength+3).find("false")!=std::string::npos) jumpDestination=0;
                                            else jumpDestination=round(std::stold(jumpValue.substr(subEquationLength+2)));

                                            calculationsFile.seekg(std::ios::beg);
                                            for(size_t i{}; i<jumpDestination-1; i++)
                                            {
                                                calculationsFile.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
                                            }

                                            if(calculationsFile.peek()=='\n') for(; calculationsFile.peek()=='\n'; 
                                            calculationsFile.seekg(static_cast<size_t>(calculationsFile.tellg())+1));
                                            conditionTrue=false;
                                            skip=false;
                                            instances.at(selectedInstance).lastScriptOutput+="Jumped to line "+std::to_string(jumpDestination)+'\n';
                                            continue;
                                        }
                                        
                                    }

                                    if(!skip || conditionTrue==true)
                                    {
                                        if(scriptEquation.find('x')<scriptEquation.find('#') && scriptEquation.find('x')!=std::string::npos)
                                        {
                                            scriptEquation.at(0)='#';
                                        }
                                        mainLoop(options, true, true, scriptEquation, nothing,instances.at(selectedInstance).lastScriptOutput,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,true); // Lines with # are comments
                                    }
                                    if(calculationsFile.peek()=='\n') for(; calculationsFile.peek()=='\n'; calculationsFile.seekg(static_cast<size_t>(calculationsFile.tellg())+1));
                                }
                                calculationsFile.close();
                            }
                        }

                        if(instances.at(selectedInstance).lastScriptOutput!="")
                        {
                            if(ImGui::BeginMenu("Output"))
                            {
                                ImGui::Text("Output of script:\n%s", instances.at(selectedInstance).lastScriptOutput.c_str());
                                ImGui::EndMenu();
                            }
                        }
                        
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Variables"))
                    {
                        if (ImGui::BeginMenu("New"))
                        {
                            ImGui::InputText("Name",&newIdentifierNameVariable);
                            ImGui::InputText("Value",&newIdentifierValueVariable);
                            if (ImGui::Button("Assign"))
                            {
                                std::string combinedStatement{"let"+newIdentifierNameVariable+"="+newIdentifierValueVariable};
                                lessetB::mainLoop(options,true,false,combinedStatement,nothing,assignVariableReport,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,true);
                            }
                            if(assignVariableReport.find('\n',assignVariableReport.find('\n')+1)!=std::string::npos)
                            {
                                assignVariableReport.erase(0,assignVariableReport.find('\n',0)+1);
                            }
                            if(assignVariableReport!="") ImGui::Text("%s", assignVariableReport.c_str());
                            ImGui::EndMenu();
                        }
                        if (ImGui::BeginMenu("Show"))
                        {
                            if(instances.at(selectedInstance).userVariables.size()==0)
                            {
                                ImGui::MenuItem("You have no variables.",NULL,false,false);
                            }
                            else ImGui::MenuItem("Click a variable to delete it.",NULL,false,false);
                            std::string formatted;
                            for(size_t i{}; i<instances.at(selectedInstance).userVariables.size(); i++)
                            {
                                formatted=instances.at(selectedInstance).userVariables.at(i).name+" = "+instances.at(selectedInstance).userVariables.at(i).value;
                                if(ImGui::MenuItem(formatted.c_str()))
                                {
                                    instances.at(selectedInstance).userVariables.erase(instances.at(selectedInstance).userVariables.begin()+i);
                                }
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenu();
                    }


                    if (ImGui::BeginMenu("Macros"))
                    {
                        if (ImGui::BeginMenu("New"))
                        {
                            ImGui::InputText("Name",&newIdentifierNameAlias);
                            ImGui::InputText("Value",&newIdentifierValueAlias);
                            if (ImGui::Button("Assign"))
                            {
                                std::string combinedStatement{"set"+newIdentifierNameAlias+"="+newIdentifierValueAlias};
                                lessetB::mainLoop(options,true,false,combinedStatement,nothing,assignAliasReport,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,true);
                            }
                            if(assignAliasReport.find('\n',assignAliasReport.find('\n')+1)!=std::string::npos)
                            {
                                assignAliasReport.erase(0,assignAliasReport.find('\n',0)+1);
                            }
                            if(assignAliasReport!="") ImGui::Text("%s", assignAliasReport.c_str());
                            ImGui::EndMenu();
                        }
                        if (ImGui::BeginMenu("Show"))
                        {
                            if(instances.at(selectedInstance).userAliases.size()==0)
                            {
                                ImGui::MenuItem("You have no macros.",NULL,false,false);
                            }
                            else ImGui::MenuItem("Click a macro to delete it.",NULL,false,false);
                            std::string formatted;
                            for(size_t i{}; i<instances.at(selectedInstance).userAliases.size(); i++)
                            {
                                formatted=instances.at(selectedInstance).userAliases.at(i).name+" = "+instances.at(selectedInstance).userAliases.at(i).value;
                                if(ImGui::MenuItem(formatted.c_str()))
                                {
                                    instances.at(selectedInstance).userAliases.erase(instances.at(selectedInstance).userAliases.begin()+i);
                                }
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Automate certain things.");


                if (ImGui::BeginMenu("Constants"))
                {
                    if(ImGui::BeginMenu("Mathematics"))
                    {
                        if(ImGui::MenuItem("π")) equation.append("π");
                        ImGui::SetItemTooltip("3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068");

                        if(ImGui::MenuItem("ℯ")) equation.append("ℯ");
                        ImGui::SetItemTooltip("2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427");
                        
                        if(ImGui::MenuItem("γ")) equation.append("γ");
                        ImGui::SetItemTooltip("0.5772156649015328606065120900824024310421593359399235988057672348848677267776646709369470632917467495");

                        if(ImGui::MenuItem("φ")) equation.append("φ");
                        ImGui::SetItemTooltip("1.618033988749894848204586834365638117720309179805762862135448622705260462818902449707207204189391137");
                        
                        if(ImGui::MenuItem("ξ")) equation.append("ξ");
                        ImGui::SetItemTooltip("A random number between 0 and 1.");

                        if(ImGui::MenuItem("rndint")) equation.append("rndint");
                        ImGui::SetItemTooltip("A random positive integer.");

                        if(ImGui::MenuItem("τ")) equation.append("τ");
                        ImGui::SetItemTooltip("6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136");
                        
                        if(ImGui::MenuItem("∞")) equation.append("∞");
                        ImGui::SetItemTooltip("Infinity... infinity... infinity... infinity... infinity...");

                        if(ImGui::MenuItem("prc")) equation.append("prc");
                        ImGui::SetItemTooltip("0.01");

                        if(ImGui::MenuItem("ppm")) equation.append("ppm");
                        ImGui::SetItemTooltip("1×10^-06");

                        if(ImGui::MenuItem("ppb")) equation.append("ppb");
                        ImGui::SetItemTooltip("1×10^-09");

                        if(ImGui::MenuItem("ppt")) equation.append("ppt");
                        ImGui::SetItemTooltip("1×10^-12");

                        if(ImGui::MenuItem("dgr")) equation.append("dgr");
                        ImGui::SetItemTooltip("0.01745329251994329576923690768488612713442871888541725456097191440171009114603449443682241569634509482\nDegrees to Radiants. Try entering with numbers into trig functions.");

                        if(ImGui::MenuItem("rad")) equation.append("rad");
                        ImGui::SetItemTooltip("57.29577951308232087679815481410517033240547246656432154916024386120284714832155263244096899585111094"); 
                        
                        ImGui::EndMenu();
                    }

                    if(ImGui::BeginMenu("Physics"))
                    {
                        if(ImGui::MenuItem("c")) equation.append("ec");
                        ImGui::SetItemTooltip("Speed of light, 299792458");

                        if(ImGui::MenuItem("ec")) equation.append("ec");
                        ImGui::SetItemTooltip("Elementary charge, 1.602176634×10^-19");

                        if(ImGui::MenuItem("G")) equation.append("G");
                        ImGui::SetItemTooltip("Gravitational Constant, 6.6743×10^-11");

                        if(ImGui::MenuItem("g")) equation.append("g");
                        ImGui::SetItemTooltip("Gravity on Earth's surface, 9.80665");
                        
                        if(ImGui::MenuItem("α")) equation.append("α");
                        ImGui::SetItemTooltip("0.0072973525693");

                        if(ImGui::MenuItem("me")) equation.append("me");
                        ImGui::SetItemTooltip("Mass of Earth, 5.9722×10^24");

                        if(ImGui::MenuItem("H0")) equation.append("H0");
                        ImGui::SetItemTooltip("2.2×10^-18");

                        if(ImGui::MenuItem("Z0")) equation.append("Z0");
                        ImGui::SetItemTooltip("376.730313668");

                        if(ImGui::MenuItem("U0")) equation.append("U0");
                        ImGui::SetItemTooltip("1.25663706212×10^-06");

                        if(ImGui::MenuItem("E0")) equation.append("E0");
                        ImGui::SetItemTooltip("8.8541878128×10^-12");

                        if(ImGui::MenuItem("ma")) equation.append("ma");
                        ImGui::SetItemTooltip("1.6605390666×10^-27");

                        if(ImGui::MenuItem("R")) equation.append("R");
                        ImGui::SetItemTooltip("8.31446261815");

                        if(ImGui::MenuItem("Na")) equation.append("Na");
                        ImGui::SetItemTooltip("6.02214076×10^23");

                        if(ImGui::MenuItem("o")) equation.append("o");
                        ImGui::SetItemTooltip("5.670374419×10^-08");

                        if(ImGui::MenuItem("k")) equation.append("k");
                        ImGui::SetItemTooltip("1.380649×10^-23");

                        if(ImGui::MenuItem("a")) equation.append("a");
                        ImGui::SetItemTooltip("0.0072973525693");

                        if(ImGui::MenuItem("h")) equation.append("h");
                        ImGui::SetItemTooltip("6.62607015×10^-34");

                        ImGui::EndMenu();
                    }
                    
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Add certain constants to your equation.");



                if (ImGui::BeginMenu("Graph"))
                {
                    if(ImGui::BeginMenu("Add functions"))
                    {
                        std::string graphEquationPlusYEquals = "y = " + graphEquation;
                        ImGui::PushItemWidth(550);
                        if(ImGui::InputText("##",&graphEquationPlusYEquals))
                        {
                            updatePreviewGraph=true;
                        }
                        else updatePreviewGraph=false;
                        if(graphEquationPlusYEquals.find("y = ")==0) graphEquation=graphEquationPlusYEquals.substr(4);
                        else graphEquation=graphEquationPlusYEquals;
                        if(ImGui::Button("Add"))
                        {
                            // If no graphs, don't try to check for duplicates, as you'd be unable to add the first graph due to the loop exiting.
                            for(int i{static_cast<int>(graphEquation.length()-1)}; i>0; i--)
                            {
                                if(std::isspace(graphEquation.at(i))) graphEquation.erase(i--,1);
                            }
                            if(0==graphsEquations.size()) 
                            {
                                graphsEquations.emplace_back(graphEquation);
                                recalculateGraphs=true;
                            }

                            else for(size_t i{}; i<graphsEquations.size(); i++)
                            {
                                if(graphsEquations.at(i)==graphEquation) break;
                                else if(i==graphsEquations.size()-1)
                                {
                                    graphsEquations.emplace_back(graphEquation); 
                                    recalculateGraphs=true;
                                }
                            }
                            
                        }
                        ImGui::SameLine();
                        if(ImGui::Checkbox("Preview", &previewGraph))
                        {
                            updatePreviewGraph=true;
                        }
                        ImGui::SetItemTooltip("Graph the equation as you are typing.");
                        ImGui::EndMenu();
                    }

                    if(graphsEquations.size()!=0)
                    {
                        if(ImGui::BeginMenu("Remove functions"))
                        {
                            ImGui::MenuItem("Click a function to remove it.",NULL,false,false);
                            if(graphsEquations.size()>4) if(ImGui::Button("Remove All"))
                            {
                                graphsEquations.clear();
                                graphsPoints.first.clear();
                                graphsPoints.second.clear();
                                lessetB::globals::points.first.clear();
                                lessetB::globals::points.second.clear();
                            }

                            for(size_t i{}; i<graphsEquations.size(); i++)
                            {
                                if(ImGui::Button(graphsEquations.at(i).c_str()))
                                {
                                    graphsEquations.erase(graphsEquations.begin()+i);
                                    if(graphsPoints.first.size()!=0)
                                    {
                                        graphsPoints.first.erase(graphsPoints.first.begin()+i);
                                        graphsPoints.second.erase(graphsPoints.second.begin()+i);
                                        recalculateGraphs=true;
                                    }
                                }
                            }
                            ImGui::EndMenu();
                        }  
                    }

                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Graph functions that include x.");

                if(ImGui::BeginMenu("Options"))
                {

                    ImGui::SliderInt("Matching digits for ≈ being true",&aroundTruthinessLeniency,0,10);
                    ImGui::SetItemTooltip("When 2 values are close to equal, how many digits after the decimal point need to match for ≈ to be true?\nExample: 0≈0.1 = true when this setting is 0. However, 0≈0.11 = false.");
                    if(aroundTruthinessLeniency<0) aroundTruthinessLeniency=0;
                    if(aroundTruthinessLeniency>98) aroundTruthinessLeniency=98;
                    options.aroundTruthinessLeniency=aroundTruthinessLeniency;


                    if(maxIndividualGraphPointsMultiplier>5) maxIndividualGraphPointsMultiplier=5;
                    if(maxIndividualGraphPointsMultiplier<1) maxIndividualGraphPointsMultiplier=1;
                    std::string shownResolution="Medium";
                    
                    switch(maxIndividualGraphPointsMultiplier)
                    {
                        case 1: {maxIndividualGraphPoints=MAXGRAPHPOINTSBASE/2; shownResolution="Low"; break;}
                        case 2: {maxIndividualGraphPoints=MAXGRAPHPOINTSBASE; shownResolution="Medium"; break;}
                        case 3: {maxIndividualGraphPoints=MAXGRAPHPOINTSBASE*2; shownResolution="High"; break;}
                        case 4: {maxIndividualGraphPoints=MAXGRAPHPOINTSBASE*4; shownResolution="Overkill"; break;}
                        case 5: {maxIndividualGraphPoints=MAXGRAPHPOINTSBASE/4; shownResolution="Too Low"; break;}

                        default: {maxIndividualGraphPoints=MAXGRAPHPOINTSBASE/2; shownResolution="Low"; break;}
                    }
                    if(ImGui::SliderInt("Resolution for graphing",&maxIndividualGraphPointsMultiplier,1,5,shownResolution.c_str()))
                    {
                        recalculateGraphs=true;
                    }
                    ImGui::SetItemTooltip("Changes how precisely graphs are calculated, how many points per graph.\nOverkill might be useful for detailed graphs.\nOnly use Too Low if you have many graphs, are creating insane functions or your computer sux.");

                    if(ImGui::BeginMenu("Calculations with x"))
                    {
                        ImGui::SliderFloat("Minimum x",&xMinFloat,-100,100);
                        options.xMin=xMinFloat;

                        ImGui::SliderFloat("Maximum x",&xMaxFloat,-100,100);
                        options.xMax=xMaxFloat;

                        ImGui::SliderFloat("Increment x by",&xStepFloat,0.01,100);
                        if(xStepFloat<0.01f) xStepFloat=0.01f;
                        options.xStep=xStepFloat;

                        ImGui::EndMenu();
                    }

                    if(ImGui::Checkbox("Interpolate discontinuities in functions",&interpolateDiscontinuities))
                    {
                        recalculateGraphs=true;
                    }
                    ImGui::SetItemTooltip("This calculator is stupid and doesn't actually know where exactly a discontinuity in a function like 1/x is.\nThus, it tries to approximate it, but sometimes ends up creating visual artifacts in continuous functions.");


                    ImGui::Checkbox("Mark special points",&markSpecialPoints);
                    ImGui::SetItemTooltip("Mark points where a function is zero, the point closest to the cursor, extremes.");

                    options.interpolateDiscontinuities=interpolateDiscontinuities;

                    ImGui::Checkbox("Prioritize implicit multiplication",&followImplicitMultiplicationPriorityConvention);
                    options.followImplicitMultiplicationPriorityConvention=followImplicitMultiplicationPriorityConvention;
                    ImGui::SetItemTooltip("Disambiguate something like 8÷2(2+2) as 8÷(2(2+2))=1 instead of (8÷2)(2+2)=16.");

                    
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Tweak some settings.");

                if(ImGui::BeginMenu("History"))
                {
                    if(resultHistory!="")
                    {
                        if(ImGui::Button("Clear"))
                        {
                            resultHistory="";
                        }
                    }
                    else
                    {
                        ImGui::MenuItem("You have no history.",NULL,false,false);
                    }
                    std::string line;
                    // ImGui::Text("%s",resultHistory.c_str());
                    
                    for(size_t i{1}; i<resultHistory.length();)
                    {
                        line=resultHistory.substr(i,resultHistory.find('\n',i)-i);
                        if(ImGui::Button(line.c_str()))
                        {
                            ImGui::SetClipboardText(line.c_str());
                        }
                        ImGui::SetItemTooltip("Click to copy.");
                        i+=line.length()+1;
                    }


                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("View past calculations.");

                if (ImGui::BeginMenu("Help"))
                {
                    if(ImGui::BeginMenu("Operators"))
                    {   
                        if(ImGui::MenuItem("+")) ImGui::SetClipboardText("+");
                        ImGui::SetItemTooltip("Adds left and right or does nothing (unary plus). Is a tooltip really needed for this..?");
                        
                        if(ImGui::MenuItem("-")) ImGui::SetClipboardText("-");
                        ImGui::SetItemTooltip("Subtracts left and right or negates right.");

                        if(ImGui::MenuItem("×")) ImGui::SetClipboardText("×");
                        ImGui::SetItemTooltip("Multiplication. (*)");

                        if(ImGui::MenuItem("÷")) ImGui::SetClipboardText("÷");
                        ImGui::SetItemTooltip("Division... please not by 0. (/)");

                        if(ImGui::MenuItem("**")) ImGui::SetClipboardText("**");
                        ImGui::SetItemTooltip("Exponentiation, undefined for negative left and decimal right. (^)");

                        if(ImGui::MenuItem("!")) ImGui::SetClipboardText("!");
                        ImGui::SetItemTooltip("Factorial. (Uses Gamma function)");

                        if(ImGui::MenuItem("!!")) ImGui::SetClipboardText("!!");
                        ImGui::SetItemTooltip("Semi-Factorial. Integers only!");

                        if(ImGui::MenuItem("%")) ImGui::SetClipboardText("%");
                        ImGui::SetItemTooltip("Modulus, division with remainder. (mod)");

                        if(ImGui::MenuItem("nPk")) ImGui::SetClipboardText("nPk");
                        ImGui::SetItemTooltip("Permutation calculation.");

                        if(ImGui::MenuItem("nCk")) ImGui::SetClipboardText("nCk");
                        ImGui::SetItemTooltip("Binomial coefficient calculation, whatever that means.");

                        if(ImGui::MenuItem("|expr|")) ImGui::SetClipboardText("||");
                        ImGui::SetItemTooltip("Absolute value.");

                        if(ImGui::MenuItem("(expr)")) ImGui::SetClipboardText("()");
                        ImGui::SetItemTooltip("Parentheses.");

                        ImGui::EndMenu();
                    }
                    if(ImGui::BeginMenu("Functions"))
                    {
                        if(ImGui::BeginMenu("Roots"))
                        {
                            if(ImGui::MenuItem("√")) ImGui::SetClipboardText("√");
                            ImGui::SetItemTooltip("Square root (sqrt) function.");

                            if(ImGui::MenuItem("∛")) ImGui::SetClipboardText("∛");
                            ImGui::SetItemTooltip("Cube root (cbrt) function.");

                            if(ImGui::MenuItem("∜")) ImGui::SetClipboardText("∜");
                            ImGui::SetItemTooltip("Quartic root (qtrt) function.");

                            if(ImGui::MenuItem("root(")) ImGui::SetClipboardText("root(");
                            ImGui::SetItemTooltip("Nth root function, denominator on the left, enumerator right.\nMay be called with one argument for sqrt.");
                            ImGui::EndMenu();
                        }

                        if(ImGui::BeginMenu("Logarithm"))
                        {
                            if(ImGui::MenuItem("log(")) ImGui::SetClipboardText("log(");
                            ImGui::SetItemTooltip("Generic log function, base on the left, expression right.\nMay be called with one argument for log10(expr).");
                            
                            if(ImGui::MenuItem("ln")) ImGui::SetClipboardText("ln");
                            ImGui::SetItemTooltip("Log with base ℯ.");
                            
                            ImGui::EndMenu();
                        }

                        if(ImGui::BeginMenu("Trig"))
                        {
                            if(ImGui::MenuItem("sin")) ImGui::SetClipboardText("sin");
                            ImGui::SetItemTooltip("Sine function.");

                            if(ImGui::MenuItem("cos")) ImGui::SetClipboardText("cos");
                            ImGui::SetItemTooltip("Cosine function.");      
 
                            if(ImGui::MenuItem("tan")) ImGui::SetClipboardText("tan");
                            ImGui::SetItemTooltip("Tangent function.");           
                            
                            if(ImGui::MenuItem("sec")) ImGui::SetClipboardText("sec");
                            ImGui::SetItemTooltip("Secant function.");         

                            if(ImGui::MenuItem("csc")) ImGui::SetClipboardText("csc");
                            ImGui::SetItemTooltip("Cosecant function.");   

                            if(ImGui::MenuItem("cot")) ImGui::SetClipboardText("cot");
                            ImGui::SetItemTooltip("Cotangent function.");   

                            if(ImGui::MenuItem("asin")) ImGui::SetClipboardText("asin");
                            ImGui::SetItemTooltip("Arcsine function.");

                            if(ImGui::MenuItem("acos")) ImGui::SetClipboardText("acos");
                            ImGui::SetItemTooltip("Arccosine function.");      
 
                            if(ImGui::MenuItem("atan")) ImGui::SetClipboardText("atan");
                            ImGui::SetItemTooltip("Arctangent function.");           
                            
                            if(ImGui::MenuItem("asec")) ImGui::SetClipboardText("asec");
                            ImGui::SetItemTooltip("Arcsecant function.");         

                            if(ImGui::MenuItem("acsc")) ImGui::SetClipboardText("acsc");
                            ImGui::SetItemTooltip("Arccosecant function.");   

                            if(ImGui::MenuItem("acot")) ImGui::SetClipboardText("acot");
                            ImGui::SetItemTooltip("Arccotangent function.");  
                            
                            ImGui::EndMenu();
                        }

                        if(ImGui::BeginMenu("Hyperbolic"))
                        {
                            if(ImGui::MenuItem("sinh")) ImGui::SetClipboardText("sinh");
                            ImGui::SetItemTooltip("Hyperbolic sine function.");

                            if(ImGui::MenuItem("cosh")) ImGui::SetClipboardText("cosh");
                            ImGui::SetItemTooltip("Hyperbolic cosine function.");      
 
                            if(ImGui::MenuItem("tanh")) ImGui::SetClipboardText("tanh");
                            ImGui::SetItemTooltip("Hyperbolic tangent function.");           
                            
                            if(ImGui::MenuItem("sech")) ImGui::SetClipboardText("sech");
                            ImGui::SetItemTooltip("Hyperbolic secant function.");         

                            if(ImGui::MenuItem("csch")) ImGui::SetClipboardText("csch");
                            ImGui::SetItemTooltip("Hyperbolic cosecant function.");   

                            if(ImGui::MenuItem("coth")) ImGui::SetClipboardText("coth");
                            ImGui::SetItemTooltip("Hyperbolic cotangent function.");   

                            if(ImGui::MenuItem("asinh")) ImGui::SetClipboardText("asinh");
                            ImGui::SetItemTooltip("Hyperbolic arcsine function.");

                            if(ImGui::MenuItem("acosh")) ImGui::SetClipboardText("acosh");
                            ImGui::SetItemTooltip("Hyperbolic arccosine function.");      
 
                            if(ImGui::MenuItem("atanh")) ImGui::SetClipboardText("atanh");
                            ImGui::SetItemTooltip("Hyperbolic arctangent function.");           
                            
                            if(ImGui::MenuItem("asech")) ImGui::SetClipboardText("asech");
                            ImGui::SetItemTooltip("Hyperbolic arcsecant function.");         

                            if(ImGui::MenuItem("acsch")) ImGui::SetClipboardText("acsch");
                            ImGui::SetItemTooltip("Hyperbolic arccosecant function.");   

                            if(ImGui::MenuItem("acoth")) ImGui::SetClipboardText("acoth");
                            ImGui::SetItemTooltip("Hyperbolic arccotangent function.");  
                            
                            ImGui::EndMenu();
                        }

                        if(ImGui::BeginMenu("Number Manip."))
                        {
                            if(ImGui::MenuItem("round")) ImGui::SetClipboardText("round");
                            ImGui::SetItemTooltip("Rounds number to nearest integer.");

                            if(ImGui::MenuItem("floor")) ImGui::SetClipboardText("floor");
                            ImGui::SetItemTooltip("Floors number to its integer part. π => 3");

                            if(ImGui::MenuItem("ceil")) ImGui::SetClipboardText("ceil");
                            ImGui::SetItemTooltip("Raises number to next integer. π => 4");

                            if(ImGui::MenuItem("abs")) ImGui::SetClipboardText("abs");
                            ImGui::SetItemTooltip("Absolute value as a function.");

                            if(ImGui::MenuItem("sabs(")) ImGui::SetClipboardText("sabs(");
                            ImGui::SetItemTooltip("Absolute value, but smooth.\nArgument 1 is f(x), argument 2 λ (blending)");

                            if(ImGui::MenuItem("sign")) ImGui::SetClipboardText("sign");
                            ImGui::SetItemTooltip("Returns sign of input. N<0 => -1, 0 => 0, N>0 => 1");

                            ImGui::EndMenu();
                        }

                        if(ImGui::BeginMenu("Statistics"))
                        {
                            if(ImGui::MenuItem("mean(")) ImGui::SetClipboardText("mean(");
                            ImGui::SetItemTooltip("Averages inputs, takes multiple arguments.");

                            if(ImGui::MenuItem("median(")) ImGui::SetClipboardText("median(");
                            ImGui::SetItemTooltip("Evaluates all inputs and returns median, takes multiple arguments.");

                            if(ImGui::MenuItem("stdev(")) ImGui::SetClipboardText("stdev(");
                            ImGui::SetItemTooltip("First argument is expected, rest is deviation, takes multiple arguments.");

                            if(ImGui::MenuItem("gcf(")) ImGui::SetClipboardText("gcf(");
                            ImGui::SetItemTooltip("Calculates the greatest common factor, takes multiple arguments.\nWill cause extreme output for numbers with many decimal places.");

                            if(ImGui::MenuItem("lcm(")) ImGui::SetClipboardText("lcm(");
                            ImGui::SetItemTooltip("Calculates the lowest common multiple, takes multiple arguments.\nWill cause extreme output for numbers with many decimal places.");

                            if(ImGui::MenuItem("min(")) ImGui::SetClipboardText("min(");
                            ImGui::SetItemTooltip("Evaluates all inputs and returns lowest, takes multiple arguments.");

                            if(ImGui::MenuItem("max(")) ImGui::SetClipboardText("max(");
                            ImGui::SetItemTooltip("Evaluates all inputs and returns greatest, takes multiple arguments.");

                            ImGui::EndMenu();
                        }

                        if(ImGui::BeginMenu("Fun and Misc."))
                        {
                            if(ImGui::MenuItem("rndint(")) ImGui::SetClipboardText("rndint(");
                            ImGui::SetItemTooltip("Takes a lower bound and upper bound, returns a random integer in range.");

                            if(ImGui::MenuItem("rndsel(")) ImGui::SetClipboardText("rndsel(");
                            ImGui::SetItemTooltip("Evaluates all inputs and returns a random one, takes multiple arguments.");

                            if(ImGui::MenuItem("derive(")) ImGui::SetClipboardText("derive(");
                            ImGui::SetItemTooltip("Approximates a derivative. Takes one argument.");

                            if(ImGui::MenuItem("smax(")) ImGui::SetClipboardText("smax(");
                            ImGui::SetItemTooltip("Smooth Maximum for 2 functions. Takes three arguments.\nArgument 1 is f(x), argument 2 g(x), argument 3 λ (blending)");

                            if(ImGui::MenuItem("smin(")) ImGui::SetClipboardText("smin(");
                            ImGui::SetItemTooltip("Smooth Minimum for 2 functions. Takes three arguments.\nArgument 1 is f(x), argument 2 g(x), argument 3 λ (blending)");

                            if(ImGui::MenuItem("sat")) ImGui::SetClipboardText("sat");
                            ImGui::SetItemTooltip("Linear transition between 0 and 1.");
                            
                            if(ImGui::MenuItem("sstep")) ImGui::SetClipboardText("sstep");
                            ImGui::SetItemTooltip("Smooth transition between 0 and 1.");

                            if(ImGui::MenuItem("sinc")) ImGui::SetClipboardText("sinc");
                            ImGui::SetItemTooltip("sinc(x) = sin(x)/x /; x ≠ 0, sinc(0) = 1.");

                            if(ImGui::MenuItem("exp")) ImGui::SetClipboardText("exp");
                            ImGui::SetItemTooltip("exp(x) = e^x");

                            if(ImGui::MenuItem("mix(")) ImGui::SetClipboardText("mix(");
                            ImGui::SetItemTooltip("Mixes args 1, 2, by a function between 0 and 1.");

                            if(ImGui::MenuItem("ReLU")) ImGui::SetClipboardText("ReLU");
                            ImGui::SetItemTooltip("if(f(x)>0), ReLU(f(x))=f(x), else ReLU(x)=0)");

                            if(ImGui::MenuItem("fish")) ImGui::SetClipboardText("fish.");
                            ImGui::SetItemTooltip("fishifies your equation.");

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenu();
                    }
                    if(ImGui::BeginMenu("Comparisons and Logicals"))
                    {
                        ImGui::MenuItem("false = 0, true = 1, if N≠0 => true",NULL,false,false);

                        if(ImGui::MenuItem("<")) ImGui::SetClipboardText("<");
                        ImGui::SetItemTooltip("Returns true if left less then right.");
                       
                        if(ImGui::MenuItem(">")) ImGui::SetClipboardText(">");
                        ImGui::SetItemTooltip("Returns true if left greater then right."); 

                        if(ImGui::MenuItem("≤")) ImGui::SetClipboardText("≤");
                        ImGui::SetItemTooltip("Returns true if left less than or equal to right. (<=)");
                        
                        if(ImGui::MenuItem("≥")) ImGui::SetClipboardText("≥");
                        ImGui::SetItemTooltip("Returns true if left greater than or equal to right. (>=)");

                        if(ImGui::MenuItem("=")) ImGui::SetClipboardText("=");
                        ImGui::SetItemTooltip("Returns true if left is exactly equal to right.");

                        if(ImGui::MenuItem("≠")) ImGui::SetClipboardText("≠");
                        ImGui::SetItemTooltip("Returns true if left not equal to right. (=!)");

                        if(ImGui::MenuItem("≈")) ImGui::SetClipboardText("≈");
                        ImGui::SetItemTooltip("Returns true if left close to equal to right. (AROUND)\nCan be configured in options.\nUseful in graphing intersections, zero points, so on.");

                        if(ImGui::MenuItem("∨")) ImGui::SetClipboardText("∨");
                        ImGui::SetItemTooltip("Logical or, returns true if either side is true. (OR)");

                        if(ImGui::MenuItem("∧")) ImGui::SetClipboardText("∧");
                        ImGui::SetItemTooltip("Logical and, returns true if either side is true. (AND)");

                        if(ImGui::MenuItem("⊕")) ImGui::SetClipboardText("⊕");
                        ImGui::SetItemTooltip("Logical exclusive or, returns true if either side is true. (XOR)");

                        ImGui::EndMenu();
                    }
                    if(ImGui::BeginMenu("Instances and Scripting"))
                    {
                        if(ImGui::BeginMenu("Instances"))
                        {
                            ImGui::Text("An instance holds its own variables and macros.\nIt is recommended to run a script in its own instance.");
                            ImGui::EndMenu();
                        }
                        if(ImGui::BeginMenu("Scripting"))
                        {
                            ImGui::Text("Scripting is a way to automate certain processes.");
                            if(ImGui::BeginMenu("Commands"))
                            {
                                if(ImGui::BeginMenu("let"))
                                {
                                    ImGui::Text("Let an expression be saved to a variable.\nMultiple assignments can be on one line.");
                                    ImGui::EndMenu();
                                }
                                if(ImGui::BeginMenu("set"))
                                {
                                    ImGui::Text("Set a macro. Only one assignment per line.");
                                    ImGui::EndMenu();
                                }

                                if(ImGui::BeginMenu("graph"))
                                {
                                    ImGui::Text("Add an equation to be graphed.\nUse numof before a variable to use its value.");
                                    ImGui::EndMenu();
                                }

                                if(ImGui::BeginMenu("if"))
                                {
                                    ImGui::Text("Run a block of commands only if an expression following 'if' is true.\nEnd block with endif, no nested if statements are supported.");
                                    ImGui::EndMenu();
                                }

                                if(ImGui::BeginMenu("jump"))
                                {
                                    ImGui::Text("Jump to any line in the script. Nothing bad will ever be caused by this statement.");
                                    ImGui::EndMenu();
                                }

                                ImGui::EndMenu();
                            }
                            if(ImGui::BeginMenu("Syntax"))
                            {
                                ImGui::Text("Scripts are almost like regular input line by line, except with extra commands.\nIt does not care about whitespace other than newlines.\nIdentifier syntax is as follows:\nlet/set name = value");
                                ImGui::EndMenu();
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenu();
                    }
                    if(ImGui::BeginMenu("Credits"))
                    {   
                        ImGui::Text("Glued together by Dummigame.\nAlso responsible for the calculator behind this n stuff.\nI am not that good at math btw. If this thing tells you 2+2=5, call me.\n\nLibraries used in this project:\nImGui, ImPlot, Boost lib\n(MIT License, https://github.com/ocornut/imgui,\nMIT License, https://github.com/epezent/implot,\nBoost Software License, https://www.boost.org/LICENSE_1_0.txt)");

                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                ImGui::SetItemTooltip("Learn about the calculator.");

                ImGui::EndMenuBar();
            }

            if(instances.size()>1)
            {
                if(selectedInstance!=0)
                {
                    ImGui::Text("You are using instance %s.",instances.at(selectedInstance).name.c_str());
                }
                else ImGui::Text("You are using the main instance.");
            }
            
            ImGui::Text("Enter your equation:");               
            ImGui::SameLine();
            
            ImGui::SetNextItemWidth(io.DisplaySize.x/3);
            ImGui::InputText(" ",&equation);          // Call Lesset
            if(equation!="") nonEmptyEquation=equation;
            
            std::string resultPlusEquals;
            if(equation!="") lastNonEmptyEquation=equation;
            else resultPlusEquals="";
            ImGui::SameLine(io.DisplaySize.x/3+150);
            resultPlusEquals="  =  " + result;

            if(equation=="fps")
            {
                resultPlusEquals = " = " + std::to_string(static_cast<int>(io.Framerate));
            }

            if(ImGui::Button(resultPlusEquals.c_str()))
            {
                result="";
                if(equation!="")
                {
                    lessetB::mainLoop(options,true,false,nonEmptyEquation,resultHistory,result,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);
                    options.ans=result;
                }
                
            }

            if(graphsEquations.size()<MANYGRAPHS) ImGui::Text("");




            const ImVec2 graphsPlotSize{io.DisplaySize.x-17,io.DisplaySize.y-105};

            if(graphsEquations.size()>=MANYGRAPHS)
            {
                if(ImGui::Checkbox("Draw many graphs",&drawManyGraphs))
                {
                    recalculateGraphs=true;
                }
            }

            if(ImPlot::BeginPlot("Graphs",graphsPlotSize)) 
            {
                ImPlot::SetupLegend(ImPlotLocation_NorthWest,ImPlotLegendFlags_NoButtons|ImPlotLegendFlags_Horizontal);
                ImPlotRect prevLimits {limits.X.Min, limits.X.Max, limits.Y.Min, limits.Y.Max};
                limits = ImPlot::GetPlotLimits();

                bool skipRestOfFrame{};
                if(graphsEquations.size()!=0 || (previewGraph))
                {

                    for(int i{}; i<graphsEquations.size();i++)
                    {
                        std::string graphEquationExpandedMacros = graphsEquations.at(i);
                        replaceAliases(graphEquationExpandedMacros, instances.at(selectedInstance));
                        if(graphEquationExpandedMacros.find('x')==std::string::npos)
                        {
                            graphsEquations.erase(graphsEquations.begin()+i);
                            i--;
                            recalculateGraphs=true;
                            skipRestOfFrame=true;
                        }
                    }


                    if(skipRestOfFrame)
                    {
                        ImPlot::EndPlot();
                        ImGui::End();
                        // Rendering
                        ImGui::Render();        ImDrawData* main_draw_data = ImGui::GetDrawData();        const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);        wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;        wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;        wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;        wd->ClearValue.color.float32[3] = clear_color.w;        if (!main_is_minimized)FrameRender(wd, main_draw_data);
                        // Update and Render additional Platform Windows
                        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable && false)        {            ImGui::UpdatePlatformWindows();            ImGui::RenderPlatformWindowsDefault();        }
                        // Present Main Platform Window
                        if (!main_is_minimized) FramePresent(wd);

                        continue;
                    }


                    float precisionDivisor=glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate/io.Framerate; // Lower precision to improve framerate for expensive graphs.
                    if(precisionDivisor<1) precisionDivisor=1;
                    if(precisionDivisor>1.25)precisionDivisor+=precisionDivisor;
                    size_t j{};
                    pastPrecisionDivisors[j]=precisionDivisor;
                    j++;
                    if(j>=100) j=0;
                    float averagedPrecisionDivisor{};
                    size_t precisionDivisorsCollected{};
                    for(size_t i{}; i<100 && pastPrecisionDivisors[i]!=0; i++)
                    {
                        precisionDivisorsCollected++;
                        averagedPrecisionDivisor+=pastPrecisionDivisors[i];
                    }
                    averagedPrecisionDivisor/=precisionDivisorsCollected;
                    averagedPrecisionDivisor=round(averagedPrecisionDivisor);
                    if(averagedPrecisionDivisor>20) averagedPrecisionDivisor=20;
                    if(averagedPrecisionDivisor<4) averagedPrecisionDivisor=4;
                    averagedPrecisionDivisor*=graphsEquations.size()/4.f+1;
                    averagedPrecisionDivisor*=maxIndividualGraphPointsMultiplier/4.0;

                    double limitsRatio {abs((limits.X.Max-limits.X.Min)/(limits.Y.Max-limits.Y.Min))};

                    if(!(prevLimits.X.Min == limits.X.Min && prevLimits.X.Max == limits.X.Max))
                    {
                        timeStationary=0;
                    }
                    else timeStationary++;

                    const bool zoomedIn{abs(limits.X.Max-limits.X.Min)<abs(prevLimits.X.Max-prevLimits.X.Min)-0.000000001f};
                    const bool zoomedOut{abs(limits.X.Max-limits.X.Min)-0.000000001f>abs(prevLimits.X.Max-prevLimits.X.Min)};
                    size_t downsizeGraphIndex{static_cast<size_t>(-1)};
                    for(size_t j{}; j<graphsPoints.first.size(); j++)
                    {
                        if(graphsPoints.first.at(j).size()>32000/graphsEquations.size()*maxIndividualGraphPointsMultiplier && graphsPoints.first.at(j).size()>2000/graphsEquations.size()/2*maxIndividualGraphPointsMultiplier)
                        {
                            downsizeGraphIndex=j;
                            break;
                        }
                    }

                    if(downsizeGraphIndex!=static_cast<size_t>(-1) && graphsEquations.size()>downsizeGraphIndex && downsizeGraphIndex<graphsPoints.first.size() && graphsPoints.first.size()<MANYGRAPHS && timeStationary<HIGHPRECISIONDRAWDELAY) // Recalculate a graph when too many points have been saved for it. Only one graph per frame being recalculated is intentional.
                    {
                        nonEmptyGraphEquation=graphsEquations.at(downsizeGraphIndex);
                        lessetB::Options graphOptions{true,
                                                    limits.X.Min,
                                                    limits.X.Max,
                                                    abs(limits.X.Max-limits.X.Min)/(maxIndividualGraphPoints/averagedPrecisionDivisor),
                                                    static_cast<size_t>(aroundTruthinessLeniency),
                                                    interpolateDiscontinuities,
                                                    followImplicitMultiplicationPriorityConvention};
                        lessetB::mainLoop(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);

                        graphsPoints.first.at(downsizeGraphIndex)=lessetB::globals::points.first;
                        graphsPoints.second.at(downsizeGraphIndex)=lessetB::globals::points.second;       
                    }

                    // Calculate and draw the live graph

                    double minimumPrecision{(graphsEquations.size()/2.f+1)};
                    if(minimumPrecision>20) minimumPrecision=20;

                    if(previewGraph && graphEquation.size()>0)
                    {
                        bool hasX{};
                        for(int i{}; i<graphEquation.length(); i++)
                        {
                            if(i==1 && graphEquation.at(1)=='x' && graphEquation.find("exp",0)!=0) hasX=true;
                            if(i>1&&graphEquation.at(i)=='x' && graphEquation.find("max",i-2)!=i-2 && graphEquation.find("exp",i-1)!=i-1) hasX=true;
                        }
                        if(graphEquation.at(0)=='x') hasX=true;

                        if(hasX)
                        {                        
                            ImPlotSpec spec{};
                            spec.LineWeight=2.f;
                            std::string previewNonEmptyGraphEquation=graphEquation;

                            if(updatePreviewGraph || !(prevLimits.X.Min == limits.X.Min && prevLimits.X.Max == limits.X.Max))
                            {
                                lessetB::Options graphOptions{true,limits.X.Min,limits.X.Max,abs(limits.X.Max-limits.X.Min)/maxIndividualGraphPoints*minimumPrecision*averagedPrecisionDivisor,static_cast<size_t>(aroundTruthinessLeniency),interpolateDiscontinuities,followImplicitMultiplicationPriorityConvention};
                                lessetB::mainLoop(graphOptions,true,false,previewNonEmptyGraphEquation,nothing,nothing,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);
                                liveGraphPoints.first=lessetB::globals::points.first;
                                liveGraphPoints.second=lessetB::globals::points.second;
                            }


                            for(size_t i{1}; i<liveGraphPoints.first.size()-2; i++)
                            {
                                const double previousDifference = (liveGraphPoints.second.at(i)-liveGraphPoints.second.at(i-1))/(liveGraphPoints.first.at(i)-liveGraphPoints.first.at(i-1));
                                const double difference = (liveGraphPoints.second.at(i+1)-liveGraphPoints.second.at(i))/(liveGraphPoints.first.at(i+1)-liveGraphPoints.first.at(i));
                                const double nextDifference = (liveGraphPoints.second.at(i+2)-liveGraphPoints.second.at(i+1))/(liveGraphPoints.first.at(i+2)-liveGraphPoints.first.at(i+1));

                                if(abs(difference-previousDifference)>abs(limits.Y.Max-limits.Y.Min)*30/abs(limits.X.Max-limits.X.Min) && !interpolateDiscontinuities)
                                {
                                    if(maxIndividualGraphPointsMultiplier!=5)
                                    {
                                        if(!isNoisy(liveGraphPoints.first,liveGraphPoints.second,i,maxIndividualGraphPointsMultiplier))
                                        {
                                            liveGraphPoints.second.at(i)=NAN;
                                            i++;
                                        }
                                    }
                                    else 
                                    {
                                        if(!isNoisy(liveGraphPoints.first,liveGraphPoints.second,i,1))
                                        {
                                            liveGraphPoints.second.at(i)=NAN;
                                            i++;
                                        }
                                    }
                                }
                            }
                            ImPlot::PlotLine("Preview", &(*liveGraphPoints.first.cbegin()), &(*liveGraphPoints.second.cbegin()), liveGraphPoints.first.size()-1,spec);
                        

                            

                            bool textAbove{};
                            float xPreviousPointMarked{-INFINITY};
                            if(markSpecialPoints)
                                for(size_t i{1}; i<liveGraphPoints.first.size()-1; i++)
                                {
                                    if(abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i)) < abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i+1)) &&
                                       abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i)) < abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i-1)))
                                    {
                                        ImPlotSpec spec{};
                                        spec.MarkerLineColor=ImVec4{0,0,0,1};
                                        spec.MarkerFillColor=ImVec4(1,1,1,1);
                                        ImPlot::PlotScatter("##", &liveGraphPoints.second.at(i), 1, 0,liveGraphPoints.first.at(i),spec);
                                        std::string coordsFormatted= "x: " + std::to_string(liveGraphPoints.first.at(i))+ "\ny: " + std::to_string(liveGraphPoints.second.at(i));
                                        ImPlot::PlotText(coordsFormatted.c_str(),liveGraphPoints.first.at(i),liveGraphPoints.second.at(i),ImVec2(80,20));
                                        
                                    }

                                    if(((liveGraphPoints.second.at(i)<liveGraphPoints.second.at(i+1) && liveGraphPoints.second.at(i)<liveGraphPoints.second.at(i-1)) ||
                                        (liveGraphPoints.second.at(i)>liveGraphPoints.second.at(i+1) && liveGraphPoints.second.at(i)>liveGraphPoints.second.at(i-1))) &&
                                    (abs(liveGraphPoints.first.at(i)-xPreviousPointMarked)>abs(limits.X.Max-limits.X.Min)/50))
                                    {
                                        xPreviousPointMarked=liveGraphPoints.first.at(i);
                                        ImPlotSpec spec{};
                                        spec.MarkerLineColor=ImVec4{0,0,0,1};
                                        spec.MarkerFillColor=ImVec4(1,1,1,1);
                                        ImPlot::PlotScatter("##", &liveGraphPoints.second.at(i), 1, 0,liveGraphPoints.first.at(i),spec);
                                        
                                        if(abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i))<(limits.X.Max-limits.X.Min)/10 && abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i))>(limits.X.Max-limits.X.Min)/100)
                                        {
                                            if(textAbove) textAbove=false;
                                            else textAbove=true;
                                            std::string coordsFormatted = std::to_string(liveGraphPoints.first.at(i)).substr(0,std::to_string(liveGraphPoints.first.at(i)).size()-TRIMMEDDECIMALPLACES)+ '\n' + std::to_string(liveGraphPoints.second.at(i)).substr(0,std::to_string(liveGraphPoints.first.at(i)).size()-TRIMMEDDECIMALPLACES);
                                            ImPlot::PlotText(coordsFormatted.c_str(),liveGraphPoints.first.at(i),liveGraphPoints.second.at(i),ImVec2(0,60-textAbove*120));
                                        }
                                    }
                                    else if(((liveGraphPoints.second.at(i)>0 && liveGraphPoints.second.at(i+1)<=0) ||
                                            (liveGraphPoints.second.at(i)<0 && liveGraphPoints.second.at(i+1)>=0)) &&
                                            (abs(liveGraphPoints.first.at(i)-xPreviousPointMarked)>abs(limits.X.Max-limits.X.Min)/50))
                                    {
                                        xPreviousPointMarked=liveGraphPoints.first.at(i);
                                        ImPlotSpec spec{};
                                        spec.MarkerLineColor=ImVec4{0,0,0,1};
                                        spec.MarkerFillColor=ImVec4(1,1,1,1);
                                        const float zero=0;
                                        ImPlot::PlotScatter("##", &zero, 1, 0,liveGraphPoints.first.at(i),spec);
                                        
                                        if(abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i))<(limits.X.Max-limits.X.Min)/10 && abs(ImPlot::GetPlotMousePos().x-liveGraphPoints.first.at(i))>(limits.X.Max-limits.X.Min)/100)
                                        {
                                            if(textAbove) textAbove=false;
                                            else textAbove=true;
                                            std::string coordsFormatted = std::to_string(liveGraphPoints.first.at(i)).substr(0,std::to_string(liveGraphPoints.first.at(i)).size()-TRIMMEDDECIMALPLACES)+ "\n0";
                                            ImPlot::PlotText(coordsFormatted.c_str(),liveGraphPoints.first.at(i),0,ImVec2(0,60-textAbove*120));
                                        }                                        
                                    }
                                }   
                        
                        }
                    }



                    if(timeStationary==0 && graphsPoints.first.size()<MANYGRAPHS && !zoomedIn) // Prepend/Append to graph
                    {

                        const double dXMin{limits.X.Min-prevLimits.X.Min};
                        const double dXMinScreenProportion{abs(dXMin)/(limits.X.Max-limits.X.Min)};

                        const double dXMax{limits.X.Max-prevLimits.X.Max};
                        const double dXMaxScreenProportion{abs(dXMax)/(limits.X.Max-limits.X.Min)};

                        if(dXMin<0)
                            for(size_t j{}; j<graphsEquations.size() && graphsPoints.first.at(j).at(0)>limits.X.Min; j++)
                            {
                                nonEmptyGraphEquation=graphsEquations.at(j);

                                lessetB::Options graphOptions{true,
                                                            limits.X.Min,
                                                            prevLimits.X.Min,
                                                            abs(dXMin)/(maxIndividualGraphPoints*dXMinScreenProportion/averagedPrecisionDivisor),
                                                            static_cast<size_t>(aroundTruthinessLeniency),
                                                            interpolateDiscontinuities,
                                                            followImplicitMultiplicationPriorityConvention};
                                lessetB::mainLoop(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);

                                graphsPoints.first.at(j).insert_range(graphsPoints.first.at(j).begin(),lessetB::globals::points.first);
                                graphsPoints.second.at(j).insert_range(graphsPoints.second.at(j).begin(),lessetB::globals::points.second);
                            }

                        if(dXMax>0)
                            for(size_t j{}; j<graphsEquations.size() && graphsPoints.first.at(j).at(graphsPoints.first.at(j).size()-1)<limits.X.Max; j++)
                            {
                                nonEmptyGraphEquation=graphsEquations.at(j);

                                lessetB::Options graphOptions{true,
                                                            prevLimits.X.Max,
                                                            limits.X.Max,
                                                            abs(dXMax)/(maxIndividualGraphPoints/averagedPrecisionDivisor*dXMaxScreenProportion),
                                                            static_cast<size_t>(aroundTruthinessLeniency),
                                                            interpolateDiscontinuities,
                                                            followImplicitMultiplicationPriorityConvention};
                                lessetB::mainLoop(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);

                                graphsPoints.first.at(j).append_range(lessetB::globals::points.first);
                                graphsPoints.second.at(j).append_range(lessetB::globals::points.second);
                            }

                    }


                    if(timeStationary==0 && graphsPoints.first.size()<MANYGRAPHS && zoomedIn) // Recalculate when zooming in
                    {
                        graphsPoints.first.clear();
                        graphsPoints.second.clear();
                        for(size_t j{}; 
                            j<graphsEquations.size() && 
                            !(prevLimits.X.Min == limits.X.Min && prevLimits.X.Max == limits.X.Max); 
                            j++)
                        {
                            nonEmptyGraphEquation=graphsEquations.at(j);

                            lessetB::Options graphOptions{true,
                                                        limits.X.Min,
                                                        limits.X.Max,
                                                        abs(limits.X.Max-limits.X.Min)/(maxIndividualGraphPoints/averagedPrecisionDivisor),
                                                        static_cast<size_t>(aroundTruthinessLeniency),
                                                        interpolateDiscontinuities,
                                                        followImplicitMultiplicationPriorityConvention};
                            lessetB::mainLoop(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);

                            graphsPoints.first.emplace_back(lessetB::globals::points.first);
                            graphsPoints.second.emplace_back(lessetB::globals::points.second);
                            graphsPoints.first.at(graphsPoints.first.size()-1).reserve(maxIndividualGraphPoints);
                            graphsPoints.second.at(graphsPoints.second.size()-1).reserve(maxIndividualGraphPoints); 
                        }
                    }
                

                    if(timeStationary==HIGHPRECISIONDRAWDELAY || recalculateGraphs) // Recalculate at a high precision
                    {
                        recalculateGraphs=false;
                        if(graphsEquations.size()>=MANYGRAPHS && drawManyGraphs || graphsEquations.size()<MANYGRAPHS)
                        {
                            graphsPoints.first.clear();
                            graphsPoints.second.clear();
                            for(size_t j{}; j<graphsEquations.size(); j++)
                            {
                                nonEmptyGraphEquation=graphsEquations.at(j);
                                lessetB::Options graphOptions{true,limits.X.Min,limits.X.Max,abs(limits.X.Max-limits.X.Min)/maxIndividualGraphPoints*minimumPrecision,static_cast<size_t>(aroundTruthinessLeniency),interpolateDiscontinuities,followImplicitMultiplicationPriorityConvention};
                                lessetB::mainLoop(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances.at(selectedInstance).userVariables,instances.at(selectedInstance).userAliases,false);

                                graphsPoints.first.emplace_back(lessetB::globals::points.first);
                                graphsPoints.second.emplace_back(lessetB::globals::points.second);
                                graphsPoints.first.at(graphsPoints.first.size()-1).reserve(maxIndividualGraphPoints);
                                graphsPoints.second.at(graphsPoints.second.size()-1).reserve(maxIndividualGraphPoints);
                            }
                        }
                    } 
                
                    for(size_t j{}; j<graphsPoints.first.size(); j++)
                    {
                        
                        if(graphsEquations.size()>=MANYGRAPHS && timeStationary<HIGHPRECISIONDRAWDELAY || graphsEquations.size()>=MANYGRAPHS && !drawManyGraphs) break;
                        
                        if(graphsEquations.size()<MANYGRAPHS && !interpolateDiscontinuities)
                        {
                            for(size_t i{}; i<graphsPoints.first.at(j).size(); i++)
                            {
                                if(i<graphsPoints.first.at(j).size()-1)
                                {
                                    if(graphsPoints.first.at(j).at(i)>graphsPoints.first.at(j).at(i+1))
                                    {
                                        graphsPoints.second.at(j).erase(graphsPoints.second.at(j).begin()+i);
                                        graphsPoints.first.at(j).erase(graphsPoints.first.at(j).begin()+i);
                                    }
                                }
                                if(i>0 && i<graphsPoints.first.at(j).size()-2)
                                {
                                    const float previousDifference = (graphsPoints.second.at(j).at(i)-graphsPoints.second.at(j).at(i-1))/(graphsPoints.first.at(j).at(i)-graphsPoints.first.at(j).at(i-1));
                                    const float difference = (graphsPoints.second.at(j).at(i+1)-graphsPoints.second.at(j).at(i))/(graphsPoints.first.at(j).at(i+1)-graphsPoints.first.at(j).at(i));
                                    const float nextDifference = (graphsPoints.second.at(j).at(i+2)-graphsPoints.second.at(j).at(i+1))/(graphsPoints.first.at(j).at(i+2)-graphsPoints.first.at(j).at(i+1));

                                    if(abs(difference-previousDifference)>abs(ImPlot::GetPlotLimits().Y.Max-ImPlot::GetPlotLimits().Y.Min)*30/abs(ImPlot::GetPlotLimits().X.Max-ImPlot::GetPlotLimits().X.Min) && !interpolateDiscontinuities)
                                    {
                                        if(!isNoisy(graphsPoints.first.at(j),graphsPoints.second.at(j),i,maxIndividualGraphPointsMultiplier))
                                        {
                                            graphsPoints.second.at(j).at(i)=NAN;
                                        }
                                    }
                                }                    
                            }
                        }

                        ImPlotSpec spec{};
                        spec.LineWeight=2.f;

                        if(timeStationary<HIGHPRECISIONDRAWDELAY) ImPlot::PlotLine(graphsEquations.at(j).c_str(), &(*graphsPoints.first.at(j).cbegin()), &(*graphsPoints.second.at(j).cbegin()), graphsPoints.second.at(j).size(),spec);
                        else if((drawManyGraphs && graphsEquations.size()>=MANYGRAPHS) || graphsEquations.size()<MANYGRAPHS) ImPlot::PlotLine(graphsEquations.at(j).c_str(), &(*graphsPoints.first.at(j).cbegin()), &(*graphsPoints.second.at(j).cbegin()), graphsPoints.second.at(j).size(),spec);
                    
                        bool textAbove{};
                        bool hasShownPoint{false};
                        double xPreviousPointMarked{-INFINITY};
                        size_t increment = 3;
                        if(timeStationary>=100) increment=9;
                        if(markSpecialPoints && graphsEquations.size()<15)
                            for(size_t i{increment}; i<graphsPoints.first.at(j).size()-20; i+=increment)
                            {
                                if(abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i)) < abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i+increment)) &&
                                abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i)) < abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i-increment)) &&
                                !hasShownPoint) 
                                {
                                    hasShownPoint=true;
                                    ImPlotSpec spec{};
                                    spec.MarkerLineColor=ImVec4{0,0,0,1};
                                    spec.MarkerFillColor=ImVec4(1,1,1,1);
                                    ImPlot::PlotScatter("##", &graphsPoints.second.at(j).at(i), 1, 0,graphsPoints.first.at(j).at(i),spec);
                                    std::string coordsFormatted= "x: " + std::to_string(graphsPoints.first.at(j).at(i))+ "\ny: " + std::to_string(graphsPoints.second.at(j).at(i));
                                    ImPlot::PlotText(coordsFormatted.c_str(),graphsPoints.first.at(j).at(i),graphsPoints.second.at(j).at(i),ImVec2(80,20));
                                    
                                }

                                if(((graphsPoints.second.at(j).at(i)<graphsPoints.second.at(j).at(i+increment) && graphsPoints.second.at(j).at(i)<graphsPoints.second.at(j).at(i-increment)) ||
                                    (graphsPoints.second.at(j).at(i)>graphsPoints.second.at(j).at(i+increment) && graphsPoints.second.at(j).at(i)>graphsPoints.second.at(j).at(i-increment))) &&
                                   (abs(graphsPoints.first.at(j).at(i)-xPreviousPointMarked)>abs(limits.X.Max-limits.X.Min)/50))
                                {
                                    xPreviousPointMarked=graphsPoints.first.at(j).at(i);
                                    ImPlotSpec spec{};
                                    spec.MarkerLineColor=ImVec4{0,0,0,1};
                                    spec.MarkerFillColor=ImVec4(1,1,1,1);
                                    ImPlot::PlotScatter("##", &graphsPoints.second.at(j).at(i), 1, 0,graphsPoints.first.at(j).at(i),spec);
                                    
                                    if(abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i))<(limits.X.Max-limits.X.Min)/10 && abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i))>(limits.X.Max-limits.X.Min)/100)
                                    {
                                        if(textAbove) textAbove=false;
                                        else textAbove=true;
                                        std::string coordsFormatted = std::to_string(graphsPoints.first.at(j).at(i)).substr(0,std::to_string(graphsPoints.first.at(j).at(i)).size()-TRIMMEDDECIMALPLACES)+ '\n' + std::to_string(graphsPoints.second.at(j).at(i)).substr(0,std::to_string(graphsPoints.first.at(j).at(i)).size()-TRIMMEDDECIMALPLACES);
                                        ImPlot::PlotText(coordsFormatted.c_str(),graphsPoints.first.at(j).at(i),graphsPoints.second.at(j).at(i),ImVec2(0,60-textAbove*120));
                                    }
                                }
                                else if(((graphsPoints.second.at(j).at(i)>0 && graphsPoints.second.at(j).at(i+increment)<=0) ||
                                         (graphsPoints.second.at(j).at(i)<0 && graphsPoints.second.at(j).at(i+increment)>=0)) &&
                                        (abs(graphsPoints.first.at(j).at(i)-xPreviousPointMarked)>abs(limits.X.Max-limits.X.Min)/50))
                                {
                                    xPreviousPointMarked=graphsPoints.first.at(j).at(i);
                                    ImPlotSpec spec{};
                                    spec.MarkerLineColor=ImVec4{0,0,0,1};
                                    spec.MarkerFillColor=ImVec4(1,1,1,1);
                                    const float zero=0;
                                    ImPlot::PlotScatter("##", &zero, 1, 0,graphsPoints.first.at(j).at(i),spec);
                                    
                                    if(abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i))<(limits.X.Max-limits.X.Min)/10 && abs(ImPlot::GetPlotMousePos().x-graphsPoints.first.at(j).at(i))>(limits.X.Max-limits.X.Min)/100)
                                    {
                                        if(textAbove) textAbove=false;
                                        else textAbove=true;
                                        std::string coordsFormatted = std::to_string(graphsPoints.first.at(j).at(i)).substr(0,std::to_string(graphsPoints.first.at(j).at(i)).size()-TRIMMEDDECIMALPLACES)+ "\n0";
                                        ImPlot::PlotText(coordsFormatted.c_str(),graphsPoints.first.at(j).at(i),0,ImVec2(0,60-textAbove*120));
                                    }                                        
                                }

                        }
                    
                    
                    }
                }
                

                ImPlot::EndPlot();
                // ImGui::Text("%f",io.Framerate);
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();        ImDrawData* main_draw_data = ImGui::GetDrawData();        const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);        wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;        wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;        wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;        wd->ClearValue.color.float32[3] = clear_color.w;        if (!main_is_minimized)FrameRender(wd, main_draw_data);
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable && false)        {            ImGui::UpdatePlatformWindows();            ImGui::RenderPlatformWindowsDefault();        }
        // Present Main Platform Window
        if (!main_is_minimized) FramePresent(wd);
    }
    // Cleanup
    err = vkDeviceWaitIdle(g_Device);    check_vk_result(err);    ImGui_ImplVulkan_Shutdown();    ImGui_ImplGlfw_Shutdown();    ImPlot::DestroyContext();    ImGui::DestroyContext();CleanupVulkanWindow(&g_MainWindowData);CleanupVulkan();glfwDestroyWindow(window);glfwTerminate();

    return 0;
}

bool isNoisy(const std::vector<double> &pointsX, const std::vector<double> &pointsY, size_t i, int maxIndividualGraphPointsMultiplier)
{
    int switches{};
    bool rising{};
    bool prevRising{};
    size_t j=i-5;
    {
        for(; j<i+5 && j<pointsX.size()-1; j++)
        {
            // if(j>0 && pointsX.at(j)-pointsX.at(j+1) != pointsX.at(j-1)-pointsX.at(j)) return true;
            if(pointsY.at(j)<pointsY.at(j+1))
            {
                rising=true;
                if(prevRising!=rising) switches++;
                if(j==i-5) switches--;
            }
            else if(pointsY.at(j)>pointsY.at(j+1))
            {
                rising=false;
                if(prevRising!=rising) switches++;
            }
            prevRising=rising;
        } 
    }

    if(switches<3) return false;

    return true; 
}


bool replaceAliases(std::string &equation, Instance &instance)
{
    if(instance.userAliases.size()==0) return false;
    for(size_t i{}; i<instance.userAliases.size(); i++)
    {
        for(int j{}; j<equation.length(); j++)
        {
            if(equation.find(instance.userAliases.at(i).name,j)==j)
            {
                if(j>=3 && equation.find("set",j-3)==j-3)
                {
                    break;
                }
                equation.erase(j,instance.userAliases.at(i).name.length());
                equation.insert(j,instance.userAliases.at(i).value);
                i=0;
                j=-1;
            }
        }
    }
    return false;
}