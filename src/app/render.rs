use std::sync::Arc;

use std::time::Duration;
use vulkano::command_buffer::allocator::{
    StandardCommandBufferAllocator, StandardCommandBufferAllocatorCreateInfo,
};
use vulkano::command_buffer::CommandBufferExecFuture;
use vulkano::descriptor_set::allocator::{DescriptorSetAllocator, StandardDescriptorSetAllocator};
use vulkano::device::physical::{PhysicalDevice, PhysicalDeviceType};
use vulkano::device::{
    Device, DeviceCreateInfo, DeviceExtensions, Features, Queue, QueueCreateInfo, QueueFlags,
};
use vulkano::image::view::ImageView;
use vulkano::image::{AttachmentImage, ImageAccess, ImageUsage, SwapchainImage};
use vulkano::instance::Instance;
use vulkano::memory::allocator::{
    FreeListAllocator, GenericMemoryAllocator, StandardMemoryAllocator,
};
use vulkano::pipeline::graphics::viewport::Viewport;
use vulkano::pipeline::GraphicsPipeline;
use vulkano::render_pass::{Framebuffer, FramebufferCreateInfo, RenderPass};
use vulkano::swapchain::{
    self, AcquireError, SurfaceInfo, Swapchain, SwapchainAcquireFuture, SwapchainCreateInfo,
    SwapchainCreationError,
};
use vulkano::sync::GpuFuture;
use winit::dpi::PhysicalSize;

use super::MainWindow;

pub struct RendererContext {
    _phy_device: Arc<PhysicalDevice>,
    device: Arc<Device>,
    _graphics_queue_index: u32,
    graphics_queues: Vec<Arc<Queue>>,
    //compute_queue_index: u32,
    //compute_queues: Vec<Arc<Queue>>,
    render_pass: Arc<RenderPass>,
    swapchain: Arc<Swapchain>,
    images: Vec<Arc<SwapchainImage>>,
    framebuffers: Vec<Arc<Framebuffer>>,
    viewport: Viewport,

    mem_alloc: GenericMemoryAllocator<Arc<FreeListAllocator>>,
    cmd_buf_alloc: StandardCommandBufferAllocator,
    desc_set_alloc: StandardDescriptorSetAllocator,
}

pub trait Renderer {
    fn new(render_context: &RendererContext) -> Self;

    fn create_pipeline(&self, render_context: &RendererContext) -> Arc<GraphicsPipeline>;

    fn update_command_buffers(&mut self, render_context: &RendererContext);
    fn update_pipeline(&mut self, render_context: &RendererContext);

    //fn execute_cmd_buf(&self, future: Box<dyn GpuFuture>, image_i: u32) -> Box<dyn GpuFuture>;
    fn execute_cmd_buf<F>(&self, future: F, image_i: u32) -> CommandBufferExecFuture<F>
    where
        F: GpuFuture + 'static;
}

fn get_render_pass(device: Arc<Device>, swapchain: &Arc<Swapchain>) -> Arc<RenderPass> {
    vulkano::single_pass_renderpass!(
        device,
        attachments: {
            color: {
                load: DontCare,
                store: Store,
                format: swapchain.image_format(),
                samples: 1,
            },
        },
        pass:
            {
                color: [color],
                depth_stencil: {},
            },

    )
    .unwrap()
}

fn create_framebuffers(
    images: &[Arc<SwapchainImage>],
    render_pass: &Arc<RenderPass>,
    mem_alloc: &StandardMemoryAllocator,
) -> Vec<Arc<Framebuffer>> {
    images
        .iter()
        .map(|image| {
            let view = ImageView::new_default(image.clone()).unwrap();

            Framebuffer::new(
                render_pass.clone(),
                FramebufferCreateInfo {
                    attachments: vec![view],
                    ..Default::default()
                },
            )
            .unwrap()
        })
        .collect::<Vec<_>>()
}

