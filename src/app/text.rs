use std::{sync::Arc, borrow::BorrowMut};

use ab_glyph::{point, Font, FontRef, Glyph, PxScale, ScaleFont};
use glam::Quat;
use image::{DynamicImage, ImageBuffer, Rgba, GenericImage};
use vulkano::{
    buffer::{BufferContents, BufferCreateInfo, BufferUsage, Subbuffer},
    command_buffer::{
        AutoCommandBufferBuilder, CommandBufferUsage, PrimaryAutoCommandBuffer,
        PrimaryCommandBufferAbstract, RenderPassBeginInfo,
    },
    device::Queue,
    image::ImmutableImage,
    memory::allocator::{AllocationCreateInfo, MemoryUsage},
    pipeline::{
        graphics::{
            input_assembly::{self, Index, InputAssemblyState, PrimitiveTopology},
            rasterization::RasterizationState,
            vertex_input::Vertex,
            viewport::ViewportState,
        },
        GraphicsPipeline,
    },
    render_pass::Subpass,
    shader::ShaderModule,
    sync::GpuFuture,
};

use super::{
    render::{Renderer, RendererContext},
    RendererError, RgbaPixel,
};

mod page_text_vert_shader {
    vulkano_shaders::shader! {
            ty: "vertex",
            path: "shaders/text.vert",
    }
}

mod page_text_frag_shader {
    vulkano_shaders::shader! {
            ty: "fragment",
            path: "shaders/text.frag",
    }
}

#[derive(Copy, Clone, BufferContents, Vertex)]
#[repr(C)]
pub struct TextQuadVert {
    #[format(R32G32_SFLOAT)]
    position: [f32; 2],
    #[format(R8G8B8A8_UNORM)]
    color: [u8; 4],
    #[format(R8G8_UNORM)]
    uv: [u8; 2],
}

pub struct TextObject {
    instance: u16,
    plane: TextPlane,
}

#[derive(Copy, Clone, BufferContents)]
#[repr(C)]
pub struct TextPlane {
    rotation: [f32; 4],
    position: [f32; 3],
    scale: [f32; 3],
    texture_index: u32,
}

pub struct TextRenderer {
    graphics_queue: Arc<Queue>,
    text_graphics_pipeline: Arc<GraphicsPipeline>,
    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,

    text_objects: Vec<TextObject>,
    text_objects_buffer: Subbuffer<[TextPlane]>,
    text_index_buffer: Subbuffer<[u16]>,
    text_vert_buffer: Subbuffer<[TextQuadVert]>,
    text_vert_shader: Arc<ShaderModule>,
    text_frag_shader: Arc<ShaderModule>,

    font_image: ImageBuffer<Rgba<u8>, Vec<u8>>,
    glyph_height: u32,
    glyph_start_pixels: Vec<u64>,

    text_images: Vec<Subbuffer<[RgbaPixel]>>,
}

