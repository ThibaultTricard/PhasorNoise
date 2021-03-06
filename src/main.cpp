#include "Framework/Framework.h"

using namespace LavaCake;
using namespace LavaCake::Geometry;
using namespace LavaCake::Framework;
using namespace LavaCake::Core;

#ifdef __APPLE__
std::string prefix = "../";
#else
std::string prefix = "";
#endif
ImGuiWrapper* gui;
UniformBuffer noiseParameter;
VertexBuffer* quadBuffer;

bool g_resize = false;

void window_size_callback(GLFWwindow* window, int width, int height)
{
  g_resize = true;
}

RenderPass* createRenderPass(Window& w, Queue* queue, CommandBuffer& commandBuffer) {

  SwapChain* s = SwapChain::getSwapChain();
  
  VkExtent2D size = s->size();
  gui->resizeGui(&w);

  GraphicPipeline* phasorPipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
  VertexShaderModule* quadRendererVert = new VertexShaderModule(prefix + "shaders/phasor.vert.spv");
  FragmentShaderModule* quadRendererFrag = new FragmentShaderModule(prefix + "shaders/phasor.frag.spv");

  phasorPipeline->setVertexModule(quadRendererVert);
  phasorPipeline->setFragmentModule(quadRendererFrag);
  phasorPipeline->setVerticesInfo(quadBuffer->getBindingDescriptions(), quadBuffer->getAttributeDescriptions(), quadBuffer->primitiveTopology());

  phasorPipeline->setVertices({ quadBuffer });
  phasorPipeline->SetCullMode(VK_CULL_MODE_NONE);

  phasorPipeline->addUniformBuffer(&noiseParameter, VK_SHADER_STAGE_FRAGMENT_BIT, 0);


  SubpassAttachment SA;
  SA.showOnScreen = true;
  SA.nbColor = 1;
  SA.storeColor = true;
  SA.useDepth = true;
  SA.showOnScreenIndex = 0;

  RenderPass* phasorPass = new RenderPass();
  phasorPass->addSubPass({ phasorPipeline, gui->getPipeline() }, SA);
  phasorPass->compile();

  return phasorPass;
}


int main() {

  ErrorCheck::PrintError(true);

  Window w("Phasor Noise", 1000, 1000);
  glfwSetWindowSizeCallback(w.m_window, window_size_callback);
  


  Device* d = Device::getDevice();
  d->initDevices(0, 1, w.m_windowParams, nullptr);
  auto dPhysical = d->getPhysicalDevice();

  SwapChain* s = SwapChain::getSwapChain();
  s->init();
  VkExtent2D size = s->size();
  Queue* queue = d->getGraphicQueue(0);
  PresentationQueue* presentQueue = d->getPresentQueue();
  CommandBuffer commandBuffer;
  commandBuffer.addSemaphore();
  
  gui = new ImGuiWrapper();

  gui->initGui(&w, queue, &commandBuffer);

  auto quadMesh = Geometry::generateQuad(true);
  quadBuffer = new VertexBuffer({ quadMesh });
  quadBuffer->allocate(queue, commandBuffer);

 

  
  float frequency = 50.0f;
  float bandwidth = 32.0f;
  float direction = 1.0f;
  int kernelPerCell = 16;
  int profile = 0;
  float pwmRatio = 0.2;
  float aspectRatio = size.width / size.height;


  noiseParameter.addVariable("_f", frequency);
  noiseParameter.addVariable("_b", bandwidth);
  noiseParameter.addVariable("_o", direction);
  noiseParameter.addVariable("_impPerKernel", kernelPerCell);
  noiseParameter.addVariable("_profile", profile);
  noiseParameter.addVariable("_pwmRatio", pwmRatio);
  noiseParameter.addVariable("_aspectRatio", aspectRatio);
  noiseParameter.end();

  RenderPass * phasorPass = createRenderPass(w, queue, commandBuffer);

  FrameBuffer* frameBuffer = new FrameBuffer(size.width, size.height);
  phasorPass->prepareOutputFrameBuffer(queue, commandBuffer ,*frameBuffer);

  const char* items[] = { "Complex view", "Complex view normalized", "Sinewave", "Sawtooth", "PWM"};
  

  while (w.running()) {
    w.updateInput();

    commandBuffer.wait();
    commandBuffer.resetFence();
    
    if (g_resize) {
      d->waitForAllCommands();
      s->resize();
      size = s->size();
      aspectRatio = size.width / size.height;
      delete phasorPass;
      phasorPass = createRenderPass(w, queue, commandBuffer);
      delete frameBuffer;
      frameBuffer = new FrameBuffer(size.width, size.height);
      phasorPass->prepareOutputFrameBuffer(queue, commandBuffer ,*frameBuffer);

      g_resize = false;
    }

    
    ImGui::NewFrame();

    ImGui::Begin("Parameters");
    ImGui::SliderFloat("Frequency", &frequency, 0.0f, 100.0f);
    ImGui::SliderFloat("Bandwidth", &bandwidth, 0.0f, 100.0f);
    ImGui::SliderFloat("Direction", &direction, 0.0f, 6.28318530718f);
    ImGui::SliderInt("Kernel Density", &kernelPerCell, 1, 32);

    if (ImGui::BeginCombo("Profile", items[profile])) 
    {
      for (int n = 0; n < IM_ARRAYSIZE(items); n++)
      {
        bool is_selected = (profile == n); 
        if (ImGui::Selectable(items[n], is_selected))
          profile = n;
        if (is_selected)
            ImGui::SetItemDefaultFocus();  
      }
      ImGui::EndCombo();
    }
    if (profile == 4) {
      ImGui::SliderFloat("Ratio", &pwmRatio, 0.0, 1.0);
    }

    ImGui::End();

    noiseParameter.setVariable("_f", frequency);
    noiseParameter.setVariable("_b", bandwidth);
    noiseParameter.setVariable("_o", direction);
    noiseParameter.setVariable("_impPerKernel", kernelPerCell); 
    noiseParameter.setVariable("_profile", profile);
    noiseParameter.setVariable("_pwmRatio", pwmRatio);
    noiseParameter.setVariable("_aspectRatio", aspectRatio);



    gui->prepareGui(queue, commandBuffer);

    SwapChainImage& image = s->acquireImage();


    std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
    wait_semaphore_infos.push_back({
      image.getSemaphore(),                     // VkSemaphore            Semaphore
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT          // VkPipelineStageFlags   WaitingStage
      });

    commandBuffer.resetFence();
    commandBuffer.beginRecord();
    noiseParameter.update(commandBuffer);

    phasorPass->setSwapChainImage(*frameBuffer, image);
    phasorPass->draw(commandBuffer, *frameBuffer, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

    commandBuffer.endRecord();

    commandBuffer.submit(queue, wait_semaphore_infos, { commandBuffer.getSemaphore(0) });
    s->presentImage(presentQueue, image, { commandBuffer.getSemaphore(0) });
  }

  d->waitForAllCommands();
}