impl RendererContext {
    pub fn new(instance: &Arc<Instance>, app_window: &MainWindow) -> Self {
        let device_ext = DeviceExtensions {
            khr_swapchain: true,
            ..DeviceExtensions::empty()
        };

        let (phy_dev, queue_family_index) = instance
            .enumerate_physical_devices()
            .expect("instance.enumerate_physical_devices() failed")
            .filter(|p| p.supported_extensions().contains(&device_ext))
            .filter_map(|p| {
                p.queue_family_properties()
                    .iter()
                    .enumerate()
                    .position(|(i, q)| {
                        q.queue_flags.contains(QueueFlags::GRAPHICS)
                            && p.surface_support(u32::try_from(i).unwrap(), &app_window.surface)
                                .unwrap_or(false)
                    })
                    .map(|q| (p, u32::try_from(q).unwrap()))
            })
            .min_by_key(|(p, _)| match p.properties().device_type {
                PhysicalDeviceType::DiscreteGpu => 0,
                PhysicalDeviceType::IntegratedGpu => 1,
                PhysicalDeviceType::VirtualGpu => 2,
                PhysicalDeviceType::Cpu => 3,
                _ => 4,
            })
            .expect("no compatible device found");

        let (device, mut queues) = Device::new(
            phy_dev.clone(),
            DeviceCreateInfo {
                enabled_extensions: device_ext,
                queue_create_infos: vec![QueueCreateInfo {
                    queue_family_index,
                    ..Default::default()
                }],
                enabled_features: Features {
                    wide_lines: true,
                    ..Default::default()
                },
                ..Default::default()
            },
        )
        .expect("Device::new failed");

        let queue = queues.next().unwrap();

        let (swapchain, images) = {
            let caps = phy_dev
                .surface_capabilities(&app_window.surface, SurfaceInfo::default())
                .expect("phy_dev.surface_capabilities failed");
            let dimensions = app_window.window.inner_size();
            let composite_alpha = caps.supported_composite_alpha.into_iter().next().unwrap();
            let image_format = Some(
                phy_dev
                    .surface_formats(&app_window.surface, SurfaceInfo::default())
                    .unwrap()[0]
                    .0,
            );
            Swapchain::new(
                device.clone(),
                app_window.surface.clone(),
                SwapchainCreateInfo {
                    min_image_count: caps.min_image_count,
                    image_format,
                    image_extent: dimensions.into(),
                    image_usage: ImageUsage::COLOR_ATTACHMENT,
                    composite_alpha,
                    ..Default::default()
                },
            )
            .unwrap()
        };

        let mem_alloc = StandardMemoryAllocator::new_default(device.clone());
        let cmd_buf_alloc = StandardCommandBufferAllocator::new(
            device.clone(),
            StandardCommandBufferAllocatorCreateInfo::default(),
        );
        let desc_set_alloc = StandardDescriptorSetAllocator::new(device.clone());

        let render_pass = get_render_pass(device.clone(), &swapchain);
        let framebuffers = create_framebuffers(&images, &render_pass, &mem_alloc);

        let viewport = Viewport {
            origin: [0.0, 0.0],
            dimensions: app_window.window.inner_size().into(),
            depth_range: 0.0..1.0,
        };

        Self {
            _phy_device: phy_dev,
            device,
            _graphics_queue_index: queue_family_index,
            graphics_queues: vec![queue],

            swapchain,
            images,
            render_pass,
            framebuffers,
            viewport,

            mem_alloc,
            cmd_buf_alloc,
            desc_set_alloc,
        }
    }

    pub fn update_swapchain(&mut self, new_dimensions: PhysicalSize<u32>) {
        let (new_swapchain, new_images) = match self.swapchain.recreate(SwapchainCreateInfo {
            image_extent: new_dimensions.into(),
            ..self.swapchain.create_info()
        }) {
            Ok(r) => r,
            Err(SwapchainCreationError::ImageExtentNotSupported { .. }) => return,
            Err(e) => panic!("swapchain.recreate failed: {e}"),
        };
        self.swapchain = new_swapchain;

        let new_framebuffers = create_framebuffers(&new_images, &self.render_pass, &self.mem_alloc);

        self.framebuffers = new_framebuffers;
    }

    pub fn update_viewport(&mut self, new_dimensions: PhysicalSize<u32>) {
        self.viewport.dimensions = new_dimensions.into();
    }

    pub fn next_image(&mut self) -> Option<(u32, bool, SwapchainAcquireFuture)> {
        match swapchain::acquire_next_image(self.swapchain.clone(), Some(Duration::from_micros(1)))
        {
            Ok(r) => Some(r),
            Err(AcquireError::OutOfDate) => None,
            Err(e) => panic!("swapchain::acquire_next_image failed: {e}"),
        }
    }

    pub fn get_device(&self) -> &Arc<Device> {
        &self.device
    }

    pub fn get_render_pass(&self) -> &Arc<RenderPass> {
        &self.render_pass
    }

    pub fn get_viewport(&self) -> &Viewport {
        &self.viewport
    }

    pub fn get_cmd_buf_alloc(&self) -> &StandardCommandBufferAllocator {
        &self.cmd_buf_alloc
    }

    pub fn get_graphics_queue(&self) -> &Arc<Queue> {
        &self.graphics_queues[0]
    }

    pub fn get_framebuffers(&self) -> &Vec<Arc<Framebuffer>> {
        &self.framebuffers
    }

    pub fn get_mem_alloc(&self) -> &GenericMemoryAllocator<Arc<FreeListAllocator>> {
        &self.mem_alloc
    }

    pub fn get_desc_set_alloc(&self) -> &StandardDescriptorSetAllocator {
        &self.desc_set_alloc
    }

    pub fn get_framebuffer_count(&self) -> usize {
        self.images.len()
    }

    pub fn get_swapchain(&self) -> &Arc<Swapchain> {
        &self.swapchain
    }
}