impl TextRenderer {
    pub fn add_text(&mut self, render_context: &RendererContext, input_str: &str) -> Result<&TextObject, RendererError> {
        let mut caret = point(0.0, 0.0) + point(0.0, font.ascent());
        let mut last_glyph: Option<Glyph> = None;
        let glyphs = input_str.chars().into_iter()
            .map(|num_glyph_char| -> _ {
                let mut glyph = font.scaled_glyph(num_glyph_char);
                if let Some(previous) = last_glyph.take() {
                    caret.x += font.kern(previous.id, glyph.id);
                }
                glyph.position = caret;

                last_glyph = Some(glyph.clone());
                caret.x += font.h_advance(glyph.id);

                glyph
            })
            .collect::<Vec<Glyph>>();

        let glyph_height = font.height().ceil() as u32;
        let glyphs_width = {
            let min_x = glyphs.first().unwrap().position.x;
            let last_glyph = glyphs.last().unwrap();
            let max_x = last_glyph.position.x + font.h_advance(last_glyph.id);
            (max_x - min_x).ceil() as u32
        };

        let mut text_image =
            DynamicImage::new_rgba8(glyphs_width + 40, glyph_height + 40).to_rgba8();

        for in_char in input_str.chars() {
            if f < '!' || f > '~' {
                let exclamation = &self.font_image.into_iter().as_slice()[self.glyph_start_pixels[0] as usize.. self.glyph_start_pixels[1 as usize] as usize];
                text_image.get_pixel_mut(x, y)

            } else if f == '\n' {

                self.font_image_buffer.slice(self.glyph_start_pixels[0]..self.glyph_start_pixels[0])
            } else {
                let font_index = f as u8 - b'!';
                
                self.font_image_buffer.slice(self.glyph_start_pixels[font_index as usize]..self.glyph_start_pixels[(font_index+1) as usize])
            }
        };

        let mut builder = AutoCommandBufferBuilder::primary(
            render_context.get_cmd_buf_alloc(),
            render_context.get_graphics_queue().queue_family_index(),
            CommandBufferUsage::MultipleSubmit,
        )
        .unwrap();

        let text_img_buffer = vulkano::buffer::Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::TRANSFER_SRC,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: MemoryUsage::DeviceOnly,
                ..Default::default()
            },
            text_img_chunks
        ).unwrap();

        let text_object_texture = ImmutableImage::from_buffer(
            render_context.get_mem_alloc(),
            text_img_buffer,
            vulkano::image::ImageDimensions::Dim2d {
                width: text_width,
                height: text_height,
                array_layers: 1,
            },
            vulkano::image::MipmapsCount::One,
            vulkano::format::Format::R8G8B8A8_UNORM,
            &mut builder,
        )
        .unwrap();

        builder
            .build()
            .unwrap()
            .execute(render_context.get_graphics_queue().clone())
            .unwrap()
            .flush()
            .unwrap();

        self.text_objects.push(TextObject {
            instance: u16::try_from(self.text_objects.len()).unwrap(),
            plane: TextPlane {
                rotation: Quat::IDENTITY.into(),
                position: [0.0, 0.0, 0.0],
                scale: [1.0, 1.0, 1.0],
                texture_index: 0,
            },
        });
        Ok(self.text_objects.last().unwrap())
    }

    fn create_text_pipeline(
        render_context: &RendererContext,
        vs: &Arc<ShaderModule>,
        fs: &Arc<ShaderModule>,
    ) -> Arc<GraphicsPipeline> {
        GraphicsPipeline::start()
            .vertex_input_state(TextQuadVert::per_instance())
            .vertex_shader(vs.entry_point("main").unwrap(), ())
            .input_assembly_state(
                InputAssemblyState::new().topology(PrimitiveTopology::TriangleStrip),
            )
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

    fn create_text_cmd_bufs(
        render_context: &RendererContext,
        pipeline: &Arc<GraphicsPipeline>,
        vert_buffer: &Subbuffer<[TextQuadVert]>,
        index_buffer: &Subbuffer<[u16]>,
        instance_count: u32,
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
                        vulkano::command_buffer::SubpassContents::Inline,
                    )
                    .unwrap();

                builder
                    .bind_pipeline_graphics(pipeline.clone())
                    .bind_vertex_buffers(0, vert_buffer.clone())
                    .bind_index_buffer(index_buffer.clone())
                    .draw(
                        u32::try_from(vert_buffer.len()).unwrap(),
                        instance_count,
                        0,
                        0,
                    )
                    .unwrap()
                    .end_render_pass()
                    .unwrap();

                Arc::new(builder.build().unwrap())
            })
            .collect()
    }
}

