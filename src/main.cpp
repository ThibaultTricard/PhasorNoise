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

int main() {

  ErrorCheck::PrintError(true);

  Window w("Phasor Noise", 1000, 1000);

  ImGuiWrapper* gui = new ImGuiWrapper();

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
  gui->initGui(&w, queue, &commandBuffer);

  auto quadMesh = Geometry::generateQuad(true);
  VertexBuffer* quadBuffer = new VertexBuffer({ quadMesh });
  quadBuffer->allocate(queue, commandBuffer);

  GraphicPipeline* phasorPipeline = new GraphicPipeline(vec3f({ 0,0,0 }), vec3f({ float(size.width),float(size.height),1.0f }), vec2f({ 0,0 }), vec2f({ float(size.width),float(size.height) }));
  VertexShaderModule* quadRendererVert = new VertexShaderModule(prefix + "shaders/phasor.vert.spv");
  FragmentShaderModule* quadRendererFrag = new FragmentShaderModule(prefix + "shaders/phasor.frag.spv");

  UniformBuffer UBO;
  UBO.addVariable("tmp", int(1));
  UBO.end();


  phasorPipeline->setVertexModule(quadRendererVert);
  phasorPipeline->setFragmentModule(quadRendererFrag);
  phasorPipeline->setVerticesInfo(quadBuffer->getBindingDescriptions(), quadBuffer->getAttributeDescriptions(), quadBuffer->primitiveTopology());

  phasorPipeline->setVertices({ quadBuffer });
  phasorPipeline->SetCullMode(VK_CULL_MODE_NONE);
  //phasorPipeline->addUniformBuffer(&UBO, VK_SHADER_STAGE_FRAGMENT_BIT);

  SubpassAttachment SA;
  SA.showOnScreen = true;
  SA.nbColor = 1;
  SA.storeColor = true;
  SA.useDepth = true;
  SA.showOnScreenIndex = 0;

  RenderPass* phasorPass = new RenderPass();
  phasorPass->addSubPass({ phasorPipeline, gui->getPipeline() }, SA);
  phasorPass->compile();

  FrameBuffer frameBuffers(size.width, size.height);
  phasorPass->prepareOutputFrameBuffer(frameBuffers);

  while (w.running()) {
    w.updateInput();

    commandBuffer.wait();
    commandBuffer.resetFence();
    
    ImGui::NewFrame();

    gui->prepareGui(queue, commandBuffer);

    SwapChainImage& image = s->acquireImage();


    std::vector<WaitSemaphoreInfo> wait_semaphore_infos = {};
    wait_semaphore_infos.push_back({
      image.getSemaphore(),                     // VkSemaphore            Semaphore
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT          // VkPipelineStageFlags   WaitingStage
      });

    commandBuffer.resetFence();
    commandBuffer.beginRecord();
    UBO.update(commandBuffer);
    phasorPass->setSwapChainImage(frameBuffers, image);
    phasorPass->draw(commandBuffer, frameBuffers, vec2u({ 0,0 }), vec2u({ size.width, size.height }), { { 0.1f, 0.2f, 0.3f, 1.0f }, { 1.0f, 0 } });

    commandBuffer.endRecord();

    commandBuffer.submit(queue, wait_semaphore_infos, { commandBuffer.getSemaphore(0) });
    s->presentImage(presentQueue, image, { commandBuffer.getSemaphore(0) });
  }

  d->end();
}