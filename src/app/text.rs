use std::{borrow::BorrowMut, sync::Arc};

use ab_glyph::{point, Font, FontRef, Glyph, PxScale, ScaleFont};
use glam::{Mat4, Quat};
use image::{DynamicImage, EncodableLayout, GenericImage, ImageBuffer, Rgba};
use vulkano::{
    buffer::{BufferContents, BufferCreateInfo, BufferUsage, Subbuffer},
    command_buffer::{
        AutoCommandBufferBuilder, CommandBufferUsage, PrimaryAutoCommandBuffer,
        PrimaryCommandBufferAbstract, RenderPassBeginInfo,
    },
    descriptor_set::{PersistentDescriptorSet, WriteDescriptorSet},
    device::Queue,
    image::{view::ImageView, ImmutableImage},
    memory::allocator::{AllocationCreateInfo, MemoryUsage},
    pipeline::{
        graphics::{
            color_blend::ColorBlendState,
            input_assembly::{self, Index, InputAssemblyState, PrimitiveTopology},
            rasterization::RasterizationState,
            vertex_input::Vertex,
            viewport::ViewportState,
        },
        GraphicsPipeline, Pipeline, PipelineLayout,
    },
    render_pass::Subpass,
    sampler::{Filter, Sampler, SamplerAddressMode, SamplerCreateInfo, SamplerMipmapMode},
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
    v_position: [f32; 2],
    #[format(R8G8B8A8_UNORM)]
    v_color: [u8; 4],
    #[format(R32G32_SFLOAT)]
    v_uv: [f32; 2],
}

#[derive(Copy, Clone, BufferContents)]
#[repr(C)]
pub struct UBO {
    mvp: Mat4,
}

pub struct TextObject {
    verts: Vec<TextQuadVert>,
}

pub struct TextRenderer {
    graphics_queue: Arc<Queue>,
    text_graphics_pipeline: Arc<GraphicsPipeline>,
    command_buffers: Vec<Arc<PrimaryAutoCommandBuffer>>,

    text_mvp_set: Arc<PersistentDescriptorSet>,
    text_font_set: Arc<PersistentDescriptorSet>,
    text_vert_buffer: Subbuffer<[TextQuadVert]>,
    text_vert_shader: Arc<ShaderModule>,
    text_frag_shader: Arc<ShaderModule>,

    glyph_height: u32,
    glyph_start_pixels: Vec<u64>,
}

impl TextRenderer {
    pub fn add_text(
        &mut self,
        render_context: &RendererContext,
        input_str: &str,
    ) -> Result<TextObject, RendererError> {
        /*let mut caret = point(0.0, 0.0) + point(0.0, font.ascent());
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
        });*/
        Ok(TextObject {
            verts: vec![TextQuadVert {
                v_position: [0.0, 0.0],
                v_color: [255, 255, 255, 255],
                v_uv: [0.0, 0.0],
            }],
        })
    }

    fn create_text_pipeline(
        render_context: &RendererContext,
        vs: &Arc<ShaderModule>,
        fs: &Arc<ShaderModule>,
    ) -> Arc<GraphicsPipeline> {
        GraphicsPipeline::start()
            .vertex_input_state(TextQuadVert::per_vertex())
            .vertex_shader(vs.entry_point("main").unwrap(), ())
            .input_assembly_state(
                InputAssemblyState::new().topology(PrimitiveTopology::TriangleList),
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
            .color_blend_state(ColorBlendState::default().blend_alpha())
            .build(render_context.get_device().clone())
            .unwrap()
    }

    fn create_text_cmd_bufs(
        render_context: &RendererContext,
        pipeline: &Arc<GraphicsPipeline>,
        vert_buffer: &Subbuffer<[TextQuadVert]>,
        mvp_desc_set: &Arc<PersistentDescriptorSet>,
        text_font_set: &Arc<PersistentDescriptorSet>,
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
                            //clear_values: vec![Some([0.0, 0.0, 0.0, 1.0].into())],
                            clear_values: vec![None],
                            ..RenderPassBeginInfo::framebuffer(fb.clone())
                        },
                        vulkano::command_buffer::SubpassContents::Inline,
                    )
                    .unwrap();