impl Renderer for TextRenderer {
    fn new(render_context: &RendererContext) -> Self {
        let text_verts = [TextQuadVert {
            position: [0.0, 0.0],
            color: [0, 0, 0, 0],
            uv: [0, 0],
        }; 6];

        let text_vert_buffer = vulkano::buffer::Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::TRANSFER_SRC,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: vulkano::memory::allocator::MemoryUsage::Upload,
                ..Default::default()
            },
            text_verts.into_iter(),
        )
        .unwrap();

        let text_index_buffer = vulkano::buffer::Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::TRANSFER_SRC,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: vulkano::memory::allocator::MemoryUsage::Upload,
                ..Default::default()
            },
            (0u16..0u16).into_iter(),
        )
        .unwrap();

        let text_objects = vec![TextPlane {
            rotation: glam::Quat::IDENTITY.into(),
            position: [0.0, 0.0, 0.0],
            scale: [1.0, 1.0, 1.0],
            texture_index: 0,
        }];

        let text_objects_buffer = vulkano::buffer::Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::TRANSFER_SRC,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: vulkano::memory::allocator::MemoryUsage::Upload,
                ..Default::default()
            },
            text_objects.into_iter(),
        )
        .unwrap();

        let text_vert_shader = page_text_vert_shader::load(render_context.get_device().clone())
            .expect("page_text_vert_shader::load failed");
        let text_frag_shader = page_text_frag_shader::load(render_context.get_device().clone())
            .expect("page_text_frag_shader::load failed");

        let text_graphics_pipeline =
            Self::create_text_pipeline(render_context, &text_vert_shader, &text_frag_shader);

        let command_buffers = Self::create_text_cmd_bufs(
            render_context,
            &text_graphics_pipeline,
            &text_vert_buffer,
            &text_index_buffer,
            1,
        );

        let font = FontRef::try_from_slice(include_bytes!(
            "/usr/share/fonts/google-noto/NotoSansMono-Regular.ttf"
        ))
        .unwrap();
        let font = font.as_scaled(PxScale::from(45.0));

        let mut caret = point(0.0, 0.0) + point(0.0, font.ascent());
        let mut last_glyph: Option<Glyph> = None;
        let glyphs = ('!'..='~')
            .map(|num_glyph_char| -> _ {
                let mut glyph = font.scaled_glyph(num_glyph_char);
                if let Some(previous) = last_glyph.take() {
                    caret.x += font.kern(previous.id, glyph.id);
                }
                glyph.position = caret;

                last_glyph = Some(glyph.clone());
                caret.x += font.h_advance(glyph.id);

                glyph
            })
            .collect::<Vec<Glyph>>();

        let glyph_height = font.height().ceil() as u32;
        let glyphs_width = {
            let min_x = glyphs.first().unwrap().position.x;
            let last_glyph = glyphs.last().unwrap();
            let max_x = last_glyph.position.x + font.h_advance(last_glyph.id);
            (max_x - min_x).ceil() as u32
        };

        let mut font_image =
            DynamicImage::new_rgba8(glyphs_width + 40, glyph_height + 40).to_rgba8();

        let mut glyph_start_pixels = Vec::<u64>::new();

        for glyph in glyphs {
            let outlined = font.outline_glyph(glyph).unwrap();
            let bounds = outlined.px_bounds();
            outlined.draw(|x, y, c| {
                let px = font_image.get_pixel_mut(x + bounds.min.x as u32, y + bounds.min.y as u32);

                glyph_start_pixels.push((x + bounds.min.x as u32) as u64);

                *px = Rgba([255, 255, 255, px.0[3].saturating_add((c * 255.0) as u8)]);
            });
        }

        let mut builder = AutoCommandBufferBuilder::primary(
            render_context.get_cmd_buf_alloc(),
            render_context.get_graphics_queue().queue_family_index(),
            CommandBufferUsage::MultipleSubmit,
        )
        .unwrap();

        let font_image_buffer = vulkano::buffer::Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                size: u64::try_from(font_image.len() * 4).unwrap(),
                usage: BufferUsage::TRANSFER_SRC,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: MemoryUsage::Upload,
                ..Default::default()
            },
            font_image.pixels().into_iter().map(|p| RgbaPixel::from(*p)),
        )
        .unwrap();

        let font_gpu_image = ImmutableImage::from_iter(
            render_context.get_mem_alloc(),
            font_image.pixels().into_iter().map(|p| RgbaPixel::from(*p)),
            vulkano::image::ImageDimensions::Dim2d {
                width: font_image.width(),
                height: font_image.height(),
                array_layers: 1,
            },
            vulkano::image::MipmapsCount::One,
            vulkano::format::Format::R8G8B8A8_UNORM,
            &mut builder,
        )
        .unwrap();

        builder
            .build()
            .unwrap()
            .execute(render_context.get_graphics_queue().clone())
            .unwrap()
            .flush()
            .unwrap();

        Self {
            graphics_queue: render_context.get_graphics_queue().clone(),
            text_vert_buffer,
            text_index_buffer,
            text_vert_shader,
            text_frag_shader,
            text_graphics_pipeline,
            command_buffers,
            font_image,
            glyph_height,
            glyph_start_pixels,
            font_image_buffer,
            font_gpu_image,
            text_objects: Vec::<TextObject>::new(),
            text_objects_buffer,
        }
    }

    fn create_pipeline(&self, render_context: &RendererContext) -> Arc<GraphicsPipeline> {
        Self::create_text_pipeline(
            render_context,
            &self.text_vert_shader,
            &self.text_frag_shader,
        )
    }

    fn update_command_buffers(&mut self, render_context: &RendererContext) {
        self.command_buffers = Self::create_text_cmd_bufs(
            render_context,
            &self.text_graphics_pipeline,
            &self.text_vert_buffer,
            &self.text_index_buffer,
            self.text_index_buffer.len() as u32,
        );
    }

    fn update_pipeline(&mut self, render_context: &RendererContext) {
        self.text_graphics_pipeline = self.create_pipeline(render_context);
        self.update_command_buffers(render_context);
    }

    fn execute_cmd_buf<F>(
        &self,
        future: F,
        image_i: u32,
    ) -> vulkano::command_buffer::CommandBufferExecFuture<F>
    where
        F: GpuFuture + 'static,
    {
        self.command_buffers[image_i as usize]
            .clone()
            .execute_after(future, self.graphics_queue.clone())
            .unwrap()
    }
}
