use std::sync::Arc;

use vulkano::{
    buffer::{BufferContents, BufferCreateInfo, BufferUsage, Subbuffer},
    command_buffer::{
        AutoCommandBufferBuilder, CommandBufferExecFuture, CommandBufferUsage,
        PrimaryAutoCommandBuffer, PrimaryCommandBufferAbstract, RenderPassBeginInfo,
        SubpassContents,
    },
    device::Queue,
    memory::allocator::{AllocationCreateInfo, MemoryUsage},
    pipeline::{
        graphics::{
            input_assembly::{InputAssemblyState, PrimitiveTopology},
            rasterization::RasterizationState,
            vertex_input::Vertex,
            viewport::ViewportState,
        },
        GraphicsPipeline, Pipeline,
    },
    render_pass::Subpass,
    shader::ShaderModule,
    sync::GpuFuture,
};

use crate::{
    app::render::{Renderer, RendererContext},
    signal::generate_test,
};

use self::page_grid_vert_shader::u_transform;

//mod board;
//mod schema;

mod page_grid_vert_shader {
    vulkano_shaders::shader! {
        ty: "vertex",
        src: r"
            #version 460

            layout(push_constant) uniform u_transform {
                vec2 u_scale;
                vec2 u_translate;
            } tf;

            layout(location = 0) in vec2 position;
            layout(location = 0) out float middle;

            void main() {
                gl_Position = vec4(position * tf.u_scale + tf.u_translate, 1.0, 1.0);
                middle = min(min(abs(position.x), abs(position.y)), 1.0);
            }
        ",
    }
}

mod page_line_vert_shader {
    vulkano_shaders::shader! {
        ty: "vertex",
        src: r"
            #version 460

            layout(push_constant) uniform u_transform {
                vec2 u_scale;
                vec2 u_translate;
            } tf;

            layout(location = 0) in vec2 position;

            void main() {
                gl_Position = vec4(position * tf.u_scale + tf.u_translate, 1.0, 1.0);
            }
        ",
    }
}

mod page_grid_frag_shader {
    vulkano_shaders::shader! {
        ty: "fragment",
        src: r"
            #version 460

            layout(location = 0) in float middle;
            layout(location = 0) out vec4 fragment_color;

            void main() {
                fragment_color = vec4(1.0, middle, 1.0, 1.0);
            }
        ",
    }
}

mod page_line_frag_shader {
    vulkano_shaders::shader! {
        ty: "fragment",
        src: r"
            #version 460

            layout(location = 0) out vec4 fragment_color;

            void main() {
                fragment_color = vec4(1.0, 0.0, 0.0, 1.0);
            }
        ",
    }
}

#[derive(BufferContents, Vertex)]
#[repr(C)]
pub struct PageVert {
    #[format(R32G32_SFLOAT)]
    position: [f32; 2],
}

const TEST_SIGNAL_BIN_COUNT: usize = 4;

pub struct Page {
    graphics_queue: Arc<Queue>,
    grid_graphics_pipeline: Arc<GraphicsPipeline>,
    line_graphics_pipeline: Arc<GraphicsPipeline>,

    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,

    grid_vert_buffer: Subbuffer<[PageVert]>,
    line_vert_buffer: Subbuffer<[PageVert]>,

    grid_vert_shader: Arc<ShaderModule>,
    line_vert_shader: Arc<ShaderModule>,

    grid_frag_shader: Arc<ShaderModule>,
    line_frag_shader: Arc<ShaderModule>,

    transform_push_constants: u_transform,
    scale: f32,

    input_slice: [[f64; 2]; TEST_SIGNAL_BIN_COUNT],
    input_slice_x: bool,
    input_slice_y: u8,
    input_slice_next: f64,
}

fn generate_grid(count: u16, spacing: f32) -> Vec<PageVert> {
    let shift = f32::from(count) * spacing / 2.0;
    (0..count)
        .flat_map(|i| -> Vec<PageVert> {
            let pos = f32::from(i) * spacing - shift;
            vec![
                PageVert {
                    position: [-1e9, pos],
                },
                PageVert {
                    position: [1e9, pos],
                },
                PageVert {
                    position: [pos, -1e9],
                },
                PageVert {
                    position: [pos, 1e9],
                },
            ]
        })
        .collect::<Vec<PageVert>>()
}

