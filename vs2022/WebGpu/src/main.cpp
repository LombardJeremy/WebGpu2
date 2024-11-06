#include "Header/Application.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <cassert>
#include <vector>

#define WEBGPU_BACKEND_WGPU

int main(int argc, char** argv) {
    Application app = Application();
    app.launch();
    std::string makanba;
    std::cin >> makanba;
    return 0;
}

int Application::launch() {

    // We create a descriptor
    desc = {};
    desc.nextInChain = nullptr;

    // We create the instance using this descriptor
    instance = wgpuCreateInstance(&desc);

    // We can check whether there is actually an instance created
    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }
    // Display the object (WGPUInstance is a simple pointer, it may be
    // copied around without worrying about its size).
    std::cout << "WGPU instance: " << instance << std::endl;


    std::cout << "Requesting adapter..." << std::endl;

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapter = requestAdapterSync(instance, &adapterOpts);

    std::cout << "Got adapter: " << adapter << std::endl;

    inspectAdapter(adapter);




    std::cout << "Requesting device..." << std::endl;

    WGPUDeviceDescriptor deviceDesc = {};
    // [...] Build device descriptor
    device = requestDeviceSync(adapter, &deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    // A function that is invoked whenever the device stops being available.
    deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
        };

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
        };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);


    WGPUQueue queue = wgpuDeviceGetQueue(device);
    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
        std::cout << "Queued work finished with status: " << status << std::endl;
        };
    wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);


    encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

    cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder); // release encoder after it's finished


    // Finally submit the command queue
    std::cout << "Submitting command..." << std::endl;
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command);

    std::cout << "Command submitted." << std::endl;


    for (int i = 0; i < 5; ++i) {
        std::cout << "Tick/Poll device..." << std::endl;
#if defined(WEBGPU_BACKEND_DAWN)
        wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
        emscripten_sleep(100);
#endif
    }


    //Glfw
    glfwInit();
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create the window
    window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

    //Surface Configuration
    surface = glfwGetWGPUSurface(instance, window);


    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    //Surface Configuration
    WGPUSurfaceConfiguration config = {};

    config.width = 640;
    config.height = 480;

    config.usage = WGPUTextureUsage_RenderAttachment;

    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
    config.format = surfaceFormat;
    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.device = device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &config);


    wgpuAdapterRelease(adapter);
    //Main LOOP
    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
        //MAINS LOOP CONTENT    
        // Get the next target texture view
        targetView = GetNextSurfaceTextureView();
        if (!targetView) return 0;



        //Render Pass ?
        renderPassDesc = {};
        renderPassDesc.nextInChain = nullptr;

        //Describe Render Pass
        renderPassColorAttachment = {};

        //Describe the atachment
        renderPassColorAttachment.view = targetView;

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

        renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        // Use Render Pass


        // At the end of the frame
        wgpuTextureViewRelease(targetView);
        wgpuSurfacePresent(surface);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    // We clean up the WebGPU instance

    wgpuRenderPassEncoderRelease(renderPass);
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuInstanceRelease(instance);
    wgpuSurfaceUnconfigure(surface);
    wgpuSurfaceRelease(surface);
    wgpuDeviceRelease(device);
    wgpuQueueRelease(queue);

    return 0;
}

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter Application::requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        }
        else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
        };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.adapter;
}

void Application::inspectAdapter(WGPUAdapter adapter) {
#ifndef __EMSCRIPTEN__
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
#else
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits);
#endif

    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    }
#endif // NOT __EMSCRIPTEN__

    std::vector<WGPUFeatureName> features;

    // Call the function a first time with a null return address, just to get
    // the entry count.
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

    // Allocate memory (could be a new, or a malloc() if this were a C program)
    features.resize(featureCount);

    // Call the function a second time, with a non-null return address
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec; // Restore decimal numbers

    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(adapter, &properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    if (properties.vendorName) {
        std::cout << " - vendorName: " << properties.vendorName << std::endl;
    }
    if (properties.architecture) {
        std::cout << " - architecture: " << properties.architecture << std::endl;
    }
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    if (properties.name) {
        std::cout << " - name: " << properties.name << std::endl;
    }
    if (properties.driverDescription) {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
    std::cout << " - backendType: 0x" << properties.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice Application::requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        }
        else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
        };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif // __EMSCRIPTEN__

    assert(userData.requestEnded);

    return userData.device;
}

// We also add an inspect device function:
void inspectDevice(WGPUDevice device) {
    std::vector<WGPUFeatureName> features;
    size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
    features.resize(featureCount);
    wgpuDeviceEnumerateFeatures(device, features.data());

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;

    WGPUSupportedLimits limits = {};
    limits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;
#else
    bool success = wgpuDeviceGetLimits(device, &limits);
#endif

    if (success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
    }
}

void onQueueWorkDone(WGPUQueueWorkDoneStatus status, void* /*pUserData*/) {
    std::cout << "Queued work finished with status: " << status << std::endl;
}

WGPUTextureView Application::GetNextSurfaceTextureView() {
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return nullptr;
    }
    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
#ifndef WEBGPU_BACKEND_WGPU
    // We no longer need the texture, only its view
    // (NB: with wgpu-native, surface textures must not be manually released)
    wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU
    return targetView;
}


