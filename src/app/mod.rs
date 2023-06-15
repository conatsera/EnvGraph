use image::Rgba;
use vulkano::{buffer::BufferContents, instance::InstanceCreateInfo};

use self::{
    render::RendererContext,
    text::{TextObject, TextPlane, TextRenderer},
};
use std::sync::Arc;

use vulkano::{
    instance::Instance,
    swapchain::{Surface, SwapchainPresentInfo},
    sync::{self, future::FenceSignalFuture, FlushError, GpuFuture},
};
use vulkano_win::VkSurfaceBuild;
use winit::{
    event::{DeviceEvent, ElementState, Event, MouseButton, MouseScrollDelta, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::{Window, WindowBuilder},
};

use crate::page::Page;

use render::Renderer;

pub mod render;
pub mod text;
pub mod ui;

pub enum RendererError {
    UNKNOWN = 0,
    FAIL = 1,
    SUCCESS = 2,
}

pub struct App {
    window: MainWindow,
    render_context: RendererContext,
    text_renderer: TextRenderer,
}

#[derive(BufferContents)]
#[repr(C)]
pub struct RgbaPixel {
    color: [u8; 4],
}

impl From<Rgba<u8>> for RgbaPixel {
    fn from(value: Rgba<u8>) -> Self {
        Self { color: value.0 }
    }
}

impl App {
    pub fn new() -> Self {
        let library = vulkano::VulkanLibrary::new().expect("vulkan library required");
        let required_exts = vulkano_win::required_extensions(&library);
        let instance = vulkano::instance::Instance::new(
            library,
            InstanceCreateInfo {
                enabled_extensions: required_exts,
                ..Default::default()
            },
        )
        .expect("failed to create vulkan instance");

        let app_window = MainWindow::new(&instance);
        let render_context = RendererContext::new(&instance, &app_window);

        Self {
            text_renderer: TextRenderer::new(&render_context),
            render_context: render_context,
            window: app_window,
        }
    }

    pub fn run(self) -> ! {
        self.window.run(self.render_context)
    }

    pub fn add_text(&mut self, render_context: &RendererContext, input_str: &str) -> Result<&TextObject, RendererError> {
        self.text_renderer.add_text(render_context, input_str)
    }
}

pub struct MainWindow {
    event_loop: EventLoop<()>,
    pub surface: Arc<Surface>,
    pub window: Arc<Window>,
}

impl MainWindow {
    pub fn new(instance: &Arc<Instance>) -> Self {
        let event_loop = EventLoop::new();
        let surface = WindowBuilder::new()
            .build_vk_surface(&event_loop, instance.clone())
            .unwrap();

        let window = surface
            .object()
            .unwrap()
            .clone()
            .downcast::<Window>()
            .unwrap();

        Self {
            event_loop,
            surface,
            window,
        }
    }
    pub fn run(self, mut render_context: RendererContext) -> ! {
        let mut page_renderer = Page::new(&render_context);
        let mut window_resized = false;
        let mut recreate_swapchain = false;

        let frames_in_flight = render_context.get_framebuffer_count();
        let mut fences: Vec<Option<Arc<FenceSignalFuture<_>>>> = vec![None; frames_in_flight];
        let mut previous_fence_i = 0;

        let mut drag = false;

        let mut last_sec = std::time::SystemTime::now();

        self.event_loop
            .run(move |event, _, control_flow| match event {
                Event::WindowEvent {
                    event: WindowEvent::CloseRequested,
                    ..
                } => *control_flow = ControlFlow::Exit,
                Event::WindowEvent {
                    event: WindowEvent::Resized(_),
                    ..
                } => window_resized = true,
                Event::WindowEvent { event: WindowEvent::MouseInput { state, button, ..}, .. } => {
                    if button == MouseButton::Left {
                        drag = state == ElementState::Pressed;
                    }
                },
                Event::DeviceEvent { event: DeviceEvent::MouseMotion { delta }, ..} => {
                    if drag {
                        page_renderer.translate(delta, &render_context);
                    }
                },
                Event::WindowEvent { event: WindowEvent::MouseWheel { delta, ..}, .. } => {
                    match delta {
                        MouseScrollDelta::LineDelta(_, y) => page_renderer.zoom(y, &render_context),
                        MouseScrollDelta::PixelDelta(p) => page_renderer.zoom(p.y as f32, &render_context),
                    }
                },
                Event::MainEventsCleared => {
                    let now= std::time::SystemTime::now();
                    let elapsed = now
                        .duration_since(last_sec)
                        .unwrap()
                        .as_secs_f32();

                    if elapsed > 1.0 {
                        last_sec = now;
                        page_renderer.update_command_buffers(&render_context);
                    }

                    if window_resized || recreate_swapchain {
                        recreate_swapchain = false;

                        let new_dimensions = self.window.inner_size();

                        render_context.update_swapchain(new_dimensions);

                        if window_resized {
                            window_resized = false;

                            render_context.update_viewport(new_dimensions);

                            page_renderer.update_pipeline(&render_context);
                        }
                    }

                    let Some((image_i, suboptimal, acquire_future)) = render_context.next_image() else {
                        recreate_swapchain = true;
                        return;
                    };

                    if suboptimal {
                        recreate_swapchain = true;
                        return;
                    }

                    if let Some(image_fence) = &fences[image_i as usize] {
                        image_fence.wait(None).unwrap();
                    }

                    let previous_future = match fences[previous_fence_i as usize].clone() {
                        None => {
                            let mut now = sync::now(render_context.get_device().clone());
                            now.cleanup_finished();

                            now.boxed()
                        }
                        Some(fence) => fence.boxed(),
                    };

                    let future = previous_future.join(acquire_future);
                    let future = page_renderer
                        .execute_cmd_buf(future.boxed(), image_i)
                        .then_swapchain_present(
                            render_context.get_graphics_queue().clone(),
                            SwapchainPresentInfo::swapchain_image_index(
                                render_context.get_swapchain().clone(),
                                image_i,
                            ),
                        )
                        .then_signal_fence_and_flush();

                    fences[image_i as usize] = match future {
                        Ok(value) => Some(Arc::new(value)),
                        Err(FlushError::OutOfDate) => {
                            recreate_swapchain = true;
                            None
                        }
                        Err(e) => {
                            println!("failed to flush future: {e}");
                            None
                        }
                    };

                    previous_fence_i = image_i;
                }
                _ => (),
            })
    }
}