fn create_grid_buffer(render_context: &RendererContext, spacing: f32) -> Subbuffer<[PageVert]> {
    //const GRID_WIDTH: u16 = u16::MAX / 768;
    let grid_verts = generate_grid(200, spacing);

    vulkano::buffer::Buffer::from_iter(
        render_context.get_mem_alloc(),
        BufferCreateInfo {
            usage: BufferUsage::VERTEX_BUFFER,
            ..Default::default()
        },
        AllocationCreateInfo {
            usage: MemoryUsage::Upload,
            ..Default::default()
        },
        grid_verts.into_iter(),
    )
    .unwrap()
}

fn create_page_pipeline(
    render_context: &RendererContext,
    vs: &Arc<ShaderModule>,
    fs: &Arc<ShaderModule>,
) -> Arc<GraphicsPipeline> {
    GraphicsPipeline::start()
        .vertex_input_state(PageVert::per_vertex())
        .vertex_shader(vs.entry_point("main").unwrap(), ())
        .input_assembly_state(InputAssemblyState::new().topology(PrimitiveTopology::LineList))
        .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([
            render_context.get_viewport().clone(),
        ]))
        .fragment_shader(fs.entry_point("main").unwrap(), ())
        .rasterization_state(RasterizationState {
            line_width: Some(2.0).into(),
            ..Default::default()
        })
        .render_pass(Subpass::from(render_context.get_render_pass().clone(), 0).unwrap())
        .build(render_context.get_device().clone())
        .unwrap()
}

fn create_signal_pipeline(
    render_context: &RendererContext,
    vs: &Arc<ShaderModule>,
    fs: &Arc<ShaderModule>,
) -> Arc<GraphicsPipeline> {
    GraphicsPipeline::start()
        .vertex_input_state(PageVert::per_vertex())
        .vertex_shader(vs.entry_point("main").unwrap(), ())
        .input_assembly_state(InputAssemblyState::new().topology(PrimitiveTopology::LineStrip))
        .viewport_state(ViewportState::viewport_fixed_scissor_irrelevant([
            render_context.get_viewport().clone(),
        ]))
        .fragment_shader(fs.entry_point("main").unwrap(), ())
        .rasterization_state(RasterizationState {
            line_width: Some(2.0).into(),
            ..Default::default()
        })
        .render_pass(Subpass::from(render_context.get_render_pass().clone(), 0).unwrap())
        .build(render_context.get_device().clone())
        .unwrap()
}

fn create_line_segment_vert_buf<const N: usize>(
    render_context: &RendererContext,
    line_points: &[[f64; 2]],
) -> Subbuffer<[PageVert]> {
    let line_segment_verts = line_points.iter().map(|p| PageVert {
        position: [p[0] as f32, p[1] as f32],
    });

    vulkano::buffer::Buffer::from_iter(
        render_context.get_mem_alloc(),
        BufferCreateInfo {
            usage: BufferUsage::VERTEX_BUFFER,
            ..Default::default()
        },
        AllocationCreateInfo {
            usage: MemoryUsage::Upload,
            ..Default::default()
        },
        line_segment_verts,
    )
    .unwrap()
}

