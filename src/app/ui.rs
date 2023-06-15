use std::sync::Arc;

use vulkano::buffer::{Buffer, BufferCreateInfo, BufferUsage};
use vulkano::command_buffer::{
    AutoCommandBufferBuilder, CommandBufferExecFuture, CommandBufferUsage,
    PrimaryCommandBufferAbstract, RenderPassBeginInfo, SubpassContents,
};
use vulkano::memory::allocator::{AllocationCreateInfo, MemoryUsage};
use vulkano::pipeline::graphics::input_assembly::InputAssemblyState;
use vulkano::pipeline::graphics::viewport::ViewportState;
use vulkano::pipeline::Pipeline;
use vulkano::render_pass::Subpass;
use vulkano::sync::GpuFuture;
use vulkano::{
    buffer::{BufferContents, Subbuffer},
    command_buffer::PrimaryAutoCommandBuffer,
    device::Queue,
    pipeline::{graphics::vertex_input::Vertex, GraphicsPipeline},
    shader::ShaderModule,
};

use super::render::{Renderer, RendererContext};

#[derive(BufferContents, Vertex)]
#[repr(C)]
struct UIVertex {
    #[format(R32G32_SFLOAT)]
    position: [f32; 2],
    #[format(R8G8B8A8_UNORM)]
    color: [u8; 4],
}

mod ui_vert_shader {
    vulkano_shaders::shader! {
        ty: "vertex",
        src: r"
            #version 460

            layout(push_constant) uniform u_transform {
                vec2 u_scale;
                vec2 u_translate;
            } tf;

            layout(location = 0) in vec2 position;
            layout(location = 1) in vec4 color;

            layout(location = 0) out vec4 vert_color;

            void main() {
                gl_Position = vec4(position * tf.u_scale + tf.u_translate, 0.0, 1.0);
                vert_color = color;
            }
        ",
    }
}

mod ui_frag_shader {
    vulkano_shaders::shader! {
        ty: "fragment",
        src: r"
            #version 460

            layout(location = 0) in vec4 vert_color;

            layout(location = 0) out vec4 f_color;

            void main() {
                f_color = vert_color;
            }
        ",
    }
}

pub struct UI {
    graphics_queue: Arc<Queue>,
    graphics_pipeline: Arc<GraphicsPipeline>,

    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,

    vert_buffer: Subbuffer<[UIVertex]>,
    vert_shader: Arc<ShaderModule>,
    frag_shader: Arc<ShaderModule>,

    transform_push_constants: ui_vert_shader::u_transform,
}

impl Renderer for UI {
    fn new(render_context: &RendererContext) -> Self {
        let vert1 = UIVertex {
            position: [-0.5, -0.5],
            color: [255, 0, 0, 255],
        };
        let vert2 = UIVertex {
            position: [0.0, 0.5],
            color: [0, 255, 0, 255],
        };
        let vert3 = UIVertex {
            position: [0.5, -0.25],
            color: [0, 0, 255, 255],
        };

        let vert_buffer = Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::VERTEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: MemoryUsage::Upload,
                ..Default::default()
            },
            vec![vert1, vert2, vert3].into_iter(),
        )
        .unwrap();

        let vert_shader =
            ui_vert_shader::load(render_context.get_device().clone()).expect("vs::load failed");
        let frag_shader =
            ui_frag_shader::load(render_context.get_device().clone()).expect("fs::load failed");

        let graphics_pipeline =
            Self::create_ui_pipeline(render_context, &vert_shader, &frag_shader);

        let transform_push_constants = ui_vert_shader::u_transform {
            u_scale: [1.0, 1.0],
            u_translate: [0.0, 0.0],
        };

        let command_buffers = Self::create_ui_command_buffers(
            render_context,
            &graphics_pipeline,
            &vert_buffer,
            &transform_push_constants,
        );

        Self {
            graphics_queue: render_context.get_graphics_queue().clone(),
            graphics_pipeline,
            command_buffers,
            vert_buffer,
            vert_shader,
            frag_shader,
            transform_push_constants,
        }
    }

    fn create_pipeline(&self, render_context: &RendererContext) -> Arc<GraphicsPipeline> {
        Self::create_ui_pipeline(render_context, &self.vert_shader, &self.frag_shader)
    }

    fn update_pipeline(&mut self, render_context: &RendererContext) {
        self.graphics_pipeline = self.create_pipeline(render_context);
        self.update_command_buffers(render_context);
    }

    fn update_command_buffers(&mut self, render_context: &RendererContext) {
        self.command_buffers = Self::create_ui_command_buffers(
            render_context,
            &self.graphics_pipeline,
            &self.vert_buffer,
            &self.transform_push_constants,
        );
    }

    fn execute_cmd_buf<F>(&self, future: F, image_i: u32) -> CommandBufferExecFuture<F>
    where
        F: GpuFuture + 'static,
    {
        self.command_buffers[image_i as usize]
            .clone()
            .execute_after(future, self.graphics_queue.clone())
            .unwrap()
    }
}

impl UI {
    fn create_ui_pipeline(
        render_context: &RendererContext,
        vs: &Arc<ShaderModule>,
        fs: &Arc<ShaderModule>,
    ) -> Arc<GraphicsPipeline> {
        GraphicsPipeline::start()
            .vertex_input_state(UIVertex::per_vertex())
            .vertex_shader(vs.entry_point("main").unwrap(), ())
            .input_assembly_state(InputAssemblyState::new())
            .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([
                render_context.get_viewport().clone(),
            ]))
            .fragment_shader(fs.entry_point("main").unwrap(), ())
            .render_pass(Subpass::from(render_context.get_render_pass().clone(), 0).unwrap())
            .build(render_context.get_device().clone())
            .unwrap()
    }

    fn create_ui_command_buffers<T, PC>(
        render_context: &RendererContext,
        pipeline: &Arc<GraphicsPipeline>,
        vertex_buffer: &Subbuffer<[T]>,
        push_constants: &PC,
    ) -> Vec<Arc<PrimaryAutoCommandBuffer>>
    where
        PC: BufferContents + Clone,
    {
        render_context
            .get_framebuffers()
            .iter()
            .map(|framebuffer| {
                let mut builder = AutoCommandBufferBuilder::primary(
                    render_context.get_cmd_buf_alloc(),
                    render_context.get_graphics_queue().queue_family_index(),
                    CommandBufferUsage::MultipleSubmit,
                )
                .unwrap();

                builder
                    .begin_render_pass(
                        RenderPassBeginInfo {
                            clear_values: vec![Some([0.0, 0.0, 1.0, 1.0].into())],
                            ..RenderPassBeginInfo::framebuffer(framebuffer.clone())
                        },
                        SubpassContents::Inline,
                    )
                    .unwrap()
                    .bind_pipeline_graphics(pipeline.clone())
                    .bind_vertex_buffers(0, vertex_buffer.clone())
                    .push_constants::<PC>(pipeline.layout().clone(), 0, push_constants.clone())
                    .draw(u32::try_from(vertex_buffer.len()).unwrap(), 1, 0, 0)
                    .unwrap()
                    .end_render_pass()
                    .unwrap();

                Arc::new(builder.build().unwrap())
            })
            .collect()
    }
}