                builder
                    .bind_descriptor_sets(
                        vulkano::pipeline::PipelineBindPoint::Graphics,
                        pipeline.layout().clone(),
                        0,
                        (mvp_desc_set.clone(), text_font_set.clone()),
                    )
                    .bind_pipeline_graphics(pipeline.clone())
                    .bind_vertex_buffers(0, vert_buffer.clone())
                    .draw(u32::try_from(vert_buffer.len()).unwrap(), 1, 0, 0)
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
        let text_verts = vec![
            TextQuadVert {
                v_position: [-0.5, -0.5],
                v_color: [255, 0, 0, 255],
                v_uv: [0.0, 0.0],
            },
            TextQuadVert {
                v_position: [-0.5, 0.5],
                v_color: [255, 255, 255, 255],
                v_uv: [0.0, 1.0],
            },
            TextQuadVert {
                v_position: [0.5, -0.5],
                v_color: [255, 255, 255, 255],
                v_uv: [1.0, 0.0],
            },
            TextQuadVert {
                v_position: [0.5, -0.5],
                v_color: [255, 255, 255, 255],
                v_uv: [1.0, 0.0],
            },
            TextQuadVert {
                v_position: [-0.5, 0.5],
                v_color: [255, 255, 255, 255],
                v_uv: [0.0, 1.0],
            },
            TextQuadVert {
                v_position: [0.5, 0.5],
                v_color: [0, 0, 255, 255],
                v_uv: [1.0, 1.0],
            },
        ];

        let text_vert_buffer = vulkano::buffer::Buffer::from_iter(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::TRANSFER_SRC | BufferUsage::VERTEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: vulkano::memory::allocator::MemoryUsage::Upload,
                ..Default::default()
            },
            text_verts.into_iter(),
        )
        .unwrap();

        let text_mvp_buffer = vulkano::buffer::Buffer::from_data(
            render_context.get_mem_alloc(),
            BufferCreateInfo {
                usage: BufferUsage::TRANSFER_SRC | BufferUsage::UNIFORM_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                usage: MemoryUsage::Upload,
                ..Default::default()
            },
            UBO {
                mvp: glam::Mat4::IDENTITY,
            },
        )
        .unwrap();

        let text_vert_shader = page_text_vert_shader::load(render_context.get_device().clone())
            .expect("page_text_vert_shader::load failed");
        let text_frag_shader = page_text_frag_shader::load(render_context.get_device().clone())
            .expect("page_text_frag_shader::load failed");

        let text_graphics_pipeline =
            Self::create_text_pipeline(render_context, &text_vert_shader, &text_frag_shader);

        let text_mvp_layout = text_graphics_pipeline
            .layout()
            .set_layouts()
            .get(0)
            .unwrap();
        let text_mvp_set = PersistentDescriptorSet::new(
            render_context.get_desc_set_alloc(),
            text_mvp_layout.clone(),
            [WriteDescriptorSet::buffer(0, text_mvp_buffer.clone())],
        )
        .unwrap();

        let font =
            FontRef::try_from_slice(include_bytes!("../../NotoSansMono-Regular.ttf")).unwrap();
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
            CommandBufferUsage::OneTimeSubmit,
        )
        .unwrap();

        let font_texture = {
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
            ImageView::new_default(font_gpu_image).unwrap()
        };

        let font_sampler = Sampler::new(
            render_context.get_device().clone(),
            SamplerCreateInfo {
                // mag_filter: Filter::Linear,
                //min_filter: Filter::Linear,
                //mipmap_mode: SamplerMipmapMode::Nearest,
                //address_mode: [SamplerAddressMode::Repeat; 3],
                //mip_lod_bias: 0.0,
                ..Default::default()
            },
        )
        .unwrap();

        builder
            .build()
            .unwrap()
            .execute(render_context.get_graphics_queue().clone())
            .unwrap()
            .flush()
            .unwrap();

        let text_font_layout = text_graphics_pipeline
            .layout()
            .set_layouts()
            .get(1)
            .unwrap();
        let text_font_set = PersistentDescriptorSet::new(
            render_context.get_desc_set_alloc(),
            text_font_layout.clone(),
            [WriteDescriptorSet::image_view_sampler(
                0,
                font_texture.clone(),
                font_sampler.clone(),
            )],
        )
        .unwrap();

        let command_buffers = Self::create_text_cmd_bufs(
            render_context,
            &text_graphics_pipeline,
            &text_vert_buffer,
            &text_mvp_set,
            &text_font_set,
        );

        Self {
            graphics_queue: render_context.get_graphics_queue().clone(),
            text_mvp_set,
            text_font_set,
            text_vert_buffer,
            text_vert_shader,
            text_frag_shader,
            text_graphics_pipeline,
            command_buffers,
            glyph_height,
            glyph_start_pixels,
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
            &self.text_mvp_set,
            &self.text_font_set,
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