fn create_page_cmd_bufs(
    render_context: &RendererContext,
    graphics_pipeline: &Arc<GraphicsPipeline>,
    vert_buffer: &Subbuffer<[PageVert]>,
    line_graphics_pipeline: &Arc<GraphicsPipeline>,
    line_vert_buffer: &Subbuffer<[PageVert]>,
    transform_push_constants: &u_transform,
) -> Vec<Arc<PrimaryAutoCommandBuffer>> {
    render_context
        .get_framebuffers()
        .iter()
        .map(|fb| {
            let mut builder = AutoCommandBufferBuilder::primary(
                render_context.get_cmd_buf_alloc(),
                render_context.get_graphics_queue().queue_family_index(),
                CommandBufferUsage::MultipleSubmit,
            )
            .unwrap();

            builder
                .begin_render_pass(
                    RenderPassBeginInfo {
                        clear_values: vec![Some([0.0, 0.0, 0.0, 1.0].into())],
                        ..RenderPassBeginInfo::framebuffer(fb.clone())
                    },
                    SubpassContents::Inline,
                )
                .unwrap();

            builder
                .bind_pipeline_graphics(graphics_pipeline.clone())
                .bind_vertex_buffers(0, vert_buffer.clone())
                .push_constants(
                    graphics_pipeline.layout().clone(),
                    0,
                    *transform_push_constants,
                )
                .draw(u32::try_from(vert_buffer.len()).unwrap(), 1, 0, 0)
                .unwrap()
                .bind_pipeline_graphics(line_graphics_pipeline.clone())
                .bind_vertex_buffers(0, line_vert_buffer.clone())
                .push_constants(
                    line_graphics_pipeline.layout().clone(),
                    0,
                    *transform_push_constants,
                )
                .draw(u32::try_from(line_vert_buffer.len()).unwrap(), 1, 0, 0)
                .unwrap()
                .end_render_pass()
                .unwrap();

            Arc::new(builder.build().unwrap())
        })
        .collect()
}

impl Page {
    fn create_page_cmd_bufs(
        &self,
        render_context: &RendererContext,
    ) -> Vec<Arc<PrimaryAutoCommandBuffer>> {
        render_context
            .get_framebuffers()
            .iter()
            .map(|fb| {
                let mut builder = AutoCommandBufferBuilder::primary(
                    render_context.get_cmd_buf_alloc(),
                    render_context.get_graphics_queue().queue_family_index(),
                    CommandBufferUsage::MultipleSubmit,
                )
                .unwrap();

                builder
                    .begin_render_pass(
                        RenderPassBeginInfo {
                            clear_values: vec![Some([0.0, 0.0, 0.0, 1.0].into())],
                            ..RenderPassBeginInfo::framebuffer(fb.clone())
                        },
                        SubpassContents::Inline,
                    )
                    .unwrap();

                builder
                    .bind_pipeline_graphics(self.grid_graphics_pipeline.clone())
                    .bind_vertex_buffers(0, self.grid_vert_buffer.clone())
                    .push_constants(
                        self.grid_graphics_pipeline.layout().clone(),
                        0,
                        self.transform_push_constants,
                    )
                    .draw(u32::try_from(self.grid_vert_buffer.len()).unwrap(), 1, 0, 0)
                    .unwrap()
                    .bind_pipeline_graphics(self.line_graphics_pipeline.clone())
                    .bind_vertex_buffers(0, self.line_vert_buffer.clone())
                    .push_constants(
                        self.line_graphics_pipeline.layout().clone(),
                        0,
                        self.transform_push_constants,
                    )
                    .draw(u32::try_from(self.line_vert_buffer.len()).unwrap(), 1, 0, 0)
                    .unwrap()
                    .end_render_pass()
                    .unwrap();

                Arc::new(builder.build().unwrap())
            })
            .collect()
    }

    pub fn zoom(&mut self, zoom_change: f32, render_context: &RendererContext) {
        if zoom_change > 0.0 {
            self.scale *= 1.025;
            self.scale *= 1.025;
        } else {
            self.scale *= 0.975;
            self.scale *= 0.975;
        }

        self.transform_push_constants.u_scale[0] = self.scale;
        self.transform_push_constants.u_scale[1] = self.scale
            * (render_context.get_viewport().dimensions[0]
                / render_context.get_viewport().dimensions[1]);
        self.update_command_buffers(render_context);
    }

    pub fn translate(&mut self, translate_delta: (f64, f64), render_context: &RendererContext) {
        self.transform_push_constants.u_translate[0] +=
            (translate_delta.0 as f32) * (2.0 / render_context.get_viewport().dimensions[0]);
        self.transform_push_constants.u_translate[1] +=
            (translate_delta.1 as f32) * (2.0 / render_context.get_viewport().dimensions[1]);

        self.update_command_buffers(render_context);
    }
}

impl Renderer for Page {
    fn new(render_context: &RendererContext) -> Self {
        let grid_vert_buffer = create_grid_buffer(render_context, 1.0);

        let input_slice: [[f64; 2]; TEST_SIGNAL_BIN_COUNT] = [[0.0, 0.0]; TEST_SIGNAL_BIN_COUNT];

        let test_signal = generate_test::<TEST_SIGNAL_BIN_COUNT>(&input_slice);
        println!("{test_signal:?}");
        let line_vert_buffer =
            create_line_segment_vert_buf::<TEST_SIGNAL_BIN_COUNT>(render_context, &test_signal);

        let grid_vert_shader = page_grid_vert_shader::load(render_context.get_device().clone())
            .expect("page_grid_vert_shader::load failed");
        let line_vert_shader = page_line_vert_shader::load(render_context.get_device().clone())
            .expect("page_line_vert_shader::load failed");

        let grid_frag_shader = page_grid_frag_shader::load(render_context.get_device().clone())
            .expect("page_grid_frag_shader::load failed");
        let line_frag_shader = page_line_frag_shader::load(render_context.get_device().clone())
            .expect("page_line_frag_shader::load failed");

        let grid_graphics_pipeline =
            create_page_pipeline(render_context, &grid_vert_shader, &grid_frag_shader);
        let line_graphics_pipeline =
            create_signal_pipeline(render_context, &line_vert_shader, &line_frag_shader);

        let aspect_ratio_x = 1.0;
        let aspect_ratio_y = 1.0
            * (render_context.get_viewport().dimensions[0]
                / render_context.get_viewport().dimensions[1]);

        let transform_push_constants = page_grid_vert_shader::u_transform {
            u_scale: [aspect_ratio_x, aspect_ratio_y],
            u_translate: [0.0, 0.0],
        };

        let command_buffers = create_page_cmd_bufs(
            render_context,
            &grid_graphics_pipeline,
            &grid_vert_buffer,
            &line_graphics_pipeline,
            &line_vert_buffer,
            &transform_push_constants,
        );

        Self {
            graphics_queue: render_context.get_graphics_queue().clone(),
            grid_graphics_pipeline,
            line_graphics_pipeline,
            command_buffers,
            grid_vert_buffer,
            line_vert_buffer,
            grid_vert_shader,
            line_vert_shader,
            grid_frag_shader,
            line_frag_shader,
            transform_push_constants,
            scale: 1.0,
            input_slice,
            input_slice_x: false,
            input_slice_y: 0u8,
            input_slice_next: 1.0,
        }
    }

    fn create_pipeline(
        &self,
        render_context: &RendererContext,
    ) -> Arc<vulkano::pipeline::GraphicsPipeline> {
        create_page_pipeline(
            render_context,
            &self.grid_vert_shader,
            &self.grid_frag_shader,
        )
    }

    fn update_pipeline(&mut self, render_context: &RendererContext) {
        self.transform_push_constants.u_scale[0] = self.scale;
        self.transform_push_constants.u_scale[1] = self.scale
            * (render_context.get_viewport().dimensions[0]
                / render_context.get_viewport().dimensions[1]);

        self.grid_graphics_pipeline = self.create_pipeline(render_context);
        self.line_graphics_pipeline = create_signal_pipeline(
            render_context,
            &self.line_vert_shader,
            &self.line_frag_shader,
        );
        self.update_command_buffers(render_context);
    }

    fn update_command_buffers(&mut self, render_context: &RendererContext) {
        let spacing = (0.1 * 2f32.powf((1.0 / self.scale).log2())).ceil();

        self.grid_vert_buffer = create_grid_buffer(render_context, spacing);
        //self.line_vert_buffer =
        //    create_line_segment_vert_buf::<TEST_SIGNAL_BIN_COUNT>(render_context, &self.);

        self.command_buffers = self.create_page_cmd_bufs(render_context);
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
